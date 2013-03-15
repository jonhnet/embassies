#include <assert.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include "Shm.h"

#define DBG_TRACK_SHM 0

ShmMapping::ShmMapping(uint32_t size, int key, int shmid)
	: size(size),
	  key(key),
	  shmid(shmid),
	  mapping(NULL)
{
#if DBG_TRACK_SHM
	if (size>24<<20) {
		fprintf(stderr, "shm ctor shmid %d\n", shmid);
	}
#endif // DBG_TRACK_SHM
}

ShmMapping::ShmMapping(void *mapping)
	: size(0),
	  key(-1),
	  shmid(-1),
	  mapping(mapping)
{
}

void *ShmMapping::map(void *req_addr)
{
	if (mapping==NULL)
	{
		int flags = 0;
		if (req_addr!=NULL)
		{
			flags |= SHM_REMAP;
		}
		mapping = shmat(shmid, req_addr, flags);
#if DBG_TRACK_SHM
		if (size>24<<20) { fprintf(stderr, "shmat shmid %d\n", shmid); }
#endif // DBG_TRACK_SHM
		assert(mapping != (void*) -1);
	}
	return mapping;
}

void ShmMapping::unmap()
{
#if DBG_TRACK_SHM
	if (size>24<<20) { fprintf(stderr, "shmdt shmid %d\n", shmid); }
#endif // DBG_TRACK_SHM
//	assert(mapping!=NULL);
// Apparently it now sometimes occurs that zftp_zoog will allocate a
// buffer and destroy it without us ever seeing it. ?
	if (mapping!=NULL)
	{
		int rc = shmdt(mapping);
		assert(rc==0);
		mapping = NULL;
	}
}

void ShmMapping::unlink()
{
	int rc = shmctl(shmid, IPC_RMID, NULL);
	assert(rc==0);
}

//////////////////////////////////////////////////////////////////////////////

uint32_t Shm::_make_uniquifier(int tunid)
{
	int zoog_tunid = (tunid) & 0x0f;
	int pid = getpid() & 0xffff;
	return (zoog_tunid << 28) | (pid<<12);
}

Shm::Shm(int tunid)
	: uniquifier(_make_uniquifier(tunid)),
	  next_identity(1)
{
}

ShmMapping Shm::allocate(uint32_t identity, uint32_t size)
{
	// If a prior process died and leaked a segment, clean it up.
	key_t key = uniquifier | identity;
	int rc;

	int shmid = shmget(key, 1 /*SHMMIN*/, 0600);
	if (shmid!=-1)
	{
		rc = shmctl(shmid, IPC_RMID, NULL);
		assert(rc==0);
	}

	int shmmode = 0600;
	if (getenv("DEBUG_OPEN_SHM_PERMS")!=NULL)
	{
		fprintf(stderr, "Security warning: DEBUG_OPEN_SHM_PERMS in effect.\n");
		shmmode = 0x666;
	}

	shmid = shmget(key, size, IPC_CREAT|IPC_EXCL|shmmode);
	if (shmid==-1)
	{
		perror("shmget");
		fprintf(stderr, "Asked for size(%d (0x%x))\n", size, size);
		const char *shmmax_path = "/proc/sys/kernel/shmmax";
		FILE *fp = fopen(shmmax_path, "r");
		int shmmax;
		rc = fscanf(fp, "%d", &shmmax);
		fclose(fp);
		fprintf(stderr,
			"FYI, %s is %dMB; request is %dMB, so that %s your problem right there\n",
			shmmax_path,
			shmmax>>20,
			size>>20,
			(size > (uint32_t) shmmax) ? "IS" : "isn't");
		fprintf(stderr, "echo %d | sudo dd of=%s # for 150MB\n", 150<<20, shmmax_path);
	}
	assert(shmid != -1);

	return ShmMapping(size, key, shmid);
}

ShmMapping Shm::allocate(uint32_t size)
{
	uint32_t identity = next_identity++;
	assert(identity < (1<<12));
		// uh oh, keyspace getting too crowded.
		// (see refactoring-notes.txt 2012.03.08)
		// Why are you sending 2^12 separate shm segments? Time to consider
		// recycling shm segments, or something.
	return Shm::allocate(identity, size);
}
