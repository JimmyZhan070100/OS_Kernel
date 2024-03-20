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