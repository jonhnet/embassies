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
// deprecated -- its functions have all been factored into NavMgr
#if 0
#pragma once

#include <poll.h>
#include "NavigationBlob.h"
#include "NavTest.h"

class SockMgr {
private:
	NavTest *_nav_test;
	int sock_fd;

	void dispatch_msg(NavigationBlob *msg);
	bool check_recipient_hack(hash_t *vendor_id_hash);
	void navigate_forward(NavigationBlob *msg);
	void navigate_backward(NavigationBlob *msg);
	void pane_revealed(NavigationBlob *msg);
	void pane_hidden(NavigationBlob *msg);

public:
	SockMgr(NavTest *nav_test);
	void setup_poll(struct pollfd *pfd);
	void process_events();
};

#endif
