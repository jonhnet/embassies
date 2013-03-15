/* Copyright (c) Microsoft Corporation                                       */
/*                                                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may   */
/* not use this file except in compliance with the License.  You may obtain  */
/* a copy of the License at http://www.apache.org/licenses/LICENSE-2.0       */
/*                                                                           */
/* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS     */
/* OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION      */
/* ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR   */
/* PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.                              */
/*                                                                           */
/* See the Apache Version 2.0 License for specific language governing        */
/* permissions and limitations under the License.                            */
/*                                                                           */
#pragma once

#include "xpbpool.h"

class PacketIfc {
public:
	enum OriginType { FROM_APP, FROM_TUNNEL };

	PacketIfc(OriginType origin_type, const char *dbg_purpose);
	virtual ~PacketIfc() {}
	virtual uint32_t len() = 0;
	virtual char *data() = 0;
	virtual PacketIfc *copy(const char *dbg_purpose) = 0;
	OriginType get_origin_type() { return origin_type; }

	const char *dbg_purpose() { return _dbg_purpose; }
	void dbg_set_purpose(const char *dbg_purpose)
		{ _dbg_purpose = dbg_purpose; }

private:
	OriginType origin_type;
	const char *_dbg_purpose;
};

class SmallPacket : public PacketIfc {
public:
	SmallPacket(const char *data, uint32_t len, OriginType origin_type, const char *dbg_purpose);
	virtual ~SmallPacket();
	virtual uint32_t len();
	virtual char *data();
	virtual PacketIfc *copy(const char *dbg_purpose);

private:
	char *_data;
	uint32_t _len;
};

class XPBPacket : public PacketIfc {
public:
	XPBPacket(XPBPoolElt *xpb, OriginType origin_type);
	virtual ~XPBPacket();
	virtual uint32_t len();
	virtual char *data();
	virtual PacketIfc *copy(const char *dbg_purpose);

private:
	XPBPoolElt *_xpbe;
};

