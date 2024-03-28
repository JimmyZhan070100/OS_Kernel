#include <comp421/hardware.h>
#include <comp421/yalnix.h>

#include "kernel_call.h"
#include "kernel.h"

int main(int argc, char **argv)
{
  TracePrintf(0, "=== Start \"init\" program ===\n");
  TracePrintf(0, "argc = %d\n", argc);
  int i = 0;
  for (; argv[i]; ++i)
  {
    TracePrintf(0, "argv[%d] = %s\n", i, argv[i]);
  }
  TracePrintf(0, "PID = %d\n", GetPid());
  printf("PID = %d\n", GetPid());
  // if(Fork()){
  //   TracePrintf(0, "Child process\n");
  // }
  // else{
  //   TracePrintf(0, "Parent process\n");
  // }
  // TracePrintf(0, "Finish fork\n");

  // while (1)
  // {
    // Delay(3);
    // Pause();
    TracePrintf(0, "In  \"init\" program pause ends!\n");
    Exit(0);
  // }
}