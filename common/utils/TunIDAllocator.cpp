#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <malloc.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "LiteLib.h"
#include "TunIDAllocator.h"

#define PREF_FILENAME	".zoog_tunid"

char* TunIDAllocator::_pref_filename()
{
	char *home_dir = getenv("HOME");
	char *path = (char*) malloc(strlen(home_dir)+1+strlen(PREF_FILENAME)+1);
	sprintf(path, "%s/%s", home_dir, PREF_FILENAME);
	return path;
}

TunIDAllocator::TunIDAllocator()
	: assigned(false),
	  tun_fd(-1)
{
	char *tunid_str = getenv("ZOOG_TUNID");
	if (tunid_str != NULL)
	{
		this->assigned = true;
		this->tunid = atoi(tunid_str);
		return;
	}

	int rc;
	char *pfn = _pref_filename();
	FILE* tfp = fopen(pfn, "r");
	free(pfn);
	if (tfp!=NULL)
	{
		char buf[100];
		memset(buf, 0, sizeof(buf));
		rc = fread(buf, 1, sizeof(buf)-1, tfp);
		fclose(tfp);
		if (rc>0)
		{
			this->assigned = true;
			this->tunid = atoi(buf);
			return;
		}
	}
	// Okay, then assign it when asked to create the tunnel.
}

int TunIDAllocator::get_tunid()
{
	if (!this->assigned)
	{
		fprintf(stderr, "You don't have a tunid assigned. Must run a monitor first to assign one.\n");
		lite_assert(false);
	}
	return tunid;
}

int TunIDAllocator::open_tunnel()
{
	if (tun_fd!=-1)
	{
		return tun_fd;
	}

	tun_fd = open("/dev/net/tun", O_RDWR);
	lite_assert(tun_fd>=0);

	bool brc;
	if (assigned)
	{
		brc = _assign_id_to_tunnel(tun_fd, tunid);
		if (!brc)
		{
			fprintf(stderr, "Error! %s.\n"
			"The most likely cause of this is that you left a port monitor\n"
			"running (on the same tunid, %d).\n",
				strerror(errno), tunid);
			lite_assert(false);
		}
	}
	else
	{
		// Ooh, this user doesn't even HAVE a tunid. Let's find one.
		for (int id=1; id<10; id++)
		{
			brc = _assign_id_to_tunnel(tun_fd, id);
			if (brc)
			{
				this->assigned = true;
				this->tunid = id;
				_write_back_tunid();
				break;
			}
		}
		if (!assigned)
		{
			fprintf(stderr, "Couldn't get any tunids to work.\n");
			lite_assert(false);
		}
	}

	return tun_fd;
}

bool TunIDAllocator::_assign_id_to_tunnel(int fd, int id)
{
	struct ifreq ifr;
	_setup_ifreq_internal(&ifr, id);
	int rc = ioctl(fd, TUNSETIFF, (void*) &ifr);
	return (rc==0);
}

void TunIDAllocator::_setup_ifreq_internal(struct ifreq* ifr, int id)
{
	memset(ifr, 0, sizeof(struct ifreq*));
	ifr->ifr_flags = IFF_TUN;
	sprintf(ifr->ifr_name, "tun%d", tunid);
}

void TunIDAllocator::setup_ifreq(struct ifreq* ifr)
{
	_setup_ifreq_internal(ifr, get_tunid());
}

void TunIDAllocator::_write_back_tunid()
{
	lite_assert(assigned);

	// NB this assignment policy -- being sure no one else has a conflicting
	// tunX open on the machine -- is kind of stupid, because they might
	// just not have it open NOW. Sigh.
	fprintf(stderr, "Assigned tunid %d to you.\n", tunid);
	char *pfn = _pref_filename();
	FILE* tfp = fopen(pfn, "w");
	free(pfn);
	fprintf(tfp, "%d\n", tunid);
	fclose(tfp);
}
