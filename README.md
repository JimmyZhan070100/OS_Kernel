# README for Yalnix Operating System Kernel

- `samples-lab2/`: tests for kernel
- `yalinx.c`: Include `kernelStart` and `setKernelBrk` function
- `handler.c & handler.h`: Functions for trap exceptions
- `kernel_call.c & kernel_call.h`: Once handler trap exceptions, call the corresponded functions
- `kernel.c & kernel.h`: Contains major functions and data structres like pageTable, memory, PCB, context switch, and terminal
- `load.c`: function `LoadProgram` that can load a Yalnix program into memory.
- `test.c`: a program called GetPid() as init program

## `yalinx` Features

This is the main kernel file that includes critical initialization functions:

- `KernelStart`: This function is the entry point for kernel execution. It performs initial setup tasks such as initializing interrupt vector table entries, setting up page tables for virtual memory, enabling virtual memory, and handling the creation and switching to the idle process. It also handles the initialization of kernel break (`kernel_brk`)
- `SetKernelBrk`: Adjusts the kernel's heap end (break) point. Before virtual memory is enabled, it simply updates `kernel_brk`. After enabling virtual memory, it allocates physical frames for the new kernel heap space and updates the Region 1 page table.

## `handler` Features

- `TrapKernelHandler`: Interprets and processes system calls, directing each to its respective kernel function.
- `TrapClockHandler`: Manages clock interrupts for process scheduling and time tracking.
- `TrapIllegalHandler`: Handles illegal operations and errors, terminating processes as necessary.
- `TrapMathHandler`: Catches arithmetic exceptions and terminates the offending process.
- `TrapMemoryHandler`: Manages memory access violations and stack overflows by allocating more memory or terminating the process.
- `TrapTTY_ReceiveHandler`: Buffers data received from terminal devices and resumes waiting processes.
- `TrapTTY_TransmitHandler`: Signals the completion of data transmission to terminal devices and schedules waiting processes for execution.

## `kernel_call` Features

- `MyFork`: Duplicates the calling process, creating a new child process with a unique process ID.

- `MyExec`: Replaces the current process's memory space with a new program specified by `filename`, passing `argvec` as the new program's arguments.

- `MyExit`: Terminates the calling process, optionally returning a `status` value to the operating system.

- `MyWait`: Suspends execution of the calling process until one of its children terminates, returning the child's process ID and exit status.

- `MyGetPid`: Returns the process ID of the calling process.

- `MyBrk`: Adjusts the end of the data segment to the specified address, effectively changing the allocated space for the process's data segment.

- `MyDelay`: Suspends the calling process for a specified number of clock ticks.

- `MyTtyRead`: Reads input from the terminal identified by `tty_id` into a buffer, up to a specified length.

- `MyTtyWrite`: Writes data from a buffer to the terminal identified by `tty_id`, up to a specified length.

## `kernel` Features

- `Global variables`: Defines global variables like `TempVirtualMem`, page tables for region 1 and 0, and the free frame count.
- `PageTable and Memory`: Implements functionalities to manage page tables and memory allocation, including getting free frames, allocating, and freeing physical addresses.
- `PCB (Process Control Block)`: Defines the structure for process control blocks including process ID, number of children, delay ticks, memory break, stack base, and the region 0 page table pointer. It also includes functions for manipulating PCBs within queues and handling process statuses.
- `Context Switch Functions`: Contains functions designed to handle different context switches within the operating system. These include initialization, delay, fork, exit, wait, terminal, and generic switching functionalities.
- `Terminal Functions`: Manages terminal read and write operations with buffer handling. Functions include pushing and popping PCBs and buffers in terminal queues.

`LoadProgram`: Loads a program into the virtual memory space of a process, replacing its current image.

`GetFreeFrame`, `FreeFrame`: Allocate and free physical memory frames, managing the linked list of free frames.

`AddPCB`, `PopPCB`, `RemovePCB`: Add, pop, and remove PCBs from a queue, assisting in process scheduling.

`AppendStatus`, `RemoveParent`: Handle process statuses and parent-child relationships.

`init_SwitchFunc`, `Delay_SwitchFunc`, `Fork_SwitchFunc`, `Exit_SwitchFunc`, `Wait_SwitchFunc`, `Tty_SwitchFunc`: Specific functions to switch the context between processes based on different system events or operations.

`PhysAddr_map_VirAddr`: Maps physical addresses to virtual addresses to facilitate memory operations.

`Push_TerminalReadPCB`, `Push_TerminalWritePCB`, `Pop_TerminalReadPCB`, `Pop_TerminalWritePCB`: Manage terminal read and write operations by queuing processes waiting for terminal input/output.

`Push_TerminalBUFF`, `Pop_TerminalBUFF`: Manage buffers for terminal operations, allowing data to be stored and retrieved as needed.

## Tests section

Currently, fail the following test files
- forktest3
- init
- init1
- init2
- init3

The others can work correctly.

## Usage
- Exucte Makefile
`$ make all`
- Choose the file you want to execute
`$ ./yalinx -s samples-lab2/[fileName]`
- If want clean all
`$ make clean`

Example:
- `$ ./yalinx -lk 5 -lu 5 -s samples-lab2/brktest`
- `$ ./yalinx -lk 5 -lu 5 -s test`