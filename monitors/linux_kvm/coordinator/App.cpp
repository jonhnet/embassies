#include <errno.h>
#include <string.h>

#include "standard_malloc_factory.h"
#include "App.h"
#include "Listener.h"

const App::ShmMode App::mode = USE_SHM;

App::App(Listener *listener, ZPubKey *pub_key, Message *connect_message, int tunnel_address)
{
	this->pub_key = pub_key;
	this->listener = listener;
	this->connect_message = connect_message;
	this->destructing = false;
	hash_table_init(&this->outstanding_lmas, standard_malloc_factory_init(),
		LongMessageIdentifierIfc::hash, LongMessageIdentifierIfc::cmp);

	_create_ifconfig(ipv4, &ifconfigs[0], tunnel_address);
	_create_ifconfig(ipv6, &ifconfigs[1], tunnel_address);

	listener->get_router()->connect_app(this);

	CMConnectComplete cmc;
	cmc.hdr.length = sizeof(cmc);
	cmc.hdr.opcode = co_connect_complete;
	lite_assert(sizeof(ifconfigs)==sizeof(cmc.ifconfigs));
	memcpy(cmc.ifconfigs, ifconfigs, sizeof(ifconfigs));
	send_cm(&cmc.hdr, cmc.hdr.length);
}

void App::_create_ifconfig(XIPVer ipver, XIPifconfig *ifconfig, int tunnel_address)
{
	ifconfig->gateway = listener->get_router()->get_gateway(ipver);
	ifconfig->netmask = listener->get_router()->get_netmask(ipver);
	ifconfig->local_interface = listener->get_router()->get_gateway(ipver);

	lite_assert((tunnel_address & ~0xff)==0);
	switch (ipver)
	{
	case ipv4:
		ifconfig->local_interface.xip_addr_un.xv4_addr =
			(ifconfig->local_interface.xip_addr_un.xv4_addr & ~0xff000000)
			| (tunnel_address<<24);
		break;
	case ipv6:
		ifconfig->local_interface.xip_addr_un.xv6_addr.addr[15] = tunnel_address;
		break;
	default:
		lite_assert(false);
	}
}

App::~App()
{
	destructing = true;
	clean_up_outstanding_messages();
	listener->disconnect(this);
	delete connect_message;
}

void App::send_cm(CMHeader *header, uint32_t length)
{
	if (destructing)
	{
		return;
	}

	int rc;

	uint8_t ref_message_buf[100];
		// NB length check asserted below; length not determined by untrusted
		// input. It's just a temp filename generated inside
		// LongMessageAllocation.
	CMHeader *ref_message = (CMHeader*) ref_message_buf;

	// TODO remove the copy by toting a long message all the way from
	// ingestion (Message::ingest_long_message) to emission (here)
	// via LongMessageAllocator.
	if (length > ZK_SMALL_MESSAGE_LIMIT)
	{
		LongMessageAllocation *lma =
			listener->get_long_message_allocator()->allocate(length);

		// Set up the reference message to point at the newly-allocated
		// long message.
		CMLongMessage *cm_long = (CMLongMessage *) ref_message_buf;
		cm_long->hdr.opcode = co_long_message;
		int idlen = lma
			->marshal(&cm_long->id, sizeof(ref_message_buf)-sizeof(*cm_long));
		cm_long->hdr.length = offsetof(CMLongMessage, id) + idlen;
		lite_assert(cm_long->hdr.length <= sizeof(ref_message_buf));

		// and copy the original message into the allocated thing.
		memcpy(lma->map(), header, length);
		lma->unmap();

		// lma->add_ref();	// for the monitor's ref
		// lma->drop_ref();	// to drop the one I have here.

		// now change our arguments so we send the
		// small indirection message instead.
		header = ref_message;
		length = ref_message->length;
	}

	lite_assert(length <= ZK_SMALL_MESSAGE_LIMIT);
	lite_assert(length == header->length);
	rc = sendto(listener->get_rendezvous_socket(), header, length, 0,
		(const struct sockaddr *) connect_message->get_remote_addr(),
		connect_message->get_remote_addr_len());
	if (rc<0)
	{
		if (errno == ECONNREFUSED)
		{
			// TODO probably should synchronize this deletion with ...
			// other stuff.
			delete this;
			return;
		}
		else
		{
			lite_assert(false);	// unanticipated error
		}
	}
	lite_assert(rc== (int) length);
}

void App::send_packet(Packet *p)
{
	if (p->get_as_cm())
	{
		send_cm(&p->get_as_cm()->hdr, p->get_cm_size());
	}
	else
	{
		LongMessageAllocation* lma = p->as_lma();
		lma->add_ref("send_packet");	// this should be freed by the receiver sending back free_long_message
		hash_table_insert(&outstanding_lmas, lma);
			// ...but it might not be, if the app exits gracelessly, so we
			// track the ref here in this linked list so we can drop
			// the ref at app-close time.
		uint8_t* buffer[1024];
		CMDeliverPacketLong* dpl = (CMDeliverPacketLong*) buffer;
		dpl->hdr.opcode = co_deliver_packet_long;
		int idlen = lma->marshal(&dpl->id, sizeof(buffer)-sizeof(*dpl));
		dpl->hdr.length = offsetof(CMDeliverPacketLong,id)+idlen;

		send_cm(&dpl->hdr, dpl->hdr.length);
	}
}

void App::record_alloc_long_message(LongMessageAllocation* lma)
{
	hash_table_insert(&outstanding_lmas, lma);
}

void App::free_long_message(CMFreeLongMessage *msg)
{
	LongMessageAllocation *lma = listener->get_long_message_allocator()->
		lookup_existing(
			&msg->id, msg->hdr.length - offsetof(CMFreeLongMessage, id));

	// remove it before we drop_ref, since that ref may be all that's
	// keeping it around.
	hash_table_remove(&outstanding_lmas, lma);

	listener->get_long_message_allocator()->drop_ref(lma, "free_long_message");
}

void App::clean_up_outstanding_messages()
{
	hash_table_remove_every(&outstanding_lmas, _clean_up_one_s, this);
}

void App::_clean_up_one_s(void *v_this, void *v_lma)
{
	App* app = (App*) v_this;
	LongMessageAllocation* lma = (LongMessageAllocation*) v_lma;
	app->listener->get_long_message_allocator()->drop_ref(lma, "clean_up");
}

XIPAddr *App::get_ip_addr(XIPVer ver)
{
	return &ifconfigs[XIPVer_to_idx(ver)].local_interface;
}

uint32_t App::hash()
{
	return (uint32_t) this;
}

int App::cmp(Hashable *other)
{
	return ((uint32_t) this) - ((uint32_t) other);
}

UIEventDeliveryIfc *App::get_uidelivery()
{
	return this;
}

ZPubKey *App::get_pub_key()
{
	return pub_key;
}

const char *App::get_label()
{
	return pub_key->getOwner()->toString();
}

//////////////////////////////////////////////////////////////////////////////

void App::deliver_ui_event(ZoogUIEvent *event)
{
	CMDeliverUIEvent cduie;
	cduie.hdr.length = sizeof(CMDeliverUIEvent);
	cduie.hdr.opcode = co_deliver_ui_event;
	cduie.event = *event;
	send_cm(&cduie.hdr, cduie.hdr.length);
}

void App::debug_remark(const char *msg)
{
	fprintf(stderr, "debug_remark: %s\n", msg);
}
