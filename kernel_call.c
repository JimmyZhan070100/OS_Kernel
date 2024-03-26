#include <stdlib.h>

#include "kernel_call.h"
#include "kernel.h"

int MyFork(void){
    TracePrintf(0, "Enter MyFork\n");

    // Build child proc with new page table
    pcb *child_proc = calloc(1, sizeof(pcb));
    child_proc->pt_r0 = GetFreeFrame();
    child_proc->pid = process_count++;
    child_proc->brk = active_proc->brk;
    child_proc->parent = active_proc;
    TracePrintf(0, "child_proc->pid = %d\n", child_proc->pid);
    unsigned int parent_pid = active_proc->pid;
    ++active_proc->num_child;
    ContextSwitch(Fork_SwitchFunc, &active_proc->ctx, active_proc, child_proc);
    if(active_proc->pid == parent_pid){
        return child_proc->pid;
    }
    return 0;
}

int MyExec(char *filename, char **argvec, ExceptionInfo *info){
    TracePrintf(0, "Enter MyExec(%s)...\n", filename);
    if (filename == NULL){
        return ERROR;
    }

    int status;
    status = LoadProgram(filename, argvec, info);

    if (status == -1){
        return ERROR;
    }
    if (status == -2){
        MyExit(ERROR);
    }
   return 0;

}

void MyExit(int status){
    TracePrintf(0, "Enter MyExit (%d)...\n", status);
    if(ready_proc.next == NULL && delay_proc.next == NULL && wait_proc.next == NULL){
        /*
         * Handling the terminal proecss
         *
        */
        TracePrintf(0, "Enter MyExit (%d), Every process is empty\n", status);
        Halt();
    }
    if(active_proc->num_child > 0){
        RemoveParent(active_proc, &ready_proc);
        RemoveParent(active_proc, &delay_proc);
        RemoveParent(active_proc, &wait_proc);
    }
    // TracePrintf(0, "active_proc->parent = 0x%lx\n", active_proc->parent);
    if(active_proc->parent){
        // AppendStatus(active_proc->parent->statusQueue, status);
        // ContextSwitch(Exit_SwitchFunc, &active_proc->ctx, active_proc, active_proc->parent);
        ContextSwitch(Exit_SwitchFunc, &active_proc->ctx, active_proc, ready_proc.next);
    }
    else{
        TracePrintf(0, "Enter Here\n");
        ContextSwitch(Exit_SwitchFunc, &active_proc->ctx, active_proc, ready_proc.next);
    }
    return;
}

int MyWait(int *status_ptr){
    TracePrintf(0, "Enter MyWait\n");
    if(active_proc->num_child == 0){
        return ERROR;
    }
    else if(active_proc->statusQueue->next == NULL){
        ContextSwitch(Wait_SwitchFunc, &active_proc->ctx, active_proc, ready_proc.next);
    }
    
    // Pop status queue
    struct Status *tmp_status = active_proc->statusQueue->next;
    active_proc->statusQueue->next = active_proc->statusQueue->next->next;
    --active_proc->num_child;
    unsigned int rtn_pid = tmp_status->pid;
    *status_ptr = tmp_status->status;
    free(tmp_status);
    return rtn_pid;
}

int MyGetPid(void){
    TracePrintf(0, "Enter MyGetPid\n");
    return active_proc->pid;
}

int MyBrk(void *addr){
    TracePrintf(0, "Enter MyBrk: Brk(0x%lx),    active_proc->brk = 0x%lx\n", addr, active_proc->brk);
    int desired_npg = (UP_TO_PAGE(addr) - (int)active_proc->brk) >> PAGESHIFT;
    if(!CheckPhysFrame(desired_npg)){
        TracePrintf(0, "Not enough physical frames\n");
        return ERROR;
    }
    unsigned long active_pfn = (unsigned long)active_proc->brk >> PAGESHIFT;
    GetfreePhysicalAddr(active_pfn, active_pfn + desired_npg,
                PROT_READ | PROT_WRITE, PROT_READ | PROT_WRITE, active_proc->pt_r0);
    active_proc->brk = addr;
    return 0;
}

int MyDelay(int clock_ticks){
    TracePrintf(0, "Enter MyDelay: Delay(%d)\n", clock_ticks);
    if(clock_ticks < 0){
        return ERROR;
    }
    else if(clock_ticks == 0){
        TracePrintf(0, "MyDelay clock_ticks=0\n");
        return 0;
    }
    active_proc->delay_clock = clock_ticks;
    ContextSwitch(Delay_SwitchFunc, &active_proc->ctx, active_proc, NULL);
    return 0;
}

int MyTtyRead(int tty_id, void *buf, int len){
    TracePrintf(0, "Enter MyTtyRead\n");
    return 0;
}

int MyTtyWrite(int tty_id, void *buf, int len){
    TracePrintf(0, "Enter MyTtyWrite\n");
    return 0; 
}