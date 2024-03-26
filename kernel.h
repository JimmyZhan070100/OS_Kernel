#ifndef kernel_h
#define kernel_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <comp421/loadinfo.h>

extern unsigned long TempVirtualMem;
extern struct pte *pt_r1;
extern struct pte *pt_r0;
extern int free_frame_count;
int LoadProgram(char *name, char **args, ExceptionInfo *info);

// Use linked list data structure to build free page frame
typedef struct PhysFrame
{
    unsigned long addr;
    struct PhysFrame *next;
} PhysFrame;

extern PhysFrame *physFrame_head;

unsigned long GetFreeFrame();

unsigned long GetFromPTMEM();

void FreeFrame(unsigned long addr);

void GetfreePhysicalAddr(unsigned long begin, unsigned long end, unsigned int kprot, unsigned uprot, struct pte* cur_ptr0);
void FreePhysicalAddr(unsigned long begin, unsigned long end, struct pte* cur_ptr0);

int CheckPhysFrame(int);

// Use queue to store status
typedef struct Status
{
    unsigned int pid;
    int status;
    struct Status *next;
}Status;

// PCB structure
typedef struct pcb {
    unsigned int pid;
    int num_child;
    int delay_clock;
    int pass_tick;
    void *brk;
    void *stack_base;
    struct pte *pt_r0;
    SavedContext ctx;
    Status *statusQueue;
    struct pcb *parent;
    struct pcb *prev;
    struct pcb *next;
}pcb;

extern unsigned int process_count;
extern pcb *active_proc;
extern pcb *idle_proc;
extern pcb *init_proc;
extern pcb *delay_proc;
extern pcb *wait_proc;
extern pcb *ready_proc;

void AddPCB(pcb *cur, pcb **queHead);

pcb *PopPCB(pcb *);

void RemovePCB(pcb *);

void AppendStatus(struct Status *status_que, int status);

void RemoveParent(pcb *cur, pcb **queHead);

// Swtich Function
RCS421RegVal Virt2Phy(unsigned long addr);

SavedContext *init_SwtichFunc(SavedContext *ctpx, void *p1, void *p2);

SavedContext *Delay_SwitchFunc(SavedContext *ctpx, void *p1, void *p2);

SavedContext *Fork_SwitchFunc(SavedContext *ctpx, void *parent, void *child);

SavedContext *Exit_SwitchFunc(SavedContext *ctxp, void *p1, void *p2);

SavedContext *Wait_SwitchFunc(SavedContext *ctxp, void *p1, void *p2);

SavedContext *Tty_SwitchFunc(SavedContext *ctxp, void *p1, void *p2);

SavedContext *SwitchFunc(SavedContext *ctxp, void *p1, void *p2);


void PhysAddr_map_VirAddr(struct pte *pageTable);

#endif