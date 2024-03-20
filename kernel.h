#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <comp421/loadinfo.h>

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

// Use queue to store status
typedef struct Status_que
{
    unsigned int pid;
    int status;
    struct Status_que *next;
}Status_que;

// PCB structure
typedef struct pcb {
    unsigned int pid;
    int num_child;
    int delay_clock;
    unsigned long brk;
    struct pte *pt_r0;
    SavedContext *ctx;
    Status_que *statusQueue;
    struct pcb *prev;
    struct pcb *next;
}pcb;

extern unsigned int process_count;
extern pcb *idle_proc;
extern pcb init_proc;