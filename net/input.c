#include "ns.h"

extern union Nsipc nsipcbuf;

// TODO
// 1) jp and its length?
// 2) memove?

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	//sys_receive_packet(void* buf);
	//sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, int perm);
	//sys_page_alloc(envid_t envid, void *va, int perm);
	while (1) {
		struct jif_pkt* packet = &(nsipcbuf.pkt);
		packet->jp_len = 0;
		while ((packet->jp_len = sys_receive_packet(packet->jp_data)) <= 0) {
			sys_yield();
		}
		cprintf("packet has been recv\n");
		assert(sys_page_alloc(0, UTEMP, PTE_P | PTE_U | PTE_W) >= 0);
		memmove(UTEMP, (void*) packet, sizeof(nsipcbuf));
		int r;
		while ((r = sys_ipc_try_send(ns_envid, NSREQ_INPUT, UTEMP, PTE_P | PTE_U | PTE_W)) == -E_IPC_NOT_RECV) {
			//cprintf("%e", r);
			sys_yield();
		}
		cprintf("sent pkt\n");
	}
}
