#include "ChannelWriterClient.h"
#include "LiteLib.h"
#include "Genode_pal.h"

namespace ZoogMonitor {

ChannelWriterClient::ChannelWriterClient(
	GenodePAL* pal,
	ZoogNetBuffer* znb,
	Session::Zoog_genode_net_buffer_id buffer_id)
	: pal(pal),
	  znb(znb),
	  buffer_id(buffer_id)
{
}

void ChannelWriterClient::open(const char *channel_name, uint32_t data_size)
{
	uint32_t name_size = lite_strlen(channel_name)+1;
		// need that '\0' terminator!
	Dispatcher d(pal, ZoogMonitor::Session::do_debug_write_channel_open);
	d.d()->un.debug_write_channel_open.in.buffer_id = buffer_id;
	d.d()->un.debug_write_channel_open.in.channel_name_size = name_size;
	d.d()->un.debug_write_channel_open.in.data_size = data_size;
	lite_assert(name_size <= znb->capacity);
	memcpy(ZNB_DATA(znb), channel_name, name_size);
	d.rpc();
}

void ChannelWriterClient::write(const uint8_t *data, uint32_t len)
{
	uint32_t offset;
	for (offset = 0; offset < len; offset+=znb->capacity)
	{
		uint32_t sub_len = min(len-offset, znb->capacity);
		write_one(data+offset, sub_len);
	}
}

void ChannelWriterClient::write_one(const uint8_t *data, uint32_t len)
{
	lite_assert(len <= znb->capacity);
	PDBG("ChannelWriterClient::write preps %x-byte write\n", len);
	Dispatcher d(pal, ZoogMonitor::Session::do_debug_write_channel_write);
	d.d()->un.debug_write_channel_write.in.buffer_id = buffer_id;
	d.d()->un.debug_write_channel_write.in.buffer_size = len;
	memcpy(ZNB_DATA(znb), data, len);
	PDBG("ChannelWriterClient::write posts %x-byte write\n", len);
	d.rpc();
	PDBG("ChannelWriterClient::write completes %x-byte write\n", len);
}

void ChannelWriterClient::close()
{
	Dispatcher d(pal, ZoogMonitor::Session::do_debug_write_channel_close);
	d.rpc();
}

void ChannelWriterClient::write_one(const char *channel_name, const char *data)
{
	uint32_t data_len = lite_strlen(data);
	open(channel_name, data_len);
	write((uint8_t*) data, data_len);
	close();
}

}
