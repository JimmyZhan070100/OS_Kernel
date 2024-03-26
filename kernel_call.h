#include <comp421/hardware.h>
#include <comp421/yalnix.h>

int MyFork(void);

int MyExec(char *, char **, ExceptionInfo *);

void Exit(int);

int MyWait(int *);

int MyGetPid(void);

int MyBrk(void *);

int MyDelay(int);

int MyTtyRead(int, void *, int);

int MyTtyWrite(int, void *, int);