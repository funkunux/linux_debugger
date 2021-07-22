#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <linenoise.h>
#include <iostream>
#include <sstream>
#include <ptrace_wrapper.h>
#include <debugger.h>

uint64_t str2hex(const std::string& str) {
    if(0 == str.find("0x") || 0 == str.find("0X"))
        return std::stoul(str, nullptr, 0);
    return std::stoul(str, nullptr, 16);
}
std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> out{};
    std::stringstream ss {s};
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        out.push_back(item);
    }

    return out;
}

void debugger::run() {
    int wait_status;
    waitpid(pid_, &wait_status, 0);
    ptrace_wrapper::set_options(pid_, PTRACE_O_EXITKILL);
    printf("Start Debugging Process[%u]\n", pid_);

    char* line = nullptr;
    while(!quit_ && (line = linenoise("(fdb) ")) != nullptr) {
        handle_command(line);
        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }
}
void debugger::quit_execution() {
    quit_ = true;
    if(!pid_terminated_) {
        printf("Terminate Process[%u]\n", pid_);
        kill(pid_, SIGKILL);
    }
}
void debugger::breakpoint_execution(uintptr_t addr) {
    if(breakpoint_list_.count(addr) == 0) {
        breakpoint_list_.emplace(addr, new gerneric_breakpoint(pid_, addr));
    }
    breakpoint& bp = *breakpoint_list_.at(addr);
    if(bp.is_enable()) {
        printf("Breakpoint at position[0x%lx] had already been set.\n", addr);
    }
    if(!bp.enable()) {
        printf("Failed to set breakpoint at position[0x%lx].\n", addr);
    }
}
void debugger::hit_breakpoint() {
    uint64_t rip_value;
    register_read("rip", rip_value);
    uint64_t hit_position = rip_value - 1;
    if(breakpoint_list_.count(hit_position) > 0) {
        breakpoint& bp = *breakpoint_list_.at(hit_position);
        if(bp.is_enable()) {
            bp.disable();
            register_write("rip", hit_position);
            step_over();
            bp.enable();
        }
    }
}
void debugger::continue_execution() {
    hit_breakpoint();

    ptrace_wrapper::continue_process(pid_);

    int wait_status;
    waitpid(pid_, &wait_status, 0);
    check_wait_status(pid_, wait_status, false);
}
bool debugger::register_read(const std::string& reg_name, uint64_t& value) {
    struct user_regs_struct regs;
    if(!ptrace_wrapper::get_register(pid_, regs)) return false;

    uint64_t* regs_value = get_register_by_name(reg_name, regs);
    if(!regs_value) return false;
    value = *regs_value;
    return true;
}
void debugger::registers_dump() {
    struct user_regs_struct regs;
    if(!ptrace_wrapper::get_register(pid_, regs)) return;

    printf("rax = 0x%016lx    rsp = 0x%016lx    cs = 0x%016lx    rip = 0x%016lx\n", *get_register_by_name("rax", regs), *get_register_by_name("rsp", regs), *get_register_by_name("cs", regs), *get_register_by_name("rip", regs));
    printf("rbx = 0x%016lx    rbp = 0x%016lx    ds = 0x%016lx    eflags = 0x%016lx\n", *get_register_by_name("rbx", regs), *get_register_by_name("rbp", regs), *get_register_by_name("ds", regs), *get_register_by_name("eflags", regs));
    printf("rcx = 0x%016lx    rsi = 0x%016lx    ss = 0x%016lx\n", *get_register_by_name("rcx", regs), *get_register_by_name("rsi", regs), *get_register_by_name("ss", regs));
    printf("rdx = 0x%016lx    rdi = 0x%016lx    es = 0x%016lx\n\n", *get_register_by_name("rdx", regs), *get_register_by_name("rdi", regs), *get_register_by_name("es", regs));
    return;
}
void debugger::register_read_execution(const std::string& reg_name) {
    if("all" == reg_name) {
        registers_dump();
        return;
    }

    uint64_t value;
    if(!register_read(reg_name, value)) {
        return;
    }
    printf("%s = %lu (0x%016lx)\n", reg_name.c_str(), value, value);
}

bool debugger::register_write(const std::string& reg_name, uint64_t value) {
    struct user_regs_struct regs;
    if(!ptrace_wrapper::get_register(pid_, regs)) return false;

    uint64_t* regs_value = get_register_by_name(reg_name, regs);
    if(!regs_value) return false;
    *regs_value = value;
    return ptrace_wrapper::set_register(pid_, regs);
}
void debugger::register_write_execution(const std::string& reg_name, uint64_t value) {
    register_write(reg_name, value);
}
void debugger::memory_read_execution(uintptr_t addr) {
    uint64_t value;
    if(!ptrace_wrapper::peek_data(pid_, addr, value)) return;
    printf("0x%016lx: 0x%016lx (%lu)\n", addr, value, value);
}
void debugger::memory_watch_execution(uintptr_t addr, uint8_t size) {
    int index;
    for(index = 0; index < 4; index++) {
        if(!memory_breakpoint_list_[index])
            break;
    }
    if(4 == index) {
        printf("Too much memory breakpoints!\n");
        return;
    }
    memory_breakpoint_list_[index].reset(new hardware_breakpoint(pid_, addr, static_cast<hardware_breakpoint::DEBUGGER_REGISTER>(index),
                                                                 static_cast<hardware_breakpoint::WATCH_SIZE>(size)));
    memory_breakpoint_list_[index]->enable();
}
void debugger::memory_write_execution(uintptr_t addr, uint64_t value) {
    ptrace_wrapper::poke_data(pid_, addr, value);
}
void debugger::step_over() {
    ptrace_wrapper::single_step(pid_);
    int wstatus;
    waitpid(pid_, &wstatus, 0);
    //check_wait_status(pid_, wstatus, false);
}
void debugger::handle_command(const std::string& line) {
    auto args = split(line,' ');
    if(args.empty() || args[0].empty()) return;
    std::string command = args[0];

    if ("c" == command) {
        continue_execution();
        return;
    }
    else if ("b" == command && args.size() == 2) {
        uintptr_t addr = str2hex(args[1]);
        breakpoint_execution(addr);
        return;
    }
    else if ("q" == command) {
        quit_execution();
        return;
    }
    else if ("reg" == command && args.size() >= 3) {
        command = args[1];
        std::string reg_name = args[2];
        if ("read" == command && args.size() == 3) {
            register_read_execution(reg_name);
            return;
        }
        if ("write" == command && args.size() == 4) {
            uint64_t value = str2hex(args[3]);
            register_write_execution(reg_name, value);
            return;
        }
    }
    else if ("mem" == command && args.size() >= 3) {
        command = args[1];
        uintptr_t addr = str2hex(args[2]);
        if ("read" == command && args.size() == 3) {
            memory_read_execution(addr);
            return;
        }
        if ("watch" == command && args.size() == 4) {
            uint64_t size = str2hex(args[3]);
            memory_watch_execution(addr, static_cast<uint8_t>(size));
            return;
        }
        if ("write" == command && args.size() == 4) {
            uint64_t value = str2hex(args[3]);
            memory_write_execution(addr, value);
            return;
        }
    }

    printf("Unknown command\n");
}
void debugger::check_wait_status(pid_t pid, int wstatus, bool mute) {
    if(WIFEXITED(wstatus)) {
        if(!mute) printf("Process[%u] terminated normally, status[0x%x]\n", pid, WEXITSTATUS(wstatus));
        pid_terminated_ = true;
    }

    if(WIFSIGNALED(wstatus)) {
        if(!mute) printf("Process[%u] was terminated by a signal [%u]\n", pid, WTERMSIG(wstatus));
        if(WCOREDUMP(wstatus))
            if(!mute) printf("Process[%u] produced a core dump.\n", pid);

        pid_terminated_ = true;
    }

    if(WIFSTOPPED(wstatus)) {
        if(!mute) printf("Process[%u] was stopped by delivery of a signal [%u]\n", pid, WSTOPSIG(wstatus));
    }

    if(WIFCONTINUED(wstatus))
        if(!mute) printf("Process[%u] was resumed by delivery of SIGCONT.\n", pid);
}

uint64_t* debugger::get_register_by_name(const std::string& reg_name, struct user_regs_struct& regs) {
    if("r15" == reg_name) return reinterpret_cast<uint64_t*>(&regs.r15);
    if("r14" == reg_name) return reinterpret_cast<uint64_t*>(&regs.r14);
    if("r13" == reg_name) return reinterpret_cast<uint64_t*>(&regs.r13);
    if("r12" == reg_name) return reinterpret_cast<uint64_t*>(&regs.r12);
    if("rbp" == reg_name) return reinterpret_cast<uint64_t*>(&regs.rbp);
    if("rbx" == reg_name) return reinterpret_cast<uint64_t*>(&regs.rbx);
    if("r11" == reg_name) return reinterpret_cast<uint64_t*>(&regs.r11);
    if("r10" == reg_name) return reinterpret_cast<uint64_t*>(&regs.r10);
    if("r9" == reg_name) return reinterpret_cast<uint64_t*>(&regs.r9);
    if("r8" == reg_name) return reinterpret_cast<uint64_t*>(&regs.r8);
    if("rax" == reg_name) return reinterpret_cast<uint64_t*>(&regs.rax);
    if("rcx" == reg_name) return reinterpret_cast<uint64_t*>(&regs.rcx);
    if("rdx" == reg_name) return reinterpret_cast<uint64_t*>(&regs.rdx);
    if("rsi" == reg_name) return reinterpret_cast<uint64_t*>(&regs.rsi);
    if("rdi" == reg_name) return reinterpret_cast<uint64_t*>(&regs.rdi);
    if("orig_rax" == reg_name) return reinterpret_cast<uint64_t*>(&regs.orig_rax);
    if("rip" == reg_name) return reinterpret_cast<uint64_t*>(&regs.rip);
    if("cs" == reg_name) return reinterpret_cast<uint64_t*>(&regs.cs);
    if("eflags" == reg_name) return reinterpret_cast<uint64_t*>(&regs.eflags);
    if("rsp" == reg_name) return reinterpret_cast<uint64_t*>(&regs.rsp);
    if("ss" == reg_name) return reinterpret_cast<uint64_t*>(&regs.ss);
    if("fs_base" == reg_name) return reinterpret_cast<uint64_t*>(&regs.fs_base);
    if("gs_base" == reg_name) return reinterpret_cast<uint64_t*>(&regs.gs_base);
    if("ds" == reg_name) return reinterpret_cast<uint64_t*>(&regs.ds);
    if("es" == reg_name) return reinterpret_cast<uint64_t*>(&regs.es);
    if("fs" == reg_name) return reinterpret_cast<uint64_t*>(&regs.fs);
    if("gs" == reg_name) return reinterpret_cast<uint64_t*>(&regs.gs);
    printf("Wrong name of register[%s]\n", reg_name.c_str());
    return nullptr;
}

