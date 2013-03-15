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

#include "Process.h"
#include "MThread.h"
#include "BlitterManager.h"

class Monitor;

class ReplyDeleter;

class Tx {
private:
	Monitor *monitor;
	Process *process;

	char remote_addr[200];
	socklen_t remote_addr_len;
	XpReqHeader *req;
	ReplyDeleter *dreply;

	uint32_t txnum;

	char read_buf[65536];
	int read_size;
		// TODO ugly. This huge buffer is here because we might get some big
		// message. (Boot blocks are actually bigger, and go via an XPB,
		// but keys & certs are variable-sized.)
		// It might be prettier to read a length and then
		// the real data, but then we have an atomicity problem with
		// different threads pulling off the same socket. Ugly.
		// (We no longer get boot blocks this way, because they're too
		// big to fit through this hole, so it's less fat.)

	static int txnum_allocator;

	void complete_xax_port_op();
	void _complete_other_thread_yield(MThread *thr, uint32_t dbg_zutex_handle);
	void _pool_full(uint32_t thread_id);
	void _pool_available();
	MThread *find_thread(int thread_id);

	BlitterManager *blitterMgr();

	enum Disp { TX_DONE, KEEPS_TX };
	Disp handle_ZutexWaitFinish(XpReqZutexWait *req, XpReplyZutexWait *reply, bool commit);

	Disp handle(XpReqConnect *req, XpReplyConnect *reply);
	Disp handle(XpReqDisconnect *req, XpReplyDisconnect *reply);
	Disp handle(XpReqXNBAlloc *req, XpReplyXNBAlloc *reply);
	Disp handle(XpReqXNBRecv *req, XpReplyXNBRecv *reply);
	Disp handle(XpReqXNBSend *req, XpReplyXNBSend *reply);
	Disp handle(XpReqXNBRelease *req, XpReplyXNBRelease *reply);
	Disp handle(XpReqGetIfconfig *req, XpReplyGetIfconfig *reply);
	Disp handle(XpReqZutexWaitPrepare *req, XpReplyZutexWaitPrepare *reply);
	Disp handle(XpReqZutexWaitCommit *req, XpReplyZutexWaitCommit *reply);
	Disp handle(XpReqZutexWaitAbort *req, XpReplyZutexWaitAbort *reply);
	Disp handle(XpReqZutexWake *req, XpReplyZutexWake *reply);
	Disp handle(XpReqLaunch *req, XpReplyLaunch *reply);
	Disp handle(XpReqEndorseMe *req, XpReplyEndorseMe *reply) ;
	Disp handle(XpReqGetAppSecret *req, XpReplyGetAppSecret *reply) ;
	Disp handle(XpReqEventPoll *req, XpReplyEventPoll *reply);
	Disp handle(XpReqSubletViewport *req, XpReplySubletViewport *reply);
	Disp handle(XpReqRepossessViewport *req, XpReplyRepossessViewport *reply);
	Disp handle(XpReqGetDeedKey *req, XpReplyGetDeedKey *reply);
	Disp handle(XpReqAcceptViewport *req, XpReplyAcceptViewport *reply);
	Disp handle(XpReqTransferViewport *req, XpReplyTransferViewport *reply);
	Disp handle(XpReqGetRandom *req, XpReplyGetRandom *reply) ;
	Disp handle(XpReqVerifyLabel *req, XpReplyVerifyLabel *reply) ;
	Disp handle(XpReqMapCanvas *req, XpReplyMapCanvas *reply);
	Disp handle(XpReqUnmapCanvas *req, XpReplyUnmapCanvas *reply);
	Disp handle(XpReqUpdateCanvas *req, XpReplyUpdateCanvas *reply);
	Disp handle(XpReqReceiveUIEvent *req, XpReplyReceiveUIEvent *reply);
	Disp handle(XpReqDebugCreateToplevelWindow *req, XpReplyDebugCreateToplevelWindow  *reply);

	template<class TReq, class TReply>
	void handle(const char *struct_name, bool fixed_len_reply);

public:
	Tx(Monitor *monitor);
	~Tx();
	void handle();
};

