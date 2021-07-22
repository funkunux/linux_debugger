#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>

#define DEBUGGER_DETECTED 0

void* sleep_loop(int time) {
    int value = 0;
    printf("Enter %s\n", __FUNCTION__);
    for(int i = time; i > 0; i--) {
        //asm("INT3");
        printf("Before modify value[%d]\n", value);
        value = i;
        printf("After modify value[%d]\n", value);
        printf("Sleeping in %ds...\n", i);
        sleep(1);
    }
}

int main()
{
#if DEBUGGER_DETECTED
    char buf[512];
    FILE* fin;
    fin = fopen("/proc/self/status", "r");
    int tpid;
    const char *needle = "TracerPid:";
    size_t nl = strlen(needle);
    while(fgets(buf, 512, fin)) {
        if(!strncmp(buf, needle, nl)) {
            sscanf(buf, "TracerPid: %d", &tpid);
            if(tpid != 0) {
                printf("Debugger detected!\n");
                return 1;
            }
        }
    }
    fclose(fin);
#endif
    printf("I'm debugee...\n");
    sleep_loop(5);
    printf("done...\n");
}

