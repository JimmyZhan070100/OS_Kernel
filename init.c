#include <comp421/hardware.h>
#include <comp421/yalnix.h>

int main(int argc, char **argv)
{
  TracePrintf(0, "argc = %d\n", argc);
  int i = 0;
  for (; argv[i]; ++i)
  {
    TracePrintf(0, "argv[%d] = %s\n", i, argv[i]);
  }
  // TracePrintf(0, "PID = %d\n", GetPid());

  while (1)
  {
    // Delay(3);
    Pause();
    TracePrintf(0, "Delay ends!\n");
  }
}