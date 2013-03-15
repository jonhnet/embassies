/* Copyright (c) Microsoft Corporation                                       */
/*                                                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may   */
/* not use this file except in compliance with the License.  You may obtain  */
/* a copy of the License at http://www.apache.org/licenses/LICENSE-2.0       */
/*                                                                           */
/* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS     */
/* OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION      */
/* ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR   */
/* PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.                              */
/*                                                                           */
/* See the Apache Version 2.0 License for specific language governing        */
/* permissions and limitations under the License.                            */
/*                                                                           */
#include "NavInstance.h"

class AwayAction {
protected:
	NavInstance* ni;
public:
	AwayAction(NavInstance* ni) : ni(ni) {}
	virtual ~AwayAction() {}
	virtual uint32_t opcode() = 0;
	virtual const char *vendor_name() = 0;
	virtual void fill_msg(NavigationBlob *msg, uint64_t rpc_nonce, Deed outgoing_deed) = 0;
	static void *_away_2_thread(void *v_aa)
	{
		AwayAction *aa = (AwayAction *) v_aa;
		aa->ni->_away_2(aa);
		return NULL;
	}
};

class ForwardAction : public AwayAction {
private:
	char *_vendor_name;
	EntryPointIfc *entry_point;
public:
	ForwardAction(NavInstance* ni,
		const char *_vendor_name,
		EntryPointIfc *entry_point)
		: AwayAction(ni),
		  _vendor_name(strdup(_vendor_name)),
		  entry_point(entry_point)
	{}
	~ForwardAction()
	{
		free(_vendor_name);
		delete entry_point;
	}
	uint32_t opcode() { return NavigateForward::OPCODE; }
	virtual const char *vendor_name() { return _vendor_name; }
	void fill_msg(NavigationBlob *msg, uint64_t rpc_nonce, Deed outgoing_deed) {
		AppIdentity ai(_vendor_name);

		NavigateForward nf;
		nf.dst_vendor_id_hash = ai.get_pub_key_hash();
		nf.rpc_nonce = rpc_nonce;
		nf.tab_deed = outgoing_deed;
		msg->append(&nf, sizeof(nf));

		NavigationBlob *ep_blob = new NavigationBlob();
		entry_point->serialize(ep_blob);
		msg->append(ep_blob);

		msg->append(ni->get_outgoing_back_token());
	}
};

class BackwardAction : public AwayAction {
private:
	hash_t vendor_id;
	NavigationBlob *opaque_prev_data;
public:
	BackwardAction(
		NavInstance* ni,
		hash_t vendor_id,
		NavigationBlob *opaque_prev_data)
	: AwayAction(ni),
	  vendor_id(vendor_id),
	  opaque_prev_data(opaque_prev_data)
	{}

	uint32_t opcode() { return NavigateBackward::OPCODE; }
	virtual const char *vendor_name() {
		return ni->get_vendor_name_from_hash(vendor_id);
	}
	void fill_msg(NavigationBlob *msg, uint64_t rpc_nonce, Deed outgoing_deed) {
		NavigateBackward nb;
		nb.dst_vendor_id_hash = vendor_id;
		nb.rpc_nonce = rpc_nonce;
		nb.tab_deed = outgoing_deed;
		msg->append(&nb, sizeof(nb));

		msg->append(opaque_prev_data);
	}
};


