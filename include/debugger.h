#pragma once
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h> //user_regs_struct
#include <stdint.h>
#include <string>
#include <map>
#include <vector>
#include <memory> //unique_ptr
#include <breakpoint.h>

class debugger
{
public:
    debugger(pid_t pid, const char* program) : pid_(pid), program_(program) {
        memory_breakpoint_list_.resize(4);
    }
    virtual ~debugger() = default;
    void run();
private:
    void quit_execution();

    void breakpoint_execution(uintptr_t addr);
    void hit_breakpoint();
    void step_over();

    void continue_execution();

    void register_read_execution(const std::string& reg_name);
    bool register_read(const std::string& reg_name, uint64_t& value);
    void registers_dump();
    void register_write_execution(const std::string& reg_name, uint64_t value);
    bool register_write(const std::string& reg_name, uint64_t value);
    uint64_t* get_register_by_name(const std::string& reg_name, struct user_regs_struct& regs);

    void memory_read_execution(uintptr_t addr);
    void memory_write_execution(uintptr_t addr, uint64_t value);
    void memory_watch_execution(uintptr_t addr, uint8_t size);

    void handle_command(const std::string& line);

    void check_wait_status(pid_t pid, int wstatus, bool mute = true);
private:
    const pid_t pid_;
    const char* program_;
    bool quit_ = false;
    bool pid_terminated_ = false;
    std::map<uint64_t, std::unique_ptr<breakpoint>> breakpoint_list_;
    std::vector<std::unique_ptr<breakpoint>> memory_breakpoint_list_;
};

