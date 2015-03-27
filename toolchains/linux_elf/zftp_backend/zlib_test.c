#include <zlib.h>
#include <string.h>
#include <malloc.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

typedef struct s_Chunk {
	uint8_t* data;
	uint32_t len;
	uint32_t capacity;
} Chunk;

void chunk_read(Chunk* ch, const char* fn) {
	FILE* fp = fopen(fn, "rb");
	fseek(fp, 0, SEEK_END);
	ch->len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	ch->capacity = ch->len;
	ch->data = malloc(ch->len);
	int rc = fread(ch->data, ch->len, 1, fp);
	assert(rc==1);
	fclose(fp);
}

void chunk_alloc(Chunk* ch, int len) {
	ch->capacity = len;
	ch->data = malloc(len);
	ch->len = 0;
}

void chunk_free(Chunk* ch) {
	free(ch->data);
	ch->data = NULL;
	ch->len = 0;
	ch->capacity = 0;
}

typedef enum { DEFLATE, INFLATE } Flate;

void* z_alloc(void* opaque, uInt items, uInt size) { return malloc(items*size); }
void z_free(void* opaque, void* address) { free(address); }

void zstream_init(z_stream* stream, Flate flate) {
	stream->zalloc = z_alloc;
	stream->zfree = z_free;
	stream->opaque = NULL;
	if (flate==INFLATE) {
		inflateInit(stream);
	} else {
		deflateInit2(stream, 1, Z_DEFLATED, 15, 8, Z_RLE);
	}
}

void zstream_close(z_stream* stream, Flate flate) {
	int rc;
	if (flate==INFLATE) {
		rc = inflateEnd(stream);
	} else {
		rc = deflateEnd(stream);
	}
	assert(rc==Z_OK);
}

void chunk_flate(z_stream* stream, Flate flate, Chunk* out, Chunk* in) {
	stream->next_out = out->data+out->len;
	stream->avail_out = out->capacity - out->len;
	stream->next_in = in->data;
	stream->avail_in = in->len;
	int rc;
	if (flate==INFLATE) {
		rc = inflate(stream, Z_SYNC_FLUSH);
	} else {
		rc = deflate(stream, Z_SYNC_FLUSH);
	}
	fprintf(stderr, "rc==%d\n", rc);
	assert(rc==Z_OK);

	// Crop output to correct size
	int count = (out->capacity - out->len) - stream->avail_out;
	out->len += count;
}

bool chunk_equal(Chunk* c0, Chunk* c1) {
	if (c0->len != c1->len) { return false; }
	return memcmp(c0, c1, c0->len)==0;
}

int main()
{
	Chunk u_data;
	chunk_read(&u_data, "/tmp/debug_uncompressed_00d45c17_0000ff3b");

	Chunk c_data;
	chunk_alloc(&c_data, 0xff3b);
//	chunk_flate(DEFLATE, &c_data, &u_data);
	Chunk u0, u1;
	u0.data = u_data.data;
	u0.len = 0xa3e9;
	u1.data = u_data.data+u0.len;
	u1.len = u_data.len - u0.len;
	assert(u1.len == 0x5b52);
	z_stream a_stream;
	zstream_init(&a_stream, DEFLATE);
	chunk_flate(&a_stream, DEFLATE, &c_data, &u0);
	chunk_flate(&a_stream, DEFLATE, &c_data, &u1);
	zstream_close(&a_stream, DEFLATE);

	Chunk ref_c_data;
	chunk_read(&ref_c_data, "/tmp/debug_compressed_00d45c17_0000ff3b");

	assert(chunk_equal(&c_data, &ref_c_data));

	Chunk re_u_data;
	chunk_alloc(&re_u_data, 1<<17);
	zstream_init(&a_stream, INFLATE);
	chunk_flate(&a_stream, INFLATE, &re_u_data, &c_data);
	zstream_close(&a_stream, INFLATE);
	assert(chunk_equal(&u_data, &re_u_data));

	chunk_free(&u_data);
	chunk_free(&c_data);
	chunk_free(&ref_c_data);
	chunk_free(&re_u_data);
	return 0;
}
