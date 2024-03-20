#include <comp421/hardware.h>

#include "handler.h"


void TrapKernelHandler(ExceptionInfo *info){
    TracePrintf(0, "Start trap kernel\n");
    Halt();
}

void TrapClockHandler(ExceptionInfo *info){
    TracePrintf(0, "Start trap clock\n");
    Halt();
}
void TrapIllegalHandler(ExceptionInfo *info){
    TracePrintf(0, "Start trap illegal\n");
    Halt();
}

void TrapMemoryHandler(ExceptionInfo *info){
    TracePrintf(0, "Start trap memory\n");
    Halt();
}

void TrapMathHandler(ExceptionInfo *info){
    TracePrintf(0, "Start trap math\n");
    Halt();
}

void TrapTTY_ReceiveHandler(ExceptionInfo *info){
    TracePrintf(0, "Start trap tty receive\n");
    Halt();
}

void TrapTTY_TransmitHandler(ExceptionInfo *info){
    TracePrintf(0, "Start trap tty transmit\n");
    Halt();
}