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
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).
	if (!(uvpt[PGNUM(addr)] & PTE_P)) {
		panic("page associated with faulting virtual addr is not present");
	}
	if ((!(uvpt[PGNUM(addr)] & PTE_COW))) {
		panic("page associated with faulting virtual addr is neither writable nor COW");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	if ((r = sys_page_alloc(0, PFTEMP, PTE_P | PTE_U | PTE_W)) < 0) {
		panic("pgfault could not allocate temp page");	
	}
	memmove(ROUNDDOWN(addr, PGSIZE), PFTEMP, PGSIZE);
	if ((r = sys_page_map(0, PFTEMP, 0, addr, PTE_P | PTE_U | PTE_W)) < 0 ) {
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
	if ((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW)) {
		int r;
		if ((r=sys_page_map(0, (void*) (pn*PGSIZE), envid, (void*) (pn*PGSIZE), PTE_COW | PTE_P | PTE_U)) < 0) {
			return r;
		}
		if (!(uvpt[pn] & PTE_COW)) {
			r = sys_page_map(0, (void*) (pn*PGSIZE), 0, (void*) (pn*PGSIZE), PTE_COW | PTE_P | PTE_U);
		}
		return r;
	} else {
		return sys_page_map(0, (void*) (pn*PGSIZE), envid, (void*) (pn*PGSIZE), PTE_P | PTE_U);
	}
}

//
// User-level fork with copy-on-write.
// 
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 5: Your code here.
	set_pgfault_handler(pgfault);
	
	envid_t envid = sys_exofork();
	
	if (envid == 0) {
		thisenv = &envs[sys_getenvid()];
	} else {
		// _pgfault_upcall will call the pgfault_handler that is in
		// our globals, and since we set those before we dropped into
		// the child's execution, that handler will be available as
		// well.
		sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);
		cprintf("this->env_pgfault_upcall: %p\n", thisenv->env_pgfault_upcall); 
		/*
		for (page table in page tables) {
			memmove(, cPT, PGSIZE);
		}
		memmove(pPD, cPD);
		*/
		for (unsigned pn = 0; pn * PGSIZE < USTACKTOP; ++pn) {
			if ((uvpd[pn >> 10] & PTE_P) && (uvpt[pn] & PTE_P)) {
				duppage(envid, pn);
			}
		}
		sys_page_alloc(envid, (void*) (UXSTACKTOP-PGSIZE), PTE_W | PTE_U | PTE_P);
		sys_env_set_status(envid, ENV_RUNNABLE);
	}
	cprintf("our envid is: %d\n", envid);
	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
