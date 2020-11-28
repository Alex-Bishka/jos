#include "ns.h"
#include "inc/lib.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	while(true) {
		sys_ipc_recv((void*) REQVA);
		struct jif_pkt* pkt = (struct jif_pkt*) REQVA;
		sys_transmit_packet(pkt->jp_data, pkt->jp_len);
	}
}
