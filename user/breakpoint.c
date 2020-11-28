// program to cause a breakpoint trap

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	asm volatile("int $3");
	int x = 5;
	int y = 1;
	int z = x + y;
	asm volatile("int $3");
}

