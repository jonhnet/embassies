#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <netinet/in.h>

#include "NavMgr.h"
#include "LiteLib.h"
#include "xax_network_utils.h"
#include "standard_malloc_factory.h"
#include "AckWait.h"
#include "AwayAction.h"


//////////////////////////////////////////////////////////////////////////////

NavMgr::NavMgr(
	NavigateFactoryIfc *navigate_factory,
	EntryPointFactoryIfc *entry_point_factory)
	: navigate_factory(navigate_factory),
	  entry_point_factory(entry_point_factory)
{
	_bind_socket();
	int rc = pthread_create(&_dispatch_thread, NULL, _dispatch_loop_s, this);
	lite_assert(rc==0);

	MallocFactory* mf = standard_malloc_factory_init();
	hash_table_init(&ack_waiters, mf, AckWait::hash, AckWait::cmp);
	linked_list_init(&nav_instances, mf);
}

void NavMgr::_bind_socket()
{
    struct sockaddr_in recv_address; 
    memset((char *) &recv_address, 0, sizeof(recv_address));
    recv_address.sin_family = AF_INET;
    recv_address.sin_port = htons(NavigationProtocol::PORT);
    recv_address.sin_addr.s_addr = INADDR_ANY;

    int bindReturn = bind(socketfd, (sockaddr*)&recv_address, sizeof(recv_address));
	lite_assert(bindReturn == 0);

    // NOTE: LWIP does not currently support setsockopt(...,SO_BROADCAST,...)
    // but we don't appear to need it for broadcast support here
    
}

void* NavMgr::_dispatch_loop_s(void* obj)
{
	((NavMgr*) obj)->_dispatch_loop();
	lite_assert(false);
	return NULL;
}

void NavMgr::_dispatch_loop()
{
	uint8_t recv_buffer[4096];
	uint32_t recv_buffer_len = sizeof(recv_buffer);

	bool done = false;
    while (!done)
    {
        ssize_t recv_message_len = recvfrom(socketfd, recv_buffer, recv_buffer_len, 0, (sockaddr*)&sender_address, &sender_address_len); 
		lite_assert(recv_message_len > 0);

		NavigationBlob *msg = new NavigationBlob(recv_buffer, recv_message_len);
		_dispatch_msg(msg);
    }

    close(socketfd);
}

void NavMgr::_dispatch_msg(NavigationBlob *msg)
{
	uint32_t opcode;
	msg->read(&opcode, sizeof(opcode));
	switch (opcode)
	{
	case NavigateForward::OPCODE:
		fprintf(stderr, "Got NavigateForward\n");
		handle_navigate_forward(msg);
		break;
	case NavigateBackward::OPCODE:
		fprintf(stderr, "Got NavigateBackward\n");
		handle_navigate_backward(msg);
		break;
	case NavigateAcknowledge::OPCODE:
		fprintf(stderr, "Got NavigateAcknowledge\n");
		nagivate_acknowledge(msg);
		break;
	case TMPaneHidden::OPCODE:
		fprintf(stderr, "Got PaneHidden\n");
		pane_hidden(msg);
		break;
	case TMPaneRevealed::OPCODE:
		fprintf(stderr, "Got PaneRevealed\n");
		pane_revealed(msg);
		break;
	case TMNewTabRequest::OPCODE:
		break;
	case TMNewTabReply::OPCODE:
		break;
	default:
		fprintf(stderr, "Ignoring nonsense opcode %d\n", opcode);
		break;
	}
	delete msg;
}

bool NavMgr::check_recipient_hack(hash_t *dest)
{
	hash_t me = get_my_identity_hash();
	return memcmp(dest, &me, sizeof(hash_t))==0;
}

void NavMgr::handle_navigate_forward(NavigationBlob *msg)
{
	NavigateForward nf;
	msg->read(&nf, sizeof(nf));
	if (!check_recipient_hack(&nf.dst_vendor_id_hash))
	{
		fprintf(stderr, "not for me.\n");
		return;
	}

	NavigateIfc *nav_ifc = navigate_factory->create_navigate_server();
	if (nav_ifc == NULL)
	{
		// No more navigation ability from this factory. (Expedient because we
		// only have one xvnc surface, and it's in use, apparently, and
		// we haven't figured out how to free & recycle it!)
		fprintf(stderr, "NavMgr::handle_navigate_forward: no slots free; ignoring\n");
		return;
	}

	NavigationBlob *ep_blob = msg->read_blob();
	EntryPointIfc* entry_point = entry_point_factory->deserialize(ep_blob);
	fprintf(stderr, "NavMgr received ep '%s'\n", entry_point->repr());
	delete ep_blob;

	NavigationBlob *back_token = msg->read_blob();
	NavInstance *ni = new NavInstance(
		this, nav_ifc, entry_point,
		back_token, nf.tab_deed, nf.rpc_nonce);
	linked_list_insert_tail(&nav_instances, ni);
}

void NavMgr::handle_navigate_backward(NavigationBlob *msg)
{
	NavigateIfc *nav_ifc = navigate_factory->create_navigate_server();
	if (nav_ifc == NULL)
	{
		fprintf(stderr, "NavMgr::handle_navigate_backward: no slots free; ignoring\n");
		return;
	}
	NavigateBackward nb;
	msg->read(&nb, sizeof(nb));
	if (!check_recipient_hack(&nb.dst_vendor_id_hash))
	{
		fprintf(stderr, "not for me.\n");
		return;
	}

	NavigationBlob *my_opaque_data = msg->read_blob();
	NavigationBlob *ep_blob = my_opaque_data->read_blob();
	EntryPointIfc* entry_point = entry_point_factory->deserialize(ep_blob);
	fprintf(stderr, "NavMgr received ep '%s'\n", entry_point->repr());
	delete ep_blob;
	NavigationBlob *back_token = my_opaque_data->read_blob();
	delete my_opaque_data;

	NavInstance *ni = new NavInstance(
		this, nav_ifc, entry_point,
		back_token, nb.tab_deed, nb.rpc_nonce);
	linked_list_insert_tail(&nav_instances, ni);
}

void NavMgr::nagivate_acknowledge(NavigationBlob *msg)
{
	NavigateAcknowledge na;
	msg->read(&na, sizeof(na));
	AckWait key(na.rpc_nonce);
	AckWait *waiter = (AckWait*) hash_table_lookup(&ack_waiters, &key);
	fprintf(stderr, "recd nagivate_acknowledge(%016llx)\n", na.rpc_nonce);
	if (waiter!=NULL)
	{
		waiter->signal();
	}
	else
	{
		fprintf(stderr, "ack received for %016llx; no-one waiting.\n",
			na.rpc_nonce);
	}
}

NavInstance* NavMgr::find_instance_by_key(DeedKey key)
{
	LinkedListIterator lli;
	for (ll_start(&nav_instances, &lli); ll_has_more(&lli); ll_advance(&lli))
	{
		NavInstance* ni = (NavInstance*) ll_read(&lli);
		if (key == ni->get_tab_deed_key())
		{
			return ni;
		}
	}
	return NULL;
}

void NavMgr::pane_revealed(NavigationBlob *msg)
{
	TMPaneRevealed tmpr;
	msg->read(&tmpr, sizeof(tmpr));
	NavInstance* ni = find_instance_by_key(tmpr.tab_deed_key);
	if (ni==NULL)
		{ fprintf(stderr, "NavMgr::pane_revealed: not for me.\n"); }
	else
		{ ni->pane_revealed(tmpr.pane_deed); }
}

void NavMgr::pane_hidden(NavigationBlob *msg)
{
	TMPaneHidden tmph;
	msg->read(&tmph, sizeof(tmph));
	NavInstance* ni = find_instance_by_key(tmph.tab_deed_key);
	if (ni==NULL)
		{ fprintf(stderr, "NavMgr::pane_hidden: not for me.\n"); }
	else
		{ ni->pane_hidden(); }
}

uint64_t NavMgr::next_nonce()
{
	_nonce_allocator++;
	return _nonce_allocator;
}

uint64_t NavMgr::transmit_navigate_msg(Deed outgoing_deed, ZPubKey *vendor_id, AwayAction *aa)
{
	uint64_t rpc_nonce = next_nonce();

	// Send a Navigate* message to transfer the tab
	NavigationBlob *msg = new NavigationBlob();
	uint32_t opcode = aa->opcode();
	msg->append(&opcode, sizeof(opcode));

	aa->fill_msg(msg, rpc_nonce, outgoing_deed);
	send_and_delete_msg(msg);

	return rpc_nonce;
}

bool NavMgr::wait_ack(uint64_t rpc_nonce, int timeout_sec)
{
	AckWait ack_wait(rpc_nonce, &sf);
	hash_table_insert(&ack_waiters, &ack_wait);
	bool recd = ack_wait.received(timeout_sec*1000);
	hash_table_remove(&ack_waiters, &ack_wait);
	return recd;
}

hash_t NavMgr::get_my_identity_hash()
{
	return _identity.get_pub_key_hash();
}
