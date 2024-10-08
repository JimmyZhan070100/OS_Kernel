#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <comp421/loadinfo.h>

#include "handler.h"
#include "kernel.h"
#include "load.c"

void *kernel_brk;
int vm_enabled = 0;


void idle(){
    TracePrintf(0, "Start idle process\n");
    while(1){
        Pause();
    }
}

void KernelStart(ExceptionInfo *info, unsigned int pmem_size, void *orig_brk, char **cmd_args){
    unsigned int i;
    kernel_brk = orig_brk;
    // TracePrintf(0, "original break = 0x%lx\n", orig_brk);
    // TracePrintf(0, "pmem_size = 0x%lx\n", pmem_size);
    // TracePrintf(0, "&_etext = 0xlx\n", &_etext);
    // TracePrintf(0, "USER_STACK_LIMIT = 0x%lx\n", USER_STACK_LIMIT);
    // TracePrintf(0, "KERNEL_STACK_BASE = 0x%lx\n", KERNEL_STACK_BASE);
    // TracePrintf(0, "KERNEL_STACK_LIMIT = 0x%lx\n", KERNEL_STACK_LIMIT);

    // Initialize the interrupt vector table entries
    static interrupt_hdr itrVectorTable[TRAP_VECTOR_SIZE];
    itrVectorTable[TRAP_KERNEL] = TrapKernelHandler;
    itrVectorTable[TRAP_CLOCK] = TrapClockHandler;
    itrVectorTable[TRAP_ILLEGAL] = TrapIllegalHandler;
    itrVectorTable[TRAP_MEMORY] = TrapMemoryHandler;
    itrVectorTable[TRAP_MATH] = TrapMathHandler;
    itrVectorTable[TRAP_TTY_RECEIVE] = TrapTTY_ReceiveHandler;
    itrVectorTable[TRAP_TTY_TRANSMIT] = TrapTTY_TransmitHandler;
    for(i=TRAP_DISK; i<TRAP_VECTOR_SIZE; i++){
        itrVectorTable[i] = NULL;
    }

    // Initialize the REG_VECTOR_BASE privileged register to point to interrupt vector table
    WriteRegister(REG_VECTOR_BASE, (RCS421RegVal)(itrVectorTable));

    // Build a structure to keep track of what page frames in physical memory are free
    // From MEM_INVALID_SIZE to KERNEL_STACK_BASE
    unsigned long frame_addr = (unsigned long)MEM_INVALID_SIZE;
    while(frame_addr < (unsigned long)KERNEL_STACK_BASE){
        FreeFrame(frame_addr >> PAGESHIFT);
        frame_addr += (unsigned long)PAGESIZE;
    }
    // From kernel_brk to pmem_size
    frame_addr = UP_TO_PAGE((unsigned long)kernel_brk+2*PAGE_TABLE_SIZE);
    while(frame_addr < pmem_size){
        FreeFrame(frame_addr >> PAGESHIFT);
        frame_addr += (unsigned long)PAGESIZE;
    }

    pt_r1 = calloc(PAGE_TABLE_LEN, sizeof(struct pte));
    pt_r0 = calloc(PAGE_TABLE_LEN, sizeof(struct pte));
    // TracePrintf(5, "pt_r1: address =  0x%lx\n", pt_r1);
    // TracePrintf(5, "pt_r0: address =  0x%lx\n", pt_r0);
    WriteRegister(REG_PTR1, (RCS421RegVal)pt_r1);
    WriteRegister(REG_PTR0, (RCS421RegVal)pt_r0);
    
    // Build the initial Region 1 table
    unsigned long addr;
    unsigned int idx=0;

    // kernel text
    for(addr = VMEM_1_BASE; addr < (u_int64_t)&_etext; addr += PAGESIZE){
        pt_r1[idx].kprot = PROT_READ | PROT_EXEC;
        pt_r1[idx].uprot = PROT_NONE;
        pt_r1[idx].valid = 1;
        pt_r1[idx].pfn = ((long)(addr) >> PAGESHIFT);
        idx++;
    }

    // TracePrintf(5, "Initialize region 1 bss & heap\n");
    // kernel bss/heap
    for (; addr < (unsigned long)kernel_brk;  addr += PAGESIZE)
    {
        pt_r1[idx].kprot = PROT_READ | PROT_WRITE;
        pt_r1[idx].uprot = PROT_NONE;
        pt_r1[idx].valid = 1;
        pt_r1[idx].pfn = addr >> PAGESHIFT;
        idx++;
    }
    
    // Revserve for enable virtual memory
    TempVirtualMem = VMEM_1_LIMIT - PAGESIZE;
    idx = ((long)(TempVirtualMem - VMEM_1_BASE) >> PAGESHIFT);
    pt_r1[idx].kprot = PROT_READ | PROT_WRITE;
    pt_r1[idx].uprot = PROT_NONE;
    pt_r1[idx].valid = 1;
    pt_r1[idx].pfn = TempVirtualMem >> PAGESHIFT;

    // Build the initial Region 0 table
    for(addr = KERNEL_STACK_BASE; addr < KERNEL_STACK_LIMIT;  addr += PAGESIZE){
        idx = addr >> PAGESHIFT;
        pt_r0[idx].valid = 1;
        pt_r0[idx].kprot = PROT_READ | PROT_WRITE;
        pt_r0[idx].uprot = PROT_NONE;
        pt_r0[idx].pfn = idx;
    }

    // Enable virtual memory
    WriteRegister(REG_VM_ENABLE, 1);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
    vm_enabled = 1;
    frame_addr = (unsigned long)PMEM_BASE;
    while (frame_addr < (unsigned long)MEM_INVALID_SIZE){
        FreeFrame(frame_addr >> PAGESHIFT);
        frame_addr += (unsigned long)PAGESIZE;
    }

    // Create idle process
    idle_proc = (pcb*)malloc(sizeof(pcb));
    idle_proc->pid = 0;
    idle_proc->brk = MEM_INVALID_SIZE;
    idle_proc->pt_r0 = pt_r0;
    active_proc = idle_proc;

    info->pc = &idle;
    info->sp = (void *)USER_STACK_LIMIT;

    int pageNumber = DOWN_TO_PAGE(USER_STACK_LIMIT - 1) >> PAGESHIFT;
    pt_r0[pageNumber].valid = ++process_count;
    pt_r0[pageNumber].pfn = GetFreeFrame();
    pt_r0[pageNumber].kprot = PROT_READ | PROT_WRITE;

    // Create init process and load into it
    if(cmd_args[0]){
        for(i=0; cmd_args[i]; i++){
            // TracePrintf(0, "cmd_args[%d] = %s\n", i, cmd_args[i]);
        }
        ContextSwitch(&init_SwtichFunc, &idle_proc->ctx, idle_proc, init_proc);
        TracePrintf(0, "Finish the first Context Switch\n");
        if(active_proc->pid == 1){
            TracePrintf(0, "=== Start into Loadprogram ===\n");
            LoadProgram(cmd_args[0], cmd_args, info);
        }
    }
}

int SetKernelBrk(void *addr){
    if((unsigned long)addr > VMEM_1_LIMIT){
        return -1;
    }
    // If virtual memory is not yet enabled, just update the kernel break
    if(vm_enabled == 0){
        TracePrintf(0, "In SetKernelBrk: (0x%lx) vm not set yet\n", addr);
        kernel_brk = addr;
    }
    else{
        // If virtual memory is enabled, adjust the kernel break with proper memory allocation
        TracePrintf(0, "In SetKernelBrk: (0x%lx) vm is set\n", addr);
        unsigned long new_brake = UP_TO_PAGE(addr);
        unsigned long old_brake = UP_TO_PAGE(kernel_brk);
        unsigned long page;
        unsigned long free_frame;
        unsigned long pageNumber;

        for(page = old_brake; page < new_brake; page+=PAGESIZE){
            pageNumber = (page - VMEM_1_BASE) >> PAGESHIFT;
            TracePrintf(2, "pt_r1 = 0x%lx\n", pt_r1);
            free_frame = GetFreeFrame();
            if(!free_frame){
                TracePrintf(2, "In SetKernelBrk: Memory allocate error\n Current frame = (%lu)\n", page);
                return -1;
            }
            if(pt_r1[pageNumber].valid == 0){
                pt_r1[pageNumber].valid = 1;
                pt_r1[pageNumber].kprot = PROT_READ | PROT_WRITE;
                pt_r1[pageNumber].uprot = PROT_NONE;
                pt_r1[pageNumber].pfn = free_frame;
            }
        }
        kernel_brk = addr;
    }
    return 0;
}
