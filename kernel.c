#include "kernel.h"

unsigned long TempVirtualMem;

PhysFrame *physFrame_head;
int free_frame_count = 0;
unsigned long next_PT_validAddr = VMEM_1_LIMIT - 2*PAGESIZE;

struct pte *pt_r1;
struct pte *pt_r0;

unsigned int process_count = 0;
pcb *active_proc;
pcb *idle_proc;
pcb *init_proc;
pcb delay_proc;
pcb wait_proc;
pcb ready_proc;

Terminal terminals[NUM_TERMINALS];

// =============================================================================
// Allocate Frame Functions
// =============================================================================


unsigned long GetFreeFrame(){
    PhysFrame *tmp = physFrame_head;
    physFrame_head = tmp->next;
    unsigned long freeAddr = tmp->addr;
    free(tmp);
    free_frame_count--;
    return freeAddr;
}

void FreeFrame(unsigned long addr){
    PhysFrame *new_frame = malloc(sizeof(PhysFrame));
    new_frame->addr = addr;
    new_frame->next = physFrame_head;
    physFrame_head = new_frame;
    free_frame_count++;
}

void GetfreePhysicalAddr(unsigned long begin, unsigned long end, unsigned int kprot, unsigned uprot, struct pte* cur_ptr0){
    // TracePrintf(0, "In GetfreePhysicalAddr function\n");
    unsigned long page;
    struct pte* TempVir2phy = TempVirtualMem;
    pcb* p1pcb = (pcb *)(cur_ptr0);
    pt_r1[(TempVirtualMem-VMEM_1_BASE) >> PAGESHIFT].pfn = (unsigned long)(cur_ptr0) >> PAGESHIFT;
    WriteRegister(REG_TLB_FLUSH, TempVirtualMem);
    for(page = begin; page < end; page++){
        // TracePrintf(0, "Page address = 0x%lx\n", page);
        TempVir2phy[page].valid = 1;
        // TracePrintf(0, "TempVir2phy[page] = 0x%lx\n", TempVir2phy[page].valid);
        TempVir2phy[page].kprot = kprot;
        TempVir2phy[page].uprot = uprot;
        TempVir2phy[page].pfn = GetFreeFrame();
    }
}

void FreePhysicalAddr(unsigned long begin, unsigned long end, struct pte* cur_ptr0){
    // TracePrintf(0, "In FreePhysicalAddr function\n");
    unsigned long new_brake = UP_TO_PAGE(end);
    unsigned long old_brake = UP_TO_PAGE(begin);
    unsigned long page;
    struct pte* TempVir2phy = TempVirtualMem;
    pcb* p1pcb = (pcb *)(cur_ptr0);
    pt_r1[(TempVirtualMem-VMEM_1_BASE) >> PAGESHIFT].pfn = (unsigned long)(cur_ptr0) >> PAGESHIFT;
    for(page = old_brake; page < new_brake; page+=PAGESIZE){
        if(TempVir2phy[page].valid == 1){
            FreeFrame(TempVir2phy[page].pfn);
        }
        TempVir2phy[page].valid = 0;
    }
}

int CheckPhysFrame(int req_count){
    // TracePrintf(0, "free_frame_count = %d vs req_count = %d\n", free_frame_count, req_count);
    return free_frame_count >= req_count;
}

// =============================================================================
// PCB Functions
// =============================================================================


void AddPCB(pcb *cur, pcb *queHead){
    // TracePrintf(0, "AddPCB: cur = 0x%lx, queHead = 0x%lx\n", cur, *queHead);
    while (queHead->next){
        queHead = queHead->next;
    }
    queHead->next = cur;
    cur->prev = queHead;
    cur->next = NULL;
    // TracePrintf(0, "Finish AddPCB\n");
}

pcb *PopPCB(pcb *que){
    pcb *temp = que->next;
    if(temp != NULL){
        que->next = temp->next;
        if(temp->next != NULL){
            temp->next->prev = que;
        }
        temp->next = NULL;
        temp->prev = NULL;
    }
    return temp;
}

void RemovePCB(pcb *cur){
    pcb *temp = cur->next;
    if(cur->prev != NULL){
        cur->prev->next = temp;
    }
    if(temp != NULL){
        temp->prev = cur->prev;
    }
    cur->next = NULL;
    cur->prev = NULL;
}

void AppendStatus(struct Status *status_que, int status)
{
  struct Status *new_status = malloc(sizeof(struct Status));
  new_status->status = status;
  new_status->pid = active_proc->pid;
  while (status_que->next)
  {
    status_que = status_que->next;
  }
  new_status->next = NULL;
  status_que->next = new_status;
}

void RemoveParent(pcb *cur, pcb *queHead) {
    pcb *node = queHead->next;
    while (node != NULL) {
        if (node->parent == cur) {
            node->parent = NULL; // Set the parent to NULL if it matches cur.
        }
        node = node->next; // Move to the next node in the list.
    }
}


// =============================================================================
// Context Switch Functions
// =============================================================================

SavedContext *init_SwtichFunc(SavedContext *ctpx, void *p1, void *p2){
    // TracePrintf(0, "=== In init_SwitchFunc from 0x%lx to 0x%lx ===\n", p1, p2);

    // Fork init process
    struct pte *init_pt_r0 = (struct pte *)(GetFreeFrame() << PAGESHIFT);
    // TracePrintf(0, "init_pt_r0 = 0x%lx\n", init_pt_r0);
    int index;
    struct pte* TempVir2phy = TempVirtualMem;
    pcb* p1pcb = (pcb *)(p1);
    struct pte* p1_ptr0 = p1pcb->pt_r0;
    for(index=0; index<PAGE_TABLE_LEN; index++){
        // pt_r1[(TempVirtualMem-VMEM_1_BASE) >> PAGESHIFT].pfn = (unsigned long)(init_pt_r0) >> PAGESHIFT;
        // WriteRegister(REG_TLB_FLUSH, TempVirtualMem);
        PhysAddr_map_VirAddr(init_pt_r0);
        if(p1_ptr0[index].valid){
            TempVir2phy[index].valid = p1_ptr0[index].valid;
            TempVir2phy[index].kprot = p1_ptr0[index].kprot;
            TempVir2phy[index].uprot = p1_ptr0[index].uprot;
            TempVir2phy[index].pfn = GetFreeFrame();
            // TracePrintf(0, "TempVir2phy[index].pfn = 0x%lx\n", TempVir2phy[index].pfn);
            pt_r1[(TempVirtualMem-VMEM_1_BASE) >> PAGESHIFT].pfn = TempVir2phy[index].pfn;
            WriteRegister(REG_TLB_FLUSH, TempVirtualMem);
            memcpy(TempVirtualMem, index*PAGESIZE, PAGESIZE);
        }
        else{
            TempVir2phy[index].valid = 0;
        }
    }
    init_proc = (pcb*)malloc(sizeof(pcb));
    init_proc->pid = process_count++;
    init_proc->brk = MEM_INVALID_SIZE;
    init_proc->pt_r0 = init_pt_r0;
    pt_r0 = init_pt_r0;
    active_proc = init_proc;

    WriteRegister(REG_PTR0, init_pt_r0);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    // TracePrintf(0, "=== End init_SwitchFunc ===\n");
    return ctpx;
}

SavedContext *Delay_SwitchFunc(SavedContext *ctpx, void *p1, void *p2){
    TracePrintf(0, "=== In Delay_SwitchFunc from 0x%lx to 0x%lx ===\n", p1, p2);
    if(ready_proc.next == NULL){
        WriteRegister(REG_PTR0, idle_proc->pt_r0);
        active_proc = idle_proc;
    }
    else{
        WriteRegister(REG_PTR0, ((pcb *)ready_proc.next)->pt_r0);
        active_proc = ready_proc.next;
        RemovePCB(ready_proc.next);
    }
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    AddPCB(p1, &delay_proc);
    TracePrintf(0, "=== End Delay_SwitchFunc ===\n");
    return &active_proc->ctx;
}

SavedContext *Fork_SwitchFunc(SavedContext *ctpx, void *parent, void *child){
    TracePrintf(0, "=== In Fork_Switch Function from 0x%lx to 0x%lx ===\n", parent, child);

    // copy content of Region 0
    struct pte *child_ptr0 = (struct pte *)(GetFreeFrame() << PAGESHIFT);
    TracePrintf(0, "child_ptr0 = 0x%lx\n", child_ptr0);
    unsigned int index;
    struct pte* TempVir2phy = TempVirtualMem;
    struct pte* parent_ptr0 = ((pcb *)parent)->pt_r0;
    TracePrintf(0, "partent_ptr0 = 0x%lx\n", parent_ptr0);

    for(index=0; index<PAGE_TABLE_LEN; index++){
        // pt_r1[(TempVirtualMem-VMEM_1_BASE) >> PAGESHIFT].pfn = (unsigned long)(parent_ptr0) >> PAGESHIFT;
        // WriteRegister(REG_TLB_FLUSH, TempVirtualMem);
        PhysAddr_map_VirAddr(parent_ptr0);
        struct pte tempEntry;
        memcpy(&tempEntry, &TempVir2phy[index], sizeof(struct pte));

        // pt_r1[(TempVirtualMem-VMEM_1_BASE) >> PAGESHIFT].pfn = (unsigned long)(child_ptr0) >> PAGESHIFT;
        // WriteRegister(REG_TLB_FLUSH, TempVirtualMem);
        PhysAddr_map_VirAddr(child_ptr0);
        if(tempEntry.valid){
            TempVir2phy[index].valid = tempEntry.valid;
            TempVir2phy[index].kprot = tempEntry.kprot;
            TempVir2phy[index].uprot = tempEntry.uprot;
            TempVir2phy[index].pfn = GetFreeFrame();
            // TracePrintf(0, "TempVir2phy[index].pfn = 0x%lx\n", TempVir2phy[index].pfn);
            pt_r1[(TempVirtualMem-VMEM_1_BASE) >> PAGESHIFT].pfn = TempVir2phy[index].pfn;
            WriteRegister(REG_TLB_FLUSH, TempVirtualMem);
            memcpy(TempVirtualMem, index*PAGESIZE, PAGESIZE);
        }
        else{
            // TracePrintf(0, "%d is invalid\n", index);
            TempVir2phy[index].valid = 0;
        }
    }
    ((pcb *)(child))->pt_r0 = child_ptr0;
    pt_r0 = child_ptr0;
    memcpy(&((pcb *)(child))->ctx, ctpx, sizeof(SavedContext));
    active_proc = child;

    WriteRegister(REG_PTR0, child_ptr0);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    TracePrintf(0, "=== End Fork_SwitchFunc ===\n");
    AddPCB((pcb *)parent, &ready_proc);
    return &active_proc->ctx;
}

SavedContext *Exit_SwitchFunc(SavedContext *ctxp, void *p1, void *p2){
    TracePrintf(0, "Exit_SwitchFunc from 0x%lx to 0x%lx\n", p1, p2);

    // Free all p1 used memory
    struct pte *tmp_ptr0 = (struct pte *)(GetFreeFrame() << PAGESHIFT);
    // TracePrintf(0, "tmp_ptr0 = 0x%lx\n", tmp_ptr0);
    unsigned int index;
    struct pte* TempVir2phy = TempVirtualMem;
    struct pte* p1_ptr0 = ((pcb *)p1)->pt_r0;
    for(index = 0; index < PAGE_TABLE_LEN; index++){
        PhysAddr_map_VirAddr(tmp_ptr0);
        if(TempVir2phy[index].valid == 1){
            FreeFrame(TempVir2phy[index].pfn);
        }
    }
    if(p2){
        active_proc = p2;
        RemovePCB(p2);
    }
    else{
        // TracePrintf(0, "idle_proc = 0x%lx\n", idle_proc);
        active_proc = idle_proc;
    }
    WriteRegister(REG_PTR0, active_proc->pt_r0);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    free((pcb *)p1);
    TracePrintf(0, "=== End Exit_SwitchFunc ===\n");
    return &active_proc->ctx;
}

SavedContext *Wait_SwitchFunc(SavedContext *ctxp, void *p1, void *p2){
    TracePrintf(0, "Wait_SwitchFunc from 0x%lx to 0x%lx\n", p1, p2);
    
    // put into wait queue
    AddPCB(p1, &wait_proc);

    if(p2){
        active_proc = p2;
    }
    else{
        active_proc = idle_proc;
    }
    WriteRegister(REG_PTR0, active_proc->pt_r0);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    return &active_proc->ctx;
}

SavedContext *Tty_SwitchFunc(SavedContext *ctxp, void *p1, void *p2){
    TracePrintf(0, "Tty_SwitchFunc from 0x%lx to 0x%lx\n", p1, p2);

    if (p2){
        active_proc = p2;
        RemovePCB(p2);
    }
    else{
        active_proc = idle_proc;
    }

    WriteRegister(REG_PTR0, active_proc->pt_r0);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    return &active_proc->ctx;
}

SavedContext *SwitchFunc(SavedContext *ctxp, void *p1, void *p2){
    TracePrintf(0, "=== SwitchFunc from 0x%lx to 0x%lx ===\n", p1, p2);
    WriteRegister(REG_PTR0, ((pcb *)p2)->pt_r0);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    RemovePCB(p2);
    active_proc = p2;
    if (((pcb *)p1)->pid > 0){
        AddPCB(p1, &ready_proc);
    }

    return &active_proc->ctx;
}

void PhysAddr_map_VirAddr(struct pte *pageTable){
    pt_r1[(TempVirtualMem-VMEM_1_BASE) >> PAGESHIFT].pfn = (unsigned long)(pageTable) >> PAGESHIFT;
    WriteRegister(REG_TLB_FLUSH, TempVirtualMem);
}

// =============================================================================
// Terminal Functions
// =============================================================================

void Push_TerminalReadPCB(int tty_id, pcb *cur_pcb){
    AddPCB(cur_pcb, &terminals[tty_id].read_pcb);
}

void Push_TerminalWritePCB(int tty_id, pcb *cur_pcb){
    AddPCB(cur_pcb, &terminals[tty_id].write_pcb);
}

pcb *Pop_TerminalReadPCB(int tty_id){
    return PopPCB(&terminals[tty_id].read_pcb);
}

pcb *Pop_TerminalWritePCB(int tty_id){
    return PopPCB(&terminals[tty_id].write_pcb);
}

void Push_TerminalBUFF(buff *que, buff *cur_buff){
    while(que->next){
        que = que->next;
    }
    que->next = cur_buff;
    cur_buff->next = NULL;
}

buff *Pop_TerminalBUFF(buff *que){
    buff *tmp = que->next;
    if(tmp){
        que->next = tmp->next;
        tmp->next = NULL;
    }
    return tmp;
}