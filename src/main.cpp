#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <debugger.h>
#include <ptrace_wrapper.h>
#include <sys/personality.h>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Program name not specified\n");
        return -1;
    }
    const char* debugee = argv[1];
    pid_t pid = fork();
    if(pid == 0)
    {
        personality(ADDR_NO_RANDOMIZE);
        ptrace_wrapper::trace_me(0);
        execl(debugee, debugee, nullptr);
    }
    else if(pid != 0)
    {
        debugger db(pid, debugee);
        db.run();
    }
    return 0;
}

