#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

void sched_halt(void);

static int seed = 1;

// Random number generator implementation from:
// https://stackoverflow.com/questions/4768180/rand-implementation
// This behaves deterministically, but it still has the right effect
static int
rand(void) {
	seed = seed * 1103515245 + 12345;
	return (unsigned int)(seed/65536) % 32768;
}

// Choose a user environment to run and run it.
void
sched_yield(void)
{
	struct Env *idle;

	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// Lottery scheduling: Each environment's chance to run is 
	// proportional to its priority

	// First, we loop through to sum up the total priority
	// for all runnable environments
	int total = 0;
	struct Env env;
	for (int i = 0; i < NENV; ++i) {
		env = envs[i];
		if (env.env_status == ENV_RUNNABLE) {
			total += env.env_priority;
		}
	}

	// If no environments are runnable, we can simply jump past
	// to see if the current one wants to run again, or perhaps
	// sched_halt
	if (!total) {
		goto done;
	}

	// We choose a random number between 0 and our total priority
	int random = rand() % total;

	// We find the appropriate runnable environment to run based
	// on our random number
	for (int i = 0; i < NENV; ++i) {
		env = envs[i];
		if (env.env_status == ENV_RUNNABLE) {
			if (env.env_priority > random) {
				env_run(&envs[i]);
			}
			random -= env.env_priority;
		}
	}

done:
	if (curenv && curenv->env_status == ENV_RUNNING) {
		env_run(curenv);
	}


	// sched_halt never returns
	sched_halt();
}



// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile (
		"movl $0, %%ebp\n"
		"movl %0, %%esp\n"
		"pushl $0\n"
		"pushl $0\n"
		"sti\n"
		"1:\n"
		"hlt\n"
		"jmp 1b\n"
	: : "a" (thiscpu->cpu_ts.ts_esp0));
}

