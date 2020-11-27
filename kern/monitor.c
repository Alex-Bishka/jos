// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>
#include <inc/error.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display the current stack backtrace", mon_backtrace },
	{ "showmappings", "Display physical page mappings for a range of virtual addresses", mon_showmappings },
	{ "setperms", "Set the permissions of a virtual address", mon_setperms },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t ebp = read_ebp();

	cprintf("Stack backtrace:\n");
	while (ebp) {
	        uint32_t* ebpPtr = (uint32_t*)ebp;
		uint32_t eip = ebpPtr[1]; 
		cprintf("  ebp %08x  eip %08x  args", ebp, eip);
		for (size_t i = 0; i <= 4; ++i) {
			cprintf(" %08x", ebpPtr[i + 2]);
		}
		cprintf("\n");
		struct Eipdebuginfo debuginfo;
		debuginfo_eip(ebpPtr[1], &debuginfo);
		cprintf("         %s:%d: %.*s+%d\n", debuginfo.eip_file, debuginfo.eip_line, debuginfo.eip_fn_namelen, debuginfo.eip_fn_name, eip - debuginfo.eip_fn_addr);
		ebp = *ebpPtr;
	}
	return 0;
}

static bool
parseAddr(char* in, uintptr_t** addr_store) {
	if (in[0] == '0' && in[1] == 'x') {
		uintptr_t addr = 0;
		char ch;
		for (int i = 2; i < strlen(in); ++i) {
			addr = addr << 4;
			ch = in[i];
			if (ch >= '0' && ch <= '9') addr += ch - '0';
			if (ch >= 'a' && ch <= 'f') addr += 10 + ch - 'a';
			if (ch >= 'A' && ch <= 'F') addr += 10 + ch - 'A';
		}
		*addr_store = &addr;
		return true;
	}
	cprintf("Unable to parse addr given by %s.\n", in);
	cprintf("Please pass in an arguement beginning with '0x'\n");
	return false;
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
	if (argc != 3) {
		cprintf("Unable to parse request...\n");
		cprintf("Please provide two space-separated virtual addresses starting with 0x, indicating the start and stop of the range\n");
		return 0;
	}
	uintptr_t* addrptr1;
	if (!parseAddr(argv[1], &addrptr1)) {
		cprintf("Parse failure for address argument 1... exiting\n");
		return 0;
	}
	uintptr_t addr1 = *addrptr1;
	uintptr_t* addrptr2;
	if (!parseAddr(argv[2], &addrptr2)) {
		cprintf("Parse failure for address argument 2... exiting\n");
		return 0;
	}
	uintptr_t addr2 = *addrptr2;
	cprintf("Successfully parsed virtual addresses. We will attempt to give information between these two addresses:\n");
	cprintf("From: 0x%x\nTo: 0x%x\n", addr1, addr2);
	addr1 = ROUNDDOWN(addr1, PGSIZE);
	pte_t* pte;
	cprintf("We will display page-aligned virtual addresses, followed by interesting details about them\n");
	for (; addr1 <= addr2; addr1 += PGSIZE) {
		cprintf("0x%x: ", addr1);
		pte = pgdir_walk(kern_pgdir, (void *) addr1, 0);
		if (!pte) {
			cprintf("No pte\n");
		} else if (!(*pte & PTE_P)) {
			cprintf("No page present\n");
		} else {
			cprintf("Page mapped at physical address 0x%x, with hex permissions %x\n", (*pte & ~0xfff), (*pte & 0xfff));
		}
		if (addr1 == -PGSIZE) return 0;
	}
	return 0;
}

int
mon_setperms(int argc, char **argv, struct Trapframe *tf)
{
	if (argc != 3) {
		cprintf("Unable to parse request...\n");
		cprintf("Please provide a virtual address and permissions\n");
		return 0;
	}
	uintptr_t* addrptr;
	if (!parseAddr(argv[1], &addrptr)) {
		cprintf("Parse failure for address argument... exiting\n");
		return 0;
	}
	uintptr_t addr = *addrptr;
	uintptr_t* permptr;
	if (!parseAddr(argv[2], &permptr)) {
		cprintf("Parse failure for perm argument... exiting\n");
		return 0;
	}
	int perms = ((int) *permptr) & 0xfff;
	cprintf("Successfully parsed with virtual address 0x%x and with permissions 0x%x\n", addr, perms);
	cprintf("We will now attempt to change the permissions of the mapping from provided virtual address\n");
	pte_t* pte;
	pte = pgdir_walk(kern_pgdir, (void *) addr, 0);
	if (!pte) {
		cprintf("No pte, please use a different virtual address\n");
	} else if (!(*pte & PTE_P)) {
		cprintf("No page present, please use a different virtual address\n");
	} else if (!(perms & PTE_P)) {
		cprintf("Please enter a set of permissions that includes the present bit\n");
	} else {
		*pte &= ~0xfff;
		*pte |= perms;
		cprintf("New page table entry is: 0x%x\n", *pte);
		tlbflush();
	}
	return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
