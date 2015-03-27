#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>

#include "xe_mark_perf_point.h"

class ScanIfc {
private:
	uint32_t accum;
public:
	ScanIfc();
	void scan(const char* name);
	virtual void scan_impl(int fd, size_t size) = 0;
	void touch_all(void *addr, size_t len);
};

ScanIfc::ScanIfc()
	: accum(0)
{
}

void ScanIfc::scan(const char* name)
{
	char path[800];
	DIR* dir = opendir(name);
	assert(dir!=NULL);
	while (true)
	{
		struct dirent* entry;
		entry = readdir(dir);
		if (entry==NULL)
		{
			return;
		}
		snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
		int fd = open(path, O_RDONLY);
		struct stat buf;
		int rc = fstat(fd, &buf);
		assert(rc==0);
		if (S_ISREG(buf.st_mode))
		{
			scan_impl(fd, buf.st_size);
		}
		close(fd);
	}

	closedir(dir);
}

void ScanIfc::touch_all(void *addr, size_t len)
{
	int off = 0;
	for (off=0; off<((int)(len-sizeof(uint32_t))); off+=4096)
	{
		uint32_t* ptr = (uint32_t*) (((uint8_t*)addr)+off);
		accum += ptr[0];
	}
}

class ScanMmap : public ScanIfc {
public:
	void scan_impl(int fd, size_t size);
};

void ScanMmap::scan_impl(int fd, size_t size)
{
	void* rc = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (rc==MAP_FAILED)
	{
		perror("crap. mmap.");
		assert(false);
	}
	touch_all(rc, size);
}

class ScanRead : public ScanIfc {
public:
	void scan_impl(int fd, size_t size);
};

void ScanRead::scan_impl(int fd, size_t size)
{
	FILE* fp = fdopen(fd, "r");
	assert(fp!=NULL);
	void* buffer = malloc(size);
	assert(buffer!=NULL);
	size_t rc = fread(buffer, size, 1, fp);
	assert(rc==1);
	touch_all(buffer, size);
}

int main(int argc, const char** argv)
{
	assert(argc==2);
	const char* image_dir = argv[1];
//#define EXP_PATH ZOOG_ROOT "/toolchains/linux_elf/zftp_time_posix/"

	char path[800];
	posix_perf_point_setup();
	ScanMmap sm;
	snprintf(path, sizeof(path), "%s/mmap", image_dir);
	sm.scan(path);
	ScanRead sr;
	snprintf(path, sizeof(path), "%s/read", image_dir);
	sr.scan(path);
	xe_mark_perf_point((char*) "done");
}
