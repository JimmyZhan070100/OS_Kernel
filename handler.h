#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <bits/siginfo.h>

typedef void (*interrupt_hdr)(ExceptionInfo *);

void TrapKernelHandler(ExceptionInfo *);

void TrapClockHandler(ExceptionInfo *);

void TrapIllegalHandler(ExceptionInfo *);

void TrapMemoryHandler(ExceptionInfo *);

void TrapMathHandler(ExceptionInfo *);

void TrapTTY_ReceiveHandler(ExceptionInfo *);

void TrapTTY_TransmitHandler(ExceptionInfo *);