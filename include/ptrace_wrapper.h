#pragma once
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <stdint.h>

class ptrace_wrapper {
public:
    static bool set_options(pid_t pid, uintptr_t options);
    static bool trace_me(pid_t pid);
    static bool continue_process(pid_t pid);
    static bool peek_data(pid_t pid, uintptr_t addr, uint64_t& data);
    static bool poke_data(pid_t pid, uintptr_t addr, uint64_t data);
    static bool single_step(pid_t pid);
    static bool get_register(pid_t pid, struct user_regs_struct& regs);
    static bool set_register(pid_t pid, const struct user_regs_struct& regs);
    static bool peek_user(pid_t pid, uintptr_t offset, uint64_t& data);
    static bool poke_user(pid_t pid, uintptr_t offset, uint64_t data);
};

