#ifndef __TCP_OUT_H_
#define __TCP_OUT_H_

#include "mtcp.h"
#include "tcp_stream.h"

enum ack_opt
{
	ACK_OPT_NOW, 
	ACK_OPT_AGGREGATE, 
	ACK_OPT_WACK
};

int
SendTCPPacketStandalone(struct mtcp_manager *mtcp, 
		uint32_t saddr, uint16_t sport, uint32_t daddr, uint16_t dport, 
		uint32_t seq, uint32_t ack_seq, uint16_t window, uint8_t flags, 
		uint8_t *payload, uint16_t payloadlen, 
		uint32_t cur_ts, uint32_t echo_ts, uint16_t ip_id, int8_t in_ifidx);

int
SendTCPPacket(struct mtcp_manager *mtcp, tcp_stream *cur_stream,
		uint32_t cur_ts, uint8_t flags, uint8_t *payload, uint16_t payloadlen);

extern inline int 
WriteTCPControlList(mtcp_manager_t mtcp, 
		struct mtcp_sender *sender, uint32_t cur_ts, int thresh);

extern inline int
WriteTCPDataList(mtcp_manager_t mtcp, 
		struct mtcp_sender *sender, uint32_t cur_ts, int thresh);

extern inline int 
WriteTCPACKList(mtcp_manager_t mtcp, 
		struct mtcp_sender *sender, uint32_t cur_ts, int thresh);

extern inline void 
AddtoControlList(mtcp_manager_t mtcp, tcp_stream *cur_stream, uint32_t cur_ts);

extern inline void 
AddtoSendList(mtcp_manager_t mtcp, tcp_stream *cur_stream);

extern inline void 
RemoveFromControlList(mtcp_manager_t mtcp, tcp_stream *cur_stream);

extern inline void 
RemoveFromSendList(mtcp_manager_t mtcp, tcp_stream *cur_stream);

extern inline void 
RemoveFromACKList(mtcp_manager_t mtcp, tcp_stream *cur_stream);

extern inline void
EnqueueACK(mtcp_manager_t mtcp, 
		tcp_stream *cur_stream, uint32_t cur_ts, uint8_t opt);

extern inline void 
DumpControlList(mtcp_manager_t mtcp, struct mtcp_sender *sender);

void
UpdatePassiveSendTCPContext(mtcp_manager_t mtcp, struct tcp_stream *cur_stream, 
			    struct pkt_ctx *pctx);

void
PostSendTCPAction(mtcp_manager_t mtcp, struct pkt_ctx *pctx, 
		  struct tcp_stream *recvside_stream, 
		  struct tcp_stream *sendside_stream);

#endif /* __TCP_OUT_H_ */
