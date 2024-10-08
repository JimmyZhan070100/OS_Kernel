#
#	Sample Makefile for COMP 421 Yalnix kernel and user programs.
#
#	The Yalnix kernel built will be named "yalnix".  *ALL* kernel
#	Makefiles for this lab must have a "yalnix" rule in them, and
#	must produce a kernel executable named "yalnix" -- we will run
#	your Makefile and will grade the resulting executable
#	named "yalnix".
#
#	Your project must be implemented using the C programming
#	language (e.g., not in C++ or other languages).
#

#
#	Define the list of everything to be made by this Makefile.
#	The list should include "yalnix" (the name of your kernel),
#	plus the list of user test programs you also want to be mae
#	by this Makefile.  For example, the definition below
#	specifies to make Yalnix test user programs test1, test2,
#	and test3.  You should modify this list to the list of your
#	own test programs.
#
#	For each user test program, the Makefile will make the
#	program out of a single correspondingly named sourc file.
#	For example, the Makefile will make test1 out of test1.c,
#	if you have a file named test1.c in this directory.
#
# ALL = yalnix test1 test2 test3
TESTS_SRCS := $(wildcard samples-lab2/*.c)
TESTS := $(TESTS_SRCS:samples-lab2/%.c=%)
ALL = yalnix init test $(TESTS)
# ALL = yalnix init

#
#	You must modify the KERNEL_OBJS and KERNEL_SRCS definitions
#	below.  KERNEL_OBJS should be a list of the .o files that
#	make up your kernel, and KERNEL_SRCS should  be a list of
#	the corresponding source files that make up your kernel.
#
KERNEL_OBJS = yalnix.o handler.o kernel.o kernel_call.o
KERNEL_SRCS = yalnix.c handler.h kernel.h kernel_call.h

#
#	You should not have to modify anything else in this Makefile
#	below here.  If you want to, however, you may modify things
#	such as the definition of CFLAGS, for example.
#

PUBLIC_DIR = /clear/courses/comp421/pub

CPPFLAGS = -I$(PUBLIC_DIR)/include
# CFLAGS = -g -Wall -Wextra -Werror
CFLAGS = -g -Wall -Wextra

LANG = gcc

%: %.o
	$(LINK.o) -o $@ $^ $(LOADLIBES) $(LDLIBS)

LINK.o = $(PUBLIC_DIR)/bin/link-user-$(LANG) $(LDFLAGS) $(TARGET_ARCH)

%: %.c
%: %.cc
%: %.cpp

all: $(ALL)

yalnix: $(KERNEL_OBJS)
	$(PUBLIC_DIR)/bin/link-kernel-$(LANG) -o yalnix $(KERNEL_OBJS)

clean:
	rm -f $(KERNEL_OBJS) $(ALL) TTY* TRACE DISK core*
	find samples-lab2/ -name '*.o' -delete
	find samples-lab2/ -type f -executable -delete
	
depend:
	$(CC) $(CPPFLAGS) -M $(KERNEL_SRCS) > .depend

$(TESTS): %: samples-lab2/%.o
	$(LINK.o) -o samples-lab2/$@ $^ $(LOADLIBES) $(LDLIBS)

#include .depend
