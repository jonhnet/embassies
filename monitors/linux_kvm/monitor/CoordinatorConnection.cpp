#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <assert.h>
#include <malloc.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include "LiteLib.h"
#include "xax_network_utils.h"
#include "safety_check.h"
#include "CoordinatorConnection.h"
#include "LongMessageAllocator.h"

#define QUEUE_MAX_BYTES		(256*(1<<20))
#define QUEUE_MIN_PACKETS	(3)

CoordinatorConnection::CoordinatorConnection(
	MallocFactory *mf,
	SyncFactory *sf,
	TunIDAllocator *tunid,
	EventSynchronizerIfc *event_synchronizer)
	: long_message_allocator(mf, sf, false, tunid),
	  sf(sf)
{
	connect_complete_recvd = false;

	this->max_bytes = QUEUE_MAX_BYTES;
	this->min_packets = QUEUE_MIN_PACKETS;
	this->tunid = tunid->get_tunid();

	ifconfigs_ready = sf->new_event(false);
	
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	count = 0;
	size_bytes = 0;
	linked_list_init(&queue, mf);

	this->uieq = new UIEventQueue(mf, sf, event_synchronizer);
	hash_table_init(&rpc_table, mf, RPCToken::hash, RPCToken::cmp);
	next_nonce = 7;
}

void CoordinatorConnection::connect(ZPubKey *pub_key)
{
	sock = socket(PF_UNIX, SOCK_DGRAM, 0);
	lite_assert(sock>=0);

	int rc;
	localaddr.sun_family = PF_UNIX;
	sprintf(localaddr.sun_path, "%s:%d.%d", ZOOG_KVM_RENDEZVOUS, tunid, getpid());
	localaddr.sun_path[sizeof(localaddr.sun_path)-1] = '\0';
	unlink(localaddr.sun_path);
	rc = bind(sock, (struct sockaddr *) &localaddr, sizeof(localaddr));
	lite_assert(rc==0);

	struct sockaddr_un dstaddr;
	dstaddr.sun_family = PF_UNIX;
	sprintf(dstaddr.sun_path, "%s:%d", ZOOG_KVM_RENDEZVOUS, tunid);
	dstaddr.sun_path[sizeof(dstaddr.sun_path)-1] = '\0';

	rc = ::connect(sock, (struct sockaddr *) &dstaddr, sizeof(dstaddr));
	lite_assert(rc==0);
/*
	struct sockaddr_un srcaddr;
	rc = getsockname(sock, (struct sockaddr *) &srcaddr, sizeof(srcaddr));
	lite_assert(rc==0);
*/

	uint32_t msg_len = sizeof(CMConnect) + pub_key->size();
	CMConnect *connect_msg = (CMConnect *) malloc(msg_len);
	connect_msg->hdr.length = msg_len;
	connect_msg->hdr.opcode = co_connect;
	connect_msg->pub_key_len = pub_key->size();
	pub_key->serialize((uint8_t*) connect_msg->pub_key);
	_send(connect_msg, connect_msg->hdr.length);
	free(connect_msg);

	pthread_create(&recv_thread, NULL, _recv_thread, this);
}

CoordinatorConnection::~CoordinatorConnection()
{
	while (count>0)
	{
		Packet *p = dequeue_packet();
		delete p;
	}
	lite_assert(size_bytes == 0);
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
	unlink(localaddr.sun_path);
}

void CoordinatorConnection::_send(void *data, uint32_t len)
{
	int rc;
	rc = send(sock, data, len, 0);
	lite_assert(rc==(int)len);
}

void CoordinatorConnection::_send_long_message(CMHeader *header, uint32_t header_len, void *data, uint32_t data_len)
{
	uint32_t total_len = header_len + data_len;
	lite_assert(header->length == total_len);

	LongMessageAllocation *lma = long_message_allocator.allocate(total_len);
	// TODO a memcpy to eviscerate
	// unimportant now, as it only happens rarely (on launch_application).
	uint8_t *mapping = (uint8_t*) lma->map();
	memcpy(mapping, header, header_len);
	memcpy(mapping+header_len, data, data_len);
	lma->unmap();

	char buf[200];
	CMLongMessage *cml = (CMLongMessage *) buf;
	int used_len = sizeof(*cml)-sizeof(cml->id);
	int id_len = lma->marshal(&cml->id, sizeof(buf)-used_len);
	cml->hdr.opcode = co_long_message;
	cml->hdr.length = used_len + id_len;
	assert(cml->hdr.length <= sizeof(buf));

	_send(cml, cml->hdr.length);
	lma->drop_ref("_send_long_message");
}

void *CoordinatorConnection::_recv_thread(void *v_this)
{
	((CoordinatorConnection*) v_this)->_recv_loop();
	return NULL;
}

void CoordinatorConnection::_recv_loop()
{
	int read_size;
	uint8_t buf[ZK_SMALL_MESSAGE_LIMIT];
	while (true)
	{
		read_size = read(sock, buf, sizeof(buf));
		if (read_size<=0)
		{
			fprintf(stderr, "*** lost connection to coordinator\n");
			lite_assert(false);
			return;
		}

		CMHeader *cmh = (CMHeader*) buf;
		lite_assert((int) cmh->length == read_size);
		dispatch_message(cmh);
	}
}

void CoordinatorConnection::dispatch_message(CMHeader *header)
{
		switch (header->opcode)
		{
		case co_connect_complete:
			connect_complete((CMConnectComplete*) header);
			break;
		case co_long_message:
			decode_long_message((CMLongMessage*) header);
			break;
		case co_deliver_packet:
			deliver_packet(header->length, (CMDeliverPacket*) header);
			break;
		case co_deliver_packet_long:
			deliver_packet_long(header->length, (CMDeliverPacketLong*) header);
			break;
		case co_sublet_viewport_reply:
		case co_repossess_viewport_reply:
		case co_get_deed_key_reply:
		case co_accept_viewport_reply:
		case co_transfer_viewport_reply:
		case co_verify_label_reply:
		case co_map_canvas_reply:
		case co_unmap_canvas_reply:
		case co_extn_debug_create_toplevel_window_reply:
		case co_get_monitor_key_pair_reply:
		case co_alloc_long_message_reply:
			accept_rpc_reply((CMRPCHeader*) header);
			break;
		case co_deliver_ui_event:
			deliver_ui_event((CMDeliverUIEvent*) header);
			break;
		default:
			lite_assert(false);	// unexpected input
			return;
		}
}

void CoordinatorConnection::decode_long_message(CMLongMessage* lm)
{
	LongMessageAllocation *lma = long_message_allocator
		.ingest(&lm->id, lm->hdr.length - offsetof(CMLongMessage, id));
	void *long_buf = lma->map();

	CMHeader *hdr = (CMHeader *) long_buf;
	lite_assert(hdr->length == lma->get_mapped_size());
	// DONE: "save a memcpy by delivering this message abstractly,
	// mmaping it directly into guest memory. Right now, Packet() copies the
	// data." -- now long messages come in as deliver_packet_long.
	dispatch_message(hdr);
	lma->unmap();

	send_free_long_message(lma);
	lma->drop_ref("decode_long_message");
}

void CoordinatorConnection::send_free_long_message(LongMessageAllocation* lma)
{
	uint8_t buffer[1000];
	CMFreeLongMessage* fml = (CMFreeLongMessage*) buffer;
	fml->hdr.opcode = co_free_long_message;
	int id_len = lma->marshal(&fml->id, sizeof(buffer)-offsetof(CMFreeLongMessage, id));
	fml->hdr.length = sizeof(CMFreeLongMessage) + id_len;

	_send(fml, fml->hdr.length);
}

void CoordinatorConnection::connect_complete(CMConnectComplete *cc)
{
	lite_assert(!connect_complete_recvd);
	lite_assert(sizeof(ifconfigs)==sizeof(cc->ifconfigs));
	memcpy(ifconfigs, cc->ifconfigs, sizeof(ifconfigs));
	ifconfigs_ready->signal();
	connect_complete_recvd = true;
}

void CoordinatorConnection::deliver_packet(uint32_t cm_size, CMDeliverPacket *dp)
{
	Packet *p = new Packet(cm_size, dp);
	_enqueue_packet(p);
}

void CoordinatorConnection::deliver_packet_long(uint32_t cm_size, CMDeliverPacketLong* dpl)
{
	LongMessageAllocation *lma = long_message_allocator
		.ingest(
			&dpl->id,
			dpl->hdr.length - offsetof(CMDeliverPacketLong, id));
	Packet *p = new Packet(lma);
	_enqueue_packet(p);
}

void CoordinatorConnection::_enqueue_packet(Packet *p)
{
	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&cond);
	linked_list_insert_tail(&queue, p);
	count += 1;
	size_bytes += p->get_dbg_size();
	_tidy_nolock();
	pthread_mutex_unlock(&mutex);
}

#define RPC_START(Treq) \
	Treq req; \
	Treq##Reply reply; \
	RPCToken* token = rpc_start(&req.rpc, (CoordinatorOpcode) Treq::OPCODE, sizeof(req), &reply.rpc, (CoordinatorOpcode) Treq##Reply::OPCODE, sizeof(reply));

RPCToken* CoordinatorConnection::rpc_start(
	CMRPCHeader *req,
	CoordinatorOpcode req_opcode,
	uint32_t req_len,
	CMRPCHeader *reply,
	CoordinatorOpcode reply_opcode,
	uint32_t reply_len)
{
	uint32_t nonce;
	pthread_mutex_lock(&mutex);
	nonce = next_nonce++;
	pthread_mutex_unlock(&mutex);

	req->hdr.length = req_len;
	req->hdr.opcode = req_opcode;
	req->rpc_nonce = nonce;
	reply->hdr.length = reply_len;	// do these even make sense?
	reply->hdr.opcode = reply_opcode;	// do these even make sense?
	reply->rpc_nonce = nonce;
	RPCToken *rpc_token = new RPCToken(req, reply, this);
	hash_table_insert(&rpc_table, rpc_token);
	return rpc_token;
}

void CoordinatorConnection::accept_rpc_reply(CMRPCHeader *rpc)
{
	RPCToken key(rpc->rpc_nonce);
	RPCToken *token = (RPCToken *) hash_table_lookup(&rpc_table, &key);
	hash_table_remove(&rpc_table, token);
	token->reply_arrived(rpc);
}

SyncFactory *CoordinatorConnection::_get_sf()
{
	return sf;
}

//////////////////////////////////////////////////////////////////////////////

void CoordinatorConnection::send_sublet_viewport(xa_sublet_viewport *xa)
{
	RPC_START(CMSubletViewport)
	req.in_tenant_viewport = xa->in_tenant_viewport;
	req.in_rectangle = xa->in_rectangle;
	token->transact();
	xa->out_landlord_viewport = reply.out_landlord_viewport;
	xa->out_deed = reply.out_deed;
}

void CoordinatorConnection::send_repossess_viewport(xa_repossess_viewport *xa)
{
	RPC_START(CMRepossessViewport)
	req.in_landlord_viewport = xa->in_landlord_viewport;
	token->transact();
}

void CoordinatorConnection::send_get_deed_key(xa_get_deed_key *xa)
{
	RPC_START(CMGetDeedKey)
	req.in_landlord_viewport = xa->in_landlord_viewport;
	token->transact();
	xa->out_deed_key = reply.out_deed_key;
}

void CoordinatorConnection::send_accept_viewport(xa_accept_viewport *xa)
{
	RPC_START(CMAcceptViewport)
	req.in_deed = xa->in_deed;
	token->transact();
	xa->out_tenant_viewport = reply.out_tenant_viewport;
	xa->out_deed_key = reply.out_deed_key;
}

void CoordinatorConnection::send_transfer_viewport(xa_transfer_viewport *xa)
{
	RPC_START(CMTransferViewport)
	req.in_tenant_viewport = xa->in_tenant_viewport;
	token->transact();
	xa->out_deed = reply.out_deed;
}

void CoordinatorConnection::send_verify_label()
{
	// NB semantics are that the chain is already verified by the
	// (trusted) monitor; this is just a synchronous call to tell
	// the coordinator to start allowing & labeling windows
	RPC_START(CMVerifyLabel)
	token->transact();
}

void CoordinatorConnection::send_map_canvas(CanvasAcceptor *ca)
{
	RPC_START(CMMapCanvas)
	req.viewport_id = ca->get_viewport_id();
	assert(ca->get_num_formats()<=4);
	memcpy(req.known_formats, ca->get_known_formats(),
		sizeof(req.known_formats[0])*ca->get_num_formats());
	req.num_formats = ca->get_num_formats();
	token->wait();

	CMMapCanvasReply* full_reply = (CMMapCanvasReply*) token->get_full_reply();
	LongMessageAllocation *lma = long_message_allocator
		.ingest(
			&full_reply->id,
			full_reply->rpc.hdr.length - offsetof(CMMapCanvasReply, id));
	void *addr = ca->allocate_memory_for_incoming_canvas(lma->get_mapped_size());
	lma->map(addr);
//	lma->drop_ref("send_map_canvas");	// don't want to shmdt it!
	ca->signal_canvas_ready(&reply.canvas);
	delete token;
}

void CoordinatorConnection::send_unmap_canvas(xa_unmap_canvas *xa)
{
	RPC_START(CMUnmapCanvas)
	req.canvas_id = xa->in_canvas_id;
	token->transact();
}

void CoordinatorConnection::send_update_canvas(xa_update_canvas *xa)
{
	CMUpdateCanvas cmuc;
	cmuc.hdr.length = sizeof(cmuc);
	cmuc.hdr.opcode = co_update_canvas;
	cmuc.canvas_id = xa->in_canvas_id;
	cmuc.rectangle = xa->in_rectangle;
	_send(&cmuc, cmuc.hdr.length);
}

void CoordinatorConnection::deliver_ui_event(CMDeliverUIEvent *du)
{
	uieq->deliver_ui_event(&du->event);
}

void CoordinatorConnection::dequeue_ui_event(ZoogUIEvent *out_event)
{
	ZoogUIEvent *event = uieq->remove();
	if (event!=NULL)
	{
		memcpy(out_event, event, sizeof(ZoogUIEvent));
	}
	else
	{
		out_event->type = zuie_no_event;
	}
}

Packet *CoordinatorConnection::_dequeue_nolock()
{
	while (queue.count==0)
	{
		pthread_cond_wait(&cond, &mutex);
	}
	Packet *p = (Packet *) linked_list_remove_head(&queue);
	count -= 1;
	size_bytes -= p->get_dbg_size();
	return p;
}

Packet *CoordinatorConnection::dequeue_packet()
{
	pthread_mutex_lock(&mutex);
	Packet *p = _dequeue_nolock();
	pthread_mutex_unlock(&mutex);
	return p;
}

void CoordinatorConnection::_tidy_nolock()
{
	while (size_bytes > max_bytes && count > min_packets)
	{
		Packet *p = _dequeue_nolock();
		fprintf(stderr, "queue overfull; tossing 0x%08x-byte packet\n",
			p->get_size());
		delete p;
	}
}

void CoordinatorConnection::send_packet(NetBuffer *nb)
{
	int rc;
	IPInfo info;
	ZoogNetBuffer *znb = nb->get_host_znb_addr();
	uint32_t trusted_capacity = nb->get_trusted_capacity();
	decode_ip_packet(&info, ZNB_DATA(znb), trusted_capacity);
	SAFETY_CHECK(info.packet_valid);
	SAFETY_CHECK(znb->capacity <= trusted_capacity);	// something stomped!
	SAFETY_CHECK(info.packet_length <= trusted_capacity); // need to implement big-packet transfer


#if 0	// the memcpy way!
	if (sizeof(CMDeliverPacket) +info.packet_length > ZK_SMALL_MESSAGE_LIMIT)
	{
		// TODO don't memcpy this long_message; figure out how to share it
		// by allocating net buffers more cleverly.
		_send_long_message(&cmhdr.hdr, sizeof(cmhdr), ZNB_DATA(znb), info.packet_length);
	}
#endif

	if (nb->get_lma()!=NULL)
	{
		uint8_t buf[1000];
		CMDeliverPacketLong* cmhdrl = (CMDeliverPacketLong*) buf;
		cmhdrl->hdr.opcode = co_deliver_packet_long;
#if 0
		lite_assert(sizeof(cmhdrl->prefix) <= info.packet_length);
			// it IS a LONG packet, right? Don't make me compute a min!
		memcpy(cmhdrl->prefix, ZNB_DATA(znb), sizeof(cmhdrl->prefix));
#endif
		int id_len = nb->get_lma()->marshal(
			&cmhdrl->id, sizeof(buf)-offsetof(CMDeliverPacketLong, id));
		int total_len = id_len + sizeof(CMDeliverPacketLong);
		cmhdrl->hdr.length = total_len;

		rc = write(sock, buf, total_len);
		lite_assert(rc==total_len);
	}
	else
	{
		CMDeliverPacket cmhdr;
		cmhdr.hdr.length = sizeof(cmhdr)+info.packet_length;
		cmhdr.hdr.opcode = co_deliver_packet;

		// for short-ish packets, we avoid one copy by issuing a gather-write.
		struct iovec iov[2];
		iov[0].iov_base = &cmhdr;
		iov[0].iov_len = sizeof(cmhdr);
		iov[1].iov_base = ZNB_DATA(znb);
		iov[1].iov_len = info.packet_length;
		rc = writev(sock, iov, 2);
		lite_assert(rc==(int) (iov[0].iov_len + iov[1].iov_len));
	}

fail:
	return;
}

XIPifconfig *CoordinatorConnection::get_ifconfigs(int *out_count)
{
	ifconfigs_ready->wait();
	*out_count = sizeof(ifconfigs)/sizeof(ifconfigs[0]);
	return ifconfigs;
}

void CoordinatorConnection::send_launch_application(void *image, uint32_t image_len)
{
	CMLaunchApplication cmla;
	cmla.hdr.length = sizeof(cmla) + image_len;
	cmla.hdr.opcode = co_launch_application;
	_send_long_message(&cmla.hdr, sizeof(cmla), image, image_len);
}

void CoordinatorConnection::send_extn_debug_create_toplevel_window(xa_extn_debug_create_toplevel_window *xa)
{
	RPC_START(CMExtnDebugCreateToplevelWindow)
	token->transact();
	xa->out_viewport_id = reply.out_viewport_id;
}

ZKeyPair* CoordinatorConnection::send_get_monitor_key_pair()
{
	RPC_START(CMGetMonitorKeyPair)
	token->wait();
	CMGetMonitorKeyPairReply* full_reply = (CMGetMonitorKeyPairReply*) token->get_full_reply();
	ZKeyPair* kp = new ZKeyPair(full_reply->monitor_key_pair, full_reply->monitor_key_pair_len);
	delete token;
	return kp;
}

LongMessageAllocation *CoordinatorConnection::send_alloc_long_message(uint32_t size)
{
	RPC_START(CMAllocLongMessage)
	req.size = size;
	token->transact();
	LongMessageAllocation *lma = long_message_allocator
		.ingest(&reply.id,
			reply.rpc.hdr.length - offsetof(CMAllocLongMessageReply, id));
	return lma;
}

