// Fork a binary tree of processes and display their structure.

#include <inc/lib.h>

static void
yield_loop(int child)
{
	for (int loop = 0; loop < 5; loop++) {
		cprintf("Loop %d for child %d\n", loop, child);
		sys_yield();
	}
	exit();
}

void
umain(int argc, char **argv)
{
	int children[7];
	for (int child = 0; child < 7; child++) {
		cprintf("Forked child %d with priority %d\n", child, 1 << child);
		if ((children[child] = fork()) == 0) {
			int priority = 1 << child;
			sys_set_priority(priority);
			yield_loop(child);
		}
		sys_env_set_status(children[child], ENV_NOT_RUNNABLE);
	}
	for (int child = 0; child < 7; child++) {
		sys_env_set_status(children[child], ENV_RUNNABLE);
	}
}
