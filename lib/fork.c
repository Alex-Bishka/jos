// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	if (!(uvpt[PGNUM(addr)] & PTE_P)) {
		cprintf("here is some interesting stuff pte: %x\n addr: %x\n", uvpt[PGNUM(addr)], addr);
		cprintf("this is the eip: %x\n", utf->utf_eip);
		cprintf("env id (pgfautl): %x\n", sys_getenvid()); 
		panic("page associated with faulting virtual addr is not present");
	}
	if ((!(uvpt[PGNUM(addr)] & PTE_COW))) {
		panic("page associated with faulting virtual addr is neither writable nor COW");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	if ((r = sys_page_alloc(0, PFTEMP, PTE_P | PTE_U | PTE_W)) < 0) {
		panic("pgfault could not allocate temp page");	
	}
	memmove(PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);
	if ((r = sys_page_map(0, PFTEMP, 0, ROUNDDOWN(addr, PGSIZE), PTE_P | PTE_U | PTE_W)) < 0 ) {
		panic("pgfault could not map temp page");
	}
	if ((r = sys_page_unmap(0, PFTEMP)) < 0) {
		panic("pgfault failed to unmap temporary page");
	}
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	if (uvpt[pn] & PTE_SHARE) {
		return sys_page_map(0, (void*) (pn*PGSIZE), envid, (void*) (pn*PGSIZE), uvpt[pn] & PTE_SYSCALL);
	}
	if ((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW)) {
		int r;
		if ((r=sys_page_map(0, (void*) (pn*PGSIZE), envid, (void*) (pn*PGSIZE), PTE_COW | PTE_P | PTE_U)) < 0) {
			return r;
		}
		if (!(uvpt[pn] & PTE_COW)) {
			r = sys_page_map(0, (void*) (pn*PGSIZE), 0, (void*) (pn*PGSIZE), PTE_COW | PTE_P | PTE_U);
		}
		return r;
	}
	return sys_page_map(0, (void*) (pn*PGSIZE), envid, (void*) (pn*PGSIZE), PTE_P | PTE_U);
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
envid_t
fork(void)
{
	// LAB 5: Your code here.
	set_pgfault_handler(pgfault);
	
	envid_t envid = sys_exofork();
	
	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		cprintf("thisenv (fork) id: %x\n", sys_getenvid());
		cprintf("thisenv addr: %x\n", thisenv);
	} else {
		// _pgfault_upcall will call the pgfault_handler that is in
		// our globals, and since we set those before we dropped into
		// the child's execution, that handler will be available as
		// well.
		
		if (sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall) < 0) {
			panic("failed to set pagefault upcall in fork()");
		}
		for (unsigned pn = 0; pn * PGSIZE < USTACKTOP; ++pn) {
			if ((uvpd[pn >> 10] & PTE_P) && (uvpt[pn] & PTE_P)) {
				if (duppage(envid, pn) < 0) {
					panic("duppage call failed in fork()");
				}
			}
		}
		if (sys_page_alloc(envid, (void*) (UXSTACKTOP-PGSIZE), PTE_W | PTE_U | PTE_P) < 0) {
			panic("sys_page_alloc failed");
		}
		if (sys_env_set_status(envid, ENV_RUNNABLE) < 0) {
			panic("sys_env_set_status failed");
		}
	}
	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
