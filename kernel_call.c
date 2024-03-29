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

    // Error check if argvec has valid pointers
    int i = 0;
    int flag = 0;
    int page_iter = (int)(((long) argvec) >> PAGESHIFT);
    struct pte* TempVir2phy = TempVirtualMem;
	while (flag == 0) {
        PhysAddr_map_VirAddr(active_proc->pt_r0);
		if ((!TempVir2phy[page_iter].kprot & PROT_READ) || !TempVir2phy[page_iter].valid) {
			MyExit(ERROR);
		}
		while (i * sizeof(char *) < ((page_iter + 1) << PAGESHIFT) - (long)argvec) {
			if (argvec[i] == NULL) {
				flag = 1;
				break;
			}
			i++;
		}
		page_iter++;
	}
    TracePrintf(0, "Exec: pass check\n");

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
    if(readyQue_pcb.next == NULL && delayQue_pcb.next == NULL && waitQue_pcb.next == NULL){
        // Handle the terminal process
        int isAllEmpty = 1;
        int tty_id;
        for(tty_id=0; tty_id<NUM_TERMINALS; tty_id++){
            if(terminals[tty_id].read_pcb.next || terminals[tty_id].write_pcb.next){
                isAllEmpty = 0;
                break;
            }
        }
        if(isAllEmpty){
            TracePrintf(0, "Enter MyExit (%d), Every process is empty\n", status);
            printf("Enter MyExit (%d), Every process is empty\n", status);
            Halt();
        }
    }
    if(active_proc->num_child > 0){
        RemoveParent(active_proc, &readyQue_pcb);
        RemoveParent(active_proc, &delayQue_pcb);
        RemoveParent(active_proc, &waitQue_pcb);
    }
    // TracePrintf(0, "active_proc->parent = 0x%lx\n", active_proc->parent);
    if(active_proc->parent){
        AppendStatus(&active_proc->parent->statusQueue, status);
        TracePrintf(0, "Enter Parent part\n");
        pcb *node = &waitQue_pcb;
        pcb *tmp;
        while(node && node->next){
            node = node->next;
            if(node->pid == active_proc->parent->pid){
                RemovePCB(node);
                AddPCB(node, &readyQue_pcb);
                node = tmp;
            }
        }
        // ContextSwitch(Exit_SwitchFunc, &active_proc->ctx, active_proc, active_proc->parent);
        ContextSwitch(Exit_SwitchFunc, &active_proc->ctx, active_proc, readyQue_pcb.next);
    }
    else{
        TracePrintf(0, "Just Exit\n");
        ContextSwitch(Exit_SwitchFunc, &active_proc->ctx, active_proc, readyQue_pcb.next);
    }
    return;
}

int MyWait(int *status_ptr){
    TracePrintf(0, "Enter MyWait\n");
    if(active_proc->num_child == 0){
        return ERROR;
    }
    else if(active_proc->statusQueue.next == NULL){
        ContextSwitch(Wait_SwitchFunc, &active_proc->ctx, active_proc, readyQue_pcb.next);
    }
    
    // Pop status queue
    struct Status *tmp_status = active_proc->statusQueue.next;
    active_proc->statusQueue.next = active_proc->statusQueue.next->next;
    --active_proc->num_child;
    unsigned int return_pid = tmp_status->pid;
    *status_ptr = tmp_status->status;
    free(tmp_status);
    return return_pid;
}

int MyGetPid(void){
    TracePrintf(0, "Enter MyGetPid\n");
    return active_proc->pid;
}

int MyBrk(void *addr){
    TracePrintf(0, "Enter MyBrk: Brk(0x%lx),    active_proc->brk = 0x%lx\n", addr, active_proc->brk);
    int desired_npg = (UP_TO_PAGE(addr) - (unsigned long)active_proc->brk) >> PAGESHIFT;
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

    // Check buffer bits & validity
    int page_iter;
	int prot_bit = PROT_WRITE;
    struct pte* TempVir2phy = TempVirtualMem;
    for (page_iter = (int)(((long)buf)>>PAGESHIFT); page_iter < (int)(UP_TO_PAGE((long)buf + len)>>PAGESHIFT); page_iter++) {
        PhysAddr_map_VirAddr(active_proc->pt_r0);
        if (!(TempVir2phy[page_iter].kprot & prot_bit) || !TempVir2phy[page_iter].valid) {
        	fprintf(stderr, "TtyRead: invalid buffer.\n");
        	MyExit(ERROR);
        }
    }

    if(tty_id >= NUM_TERMINALS || len > TERMINAL_MAX_LINE){
        return ERROR;
    }
    if(terminals[tty_id].read_buff.next == NULL){
        // Wait for a new read buffer
        Push_TerminalReadPCB(tty_id, active_proc);
        ContextSwitch(Tty_SwitchFunc, &active_proc->ctx, active_proc, readyQue_pcb.next);
    }
    // Have read buffer
    int readLength;
    if(terminals[tty_id].read_buff.next->count > len){
        memcpy(buf, terminals[tty_id].read_buff.next->text + terminals[tty_id].read_buff.next->cur_pos, len);
        readLength = len;
        terminals[tty_id].read_buff.next->cur_pos += len;
        terminals[tty_id].read_buff.next->count -= len;
    }
    else{
        memcpy(buf, terminals[tty_id].read_buff.next->text + terminals[tty_id].read_buff.next->cur_pos, 
                terminals[tty_id].read_buff.next->count);
        readLength = terminals[tty_id].read_buff.next->count;
        struct buff *tmp = Pop_TerminalBUFF(&terminals[tty_id].read_buff);
        free(tmp);
    }
    return readLength;
}

int MyTtyWrite(int tty_id, void *buf, int len){
    TracePrintf(0, "Enter MyTtyWrite\n");

    // Check buffer bits & validity
    int page_iter;
	int prot_bit = PROT_READ;
    struct pte* TempVir2phy = TempVirtualMem;
    for (page_iter = (int)(((long)buf)>>PAGESHIFT); page_iter < (int)(UP_TO_PAGE((long)buf + len)>>PAGESHIFT); page_iter++) {
        PhysAddr_map_VirAddr(active_proc->pt_r0);
        if (!(TempVir2phy[page_iter].kprot & prot_bit) || !TempVir2phy[page_iter].valid) {
        	fprintf(stderr, "TtyWrite: invalid buffer.\n");
        	MyExit(ERROR);
        }
    }

    if(tty_id >= NUM_TERMINALS || len > TERMINAL_MAX_LINE){
        return ERROR;
    }

    // Copy chars into write_buffer
    buff *write_buffer = calloc(1, sizeof(struct buff));
    write_buffer->count = len;
    write_buffer->cur_pos = 0;
    memcpy(write_buffer->text, buf, len);
    Push_TerminalBUFF(&terminals[tty_id].write_buff, write_buffer);

    // Put active_proc into write_pcb queue
    Push_TerminalWritePCB(tty_id, active_proc);
    if(terminals[tty_id].write_pcb.next == active_proc){
        TtyTransmit(tty_id, write_buffer->text, len);
    }

    // Switch to next ready process
    ContextSwitch(Tty_SwitchFunc, &active_proc->ctx, active_proc, readyQue_pcb.next);
    return len;
}