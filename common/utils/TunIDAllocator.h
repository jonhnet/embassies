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
#pragma once

// monitors (coordinators) use resources in two scarce address spaces:
// a network interface tunN (where N is a rather small integer),
// and an IP address 10.N.0.1 (where 0 < N <255).
// They also have to agree with pals (monitors) on a rendezvous point
// (say, in /tmp). That's not a scarce address space, but we just pick
// one identifier and use it in every context.
//
// To pick the identifier, we allocate a tunN, take the id, and then
// save it to a file ~/.zoog_tunid.
// To override at runtime, supply env param ZOOG_TUNID=N to
// a monitor or PAL, and it'll use that instead.

class TunIDAllocator {
private:
	bool assigned;
	int tunid;
	int tun_fd;

	char* _pref_filename();
	void _write_back_tunid();
	bool _assign_id_to_tunnel(int fd, int id);
	void _setup_ifreq_internal(struct ifreq* ifr, int id);

public:
	TunIDAllocator();
	int get_tunid();
	int open_tunnel();	// returns a file descriptor. Idempotent.
	void setup_ifreq(struct ifreq* ifr);
};
