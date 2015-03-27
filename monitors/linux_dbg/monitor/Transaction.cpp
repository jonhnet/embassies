#include <assert.h>
#include <malloc.h>
#include <alloca.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "Transaction.h"
#include "xax_network_utils.h"
#include "ZCert.h"
#include "ZCertChain.h"
#include "ZPubKey.h"
#include "ZBinaryRecord.h"
#include "Monitor.h"
#include "BlitterCanvas.h"
#include "BlitterViewport.h"
#include "LinuxDbgCanvasAcceptor.h"
#include "BlitProviderIfc.h"

#define DEBUG_ZUTEX 0

#define XAXOP_STRACE(e) // fprintf(stderr, "handling %s\n", e);

//////////////////////////////////////////////////////////////////////////////
// A pukey polymorphic wrapper for making most reply objects magically
// allocated in template handler dispatcher, but leaving allocation of
// variable-length replies to those handlers (EndorseMe) that need to do so.

class ReplyDeleter {
public:
	XpReplyHeader *reply;
	ReplyDeleter(int len)
	{
		reply = (XpReplyHeader*) malloc(len);
		reply->message_len = len;
	}
	~ReplyDeleter()
	{
		free(reply);
	}
};

//////////////////////////////////////////////////////////////////////////////

int Tx::txnum_allocator = 0;

Tx::Tx(Monitor *monitor)
{
	this->monitor = monitor;
	process = NULL;
	req = NULL;
	dreply = NULL;

	remote_addr_len = sizeof(remote_addr);
	read_size = recvfrom(monitor->get_xax_port_socket(),
		read_buf, sizeof(read_buf),
		0, (struct sockaddr *) remote_addr, &remote_addr_len);

	this->txnum = txnum_allocator++;	// TODO racey
//	print_time("TX START", tx->txnum);
}

Tx::~Tx()
{
	delete dreply;
}

void print_time(const char *msg, int txnum)
{
	static time_t start_time = -1;

	struct timeval tv;
	struct timezone tz;
	int rc = gettimeofday(&tv, &tz);

	if (start_time == -1)
	{
		start_time = tv.tv_sec;
	}

	assert(rc==0);
	fprintf(stderr, "%s %8x %3d.%06d\n",
		msg, txnum, (int) (tv.tv_sec - start_time), (int) (tv.tv_usec));
}

void Tx::complete_xax_port_op()
{
	sendto(monitor->get_xax_port_socket(),
		dreply->reply, dreply->reply->message_len,
		0, (const struct sockaddr *) remote_addr, remote_addr_len);
	delete dreply;
	dreply = NULL;
}

BlitterManager *Tx::blitterMgr()
{
	return monitor->get_blitter_manager();
}

Tx::Disp Tx::handle(XpReqConnect *req, XpReplyConnect *reply)
{
	assert(process==NULL);
	ZPubKey* pub_key;
	if (req->pub_key_len > 0)
	{
		pub_key = new ZPubKey((uint8_t*) &req[1], req->pub_key_len);
	}
	else
	{
		fprintf(stderr, "WARNING: empty public key in connect message; things will probably break.\n");
		pub_key = NULL;
	}
	process = monitor->connect_process(req->process_id, pub_key);
	complete_xax_port_op();
	print_time("TX Connect END  ", txnum);
	return TX_DONE;
}

// TODO Would like to know how to detect disconnection!
Tx::Disp Tx::handle(XpReqDisconnect *req, XpReplyDisconnect *reply)
{
	monitor->disconnect_process(process);
	complete_xax_port_op();
	print_time("TX Connect END  ", txnum);
	return TX_DONE;
}

#define BUFFER_ACCOUNTING 0

#if BUFFER_ACCOUNTING
static int n_alloc=0, n_recv=0;
void buffer_accounting(int d_alloc, int d_recv)
{
	n_alloc += d_alloc;
	n_recv += d_recv;
	//fprintf(stderr, "bufs alloced: %d recvd: %d\n", n_alloc, n_recv);
}
#endif // BUFFER_ACCOUNTING

Tx::Disp Tx::handle(XpReqXNBAlloc *req, XpReplyXNBAlloc *reply)
{
	XNB_NOTE("Alloc\n");
	ZoogNetBuffer *znb =
		process->get_buf_pool()->get_net_buffer(req->payload_size, reply);
	if (znb!=NULL)
	{
		complete_xax_port_op();
#if BUFFER_ACCOUNTING
		buffer_accounting(1, 0);
#endif // BUFFER_ACCOUNTING
	}
	else
	{
		assert(false);
		// this path doesn't even work -- we never loop around to fill in reply!
		_pool_full(req->thread_id);
	}
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqXNBRecv *req, XpReplyXNBRecv *reply)
{
	XNB_NOTE("Recv\n");
	PacketIfc *p = pq_remove(&process->get_app_interface()->pq);
//	fprintf(stderr, "Remove pid %d leaves (%d) packets\n",
//		process->process_id, process->net_connection.pq.count);
	if (p==NULL)
	{
		reply->type = no_packet_ready;
		complete_xax_port_op();
	}
	else
	{
		ZoogNetBuffer *znb =
			process->get_buf_pool()->get_net_buffer(p->len(), reply);
		if (znb!=NULL)
		{
			assert(znb->capacity >= (uint32_t) p->len());
			memcpy(&znb[1], p->data(), p->len());

			packet_account_free(monitor->get_packet_account(), p);
			delete p;

			complete_xax_port_op();
#if BUFFER_ACCOUNTING
			buffer_accounting(0, 1);
#endif // BUFFER_ACCOUNTING
			return TX_DONE;
		}
		else
		{
			assert(false);
			// this path doesn't even work -- we never loop around to fill in reply!
			_pool_full(req->thread_id);
			// Need to put tx somewhere if we do.
			return KEEPS_TX;
		}
	}
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqXNBSend *req, XpReplyXNBSend *reply)
{
	XNB_NOTE("Send\n");
	// I was about to write code to check for forged packets,
	// when I remembered: It's the high seas out thar'!
	// Apps can transmit CANNONBALLS if they want!

	XPBPoolElt *xpbe =
		process->get_buf_pool()->lookup_elt(req->open_buffer_handle);

	send_packet(monitor->get_netifc(), xpbe);

	if (req->release)
	{
		process->get_buf_pool()->release_buffer(req->open_buffer_handle, false);
		_pool_available();
#if BUFFER_ACCOUNTING
		buffer_accounting(-1, 0);
#endif // BUFFER_ACCOUNTING
	}

	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqXNBRelease *req, XpReplyXNBRelease *reply)
{
	XNB_NOTE("Release\n");
	process->get_buf_pool()->release_buffer(
		req->open_buffer_handle,
		req->discarded_at_pal);
	_pool_available();
	complete_xax_port_op();
#if BUFFER_ACCOUNTING
	buffer_accounting(0, -1);
#endif // BUFFER_ACCOUNTING
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqGetIfconfig *req, XpReplyGetIfconfig *reply)
{
	reply->count = 2;
	memcpy(
		&reply->ifconfig_out[0],
		&process->get_app_interface()->conn4.host_ifconfig,
		sizeof(reply->ifconfig_out[0]));
	memcpy(
		&reply->ifconfig_out[1],
		&process->get_app_interface()->conn6.host_ifconfig,
		sizeof(reply->ifconfig_out[1]));
	complete_xax_port_op();
	return TX_DONE;
}

int cmp_ptr(const void *p0, const void *p1)
{
	return ((uint32_t)p0) - ((uint32_t)p1);
}

Tx::Disp Tx::handle(XpReqZutexWaitPrepare *req, XpReplyZutexWaitPrepare *reply)
{
	uint32_t i;
	ZQ **zqs = (ZQ**) alloca(req->count);

	DECLARE_ZLO;

	for (i=0; i<req->count; i++)
	{
		zqs[i] = process->lookup_zq(req->specs[i]);
	}
	// sort the request to provide a deadlock-avoiding lock acquisition order
	qsort(zqs, req->count, sizeof(ZQ*), cmp_ptr);
	for (i=0; i<req->count; i++)
	{
		zqs[i]->lock(TRANSMIT_ZLO);
	}
	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle_ZutexWaitFinish(XpReqZutexWait *req, XpReplyZutexWait *reply, bool commit)
{
	int i;
	MThread *thr = process->lookup_thread(req->thread_id);
	thr->wait_setup();

	// collect the ZQ list up front. We need to keep a copy of it here,
	// because once we release the thr mutex below, the transaction doesn't
	// belong to us anymore, and may get completed and freed by
	// a different thread.
	// (Really, that shouldn't happen, because any such thread would also
	// need to grab some zq lock ... that we're holding right now. So
	// maybe this isn't actually fixing the bug I'm chasing so recklessly.)
	int zq_count = req->count;
	ZQ **zqs = (ZQ**) alloca(sizeof(ZQ*) * zq_count);
	for (i=0; i<zq_count; i++)
	{
		zqs[i] = process->lookup_zq(req->specs[i]);
	}

	if (commit)
	{
		for (i=0; i<zq_count; i++)
		{
#if 0
			// jonh wondering how you get on a list twice.
			int j;
			for (j=0; ; j++)
			{
				MThread *queued_thread = zq_peek(zqs[i], j);
				if (queued_thread == NULL) { break; }
				assert(queued_thread != thr);
			}
#endif

			zqs[i]->append(thr);
			thr->wait_push(zqs[i]);
		}
#if DEBUG_ZUTEX
		fprintf(stderr, "ZUTEX_WAIT finishes with YIELD (tid %d, tx %08x)\n  ",
			thr->thread_id, (uint32_t) this);
			for (i=0; i<zq_count; i++)
			{
				fprintf(stderr, "0x%08x{0x%08x} ", (uint32_t) zqs[i], zqs[i]->dbg_zutex_handle);
			}
			fprintf(stderr, "\n");
#endif // DEBUG_ZUTEX
		thr->wait_complete(this);
		// leave the Transaction.hanging. It's hanging in the thr object,
		// but we don't complete the RPC now. We do, however, return
		// to the caller, who deallocates the original tx from the stack,
		// hence the goofy copy. TODO clean up Tx object lifecycle.
	}
	else
	{
		thr->wait_complete(NULL);
	}

	for (i=0; i<zq_count; i++)
	{
		zqs[i]->unlock();
	}
	// now someone might race, but if they do, they'll be grabbing this
	// thread and scheduling it.
	// Ah, there's the deadlock: they'll grab the thread lock after the
	// queue lock. Unless they can release the queue lock first? Yeah, they
	// can, since they own the thread release (in their program counter).
	if (!commit)
	{
		complete_xax_port_op();
		return TX_DONE;
	}
	else
	{
		return KEEPS_TX;
	}
}

Tx::Disp Tx::handle(XpReqZutexWaitCommit *req, XpReplyZutexWaitCommit *reply)
{
	return handle_ZutexWaitFinish(req, reply, true);
}

Tx::Disp Tx::handle(XpReqZutexWaitAbort *req, XpReplyZutexWaitAbort *reply)
{
	return handle_ZutexWaitFinish(req, reply, false);
}

Tx::Disp Tx::handle(XpReqZutexWake *req, XpReplyZutexWake *reply)
{
	if (req->n_requeue > 0)
	{
		// TODO requeue not implemented (well, not tested; see below) yet.
		// instead, return an error; that should get pthreads to fall back
		// on wake-all behavior, which is correct if slow.
		reply->success = false;
		complete_xax_port_op();
		return TX_DONE;
	}

	DECLARE_ZLO;

	ZQ private_wake_queue(monitor->get_mf(), 0);
	ZQ *zq = process->lookup_zq(req->zutex);
	zq->lock(TRANSMIT_ZLO);
	// We ignore match_val, since we can't evaluate it from here.
	// I think that's only a performance
	// penalty; we end up waking up things that might not have needed
	// waking, but that's always legal.

	int i;

#if DEBUG_ZUTEX
	bool especially_detailed = (((uint32_t) req->zutex)==0x20086d64);
	if (especially_detailed)
	{
		fprintf(stderr, "before wake: hdl 0x%08x zq: ", zq->dbg_zutex_handle);
		for (i=0; i<(int)zq->count; i++)
		{
			fprintf(stderr, "%d ", zq->thr[i]->thread_id);
		}
		fprintf(stderr, "\n");
	}
#endif // DEBUG_ZUTEX

	lite_assert(req->n_wake > 0);	// because otherwise that'd be nuts.
	for (i=0; i<req->n_wake; i++)
	{
		MThread *queued = zq->pop();
		if (queued==NULL)
			{ break; }
		private_wake_queue.append(queued);
	}

	zq->dbg_note_wake(req->thread_id, req->match_val, &private_wake_queue);

	// ////////////////////////////////////////////////////////////
	// Requeue
	// ////////////////////////////////////////////////////////////
	if (req->n_requeue > 0)
	{
		assert(0);	// Write a unit test (zutexercise) before
			// depending on this in a harder-to-debug program.

#if 0
		ZQ *zq_requeue  = process->lookup_zq(req->requeue_zutex);
		zq_requeue->lock(TRANSMIT_ZLO);
		for (i=0; i<req->n_requeue; i++)
		{
			MThread *thr = zq_pop(zq);
			if (thr==NULL)
				{ break; }

			pthread_mutex_lock(&thr->mutex);
			zq_remove(zq, thr);
			zq_append(zq_requeue, thr);

			// Now thr has been moved to the new queue. Update its
			// queue membership list, so that the removal step of a later wake
			// will take it out of the correct queue.
			int j;
			for (j=0; j<thr->zq_count; j++)
			{
				if (thr->zq[j]==zq)
				{
					thr->zq[j] = zq_requeue;
					break;
					// by quitting after fixing one, we might end up missing
					// a second copy of the same queue; no harm, since zq_remove
					// removes duplicate copies. We just need to ensure each
					// queue the thread is in is listed at least once in the
					// thread's zq list.
				}
			}
			pthread_mutex_unlock(&thr->mutex);
		}
		zq_requeue->unlock();
#endif
	}
#if DEBUG_ZUTEX
	if (especially_detailed)
	{
		fprintf(stderr, "before wake: hdl 0x%08x zq: ", zq->dbg_zutex_handle);
		for (i=0; i<(int)zq->count; i++)
		{
			fprintf(stderr, "%d ", zq->thr[i]->thread_id);
		}
		fprintf(stderr, "\n");
	}
#endif // DEBUG_ZUTEX
	zq->unlock();

	// ////////////////////////////////////////////////////////////
	// Wake
	// ////////////////////////////////////////////////////////////
	while (true)
	{
		MThread *thr = private_wake_queue.pop();
		if (thr==NULL) { break; }

		thr->wake_setup();
		// take thread off any other queues.
		// This can race with being taken off another queue
		// by a wake, but that's okay; both wakes result in only
		// one wake; it's up to user-mode to re-check all zutexes
		// before waiting again. (or they don't, and they get awoken
		// right away with EWOULDBLOCK -- via an ABORT in the XaxPort
		// implementation)
		while (true)
		{
			ZQ *zq = thr->wake_pop_zq();
			if (zq==NULL) { break; }
			zq->lock(TRANSMIT_ZLO);
			zq->remove(thr);
			zq->unlock();
		}

		_complete_other_thread_yield(thr, (uint32_t) req->zutex);
		thr->wake_complete();
	}

	// complete the wake
	reply->success = true;
	complete_xax_port_op();
	return TX_DONE;
}

void Tx::_complete_other_thread_yield(MThread *thr, uint32_t dbg_zutex_handle)
{
	Tx *woken_tx = thr->wake_get_tx();
	if (woken_tx!=NULL)
	{
#if DEBUG_ZUTEX
		fprintf(stderr, "wake (0x%08x: %d, tx %08x)\n",
			dbg_zutex_handle,
			thr->thread_id,
			(uint32_t) woken_tx);
#endif //DEBUG_ZUTEX
		// wake up the yielded tx by completing his wait()
		woken_tx->complete_xax_port_op();
		delete woken_tx;
	}
}

void Tx::_pool_full(uint32_t thread_id)
{
#if 0
	MThread *thr = process->lookup_thread(thread_id);
	pthread_mutex_lock(&process->pool_full_queue_mutex);
	pthread_mutex_lock(&thr->mutex);
	ZQ *pool_full_queue = &process->pool_full_queue;
	if (zq_len(pool_full_queue)==0)
	{
		fprintf(stderr, "pool_full: queue starts.\n");
	}
	zq_append(pool_full_queue, thr);
	pthread_mutex_unlock(&thr->mutex);
	pthread_mutex_unlock(&process->pool_full_queue_mutex);
#endif
}

void Tx::_pool_available()
{
#if 0
	pthread_mutex_lock(&process->pool_full_queue_mutex);
	ZQ *pool_full_queue = &process->pool_full_queue;
	if (zq_len(pool_full_queue)>0)
	{
		MThread *thr = zq_pop(pool_full_queue);
		pthread_mutex_lock(&thr->mutex);
		_complete_other_thread_yield(thr, pool_full_queue->dbg_zutex_handle);
		pthread_mutex_unlock(&thr->mutex);
		if (zq_len(pool_full_queue)>0)
		{
			fprintf(stderr, "pool_available: queue emptied.\n");
		}
	}
	pthread_mutex_unlock(&process->pool_full_queue_mutex);
#endif
}

Tx::Disp Tx::handle(XpReqLaunch *req, XpReplyLaunch *reply)
{
	XPBPoolElt *xpbe =
		process->get_buf_pool()->lookup_elt(req->open_buffer_handle);

	SignedBinary *sb = (SignedBinary *) (xpbe->xpb->un.znb._data);
	uint32_t cert_len = Z_NTOHG(sb->cert_len);
	uint32_t binary_len = Z_NTOHG(sb->binary_len);
	uint8_t *certBytes = ((uint8_t*) &sb[1]);
	uint8_t *image = ((uint8_t*) &sb[1])+cert_len;

	ZCert* cert = new ZCert(certBytes, cert_len);
	assert(cert != NULL);
	ZPubKey* principalKey = new ZPubKey(cert->getEndorsingKey());
	assert(principalKey != NULL);
	ZBinaryRecord* binaryRecord = new ZBinaryRecord(image, binary_len);
	assert(binaryRecord != NULL);

	// Cert must be for the binary we're about to launch, and the signature must pass muster
	assert(binaryRecord->isEqual(cert->getEndorsedBinary()) && cert->verify(principalKey));

	delete cert;
	delete binaryRecord;

    uint32_t sb_len = sizeof(SignedBinary) + cert_len + binary_len;
	int rc;
	char tmp_signed_binary_fn[800];
	static int uniquifier = 0;
	snprintf(tmp_signed_binary_fn, sizeof(tmp_signed_binary_fn),
		"/tmp/pal_launch_image_%d.%d.%d.%d.sb",
		(int) time(NULL),
		rand(),
		getpid(),
		uniquifier++);
	rc = unlink(tmp_signed_binary_fn);
	assert(rc==0 || errno==ENOENT);
	FILE *tmp_binary_image_fp = fopen(tmp_signed_binary_fn, "w");
	rc = fwrite(sb, sb_len, 1, tmp_binary_image_fp);
	fprintf(stderr, "wrote %d records of length %d\n", rc, sb_len);
	assert(rc==1);
	fclose(tmp_binary_image_fp);

	process->get_buf_pool()->release_buffer(req->open_buffer_handle, false);
	_pool_available();
#if BUFFER_ACCOUNTING
	buffer_accounting(-1, 0);
#endif // BUFFER_ACCOUNTING

	int pid = fork();
	if (pid==0)
	{
		// child
		int my_pid = getpid();
		char my_dir[1000];
		snprintf(my_dir, sizeof(my_dir), "pal_%d", my_pid);
		char clean_cmd[1000];
		snprintf(clean_cmd, sizeof(clean_cmd), "rm -rf %s", my_dir);
		system(clean_cmd);
		mkdir(my_dir, 0755);
		chdir(my_dir);

		char *pal_executable = (char*)
			ZOOG_ROOT "/monitors/linux_dbg/pal/build/xax_port_pal";
		char *args[3];
		int argc=0;
		args[argc++] = pal_executable;
		args[argc++] = tmp_signed_binary_fn;
//		args[argc++] = (char*)"--wait-for-debugger";
		args[argc++] = NULL;
		execv(pal_executable, args);
		fprintf(stderr, "Expecting PAL at %s\n", pal_executable);
		perror("exec of PAL failed");
		exit(-1);
	}

	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqEndorseMe *req, XpReplyEndorseMe *reply) 
{
	ZPubKey* appPubKey = new ZPubKey(req->app_key_bytes, req->app_key_bytes_len);

	ZCert* endorsement = new ZCert(getCurrentTime() - 1,          // Valid starting 1 second ago
	                               getCurrentTime() + 31536000,   // Valid for a year
	                               appPubKey,
	                               process->get_pub_key());
	assert(endorsement != NULL);
	endorsement->sign(monitor->get_monitorKeyPair()->getPrivKey());

	// NB clunkiness: we have a variable-length payload,
	// so we don't get a preallocated reply.
	assert(dreply==NULL && reply==NULL);
	int sized_len = sizeof(XpReplyEndorseMe)+endorsement->size();
	dreply = new ReplyDeleter(sized_len);
	reply = (XpReplyEndorseMe*) dreply->reply;
	reply->app_cert_bytes_len = endorsement->size();
	endorsement->serialize(reply->app_cert_bytes);

	delete appPubKey;
	delete endorsement;
	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqGetAppSecret *req, XpReplyGetAppSecret *reply) 
{
	// Use the monitor's key to derive an app-specific key
	KeyDerivationKey* masterKey = monitor->get_app_master_key();
	ZPubKey* appPubKey = process->get_pub_key();
	KeyDerivationKey* appKey = masterKey->deriveKeyDerivationKey(appPubKey);

	uint32_t app_secret_len = req->num_bytes_requested <= appKey->size() ? 
	                          req->num_bytes_requested : appKey->size();

	assert(dreply==NULL && reply==NULL);
	int sized_len = sizeof(XpReplyGetAppSecret)+app_secret_len;
	dreply = new ReplyDeleter(sized_len);
	reply = (XpReplyGetAppSecret*) dreply->reply;
	reply->num_bytes_returned = app_secret_len;
	memcpy(reply->secret_bytes, appKey->getBytes(), app_secret_len);

	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqEventPoll *req, XpReplyEventPoll *reply)
{
	process->get_event_synchronizer()->wait(req->known_sequence_numbers, reply->new_sequence_numbers);
	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqSubletViewport *req, XpReplySubletViewport *reply)
{
	BlitterViewport *parent_viewport = blitterMgr()->get_tenant_viewport(req->tenant_viewport, process->get_id());
	BlitterViewport *child_viewport = parent_viewport->sublet(&req->rectangle, process->get_id());
	reply->out_landlord_viewport = child_viewport->get_landlord_id();
	reply->out_deed = child_viewport->get_deed();
	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqRepossessViewport *req, XpReplyRepossessViewport *reply)
{
	BlitterViewport *landlord_viewport = blitterMgr()->get_landlord_viewport(req->landlord_viewport, process->get_id());
	delete landlord_viewport;
	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqGetDeedKey *req, XpReplyGetDeedKey *reply)
{
	BlitterViewport *landlord_viewport = blitterMgr()->get_landlord_viewport(req->landlord_viewport, process->get_id());
	reply->out_deed_key = landlord_viewport->get_deed_key();
	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqAcceptViewport *req, XpReplyAcceptViewport *reply)
{
	BlitterViewport *viewport = blitterMgr()->accept_viewport(req->deed, process->get_id());	// this lookup is global, as the deed is a cryptographic capability
	lite_assert(viewport!=NULL);	// safety: deed was bogus
	reply->tenant_viewport = viewport->get_tenant_id();
	reply->deed_key = viewport->get_deed_key();
	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqTransferViewport *req, XpReplyTransferViewport *reply)
{
	BlitterViewport *tenant_viewport = blitterMgr()->get_tenant_viewport(req->tenant_viewport, process->get_id());
	lite_assert(tenant_viewport->get_tenant_id()==req->tenant_viewport);	// safety: tenant_viewport handle was a landlord handle; what you tryin' to pull?
	reply->deed = tenant_viewport->transfer();
	complete_xax_port_op();
	return TX_DONE;
}

#define MAX_RAND_BYTES 1000

Tx::Disp Tx::handle(XpReqGetRandom *req, XpReplyGetRandom *reply) 
{
	uint32_t rand_bytes_len = req->num_bytes_requested <= MAX_RAND_BYTES ? 
	                          req->num_bytes_requested : MAX_RAND_BYTES;

	assert(dreply==NULL && reply==NULL);
	int sized_len = sizeof(XpReplyGetRandom)+rand_bytes_len;
	dreply = new ReplyDeleter(sized_len);
	reply = (XpReplyGetRandom*) dreply->reply;
	reply->num_bytes_returned = rand_bytes_len;
	monitor->get_random_bytes(reply->random_bytes, rand_bytes_len);

	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqVerifyLabel *req, XpReplyVerifyLabel *reply) 
{
	ZCertChain* chain = new ZCertChain(req->cert_chain_bytes, req->message_len);
	assert(chain != NULL);

	if (process->get_pub_key()->verifyLabel(chain, monitor->get_zoog_ca_root_key()))
	{
		process->mark_labeled();
		reply->success = true;
	} else {
		reply->success = false;
	}

	delete chain;
  

	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqMapCanvas *req, XpReplyMapCanvas *reply)
{
	LinuxDbgCanvasAcceptor acceptor(process, reply);
	const char *label = process->get_label();
	if (label==NULL && monitor->get_dbg_allow_unlabeled_ui())
	{
		label = "*DEBUG*NO LABEL*";
	}

	if (label!=NULL)
	{
		blitterMgr()->get_tenant_viewport(req->viewport_id, process->get_id())
			->map_canvas(
				process, req->known_formats, req->num_formats, &acceptor,
				label);
	}
	else
	{
		// app accepted a canvas before labeling itself.
		lite_assert(false);
	}

	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqUnmapCanvas *req, XpReplyUnmapCanvas *reply)
{
	BlitterCanvas *canvas = blitterMgr()->get_canvas(req->canvas_id);
	canvas->unmap();
	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqUpdateCanvas *req, XpReplyUpdateCanvas *reply)
{
	BlitterCanvas *canvas = blitterMgr()->get_canvas(req->canvas_id);
	lite_assert(canvas!=NULL);
	lite_assert(canvas->get_palifc() == process->as_palifc());	// ownership check
	canvas->update_canvas(&req->rectangle);
	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqReceiveUIEvent *req, XpReplyReceiveUIEvent *reply)
{
	ZoogUIEvent *zuie = process->get_uieq()->remove();
	if (zuie!=NULL)
	{
		reply->event = *zuie;
		delete zuie;
	}
	else
	{
		reply->event.type = zuie_no_event;
	}
	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqDebugCreateToplevelWindow *req, XpReplyDebugCreateToplevelWindow  *reply)
{
	reply->tenant_id = blitterMgr()->new_toplevel_viewport(process->get_id());
	complete_xax_port_op();
	return TX_DONE;
}

#if 0
Tx::Disp Tx::handle(XpReqDelegateViewport *req, XpReplyDelegateViewport *reply)
{
	ZPubKey *recipient = new ZPubKey((uint8_t*) &req[1], req->app_pub_key_bytes_len);
	BlitterCanvas *canvas = blitterMgr()->get_canvas(req->canvas_id);
	lite_assert(canvas->get_palifc() == process->as_palifc());	// ownership check
	reply->viewport_id = canvas->delegate_viewport(recipient);
	complete_xax_port_op();
	return TX_DONE;
}

Tx::Disp Tx::handle(XpReqUpdateViewport *req, XpReplyUpdateViewport *reply)
{
	BlitterViewport *viewport = blitterMgr()->get_viewport(req->viewport_id);
	lite_assert(viewport->get_palifc() == process->as_palifc());	// ownership check
	WindowRect wr(req->x, req->y, req->width, req->height);
	viewport->update_viewport(&wr);
	complete_xax_port_op();
	return TX_DONE;
}
#endif

//////////////////////////////////////////////////////////////////////////////

enum RequestSize { FIXED, VARIABLE };

template<class TReq, class TReply>
void Tx::handle(const char *struct_name, bool fixed_len_reply)
{
	if (TReq::fixed_length && (read_size!=sizeof(TReq))) {
		fprintf(stderr,
			"Packet wrong size for struct %s; got %d, expected %d\n",
			struct_name, read_size, sizeof(TReq));
		assert(0);
		return;
	}

	TReply *reply;
	if (fixed_len_reply)
	{
		dreply = new ReplyDeleter(sizeof(TReply));
		reply = (TReply*) dreply->reply;
	}
	else
	{
		// defer allocation of reply to specific handler
		dreply = NULL;
		reply = NULL;
	}
	Disp disposition = handle((TReq *) read_buf, reply);
	if (disposition==TX_DONE)
	{
		delete this;
	}
}

void Tx::handle()
{
	assert(read_size < (int) sizeof(read_buf));	// overflow

	if (read_size<0)
	{
		perror("read socket");
		return;
	}
	if (read_size==0)
	{
		// channel EOF. Hmm, maybe we should close our end?
		// OH. there's no channel here. this case can't happen.
		assert(false);
		return;
	}
	if (read_size < (int) sizeof(XpReqHeader))
	{
		fprintf(stderr,
			"Packet too short (%d) for XpReqHeader (%d)\n**** RESETTING ****\n",
			read_size, sizeof(XpReqHeader));
		assert(false);
		return;
	}

#if DEBUG_ZUTEX_OWNER
	dzt_log(&monitor->dzt, &xpm->hdr, xf, "start");
#endif // DEBUG_ZUTEX_OWNER

	req = (XpReqHeader*) read_buf;
	process = monitor->lookup_process(req->process_id);

	if (process==NULL && req->opcode != xpConnect)
	{
		fprintf(stderr, "Dropping message from culled process %d. Sorry!\n",
			req->process_id);
		return;
	}

	switch (req->opcode)
	{

#define CASE(NN) \
	case xp##NN: \
		XAXOP_STRACE(#NN); \
		handle<XpReq##NN,XpReply##NN>(#NN ,true); \
		break; \

#define CASE_VARIABLE_REPLY(NN) \
	case xp##NN: \
		XAXOP_STRACE(#NN); \
		handle<XpReq##NN,XpReply##NN>(#NN,false); \
		break; \

		CASE(Connect)
		CASE(Disconnect)
		CASE(XNBAlloc)
		CASE(XNBRecv)
		CASE(XNBSend)
		CASE(XNBRelease)
		CASE(GetIfconfig)
		CASE(ZutexWaitPrepare)
		CASE(ZutexWaitCommit)
		CASE(ZutexWaitAbort)
		CASE(ZutexWake)
		CASE(Launch)
		CASE(EventPoll)
		CASE(SubletViewport)
		CASE(RepossessViewport)
		CASE(GetDeedKey)
		CASE(AcceptViewport)
		CASE(TransferViewport)
		CASE(VerifyLabel)
		CASE(MapCanvas)
		CASE(UnmapCanvas)
		CASE(UpdateCanvas)
		CASE(ReceiveUIEvent)
		CASE_VARIABLE_REPLY(EndorseMe)
		CASE_VARIABLE_REPLY(GetAppSecret)
		CASE(DebugCreateToplevelWindow)
		CASE_VARIABLE_REPLY(GetRandom)
		default:
			assert(0);
	}

#if DEBUG_ZUTEX_OWNER
	dzt_log(&tx->m->dzt, &xpm->hdr, xf, "end");
#endif // DEBUG_ZUTEX_OWNER
}
