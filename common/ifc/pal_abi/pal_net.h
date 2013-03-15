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
/*----------------------------------------------------------------------
| PAL_NET
|
| Purpose: Process Abstraction Layer Networking
----------------------------------------------------------------------*/

#ifndef _PAL_NET_H
#define _PAL_NET_H

#include "pal_abi/pal_types.h"

/*
TODO note that all address-declaring structs are in service to
zf_zoog_get_ifconfig. This is a stopgap ABI call that's here until
we figure out the right way to assign addresses to apps. The correct
answer will probably be DHCP (or its IPv6 variant), but we haven't
gotten around to defining it yet. So this complexity sits here.
Rationale: get rid of it!
*/

// 
// 
/*----------------------------------------------------------------------
| typedef: XIP4Addr
|
| Purpose: IPv4 Addressx
|
| Remarks: The zoog equivalent of a sockaddr_in
|   It is defined here because addresses are exposed in ...get_ifconfig() abi.
----------------------------------------------------------------------*/
typedef uint32_t XIP4Addr;

/*----------------------------------------------------------------------
| Struct: XIP6Addr
|
| Purpose: IPv6 Address
|
| Remarks: The zoog equivalent of a sockaddr_in6
|   It is defined here because addresses are exposed in ...get_ifconfig() abi.
----------------------------------------------------------------------*/
typedef struct {
    uint8_t addr[16];
} XIP6Addr;


/*----------------------------------------------------------------------
| Enum: XIPVer
|
| Purpose: IP Address Version
----------------------------------------------------------------------*/
typedef enum { 
    ipv_err=-1, // Invalid
    ipv4=4,     // IPv4
    ipv6=6      // IPv6
} XIPVer;


/*----------------------------------------------------------------------
| Struct: XIPAddr
|
| Purpose: IP Address
----------------------------------------------------------------------*/
typedef struct {
    // Address version (IPv4/IPv6)
    XIPVer version;

    // Address
    union {
        XIP4Addr xv4_addr;
        XIP6Addr xv6_addr;
    } xip_addr_un;

} XIPAddr;

// parts always in network order

/*----------------------------------------------------------------------
| Struct: XIPifconfig
|
| Purpose: IP Interface Configurtion
----------------------------------------------------------------------*/
typedef struct {
    XIPAddr local_interface; // Local interface address
    XIPAddr gateway;         // Gateway address
    XIPAddr netmask;         // Subnet mask
} XIPifconfig;


/*----------------------------------------------------------------------
| Struct: ZoogNetBuffer
|
| Purpose: Variable sized buffer that allows applications to transmit or
|   receive network data
|   Each ZoogNetBuffer contains an implied variable-sized data payload
|   field following the defined fixed part. We'd declare it as uint8_t
|   data[0], except some compliers won't go along with that definition,
|   so instead it's left implied and the ZNB_DATA macro below is used to
|   access the implied field.
|
| Remarks: Size of network payload is equal to length.  Layout is:
|   /-- sizeof(ZoogNetBuffer) = 4 --\ /---------- length ------- \
|              length                  data....
|
----------------------------------------------------------------------*/
typedef struct {
    uint32_t capacity;	// payload capacity. Doesn't count this header.
} ZoogNetBuffer;


/*----------------------------------------------------------------------
| Function: ZNB_DATA
|
| Purpose: This inline function exists to reach into the implied
|   data payload part of the ZoogNetBuffer object.
|
| Parameters:
|   znb (IN) -- Network buffer
|
| Returns: Pointer to payload data in the network buffer
----------------------------------------------------------------------*/
static __inline void *ZNB_DATA(ZoogNetBuffer *znb) { return (void*) (znb+1); }


/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_get_ifconfig
|
| Purpose: Get the interface configuration object(s)
|
| Remarks: This is a temporary solution for configuring the network 
|   interface in a lightweight fashion. TODO open for negotiation
|
| Parameters:
|   ifconfig    (OUT) -- Pointer to an array of XIPifconfigs
|   inout_count (IN/OUT) -- IN: ifconfig array length, OUT: count of ifconfigs returned
|
| Returns: none
----------------------------------------------------------------------*/
typedef void (*zf_zoog_get_ifconfig)(XIPifconfig *ifconfig, int *inout_count);


/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_alloc_net_buffer
|
| Purpose: Allocate a buffer used for network transmissions
|
| Parameters:
|   payload_size (IN) -- The return value will have a capacity field of 
|       at least this size, in bytes
|
| Returns: a pointer to space that is
|       (a) visible to the monitor, so that it may
|           be passed to [send|receive]_net_buffer, and
|       (b) if cast to ZoogNetBuffer, has an ZNB_DATA() region of
|           at least payload_size bytes long. 
|           (That is, the raw buffer has size sizeof(uint32_t)+payload_size.)
|           So, if the application has a 1500-byte IP packet to send:
|           ZoogNetBuffer *znb = (zoog_alloc_net_buffer)(1500);
|           znb->length = 1500;
|           memcpy(znb->data, buffer, 1500);
|       (c) not initialized to anything in particular.
|           That is, don't expect a sane ZoogNetBuffer in there.
|           If you're sending, you'll fill it in; if you're receiving,
|           the receive_ call will fill it in.
|           (NB it won't leak information from outside your process to you;
|           however, it may give you back some garbage your process left
|           behind in a previous buffer or memory region.)
|
Rationale:
This call is only different than zoog_alloc_memory because we want
to provide a hint to the PAL that *this* memory will be used to
transmit a network packet. The host can use that hint to arrange VM
placement to facilitate faster (fewer- or zero-copy) transfers of the
packet at the send call.

In fact, we anticipate an even slightly bigger API wherein the allocate call
tells the kernel where the send call will be going; this will enable
the PAL to arrange a different allocation for packets destined for another
app on the same machine (a virtual link servicable with MMU magic)
versus packets destined for the ethernet card (perhaps serviced by providing
the app with memory-mapped access to a buffer right on the card).
----------------------------------------------------------------------*/
typedef ZoogNetBuffer *(*zf_zoog_alloc_net_buffer)(uint32_t payload_size);


/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_send_net_buffer
|
| Purpose: Sends the packet described by the ZoogNetBuffer struct
|
| Remarks: App should set length to the size of the packet to transmit
|   (not the capacity of the buffer). It should agree with the
|   IP length inside the packet.
|
| Parameters:
|   znb     (IN) -- Network buffer to send
|   release (IN) -- If true, the znb belongs to the system after the send;
|       the corresponding memory SHOULD be unmapped from the process'
|       address space. (A copying implementation may leave a readable or
|       writable region in place, but accesses after moment send "occurs",
|       between the zoogcall and return, cannot change the effect of the send.
|   
|       If false, the znb stays in the process' address space;
|       the system makes a copy (e.g. COW) of the region to send the data
|       asynchronously. 
|
| Returns:  none
|
Rationale:
  If the release flag is 'false', the app continues to own the ZNB.
  We expect this case to be used by content caches for fast app start:
  the cache simply changes the IP header to slop the same packet to
  another process that wants the same image. A PAL would implement this
  efficiently with COW, so that the only new page allocated for each
  send of a possibly-huge (100MB) packet is the one containing the IP header.

  We envision apps sharing signficant but not complete process images;
  to facilitate that in a zero-copy way, we may need to expand this
  interface to have a 'gather' interface. If the gathered pieces were
  on page-aligned boundaries, then the host could still play MMU COW games
  to implement the copy-by-value sematics with only MMU references.
  TODO we'd really need to decide on how to expose page granularity,
  at that point.
----------------------------------------------------------------------*/
typedef void (*zf_zoog_send_net_buffer)(ZoogNetBuffer *znb, bool release);

/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_receive_net_buffer
|
| Purpose: Receive a network buffer
|
| Remarks: Application may use it, then call free_net_buffer when
|   done. If the app wishes, it may modify the contents (eg header) and
|   call send() to send out the buffer to a new destination.
|
| This call does not block. If no network buffer is available, it
| returns NULL.
|
| Parameters: none
|
| Returns: Pointer to the received network buffer
----------------------------------------------------------------------*/
typedef ZoogNetBuffer *(*zf_zoog_receive_net_buffer)();


/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_free_net_buffer
|
| Purpose: Deallocate a network buffer
|
| Remarks: When you're done with the buffer you got from alloc_net_buffer
|   or zoog_receive_net_buffer, give it back here.
|
| Parameters: 
|   znb (IN) -- Network buffer to deallocate
|
| Returns: none
----------------------------------------------------------------------*/
typedef void (*zf_zoog_free_net_buffer)(ZoogNetBuffer *znb);

#endif //_PAL_NET_H
