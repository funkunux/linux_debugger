#include <breakpoint.h>
#include <ptrace_wrapper.h>

bool gerneric_breakpoint::enable() {
    if(enabled_) return true;

    if(!ptrace_wrapper::peek_data(pid_, position_, original_data_)) return false;
    //printf("Peek data[0x%lx] from addr[0x%lx]\n", original_data_, position_);
    uint64_t data_with_int3 = ((original_data_ & (~0xff)) | INT3);

    if(!ptrace_wrapper::poke_data(pid_, position_, data_with_int3)) return false;
    enabled_ = true;
    return true;
}
bool gerneric_breakpoint::disable() {
    if(!enabled_) return true;

    if(!ptrace_wrapper::poke_data(pid_, position_, original_data_)) return false;
    enabled_ = false;
    return true;
}

hardware_breakpoint::hardware_breakpoint(pid_t pid, uintptr_t addr, DEBUGGER_REGISTER dr_num, uint8_t size, TRIGGER_MODE mode)
    : breakpoint(pid, addr), dr_num_(dr_num), trigger_mode_(mode) {
    switch(size) {
        case 1:
            watch_size_ = k8Bit;
            break;
        case 2:
            watch_size_ = k16Bit;
            break;
        case 4:
            watch_size_ = k32Bit;
            break;
        case 8:
            watch_size_ = k64Bit;
            break;
        default:
            watch_size_ = static_cast<WATCH_SIZE>(-1);
            break;
    }
}
bool hardware_breakpoint::enable() {
    if(!validate()) {
        printf("Invalid params for hardware breakpoint!\n");
        print_error();
        return false;
    }
    uint64_t offset = reinterpret_cast<uint64_t>(OFFSET_OF_DR(dr_num_));
    if(!ptrace_wrapper::poke_user(pid_, offset, position_)) {
        print_error();
        return false;
    }

    uint64_t debug_info;
    offset = reinterpret_cast<uint64_t>(OFFSET_OF_DR(kDR7));
    if(!ptrace_wrapper::peek_user(pid_, offset, debug_info)) {
        print_error();
        return false;
    }
    //printf("Origin data of DR[7]:0x%08lx\n", ret);
    debug_info |= (1 << (dr_num_ * 2) | (trigger_mode_ << (16 + dr_num_ * 4)) | (watch_size_ << (dr_num_ * 4 + 18)));
    //printf("Try to poke data[0x08%lx] to DR[%d], offset[%lu]!\n", debug_info, kDR7, OFFSET_OF_DR(kDR7));
    if(!ptrace_wrapper::poke_user(pid_, offset, debug_info)) {
        print_error();
        return false;
    }

    return true;
}
bool hardware_breakpoint::disable() { return false;}

bool hardware_breakpoint::validate() {
    if(dr_num_ > kDR3)
        return false;
    switch(trigger_mode_) {
        case kExcution:
        case kWrite:
        case kRreadOrWrite:
            break;
        default:
            return false;
    }
    switch(watch_size_) {
        case k8Bit:
        case k16Bit:
        case k32Bit:
        case k64Bit:
            break;
        default:
            return false;
    }
    return true;
}

