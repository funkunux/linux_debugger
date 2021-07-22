#pragma once
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>

#define OFFSET_OF(STRUCT, MEMBER) ((void*)(&((STRUCT*)0)->MEMBER))
#define OFFSET_OF_DR(NUM) (OFFSET_OF(struct user, u_debugreg[NUM]))

class breakpoint {
public:
    breakpoint(pid_t pid, uintptr_t addr) : pid_(pid), position_(addr) {}
    virtual ~breakpoint() = default;
    virtual bool enable() = 0;
    virtual bool disable() = 0;
    virtual bool is_enable() { return enabled_; }
protected:
    const pid_t pid_;
    const uintptr_t position_;
    bool enabled_ = false;
};

class gerneric_breakpoint : public breakpoint
{
public:
    gerneric_breakpoint(pid_t pid, uintptr_t addr) : breakpoint(pid, addr) {}
    bool enable() override;
    bool disable() override;
private:
    const uint8_t INT3 = 0xCC;
    uint64_t original_data_ = 0;
};

class hardware_breakpoint : public breakpoint {
public:
    enum DEBUGGER_REGISTER { 
        kDR0 = 0,
        kDR1 = 1,
        kDR2 = 2,
        kDR3 = 3,
        kDR6 = 6,
        kDR7 = 7
    };
    enum TRIGGER_MODE {
        kExcution = 0,
        kWrite = 1,
        kRreadOrWrite = 3 
    };
    enum WATCH_SIZE {
        k8Bit = 0,
        k16Bit = 1,
        k32Bit = 3,
        k64Bit = 2,
    };
    hardware_breakpoint(pid_t pid, uintptr_t addr, DEBUGGER_REGISTER dr_num, uint8_t size, TRIGGER_MODE mode = kWrite);
    bool enable() override ;
    bool disable() override;
private:
    void print_error() {
        printf("Failed to watch memory[0x%016lx]\n", position_);
    }
    bool validate();
    DEBUGGER_REGISTER dr_num_;
    WATCH_SIZE watch_size_;
    TRIGGER_MODE trigger_mode_;
};


