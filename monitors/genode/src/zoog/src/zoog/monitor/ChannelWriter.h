#pragma once

#include <base/stdint.h>
#include <base/lock.h>
#include <block_session/connection.h>
#include <base/allocator_avl.h>

/*
 Read algorithm:

	uint32_t generation_number = fread(4);
	while (true)
	{
		uint32_t record_length = fread(4);
		if (record_length == 0) { break; }

		uint32_t name_length = fread(4);
		uint32_t data_length = fread(4);
		char *name = fread(name_length);
		char *data = fread(data_length);
		fread(record_length-4-4-4-name_length-data_length);
	}

	pad is used to align entire record to a 4-byte boundary, so that
		record_length field doesn't cross sector boundaries, to make
		it easier to ensure that the log invariant is maintained as we
		update it.
*/

using namespace Genode;

class CtorDbg1 { public: CtorDbg1() { PDBG("CtorDbg1\n"); } };
class CtorDbg2 { public: CtorDbg2() { PDBG("CtorDbg2\n"); } };
class CtorDbg3 { public: CtorDbg3() { PDBG("CtorDbg3\n"); } };
class CtorDbg4 { public: CtorDbg4() { PDBG("CtorDbg4\n"); } };

class ChannelWriter {
private:
	enum { SECTOR_SIZE = 512, BULK_BUFFER_SIZE=32*SECTOR_SIZE };
	enum { ALIGN_CONSTRAINT = sizeof(uint32_t) };

	class Transaction {
	public:
		uint32_t payload_offset;
		uint32_t payload_len;
		uint32_t remaining_len() { return payload_len - payload_offset; }
		uint32_t pad_len;
	};

	class Buffer {
	private:
		ChannelWriter *_cw;
		Block::Packet_descriptor _pkt;
		uint8_t *_data;
		uint32_t _offset;
		uint32_t _len;

		enum State { INTERMEDIATE, TAIL };
		State _state;

	public:
		Buffer(ChannelWriter *cw, uint32_t size);
		~Buffer();

		void write(const uint8_t *data, uint32_t len);
		void commit();
		void close();
		void complete();
		void become_tail();
		void rewrite_len(uint32_t len);

		uint32_t available() { return _len - _offset; }
	};

	CtorDbg1 d1;
	Genode::Allocator_avl _tx_block_alloc;
	CtorDbg2 d2;
	Block::Connection disk;
	CtorDbg3 d3;
	Block::Session::Tx::Source *source;
	CtorDbg4 d4;
	
	uint32_t _next_sector;
	Buffer *tail_buffer;

	// These are only valid between open() and close()
	Transaction transaction;
		// keeps track of whether the caller filled in all the bytes he
		// said he would.
	Buffer *current_buffer;
		// the intermediate buffers we're using to ferry that data out to
		// disk.
	Lock transaction_lock;
		// keep another thread from opening while a first thread has us open.

	uint32_t read_generation_number();

public:
	ChannelWriter();

	// multi-call protocol for moving big data (core file)
	void open(const char *channel_name, uint32_t data_size);
	void write(const uint8_t *data, uint32_t len);
	void close();

	// short protocol for emitting strings (debug_stderr)
	void write_one(const char *channel_name, const char *data);
};
