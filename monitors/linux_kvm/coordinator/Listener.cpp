#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <malloc.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fstream>

using namespace std;

#include "LiteLib.h"
#include "standard_malloc_factory.h"
#include "SyncFactory_Pthreads.h"
#include "ThreadFactory_Pthreads.h"
#include "CoordinatorProtocol.h"
#include "Listener.h"
#include "App.h"
#include "MonitorPacket.h"
#include "LongPacket.h"
#include "SockaddrHashable.h"
#include "LinuxKvmCanvasAcceptor.h"
#include "BlitterViewport.h"
#include "BlitterCanvas.h"

#define MONITOR_PATH	ZOOG_ROOT "/monitors/linux_kvm/monitor/build/zoog_kvm_monitor"

Listener::Listener(MallocFactory *mf)
	: mf(mf),
	  sf(new SyncFactory_Pthreads()),
	  crypto(MonitorCrypto::WANT_MONITOR_KEY_PAIR)
{
	tunnel_address_allocator = new CoalescingAllocator(sf, false);
	tunnel_address_allocator->create_empty_range(Range(2,255));

	sockaddr_to_app_table = new Table(mf);

	ThreadFactory* tf = new ThreadFactory_Pthreads();
	Xblit *xblit = new Xblit(tf, sf);
	blitter_mgr = new BlitterManager(xblit, mf, sf, crypto.get_random_supply());

	router = new Router(sf, mf, &tunid);

	long_message_allocator = new LongMessageAllocator(mf, sf, true, &tunid);
	shm = new Shm(tunid.get_tunid());

	rendezvous_addr.sun_family = PF_UNIX;
	sprintf(rendezvous_addr.sun_path, "%s:%d", ZOOG_KVM_RENDEZVOUS, tunid.get_tunid());
	rendezvous_addr.sun_path[sizeof(rendezvous_addr.sun_path)-1] = '\0';

	int rc;
	rc = unlink(rendezvous_addr.sun_path);
	if (!(rc==0 || errno==ENOENT))
	{
		fprintf(stderr, "Can't remove %s: %s.\n",
			rendezvous_addr.sun_path, strerror(errno));
		assert(false);
	}

	rendezvous_socket = socket(PF_UNIX, SOCK_DGRAM, 0);
	assert(rendezvous_socket>=0);

	rc = fcntl(rendezvous_socket, F_SETFD, FD_CLOEXEC);
	assert(rc==0);

	rc = bind(rendezvous_socket, (struct sockaddr*) &rendezvous_addr, sizeof(rendezvous_addr));
	assert(rc==0);

	rc = chmod(rendezvous_addr.sun_path, 0777);
	if (rc!=0)
	{
		perror("chmod");
		assert(rc==0);
	}
}

void Listener::run()
{
	while (true)
	{
		Message *msg = new Message(rendezvous_socket);
		dispatch(msg);
	}
}

App *Listener::msg_to_app(Message *msg, CoordinatorOpcode opcode)
{
	switch (opcode)
	{
	case co_free_long_message:
	case co_sublet_viewport:
	case co_repossess_viewport:
	case co_get_deed_key:
	case co_accept_viewport:
	case co_transfer_viewport:
	case co_verify_label:
	case co_map_canvas:
	case co_unmap_canvas:
	case co_extn_debug_create_toplevel_window:
	case co_get_monitor_key_pair:
	case co_alloc_long_message:
	{
		SockaddrHashable saddr(msg->get_remote_addr(), msg->get_remote_addr_len());
		App *app = (App*) sockaddr_to_app_table->lookup_left(&saddr);
		return app;
	}
	default:
		return NULL;
	}
}

void Listener::dispatch(Message *msg)
{
	CMHeader *hdr = (CMHeader *) msg->get_payload();

	lite_assert(hdr->length == msg->get_payload_size());
	App* app = msg_to_app(msg, hdr->opcode);
	switch (hdr->opcode)
	{
	case co_connect:
		connect(msg, (CMConnect*) hdr);
		msg = NULL;
		break;
	case co_deliver_packet:
		deliver_packet(msg);
		msg = NULL;
		break;
	case co_deliver_packet_long:
		deliver_packet_long((CMDeliverPacketLong*) hdr);
		break;
	case co_launch_application:
		launch_application((CMLaunchApplication*) hdr);
		break;
	case co_long_message:
		msg->ingest_long_message(get_long_message_allocator(), (CMLongMessage *)hdr);
		dispatch(msg);
		msg = NULL;	// it's the recursive invocation's job to clean up msg
		break;
	case co_free_long_message:
		free_long_message(app, (CMFreeLongMessage *) hdr);
		break;
	case co_sublet_viewport:
		sublet_viewport(app, (CMSubletViewport*) hdr);
		break;
	case co_repossess_viewport:
		repossess_viewport(app, (CMRepossessViewport*) hdr);
		break;
	case co_get_deed_key:
		get_deed_key(app, (CMGetDeedKey*) hdr);
		break;
	case co_accept_viewport:
		accept_viewport(app, (CMAcceptViewport*) hdr);
		break;
	case co_transfer_viewport:
		transfer_viewport(app, (CMTransferViewport*) hdr);
		break;
	case co_verify_label:
		verify_label(app, (CMVerifyLabel*) hdr);
		break;
	case co_map_canvas:
		map_canvas(app, (CMMapCanvas *) hdr);
		break;
	case co_unmap_canvas:
		unmap_canvas(app, (CMUnmapCanvas *) hdr);
		break;
	case co_update_canvas:
		update_canvas((CMUpdateCanvas *) hdr);
		break;
	case co_extn_debug_create_toplevel_window:
		extn_debug_create_toplevel_window(app, (CMExtnDebugCreateToplevelWindow*) hdr);
		break;
	case co_get_monitor_key_pair:
		get_monitor_key_pair(app, (CMGetMonitorKeyPair*) hdr);
		break;
	case co_alloc_long_message:
		alloc_long_message(app, (CMAllocLongMessage*) hdr);
		break;
	default:
		lite_assert(false);
	}

	if (msg!=NULL)
	{
		delete msg;
	}
}

void Listener::connect(Message *msg, CMConnect *conn)
{
	TunnelAddress *ta = new TunnelAddress();
	Range range;
	bool rc = tunnel_address_allocator->allocate_range(1, ta, &range);
	if (!rc)
	{
		delete ta;
		lite_assert(false);	// out of addresses
		// TODO signal app that it is screwed.
		return;
	}

	ZPubKey *pub_key = new ZPubKey((uint8_t*) &conn[1], conn->pub_key_len);
	App *app = new App(this, pub_key, msg, range.start);
	SockaddrHashable *saddr = new SockaddrHashable(msg->get_remote_addr(), msg->get_remote_addr_len());
	sockaddr_to_app_table->insert(saddr, app);

	blitter_mgr->register_palifc(app);
	blitter_mgr->new_toplevel_viewport(app->get_id());
}

void Listener::launch_application(CMLaunchApplication *la)
{
	const char *boot_block_temp = "/tmp/zoog_boot_block.XXXXXX";
	char tempfile[100];
	strcpy(tempfile, boot_block_temp);
	int fd = mkstemp(tempfile);
	FILE *fp = fdopen(fd, "w");
	int rc = fwrite(
		la->boot_block, la->hdr.length-sizeof(CMLaunchApplication), 1, fp);
	lite_assert(rc==1);
	fclose(fp);

	int pid = fork();
	if (pid==0)
	{
		// child
		// TODO figure out how to detach child so it doesn't end up <defunct>.
		char *argv[10];
		argv[0] = (char*) MONITOR_PATH;
		argv[1] = (char*) "--delete-image-file";
		argv[2] = (char*) "true";
		argv[3] = (char*) "--image-file";
		argv[4] = tempfile;
#if 1
		argv[5] = NULL;
#else
		argv[5] = (char*) "--wait-for-debugger";
		argv[6] = (char*) "true";
		argv[7] = NULL;
#endif
		execve(MONITOR_PATH, argv, NULL);
		fprintf(stderr, "exec(%s) failed\n", MONITOR_PATH);
		perror("execve");
		assert(false);	// notreached
	}
}

void Listener::free_long_message(App *app, CMFreeLongMessage *flm)
{
	app->free_long_message(flm);
}

void Listener::deliver_packet(Message *msg)
{
	Packet *packet = new MonitorPacket(msg);
	router->deliver_packet(packet);
}

void Listener::deliver_packet_long(CMDeliverPacketLong* dpl)
{
	LongMessageAllocation* lma = get_long_message_allocator()
		->lookup_existing(
			&dpl->id,
			dpl->hdr.length - offsetof(CMDeliverPacketLong, id));
	Packet *packet = new LongPacket(dpl, lma);
	router->deliver_packet(packet);
}

#define DECLARE_RPC_REPLY(Treply) \
	Treply reply; \
	reply.rpc.hdr.length = sizeof(Treply); \
	reply.rpc.hdr.opcode = (CoordinatorOpcode) Treply::OPCODE; \
	reply.rpc.rpc_nonce = hdr->rpc.rpc_nonce;
#define SEND_REPLY() \
	app->send_cm(&reply.rpc.hdr, reply.rpc.hdr.length);

void Listener::sublet_viewport(App* app, CMSubletViewport* hdr)
{
	BlitterViewport *parent_viewport = blitterMgr()->get_tenant_viewport(hdr->in_tenant_viewport, app->get_id());
	BlitterViewport *child_viewport = parent_viewport->sublet(&hdr->in_rectangle, app->get_id());

	DECLARE_RPC_REPLY(CMSubletViewportReply)
	reply.out_landlord_viewport = child_viewport->get_landlord_id();
	reply.out_deed = child_viewport->get_deed();
	SEND_REPLY();
}

void Listener::repossess_viewport(App* app, CMRepossessViewport* hdr)
{
	BlitterViewport *landlord_viewport = blitterMgr()->get_landlord_viewport(hdr->in_landlord_viewport, app->get_id());
	delete landlord_viewport;

	DECLARE_RPC_REPLY(CMRepossessViewportReply)
	SEND_REPLY();
}

void Listener::get_deed_key(App* app, CMGetDeedKey* hdr)
{
	BlitterViewport *landlord_viewport = blitterMgr()->get_landlord_viewport(hdr->in_landlord_viewport, app->get_id());

	DECLARE_RPC_REPLY(CMGetDeedKeyReply)
	reply.out_deed_key = landlord_viewport->get_deed_key();
	SEND_REPLY();
}

void Listener::accept_viewport(App* app, CMAcceptViewport* hdr)
{
	BlitterViewport *viewport = blitterMgr()->accept_viewport(hdr->in_deed, app->get_id());	// this lookup is global, as the deed is a cryptographic capability
	lite_assert(viewport!=NULL);	// safety: deed was bogus

	DECLARE_RPC_REPLY(CMAcceptViewportReply)
	reply.out_tenant_viewport = viewport->get_tenant_id();
	reply.out_deed_key = viewport->get_deed_key();
	SEND_REPLY();
}

void Listener::transfer_viewport(App* app, CMTransferViewport* hdr)
{
	BlitterViewport *tenant_viewport = blitterMgr()->get_tenant_viewport(hdr->in_tenant_viewport, app->get_id());
	lite_assert(tenant_viewport->get_tenant_id()==hdr->in_tenant_viewport);	// safety: tenant_viewport handle was a landlord handle; what you tryin' to pull?

	DECLARE_RPC_REPLY(CMTransferViewportReply)
	reply.out_deed = tenant_viewport->transfer();
	SEND_REPLY();
}


void Listener::verify_label(App* app, CMVerifyLabel *hdr)
{
	app->verify_label();

	DECLARE_RPC_REPLY(CMVerifyLabelReply)
	SEND_REPLY();
}

void Listener::map_canvas(App* app, CMMapCanvas * hdr)
{
	const char *label = app->get_label();
	if (label == NULL)
	{
		lite_assert(false); // Client asked to accept_canvas before verify_label
		return;
	}

	LinuxKvmCanvasAcceptor acceptor(app, get_long_message_allocator());
	blitterMgr()->get_tenant_viewport(hdr->viewport_id, app->get_id())
		->map_canvas(app, hdr->known_formats, hdr->num_formats, &acceptor, label);

	ZCanvas *canvas = acceptor.get_canvas();
	assert(canvas!=NULL);	// NULL => app asked too soon, and we don't have a way to return NULL to the app yet. Sigh.

	// hand-assemble this RPC reply because it has a variable-sized
	// component tacked on the end.
	uint8_t buf[200];
	CMMapCanvasReply *cmmcr = (CMMapCanvasReply *) buf;
	cmmcr->rpc.hdr.opcode = (CoordinatorOpcode) CMMapCanvasReply::OPCODE;
	cmmcr->rpc.rpc_nonce = hdr->rpc.rpc_nonce;

	int idlen = acceptor.get_lma()->marshal(
		&cmmcr->id, sizeof(buf)-sizeof(*cmmcr));
	cmmcr->rpc.hdr.length = offsetof(CMMapCanvasReply, id)+idlen;
	assert(cmmcr->rpc.hdr.length <= sizeof(buf));

	cmmcr->canvas = *canvas;

	app->send_cm(&cmmcr->rpc.hdr, cmmcr->rpc.hdr.length);
}

void Listener::unmap_canvas(App* app, CMUnmapCanvas * hdr)
{
	// TODO this get_canvas call should be filtered by app get_id
	BlitterCanvas *canvas = blitterMgr()->get_canvas(hdr->canvas_id);
	canvas->unmap();

	DECLARE_RPC_REPLY(CMUnmapCanvasReply)
	SEND_REPLY();
}

void Listener::update_canvas(CMUpdateCanvas *hdr)
{
	blitter_mgr->get_canvas(hdr->canvas_id)->update_canvas(&hdr->rectangle);
}

void Listener::extn_debug_create_toplevel_window(App* app, CMExtnDebugCreateToplevelWindow * hdr)
{
	ViewportID viewport = blitterMgr()->new_toplevel_viewport(app->get_id());

	DECLARE_RPC_REPLY(CMExtnDebugCreateToplevelWindowReply)
	reply.out_viewport_id = viewport;
	SEND_REPLY();
}

void Listener::get_monitor_key_pair(App* app, CMGetMonitorKeyPair* hdr)
{

	// DECLARE_RPC_REPLY(CMGetMonitorKeyPairReply)
	uint32_t key_pair_len = crypto.get_monitorKeyPair()->size();
	uint32_t len = sizeof(CMGetMonitorKeyPairReply) + key_pair_len;
	CMGetMonitorKeyPairReply *reply = (CMGetMonitorKeyPairReply *) malloc(len);
	reply->rpc.hdr.length = len;
	reply->rpc.hdr.opcode = (CoordinatorOpcode) CMGetMonitorKeyPairReply::OPCODE;
	reply->rpc.rpc_nonce = hdr->rpc.rpc_nonce;
	reply->monitor_key_pair_len = key_pair_len;
	crypto.get_monitorKeyPair()->serialize(reply->monitor_key_pair);

	// SEND_REPLY();
	app->send_cm(&reply->rpc.hdr, reply->rpc.hdr.length);
}

void Listener::alloc_long_message(App* app, CMAllocLongMessage* hdr)
{
	LongMessageAllocation* lma = get_long_message_allocator()->allocate(hdr->size);
	app->record_alloc_long_message(lma);

	// lma comes with a reference, which represents the remote side;
	// to be dropped by free_long_message
	DECLARE_RPC_REPLY(CMAllocLongMessageReply)
	lma->marshal(&reply.id, sizeof(reply.id)); // TODO this marshal will only work with Shm, since we don't allocate room for a variable-length mmap id. Which is fine since we're not using Mmap anymore.
	SEND_REPLY();
}

void Listener::disconnect(App *app)
{
	get_router()->disconnect_app(app);
	sockaddr_to_app_table->remove_right(app);
	blitter_mgr->unregister_palifc(app->as_palifc());
}
