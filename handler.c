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
        case YALNIX_FORK:
            info->regs[0] = MyFork();
            break;
        case YALNIX_DELAY:
            info->regs[0] = MyDelay((int)(info->regs[1]));
            break;
        case YALNIX_BRK:
            info->regs[0] = MyBrk((void *)(info->regs[1]));
            break;
        case YALNIX_EXEC:
            MyExec((char *)(info->regs[1]), (char **)(info->regs[2]), info);
            break;
        case YALNIX_WAIT:
            info->regs[0] = MyWait((int *)(info->regs[1]));
            break;
        case YALNIX_EXIT:
            MyExit((int)(info->regs[1]));
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
            // break;
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
    switch (info->code){
        case TRAP_ILLEGAL_ILLOPC:
            printf("Illegal opcode\n");
            break;
        case TRAP_ILLEGAL_ILLOPN:
            printf("Illegal operand\n");
            break;
        case TRAP_ILLEGAL_ILLADR:
            printf("Illegal addressing mode\n");
            break;
        case TRAP_ILLEGAL_ILLTRP:
            printf("Illegal software trap\n");
            break;
        case TRAP_ILLEGAL_PRVOPC:
            printf("Privileged opcode\n");
            break;
        case TRAP_ILLEGAL_PRVREG:
            printf("Illegal register\n");
            break;
        case TRAP_ILLEGAL_COPROC:
            printf("Coprocessor error\n");
            break;
        case TRAP_ILLEGAL_BADSTK:
            printf("Bad stack\n");
            break;
        case TRAP_ILLEGAL_KERNELI:
            printf("Linux kernel sent SIGILL\n");
            break;
        case TRAP_ILLEGAL_USERIB:
            printf("Received SIGILL from user\n");
            break;
        case TRAP_ILLEGAL_ADRALN:
            printf("Invalid address alignment\n");
            break;
        case TRAP_ILLEGAL_ADRERR:
            printf("Non-existant physical address\n");
            break;
        case TRAP_ILLEGAL_OBJERR:
            printf("Object-specific HW error\n");
            break;
        case TRAP_ILLEGAL_KERNELB:
            printf("Linux kernel sent SIGBUS\n");
            break;
        default:
            printf("Happend TRAP_ILLEGAL\n");
            break;
  }
  MyExit(ERROR);
}

void TrapMemoryHandler(ExceptionInfo *info){
    TracePrintf(0, "Start trap memory\n");
    TracePrintf(0, "info->addr = 0x%lx\n", info->addr);
    TracePrintf(0, "info->sp = 0x%lx\n", info->sp);
    TracePrintf(0, "active_proc->pid = %d\n", active_proc->pid);
    int pg;
    unsigned long addr_pg = (unsigned long)info->addr >> PAGESHIFT;
    unsigned long cur_pg = (unsigned long)active_proc->stack_base >> PAGESHIFT;
    TracePrintf(0, "active_proc->stack_base = 0x%lx, cur_pg = %lu\n", active_proc->stack_base, cur_pg);
    TracePrintf(0, "active_proc->brk = 0x%lx\n", active_proc->brk);
    if(info->addr < active_proc->stack_base && info->addr > active_proc->brk && CheckPhysFrame(cur_pg - addr_pg)){
        for(pg = (unsigned long)active_proc->stack_base - 1 >> PAGESHIFT; pg >= addr_pg; pg--){
            PhysAddr_map_VirAddr(active_proc->pt_r0);
            struct pte *tmp = (struct pte *)TempVirtualMem;
            tmp[pg].valid = 1;
            tmp[pg].kprot = PROT_READ | PROT_WRITE;
            tmp[pg].uprot = PROT_READ | PROT_WRITE;
            tmp[pg].pfn = GetFreeFrame();
            // TracePrintf(3, "tmp[pg].pfn = 0x%lx\n", tmp[pg].pfn);
        }
        active_proc->stack_base = info->addr;
    }
    else{
        TracePrintf(0, "Not enough Memory\n");
        return MyExit(ERROR);
    }
}

void TrapMathHandler(ExceptionInfo *info){
    TracePrintf(0, "Start trap math\n");
    switch (info->code){
        case TRAP_MATH_INTDIV:
            printf("Integer divide by zero \n");
            break;
        case TRAP_MATH_INTOVF:
            printf("Integer overflow \n");
            break;
        case TRAP_MATH_FLTDIV:
            printf("Floating divide by zero \n");
            break;
        case TRAP_MATH_FLTOVF:
            printf("Floating overflow \n");
            break;
        case TRAP_MATH_FLTUND:
            printf("Floating underflow \n");
            break;
        case TRAP_MATH_FLTRES:
            printf("Floating inexact result\n");
            break;
        case TRAP_MATH_FLTINV:
            printf("Invalid floating operation \n");
            break;
        case TRAP_MATH_FLTSUB:
            printf("FP subscript out of range \n");
            break;
        case TRAP_MATH_KERNEL:
            printf("Linux kernel sent SIGFPE \n");
            break;
        case TRAP_MATH_USER:
            printf("Received SIGFPE from user \n");
            break;
        default:
            break;
    }
    MyExit(ERROR);
}

void TrapTTY_ReceiveHandler(ExceptionInfo *info){
    TracePrintf(0, "Start trap tty receive\n");

    int tty_id = info->code;
    buff *read_buffer = calloc(1, sizeof(struct buff));
    read_buffer->count = TtyReceive(tty_id, (void *)read_buffer->text, TERMINAL_MAX_LINE);
    read_buffer->cur_pos = 0;

    Push_TerminalBUFF(&terminals[tty_id].read_buff, read_buffer);
    if(terminals[tty_id].read_pcb.next){
        pcb *next_readPCB = Pop_TerminalReadPCB(tty_id);
        AddPCB(next_readPCB, &ready_proc);
    }
}

void TrapTTY_TransmitHandler(ExceptionInfo *info){
    TracePrintf(0, "Start trap tty transmit\n");
    
    int tty_id = info->code;
    pcb *write_proc = Pop_TerminalWritePCB(tty_id);
    AddPCB(write_proc, &ready_proc);
    struct buff *tmp = Pop_TerminalBUFF(&terminals[tty_id].write_buff);
    free(tmp);
    if(terminals[tty_id].write_buff.next){
        TtyTransmit(tty_id, (void *)terminals[tty_id].write_buff.next->text, terminals[tty_id].write_buff.next->count);
    }
    ContextSwitch(SwitchFunc, &active_proc->ctx, active_proc, ready_proc.next);
}