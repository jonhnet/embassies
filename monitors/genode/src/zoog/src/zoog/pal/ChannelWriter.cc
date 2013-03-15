#include "ChannelWriter.h"
#include "LiteLib.h"

ChannelWriter::Buffer::Buffer(ChannelWriter *cw, uint32_t size)
{
	this->_cw = cw;
	uint32_t num_sectors = size / SECTOR_SIZE;
	lite_assert(num_sectors * SECTOR_SIZE == size);

	_pkt = Block::Packet_descriptor(_cw->source->alloc_packet(size),
		Block::Packet_descriptor::WRITE,
		cw->_next_sector,
		num_sectors);
/*
	PDBG("allocating pkt at sector %d count %d (%d, %d)",
		cw->_next_sector,
		num_sectors,
		_pkt.block_number(),
		_pkt.block_count());
*/
	_cw->_next_sector += num_sectors;
	_data = (uint8_t*) _cw->source->packet_content(_pkt);
	_len = size;
	_offset = 0;
	_state = INTERMEDIATE;
}

ChannelWriter::Buffer::~Buffer()
{
	_cw->source->release_packet(_pkt);
}

void ChannelWriter::Buffer::write(const uint8_t *data, uint32_t len)
{
	lite_assert(_offset + len <= _len);
//	PDBG("copying %d bytes from 0x%08x to 0x%0x; _offset 0x%08x _len 0x%08x", len, (uint32_t) data, (uint32_t)_data+_offset, _offset, _len);
	lite_memcpy(_data+_offset, data, len);
	_offset += len;
}

void ChannelWriter::Buffer::commit()
{
/*
	PDBG("submitting packet at sector %d nsectors %d length %d",
		_pkt.block_number(), _pkt.block_count(), _len);
*/
	_cw->source->submit_packet(_pkt);
	_pkt = _cw->source->get_acked_packet();
	if (!_pkt.succeeded())
	{
		PERR("Disk write failed");
	}
}

void ChannelWriter::Buffer::close()
{
//	PDBG("_offset %d _len %d\n", _offset, _len);
	lite_assert(_offset == _len);
	commit();
	delete this;
}

void ChannelWriter::Buffer::complete()
{
	if (_state==INTERMEDIATE)
	{
		close();
	}
}

void ChannelWriter::Buffer::become_tail()
{
	_state = TAIL;
}

void ChannelWriter::Buffer::rewrite_len(uint32_t len)
{
	// invariant: outside an open()/close() transaction,
	// tail_buffer->_offset >= sizeof(uint32_t)...
	// just enough space for the zero we'll need to back over and rewrite.
	lite_assert(_offset >= sizeof(uint32_t));
	uint32_t *ptr = (uint32_t*) (_data+_offset - sizeof(uint32_t));
/*
	PDBG("Rewriting record at offset %d, old val %d",
		_offset-sizeof(uint32_t), ptr[0]);
*/
	lite_assert(ptr[0]==0);	// expected to overwrite previous EOF
	ptr[0] = len;
}

//////////////////////////////////////////////////////////////////////////////

ChannelWriter::ChannelWriter()
	: _tx_block_alloc(env()->heap()),
	  disk(&_tx_block_alloc),
	  source(disk.tx()),
	  _next_sector(0),
	  tail_buffer(new Buffer(this, SECTOR_SIZE)),
	  current_buffer(NULL)
{
	uint32_t generation_number = read_generation_number();
	generation_number += 1;
//	PDBG("generation_number 0x%08x", generation_number);
	tail_buffer->write((uint8_t*) &generation_number, sizeof(generation_number));
	uint32_t zero = 0;
//	PDBG("initial zero");
	tail_buffer->write((uint8_t*) &zero, sizeof(zero));
	tail_buffer->become_tail();
	tail_buffer->commit();
}

uint32_t ChannelWriter::read_generation_number()
{
	Block::Packet_descriptor rpkt;
	rpkt = Block::Packet_descriptor(source->alloc_packet(SECTOR_SIZE),
		Block::Packet_descriptor::READ,
		0,
		1);
	source->submit_packet(rpkt);
	rpkt = source->get_acked_packet();
	uint32_t generation_number = 0;
	if (!rpkt.succeeded())
	{
		PERR("Disk read failed");
	}
	else
	{
		generation_number = ((uint32_t*) source->packet_content(rpkt))[0];
		source->release_packet(rpkt);
	}
	return generation_number;
}

void ChannelWriter::open(const char *channel_name, uint32_t data_size)
{
//	PDBG("locking lock in this %p", this);
	transaction_lock.lock();

	uint32_t channel_name_len = lite_strlen(channel_name);
	uint32_t preamble_len = sizeof(uint32_t)*2 + channel_name_len;
	uint32_t payload_len = preamble_len+data_size;
	uint32_t rounded_payload_len =
		((payload_len-1)&~(ALIGN_CONSTRAINT-1))+ALIGN_CONSTRAINT;
/*
	PDBG("payload_len %08x rounded_payload_len %08x",
		payload_len, rounded_payload_len);
*/
	transaction.payload_offset = 0;
		// measures everything *except* the record_len field, since
		// we don't write that until the end.
	transaction.pad_len = rounded_payload_len - payload_len;
	transaction.payload_len = rounded_payload_len;

	uint32_t record_len = sizeof(uint32_t) + transaction.payload_len;
	tail_buffer->rewrite_len(record_len);

	lite_assert(current_buffer == NULL);
	current_buffer = tail_buffer;
	
//	PDBG("channel_name_len");
	write((uint8_t*) &channel_name_len, sizeof(channel_name_len));
//	PDBG("data_size");
	write((uint8_t*) &data_size, sizeof(data_size));
//	PDBG("channel_name (%d)", channel_name_len);
	write((uint8_t*) channel_name, channel_name_len);
	lite_assert(transaction.payload_offset == preamble_len);
}

void ChannelWriter::write(const uint8_t *data, uint32_t len)
{
//	PDBG("len %d", len);
	uint32_t offset = 0;
	while (offset < len)
	{
		if (current_buffer->available()==0)
		{
			// write intermediate buffers; but hold the prior tail_buffer
			current_buffer->complete();
			// allocate buffers that are "only big enough", because
			// we don't want to get stuck holding a huge, partially-full
			// one at the end, having to rewrite it repeatedly.
			uint32_t buffer_size =
				transaction.remaining_len() > BULK_BUFFER_SIZE
				? BULK_BUFFER_SIZE
				: SECTOR_SIZE;
			current_buffer = new Buffer(this, buffer_size);
		}

		uint32_t request_remaining = len-offset;
		uint32_t this_write =
			min(current_buffer->available(), request_remaining);
//		PDBG("offset %d this_write %d bytes of %d remaining %d", offset, this_write, len, request_remaining);
		current_buffer->write(data+offset, this_write);

		offset += this_write;
	}
	transaction.payload_offset += len;
}

void ChannelWriter::close()
{
	uint32_t zero = 0;
//	PDBG("pad");
	write((uint8_t*) &zero, transaction.pad_len);
	lite_assert(transaction.remaining_len()	== 0);

//	PDBG("EOF");
	write((uint8_t*) &zero, sizeof(uint32_t));

	if (tail_buffer == current_buffer)
	{
		// haven't actually advanced the buffer; just rewrite it
		// and keep it.
//		PDBG("close: rewrite existing tail");
		tail_buffer->commit();
	}
	else
	{
		// have advanced the buffer; close out old one and use new
		// as next tail buffer.
//		PDBG("close: commit old tail, advance to new");
		current_buffer->commit();	// be sure all new data is flushed to EOF
		current_buffer->become_tail();
		tail_buffer->close();
		tail_buffer = current_buffer;
	}
	current_buffer = NULL;

	transaction_lock.unlock();
}

void ChannelWriter::write_one(const char *channel_name, const char *data)
{
	uint32_t data_len = lite_strlen(data);
	open(channel_name, data_len);
//	PDBG("Here comes a write with data_len %d (data %s)\n", data_len, data);
	write((uint8_t*) data, data_len);
	close();
}
