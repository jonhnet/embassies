/*
 * \brief  Zoog Monitor -- trusted components of Zoog ABI
 * \author Jon Howell
 * \date   2011-12-22
 */

#pragma once

#include <util/list.h>
#include <util/string.h>
#include <base/env.h>
#include "pal_abi/pal_types.h"

// #include <base/printf.h>
// #include <base/sleep.h>
// #include <cap_session/connection.h>
// #include <root/component.h>
// #include <base/rpc_server.h>
// #include <rom_session/connection.h>
// #include <base/process.h>
// #include <ram_session/connection.h>
// #include <cpu_session/connection.h>
// #include <cap_session/connection.h>
// 
// #include <nic_session/connection.h>
// 
// #include <zoog_monitor_session/zoog_monitor_session.h>
// 
// #include "standard_malloc_factory.h"
// #include "xax_network_utils.h"
// #include "hash_table.h"
// #include "AllocatedBuffer.h"
// #include "DebugFlags.h"
// #include "common/SyncFactory_Genode.h"
// #include "ZKeyPair.h"
// #include "zoog_network_order.h"



extern char _binary_zoog_boot_signed_start;
extern char _binary_zoog_boot_signed_end;

namespace ZoogMonitor {
	class BootBlock
		: public Genode::List<BootBlock>::Element
	{
	private:
		uint8_t* _storage;
		uint32_t _len;
		uint32_t _cert_len;
		uint32_t _binary_len;
	public:
		BootBlock(SignedBinary* signed_binary_in, uint32_t capacity);
		~BootBlock();
		void* binary_start() { return _storage+sizeof(SignedBinary)+_cert_len; }
		uint32_t binary_len() { return _binary_len; }
		ZCert* copy_cert();

		static BootBlock* get_initial_boot_block();
	};
}
