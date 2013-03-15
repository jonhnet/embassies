#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <netinet/in.h>

#include "NavInstance.h"
#include "LiteLib.h"
#include "xax_network_utils.h"
#include "AppIdentity.h"
#include "standard_malloc_factory.h"
#include "xax_extensions.h"
#include "AwayAction.h"
#include "TabIconPainter.h"
#include "ZRectangle.h"
#include "NavMgr.h"
#include "hash_to_hex.h"

#ifndef ZOOG_ROOT
#include "zoog_root.h"
#endif // ZOOG_ROOT

NavInstance::NavInstance(
	NavMgr *nav_mgr, NavigateIfc *nav_ifc, EntryPointIfc *entry_point,
	NavigationBlob *_back_token, Deed _tab_deed, uint64_t _rpc_nonce)
	: nav_mgr(nav_mgr),
	  nav_ifc(nav_ifc),
	  entry_point(entry_point),
	  back_token(_back_token),
	  pane_visible(false),
	  _action_pending(NULL)
{
	nav_ifc->setup_callout_ifc(this);

	map_tab(_tab_deed);
	nav_ifc->navigate(entry_point);

	transmit_acknowledge(_rpc_nonce);
	transmit_transfer_complete();
}

void NavInstance::map_tab(Deed tab_deed)
{
	_zdt()->zoog_accept_viewport(&tab_deed,
	 	&_tab_tenant, &_tab_deed_key);
	PixelFormat pixel_format = zoog_pixel_format_truecolor24;
	_zdt()->zoog_map_canvas(
		_tab_tenant, &pixel_format, 1, &_tab_canvas);
	fprintf(stderr, "NavInstance: mapping tab canvas %d (this %p)\n",
		_tab_canvas.canvas_id, this);

	_paint_canvas(&_tab_canvas);
}

void NavInstance::_paint_canvas(ZCanvas *zcanvas)
{
	TabIconPainter tip;
	tip.paint_canvas(zcanvas);
	ZRectangle zr = NewZRectangle(0, 0, zcanvas->width, zcanvas->height);
	_zdt()->zoog_update_canvas(zcanvas->canvas_id, &zr);
}

void NavInstance::transmit_acknowledge(uint64_t rpc_nonce)
{
	NavigationBlob *msg = new NavigationBlob();
	uint32_t opcode = NavigateAcknowledge::OPCODE;
	msg->append(&opcode, sizeof(opcode));

	NavigateAcknowledge na;
	na.rpc_nonce = rpc_nonce;
	msg->append(&na, sizeof(na));

	fprintf(stderr, "transmit_acknowledge(%016llx)\n", rpc_nonce);

	nav_mgr->send_and_delete_msg(msg);
}

void NavInstance::transmit_transfer_complete()
{
	// Tell TabMan I've collected the transfer, and to please
	// send me the pane, if I'm owed it.
	NavigationBlob *msg = new NavigationBlob();
	uint32_t opcode = TMTabTransferComplete::OPCODE;
	msg->append(&opcode, sizeof(opcode));
	
	TMTabTransferComplete tmttc;
	// "route" message to correct tab implementation using tab's DeedKey
	tmttc.tab_deed_key = _tab_deed_key;
	msg->append(&tmttc, sizeof(tmttc));
	nav_mgr->send_and_delete_msg(msg);
}

void NavInstance::pane_revealed(Deed pane_deed)
{
	if (pane_visible)
	{
		fprintf(stderr, "supressing pane_revealed when already visible\n");
		return;
	}
	pane_visible = true;
	DeedKey _pane_key;
	_zdt()->zoog_accept_viewport(&pane_deed,
	 	&_pane_tenant, &_pane_key);
	nav_ifc->pane_revealed(_pane_tenant);
}

void NavInstance::pane_hidden()
{
	if (!pane_visible)
	{
		fprintf(stderr, "supressing pane_hidden when already invisible\n");
		return;
	}
	pane_visible = false;
	nav_ifc->pane_hidden();
	if (_action_pending!=NULL)
	{
		fprintf(stderr, "pane_hidden releases aa=%p\n", _action_pending);
		_start_away_2(_action_pending);
		_action_pending = NULL;
	}
	else
	{
		fprintf(stderr, "...no _action_pending; apparently I didn't think I was the tenant...?\n");
	}
}

void NavInstance::transfer_forward(
	const char *vendor_name,
	EntryPointIfc* entry_point)
{
	away(new ForwardAction(this, vendor_name, entry_point));
}

void NavInstance::transfer_backward()
{
	if (back_token->size()==0)
	{
		fprintf(stderr, "nowhere to go backwards.\n");
		return;
	}

	hash_t back_vendor_id;
	back_token->read(&back_vendor_id, sizeof(back_vendor_id));
	NavigationBlob *opaque_prev_data = back_token->read_blob();

	away(new BackwardAction(this, back_vendor_id, opaque_prev_data));
}

void NavInstance::away(AwayAction *aa)
{
	NavigationBlob *msg = new NavigationBlob();
	uint32_t opcode = TMTabTransferStart::OPCODE;
	msg->append(&opcode, sizeof(opcode));
	
	TMTabTransferStart tmtts;
	// "route" message to correct tab implementation using tab's DeedKey
	tmtts.tab_deed_key = _tab_deed_key;
	msg->append(&tmtts, sizeof(tmtts));
	nav_mgr->send_and_delete_msg(msg);

	if (_pane_tenant==0)
	{
		fprintf(stderr, "away: we're already unmapped; can proceed immediately to send the NF. aa=%p\n", aa);
		_start_away_2(aa);
	}
	else
	{
		// Make a note to self that when I next unmap, I can send the NF to
		// the next guy.
		fprintf(stderr, "away: was _pane_tenant; _action_pending for later; aa=%p\n", aa);
		this->_action_pending = aa;
	}
}

void NavInstance::_start_away_2(AwayAction *aa)
{
	fprintf(stderr, "_start_away_2(%p)\n", aa);
	pthread_create(&away_2_pt, NULL, AwayAction::_away_2_thread, aa);
}

ZoogDispatchTable_v1* NavInstance::_zdt()
{
	return nav_mgr->get_zdt();
}

void NavInstance::_away_2(AwayAction *aa)
{
	fprintf(stderr, "NavInstance %p hides its tab canvas to transfer it.\n", this);

	// NB monitor should do this automatically with a transfer, but
	// right now we're in assertion-city to keep everybody disciplined.

	Deed outgoing_deed;
	_zdt()->zoog_transfer_viewport(_tab_tenant, &outgoing_deed);

	SignedBinary *sb = NULL;
	ZPubKey *vendor_pub_key = NULL;
	bool started_one = false;
	bool start_only_one = true;

	while (true)
	{
		uint64_t rpc_nonce = nav_mgr->transmit_navigate_msg(outgoing_deed, vendor_pub_key, aa);
		int timeout_sec = 1;
		int stall_sec = 8;

		bool recd = nav_mgr->wait_ack(rpc_nonce, timeout_sec);
		if (recd)
		{
			fprintf(stderr, "Ack received! Yay.\n");
			break;
		}

		if (sb==NULL)
		{
			fprintf(stderr, "Fetching boot block for %s\n", aa->vendor_name());
			// The first time we try this for 'back', it's going to fail,
			// because we don't know how to get from vendor key
			// hashes back to names. We should build that into
			// our pretend PKI too; why the heck not?
			sb = lookup_boot_block(aa->vendor_name());
			vendor_pub_key = pub_key_from_signed_binary(sb);
		}

		fprintf(stderr, "didn't hear from server in %d sec; starting vendor; sleeping %d sec\n", timeout_sec, stall_sec);
		if (started_one && start_only_one)
		{
			fprintf(stderr, "Actually, let's keep things to one app start\n");
		}
		else
		{
			fprintf(stderr, "Launch...\n");
			started_one = true;
			_zdt()->zoog_launch_application(sb);
			sleep(stall_sec);
		}
	}
	free(sb);
	delete aa;

	nav_ifc->no_longer_used();
}

uint8_t *NavInstance::load_all(const char *path)
{
	FILE *fp = fopen(path, "rb");

	int rc = fseek(fp, 0, SEEK_END);
	lite_assert(rc==0);
	uint32_t len = ftell(fp);
	rc = fseek(fp, 0, SEEK_SET);
	lite_assert(rc==0);

	uint8_t* buf = (uint8_t*) malloc(len);
	rc = fread(buf, len, 1, fp);
	lite_assert(rc==1);
	fclose(fp);
	return buf;
}

SignedBinary* NavInstance::lookup_boot_block(const char *vendor_name)
{
	char rewrite[100];
	strncpy(rewrite, vendor_name, sizeof(rewrite));
	for (int i=0; rewrite[i]!='\0'; i++)
	{
		if (rewrite[i]=='.' || rewrite[i]=='-')
		{
			rewrite[i] = '_';
		}
	}

	char buf[1000];
	snprintf(buf, sizeof(buf),
		ZOOG_ROOT "/toolchains/linux_elf/elf_loader/build/elf_loader.%s.signed",
		rewrite);
	return (SignedBinary*) load_all(buf);
}

ZPubKey* NavInstance::pub_key_from_signed_binary(SignedBinary* sb)
{
	uint32_t certlen = ntohl(sb->cert_len);
	uint8_t* certbits = ((uint8_t*) sb)+sizeof(SignedBinary);
	ZCert *cert = new ZCert(certbits, certlen);
	ZPubKey *pub_key = cert->getEndorsedKey();
	// TODO leaks cert, because deleting it would delete pub_key, I think. Bah.
	return pub_key;
}

NavigationBlob *NavInstance::get_outgoing_back_token()
{
	NavigationBlob my_opaque_prev_data;
	NavigationBlob ep;
	entry_point->serialize(&ep);
	my_opaque_prev_data.append(&ep);
	my_opaque_prev_data.append(back_token);

	NavigationBlob *outgoing_back_token = new NavigationBlob();
	hash_t my_vendor_id = nav_mgr->get_my_identity_hash();
	outgoing_back_token->append(&my_vendor_id, sizeof(my_vendor_id));
	outgoing_back_token->append(&my_opaque_prev_data);
	return outgoing_back_token;
}

const char *NavInstance::get_vendor_name_from_hash(hash_t vendor_id)
{
	char hexbuf[100];
	hash_to_hex(vendor_id, hexbuf);
	char buf[100];	// TODO buf overflow
	snprintf(buf, sizeof(buf),
		ZOOG_ROOT "/toolchains/linux_elf/crypto/hash_to_name/%s",
		hexbuf);
	char readbuf[1000];
	FILE *fp = fopen(buf, "rb");
	lite_assert(fp!=NULL);	// can't find hash-inversion in /etc/hosts...
	fread(readbuf, 1, sizeof(readbuf), fp);
	fclose(fp);
	return strdup(readbuf);
}
