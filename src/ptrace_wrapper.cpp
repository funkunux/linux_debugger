#include <ptrace_wrapper.h>
#include <stdio.h>      //perror
#include <errno.h>

bool ptrace_wrapper::set_options(pid_t pid, uintptr_t options) {
    if(0 != ptrace(PTRACE_SETOPTIONS, pid, nullptr, options)) {
        fprintf(stderr, "Failed to set options[0x%016lx] to pid[%d]: ", options, pid);
        perror("");
        return false;
    }
    return true;
}
bool ptrace_wrapper::trace_me(pid_t pid) {
    if(0 != ptrace(PTRACE_TRACEME, pid, nullptr, nullptr)) {
        fprintf(stderr, "Failed to request PTRACE_TRACEME on pid[%d]: ", pid);
        perror("");
        return false;
    }
    return true;
}
bool ptrace_wrapper::continue_process(pid_t pid) {
    if(0 != ptrace(PTRACE_CONT, pid, nullptr, nullptr)) {
        fprintf(stderr, "Failed to continue pid[%d]: ", pid);
        perror("");
        return false;
    }
    return true;
}
bool ptrace_wrapper::peek_data(pid_t pid, uintptr_t addr, uint64_t& data) {
    errno = 0;
    long ret = ptrace(PTRACE_PEEKDATA, pid, addr, nullptr);
    if(-1 == ret && errno != 0) {
        fprintf(stderr, "Failed to peek data of addr[0x%lx] on pid[%d]: ", addr, pid);
        perror("");
        return false;
    }
    data = ret;
    return true;
}
bool ptrace_wrapper::poke_data(pid_t pid, uintptr_t addr, uint64_t data) {
    errno = 0;
    long ret = ptrace(PTRACE_POKEDATA, pid, addr, data);
    if(-1 == ret && errno != 0) {
        fprintf(stderr, "Failed to poke data[0x%016lx] of addr[0x%016lx] on pid[%d]: ", data, addr, pid);
        perror("");
        return false;
    }
    return true;
}
bool ptrace_wrapper::single_step(pid_t pid) {
    if(0 != ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr)) {
        fprintf(stderr, "Failed to set single step on pid[%d]: ", pid);
        perror("");
        return false;
    }
    return true;
}
bool ptrace_wrapper::get_register(pid_t pid, struct user_regs_struct& regs) {
    if(0 != ptrace(PTRACE_GETREGS, pid, nullptr, &regs)) { 
        fprintf(stderr, "Failed to get registers of pid[%d]: ", pid);
        perror("");
        return false;
    }
    return true;
}
bool ptrace_wrapper::set_register(pid_t pid, const struct user_regs_struct& regs) {
    if(0 != ptrace(PTRACE_SETREGS, pid, nullptr, &regs)) {
        fprintf(stderr, "Failed to set registers of pid[%d]: ", pid);
        perror("");
        return false;
    }
    return true;
}
bool ptrace_wrapper::peek_user(pid_t pid, uintptr_t offset, uint64_t& data) {
    errno = 0;
    long ret = ptrace(PTRACE_PEEKUSER, pid, offset, nullptr);
    if(-1 == ret && errno != 0) {
        fprintf(stderr, "Failed to peek data at offset[%lu] of struct user of pid[%d]: ", offset, pid);
        perror("");
        return false;
    }
    data = ret;
    return true;
}
bool ptrace_wrapper::poke_user(pid_t pid, uintptr_t offset, uint64_t data) {
    errno = 0;
    long ret = ptrace(PTRACE_POKEUSER, pid, offset, data);
    if(-1 == ret && errno) {
        fprintf(stderr, "Failed to poke data[0x%016lx] at offset[%lu] of struct user of pid[%d]: ", data, offset, pid);
        perror("");
        return false;
    }
    return true;
}

