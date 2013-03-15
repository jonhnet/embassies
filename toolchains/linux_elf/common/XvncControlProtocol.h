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

#include "pal_abi/pal_ui.h"

#define XVNC_CONTROL_SOCKET_PATH "/tmp/xvnc_control_socket"

enum XCOpcode {
	xcChangeViewport,			// one-way to xvnc
	xcUnmapCanvas,				// one-way to xvnc	
	xcReadViewportRequest,		// RPC request
	xcReadViewportReply,		// RPC reply
};

struct XCHeader {
	XCOpcode opcode;
};

struct XCChangeViewport : public XCHeader {
	static const XCOpcode OPCODE = xcChangeViewport;
	ViewportID viewport_id;
};

struct XCUnmapCanvas : public XCHeader {
	static const XCOpcode OPCODE = xcUnmapCanvas;
};

struct XCReadViewportRequest : public XCHeader {
	static const XCOpcode OPCODE = xcReadViewportRequest;
};

struct XCReadViewportReply : public XCHeader {
	static const XCOpcode OPCODE = xcReadViewportReply;
	ViewportID viewport_id;
};

class XCClient {
private:
	int pipesock;
	bool used;
	void use();

public:
	XCClient();
	~XCClient();

	void change_viewport(ViewportID viewport_id);
	void unmap_canvas();
	ViewportID read_viewport();
};
