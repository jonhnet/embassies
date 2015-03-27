#pragma once

#include "pal_abi/pal_types.h"
#include "zoog_monitor_session/client.h"

class GenodePAL;

namespace ZoogMonitor {

class ChannelWriterClient
{
private:
	GenodePAL* pal;
	ZoogNetBuffer* znb;
	ZoogMonitor::Session::Zoog_genode_net_buffer_id buffer_id;

	void write_one(const uint8_t *data, uint32_t sub_len);

public:
	ChannelWriterClient(
		GenodePAL* pal,
		ZoogNetBuffer* znb,
		Session::Zoog_genode_net_buffer_id buffer_id);

	// multi-call protocol for moving big data (core file)
	void open(const char *channel_name, uint32_t data_size);
	void write(const uint8_t *data, uint32_t len);
	void close();
	void write_one(const char *channel_name, const char *data);
};

}
