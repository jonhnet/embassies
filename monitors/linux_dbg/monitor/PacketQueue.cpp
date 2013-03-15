#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <pthread.h>

#include "xax_network_defs.h"
#include "PacketQueue.h"
#include "LiteLib.h"
#include "standard_malloc_factory.h"

#define TRACK_DOWN_STOMP 0

static void _pq_tidy_nolock(PacketQueue *pq);

typedef struct {
	PacketAccount *pa;
	int total_packets;
	int total_bytes;
	bool verbose;
} PacketAccountReport;

void packet_account_report_init(PacketAccountReport *par, PacketAccount *pa, bool verbose)
{
	par->pa = pa;
	par->total_packets = 0;
	par->total_bytes = 0;
	par->verbose = verbose;
}

static void packet_account_report_visit(void *user_obj, void *a)
{
	PacketAccountReport *par = (PacketAccountReport *) user_obj;
	PacketIfc *p = (PacketIfc *) a;
	par->total_packets += 1;
	par->total_bytes += p->len();
	if (par->verbose)
	{
		fprintf(stderr, "  packet @0x%08x len 0x%x for %s\n",
			(uint32_t) p, p->len(), p->dbg_purpose());
	}
}

void packet_account_report(PacketAccount *pa, bool verbose)
{
	if (pa==NULL)
	{
		fprintf(stderr, "Packet accounting disabled.\n");
		return;
	}

	PacketAccountReport par;
	packet_account_report_init(&par, pa, verbose);
	hash_table_visit_every(&pa->outstanding_packets,
		packet_account_report_visit, &par);
	lite_assert(pa->total_alloc - pa->total_free == par.total_packets);
	fprintf(stderr, "totals %d packets alloced, %d remain with 0x%08x bytes\n",
		pa->total_alloc,
		par.total_packets,
		par.total_bytes);
}

//////////////////////////////////////////////////////////////////////////////

uint32_t hash_packet(const void *v_p)
{
	return (uint32_t) v_p;
}

int cmp_packet(const void *v_a, const void *v_b)
{
	return ((uint32_t)v_a)-((uint32_t)v_b);
}

void packet_account_init(PacketAccount *pa, MallocFactory *mf)
{
	pthread_mutex_init(&pa->mutex, NULL);
	hash_table_init(&pa->outstanding_packets, mf, hash_packet, cmp_packet);
	pa->total_alloc = 0;
	pa->total_free = 0;
	pa->packet_history = NULL;
	// pa->packet_history = fopen("packet_history", "w");
}

void packet_account_create(PacketAccount *pa, PacketIfc *p)
{
	if (pa==NULL)
	{
		return;
	}

	pthread_mutex_lock(&pa->mutex);
	hash_table_insert(&pa->outstanding_packets, p);
	pa->total_alloc += 1;
	if (pa->packet_history!=NULL)
	{
		fprintf(pa->packet_history, "create @0x%08x len 0x%08x\n", (uint32_t) p, p->len());
		fflush(pa->packet_history);
	}
//	packet_account_report(pa, true);
	pthread_mutex_unlock(&pa->mutex);
}

void packet_account_free(PacketAccount *pa, PacketIfc *p)
{
	if (pa==NULL)
	{
		return;
	}

	pthread_mutex_lock(&pa->mutex);
	hash_table_remove(&pa->outstanding_packets, p);
	pa->total_free += 1;
	if (pa->packet_history!=NULL)
	{
		fprintf(pa->packet_history, "free @0x%08x len 0x%08x\n", (uint32_t) p, p->len());
		fflush(pa->packet_history);
	}
//	packet_account_report(pa, true);
	pthread_mutex_unlock(&pa->mutex);
}

//////////////////////////////////////////////////////////////////////////////

#if 0
Packet *packet_init(char *data, int len, PacketAccount *pa, const char *purpose)
{
	Packet *p = packet_alloc(len, pa, purpose);
	memcpy(packet_data(p), data, len);
	return p;
}

Packet *packet_alloc(int len, PacketAccount *pa, const char *purpose)
{
	Packet *p = (Packet*)malloc(sizeof(Packet));
	p->len = len;
#if TRACK_DOWN_STOMP
	p->_data = malloc(len+8);
	((uint32_t*)p->_data)[0] = 0xf0000f1e;
	((uint32_t*)(p->_data+4+len))[0] = 0x0f000f1e;
#else
	p->_data = (char*) malloc(len);
#endif // TRACK_DOWN_STOMP
	p->dbg_purpose = purpose;
	packet_account_create(pa, p);
	return p;
}

Packet *packet_copy(Packet *p, PacketAccount *pa, const char *purpose)
{
	return packet_init(packet_data(p), p->len, pa, purpose);
}

void packet_free(Packet *packet, PacketAccount *pa)
{
#if TRACK_DOWN_STOMP
	assert(((uint32_t*)packet->_data)[0] == 0xf0000f1e);
	assert(((uint32_t*)(packet->_data+4+packet->len))[0] == 0x0f000f1e);
	((uint32_t*)packet->_data)[0] = 0x33443344;
	((uint32_t*)(packet->_data+4+packet->len))[0] = 0x77887788;
#endif // TRACK_DOWN_STOMP
	packet_account_free(pa, packet);
	free(packet->_data);
	free(packet);
}

char *packet_data(Packet *p)
{
#if TRACK_DOWN_STOMP
	return p->_data+4;
#else
	return p->_data;
#endif // TRACK_DOWN_STOMP
}
#endif

//////////////////////////////////////////////////////////////////////////////

void pq_init(PacketQueue *pq, PacketAccount *packet_account, uint32_t max_bytes, int min_packets, EventSynchronizer *event_synchronizer)
{
	linked_list_init(&pq->queue, standard_malloc_factory_init());
	pthread_mutex_init(&pq->mutex, NULL);
#if 0
	pthread_cond_init(&pq->cond, NULL);
#endif
	pq->size_bytes = 0;
	pq->packet_account = packet_account;
	pq->max_bytes = max_bytes;
	pq->min_packets = min_packets;
	pq->event_synchronizer = event_synchronizer;
	pq->sequence_number = 0;
	pq->event_synchronizer->update_event(alarm_receive_net_buffer, pq->sequence_number);
}

void pq_free(PacketQueue *pq)
{
	pthread_mutex_destroy(&pq->mutex);
#if 0
	pthread_cond_destroy(&pq->cond);
#endif
	// TODO actually release space
	assert(pq->queue.count == 0);
	assert(pq->size_bytes == 0);
}

void pq_insert(PacketQueue *pq, PacketIfc *p)
{
	pthread_mutex_lock(&pq->mutex);
#if 0
	pthread_cond_signal(&pq->cond);
#endif
	pq->sequence_number += 1;
	pq->event_synchronizer->update_event(alarm_receive_net_buffer, pq->sequence_number);
	linked_list_insert_tail(&pq->queue, p);
	pq->size_bytes += p->len();
	_pq_tidy_nolock(pq);
	pthread_mutex_unlock(&pq->mutex);
}

static PacketIfc *_pq_remove_nolock(PacketQueue *pq)
{
	if (pq->queue.count == 0)
	{
		return NULL;
	}
#if 0 // old blocking code.
	while (pq->hdr.next==NULL)
	{
//		fprintf(stderr, "remove waits\n");
		pthread_cond_wait(&pq->cond, &pq->mutex);
//		fprintf(stderr, "remove ready\n");
	}
#endif
	PacketIfc *p = (PacketIfc*) linked_list_remove_head(&pq->queue);
	pq->size_bytes -= p->len();
	return p;
}

PacketIfc *pq_remove(PacketQueue *pq)
{
	pthread_mutex_lock(&pq->mutex);
	PacketIfc *p = _pq_remove_nolock(pq);
	pthread_mutex_unlock(&pq->mutex);

	return p;
}

#if 0 // dead code
uint32_t pq_block(PacketQueue *pq, uint32_t known_sequence_number)
{
	pthread_mutex_lock(&pq->mutex);
	while (pq->sequence_number == known_sequence_number)
	{
//		fprintf(stderr, "block waits\n");
		pthread_cond_wait(&pq->cond, &pq->mutex);
//		fprintf(stderr, "block ready\n");
	}
	uint32_t new_sequence_number = pq->sequence_number;
	pthread_mutex_unlock(&pq->mutex);
	return new_sequence_number;
}
#endif

static void _pq_tidy_nolock(PacketQueue *pq)
{
	while (pq->size_bytes > pq->max_bytes && pq->queue.count > pq->min_packets)
	{
		PacketIfc *p = _pq_remove_nolock(pq);
		fprintf(stderr, "queue overfull; tossing 0x%08x-byte packet\n",
			p->len());
		packet_account_free(pq->packet_account, p);
		delete p;
	}
}
