#include <comp421/hardware.h>

#include "kernel_call.h"
#include "kernel.h"
#include "handler.h"


void TrapKernelHandler(ExceptionInfo *info){
    TracePrintf(0, "Start trap kernel\n");
    switch (info->code){
        case YALNIX_GETPID:
            info->regs[0] = MyGetPid();
            break;
        case YALNIX_DELAY:
            info->regs[0] = MyDelay((int)(info->regs[1]));
            break;
        case YALNIX_BRK:
            info->regs[0] = MyBrk((void *)(info->regs[1]));
            break;
        case YALNIX_FORK:
            info->regs[0] = MyFork();
            break;
        case YALNIX_EXEC:
            MyExec((char *)(info->regs[1]), (char **)(info->regs[2]), info);
            break;
        case YALNIX_EXIT:
            MyExit((int)(info->regs[1]));
            break;
        case YALNIX_WAIT:
            info->regs[0] = MyWait((int *)(info->regs[1]));
            break;
        case YALNIX_TTY_READ:
            info->regs[0] = MyTtyRead((int)(info->regs[1]), (void *)(info->regs[2]), (int)(info->regs[3]));
            break;
        case YALNIX_TTY_WRITE:
            info->regs[0] = MyTtyWrite((int)(info->regs[1]), (void *)(info->regs[2]), (int)(info->regs[3]));
            break;
        default:
            break;
    }
}

void TrapClockHandler(ExceptionInfo *info){
    TracePrintf(0, "Start trap clock\n");
    // Update passed ticks of current process
    if(active_proc->pid > 0){
        active_proc->pass_tick++;
    }

    // Check delay pcb's time is up
    pcb *node = &delay_proc;
    pcb *tmp;
    while(node && node->next){
        node = node->next;
        node->delay_clock--;
        // TracePrintf(0, "node->delay_clock = %d\n", node->delay_clock);
        if(node->delay_clock <= 0){
            TracePrintf(0, "PID = %d time's up!\n", node->pid);
            tmp = node->next;
            RemovePCB(node);
            AddPCB(node, &ready_proc);
            node = tmp;
            break;
        }
    }
    // Context switch if the current process has used 2 ticks
    if ((active_proc->pid == 0 || active_proc->pass_tick >= 2) && ready_proc.next)
    {
        TracePrintf(0, "clock switch from pid %d to pid %d\n", active_proc->pid, ready_proc.next->pid);
        active_proc->pass_tick = 0;
        ContextSwitch(SwitchFunc, &active_proc->ctx, active_proc, ready_proc.next);
    }
}
void TrapIllegalHandler(ExceptionInfo *info){
    TracePrintf(0, "Start trap illegal\n");
    Halt();
//     switch (info->code){
//         case TRAP_ILLEGAL_ILLOPC:
//             printf("Illegal opcode\n");
//             break;
//         case TRAP_ILLEGAL_ILLOPN:
//             printf("Illegal operand\n");
//             break;
//         case TRAP_ILLEGAL_ILLADR:
//             printf("Illegal addressing mode\n");
//             break;
//         case TRAP_ILLEGAL_ILLTRP:
//             printf("Illegal software trap\n");
//             break;
//         case TRAP_ILLEGAL_PRVOPC:
//             printf("Privileged opcode\n");
//             break;
//         case TRAP_ILLEGAL_PRVREG:
//             printf("Illegal register\n");
//             break;
//         case TRAP_ILLEGAL_COPROC:
//             printf("Coprocessor error\n");
//             break;
//         case TRAP_ILLEGAL_BADSTK:
//             printf("Bad stack\n");
//             break;
//         case TRAP_ILLEGAL_KERNELI:
//             printf("Linux kernel sent SIGILL\n");
//             break;
//         case TRAP_ILLEGAL_USERIB:
//             printf("Received SIGILL from user\n");
//             break;
//         case TRAP_ILLEGAL_ADRALN:
//             printf("Invalid address alignment\n");
//             break;
//         case TRAP_ILLEGAL_ADRERR:
//             printf("Non-existant physical address\n");
//             break;
//         case TRAP_ILLEGAL_OBJERR:
//             printf("Object-specific HW error\n");
//             break;
//         case TRAP_ILLEGAL_KERNELB:
//             printf("Linux kernel sent SIGBUS\n");
//             break;
//         default:
//             break;
//   }
//   MyExit(ERROR);
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