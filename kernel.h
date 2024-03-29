#ifndef kernel_h
#define kernel_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <comp421/loadinfo.h>

// =============================================================================
// Gloabal variables Section
// =============================================================================

extern unsigned long TempVirtualMem;
extern struct pte *pt_r1;
extern struct pte *pt_r0;
extern int free_frame_count;
int LoadProgram(char *name, char **args, ExceptionInfo *info);

// =============================================================================
// PageTable and Memory Section
// =============================================================================

// Use linked list data structure to build free page frame
typedef struct PhysFrame
{
    unsigned long addr;
    struct PhysFrame *next;
} PhysFrame;

extern PhysFrame *physFrame_head;

unsigned long GetFreeFrame();

void FreeFrame(unsigned long addr);

void GetfreePhysicalAddr(unsigned long begin, unsigned long end, unsigned int kprot, unsigned uprot, struct pte* cur_ptr0);

void FreePhysicalAddr(unsigned long begin, unsigned long end, struct pte* cur_ptr0);

int CheckPhysFrame(int req_count);

// =============================================================================
// PCB Section
// =============================================================================

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
    Status statusQueue;
    struct pcb *parent;
    struct pcb *prev;
    struct pcb *next;
}pcb;

extern unsigned int process_count;
extern pcb *active_proc;
extern pcb *idle_proc;
extern pcb *init_proc;

// Dummy node for linked list
extern pcb delayQue_pcb;
extern pcb waitQue_pcb;
extern pcb readyQue_pcb;

void AddPCB(pcb *cur, pcb *queHead);

pcb *PopPCB(pcb *);

void RemovePCB(pcb *);

void AppendStatus(struct Status *status_que, int status);

void RemoveParent(pcb *cur, pcb *queHead);

// =============================================================================
// Context Switch Functions
// =============================================================================

SavedContext *init_SwtichFunc(SavedContext *ctpx, void *p1, void *p2);

SavedContext *Delay_SwitchFunc(SavedContext *ctpx, void *p1, void *p2);

SavedContext *Fork_SwitchFunc(SavedContext *ctpx, void *parent, void *child);

SavedContext *Exit_SwitchFunc(SavedContext *ctxp, void *p1, void *p2);

SavedContext *Wait_SwitchFunc(SavedContext *ctxp, void *p1, void *p2);

SavedContext *Tty_SwitchFunc(SavedContext *ctxp, void *p1, void *p2);

SavedContext *SwitchFunc(SavedContext *ctxp, void *p1, void *p2);

void PhysAddr_map_VirAddr(struct pte *pageTable);

// =============================================================================
// Terminal Section
// =============================================================================

typedef struct buff{
    int count;
    int cur_pos;
    char text[TERMINAL_MAX_LINE];
    struct buff *next;
}buff;

typedef struct Terminal{
    buff read_buff;
    buff write_buff;
    pcb write_pcb;  // dummy node as linked list
    pcb read_pcb;   // dummy node as linked list
}Terminal;

extern struct Terminal terminals[NUM_TERMINALS];

void Push_TerminalReadPCB(int tty_id, pcb *cur_pcb);

void Push_TerminalWritePCB(int tty_id, pcb *cur_pcb);

pcb *Pop_TerminalReadPCB(int tty_id);

pcb *Pop_TerminalWritePCB(int tty_id);

void Push_TerminalBUFF(buff *que, buff *cur_buff);

buff *Pop_TerminalBUFF(buff *que);

#endif