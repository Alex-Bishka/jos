#include "ns.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	while (1) {
		struct jif_pkt* packet = &(nsipcbuf.pkt);
		packet->jp_len = 0;
		while ((packet->jp_len = sys_receive_packet(packet->jp_data, PGSIZE - sizeof(packet->jp_len))) <= 0) {
			sys_yield();
		}
		assert(sys_page_alloc(0, UTEMP, PTE_P | PTE_U | PTE_W) >= 0);
		memmove(UTEMP, (void*) packet, sizeof(nsipcbuf));
		int r;
		while ((r = sys_ipc_try_send(ns_envid, NSREQ_INPUT, UTEMP, PTE_P | PTE_U | PTE_W)) == -E_IPC_NOT_RECV) {
			sys_yield();
		}
	}
}
