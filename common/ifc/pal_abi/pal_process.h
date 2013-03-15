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
| PAL_PROCESS
|
| Purpose: Process-management abstractions of platform abstraction layer
----------------------------------------------------------------------*/

#pragma once

#include "pal_abi/pal_types.h"

/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_lookup_extension
|
| Purpose: A hook for monitors to provide some performance-enhancing
|   extension.
|
| Remarks: We really hope not to use this much, going with the
|   "small code grown rarely" philosophy.
|   We presently use it only for connecting to debug features in
|   a debug-mode PAL.
|
| Parameters:
|     name (IN) -- Extension name
|
| Returns: Function pointer to extension function
----------------------------------------------------------------------*/
typedef void *(*zf_zoog_lookup_extension)(const char *name);


/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_allocate_memory
|
| Purpose: Allocate a block of /length/ bytes of memory. Memory should be
|   zero-filled before returning.
|
| Parameters:
|   length (IN) -- Size of the memory block, in bytes.
|
| Returns: On success, a pointer to the memory block allocated.
|   On failure, the NULL pointer.
|
| The allocated memory shall not leak any data from other applications.
| The simplest approach is to always supply zeroed memory.
| (NB present app code asserts that the memory is actually zeroed, but
| maybe we wish to weaken that requirement.)
|
| The allocated memory will be page-aligned and the length page-rounded,
| to 4K pages (0x1000).
|
| Rationale: that page size seems to be pretty stable, but is it?
| It's okay for a platform to return memory that's more-aligned, if its
| page table hardware does better that way. Is there a case where the
| app requires more-aligned memory? (There are GCs that work this way,
| using a big aligned region to infer the beginning from any address;
| they work by over-allocating and returning the part they don't want.
| Hence our free-partial semantics.)
| So I think it's okay to declare 4K pages as The Final Value For All Time. 
| 
| Rationale: The only way applications get memory is with this call;
| apps (libraries) acquire stacks, heaps, and map executables all through
| this interface, which, NB, doesn't give the app the opportunity to
| place the acquired memory in its memory map.
| We chose to give the host *complete* control of its memory map because
| that choice facilitates our goal of host generality.
| In particular, our experiences with Xax taught us that various hosts
| put various arbitrary restrictions on their memory layouts. Each time
| we introduced a new toolchain or new host, there would be a new virtual
| address space conflict to resolve. Ultimately, there was the threat that
| some established zoog applications could come to insist on a particular
| memory layout that was incompatible with some desirable new host;
| there was no clean compromise to resolve potential conflicts. 
| 
| One alternative is to push all the responsibility onto the host, forcing
| it to enable the guest to have complete memory map freedom. This approach
| is basically infeasible; every modern OS statically maps some chunks of
| virtual address space before the app has a chance to disagree.
| 
| So JL argued for the alternative: the cleanest resolution is to push
| all the responsibility for memory layout onto the application. This gives
| hosts complete freedom in memory layout -- put differently, it means that
| any app obeying this constraint can be made to run on any host, regardless
| of arbitrary VM layout constraints established by the host environment.
| The cost is a burden on the toolchain. In the case of posix/linux, we
| have to rebuild top-level app binaries with the -pie flag.
| Another example is that some linux garbage collection libraries (libgc,
| and another) try heuristics to learn the stack bounds, and sanity-check
| those heuristics by looking for "impossibly low" addresses -- addresses
| never assigned to stacks in Linux, apparently, but which zoog makes no
| promise of not assigning.
| The burden's a little greater with the Windows toolchain, where the kernel
| and applications share a compiled-in agreement about a particular shared
| page, but it can be addressed locally, in the NT compatibility layer.
| 
----------------------------------------------------------------------*/
typedef void *(*zf_zoog_allocate_memory)(size_t length);


/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_free_memory
|
| Purpose: Deallocate a block of memory. Cannot fail since already-freed 
|   ranges are ignored.
|
|	Monitor is responsible to handle partial frees (where base/length
|	don't line up with the allocate call that created the region).
|	(Ideally this is accomplished by partially freeing immediately,
|	but I suppose a monitor could track partial frees and free the underlying
|	storage once the last page of the region is finally freed.)
|
|   TODO: if start and start+length don't fall on 'page boundaries',
|	the monitor may round them to boundaries to ensure all the mentioned
|	memory gets freed. That may free more than expected.
|	But there's no way yet for the app to tell what the page-size
|	granularity is, so it can't tell when this can occur.
|
| Parameters:
|   base   (IN) -- Pointer to a memory block.
|   length (IN) -- Size of the memory block, in bytes.
|
| Returns: (none)
----------------------------------------------------------------------*/
typedef void (*zf_zoog_free_memory)(void *base, size_t length);


/*----------------------------------------------------------------------
| Function Signature: zoog_thread_start_f
|
| Purpose: Zoog thread entry-point
|
| Remark: The thread signiture is not enforced (see zf_zoog_thread_create)
|   Since thread creation is managed by the zoog application and not the
|   underlying operating system
|
| Parameters: (none)
|
| Returns: (none)
----------------------------------------------------------------------*/
typedef void (zoog_thread_start_f)(void);


/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_thread_create
|
| Purpose: Create and start a new thread in the same process as the caller.
|
| Remarks: Sets the EIP to func and ESP to stack and runs.
|
| Parameters:
|   func  (IN) -- initial value of program counter in new thread
|                 TODO change to void* instead of zoog_thread_start_f since 
|                 there is no calling convention
|   stack (IN) -- initial value of stack pointer in new thread
|
| Note the very Spartan semantics: this function does not allocate
| a stack, or synchronize the new thread with the caller, or pass
| any parameters. The func and stack values are unexamined and
| uninterpreted; they're simply loaded into the processor registers and
| the thread started. They can point at unallocated memory for all
| the host monitor cares.
|
| It's the caller's job to allocate a suitable stack,
| pass parameters and initial register values through that stack,
| and do any required synchronization.
|
| Returns: true for successful create; false otherwise.
----------------------------------------------------------------------*/
typedef bool (*zf_zoog_thread_create)(zoog_thread_start_f *func, void *stack);


/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_thread_exit
|
| Purpose: Terminate the current thread.
|  Does not reclaim current thread's stack or any resources other than
|  the thread scheduling context.
|
| Remarks: The only resource freed is the thread itself. The stack and
|   other resources are not.
|
| Parameters: (none)
|
| Returns: Does not return.
----------------------------------------------------------------------*/
typedef void (*zf_zoog_thread_exit)(void);


/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_exit
|
| Purpose: Terminate the current processes, terminating all associated
|   threads, freeing all memory, and (future) releasing any other resources
|   such disconnecting from any open display canvases.
|
| Parameters: (none)
|
| Returns: Does not return.
----------------------------------------------------------------------*/
typedef void (*zf_zoog_exit)(void);


/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_x86_set_segments
|
| Purpose: Set the segment register values
|
| Remarks: x86 has a paltry register file.
|   Since zoog assumes all memory protection comes from paging,
|   the segment registers have no security function.
|   The same assumption is used in windows and linux libraries to
|   recycle one (or more?) auxilliary segment register base value as a
|   thread-local storage pointer -- it's expensive to write (incurring
|   a ring-0 crossing), but rarely written.
|   So, to facilitate porting, zoog's x86 ABI includes the needed
|   call to write the segment base addresses.
|
|  Rationale: The presence of this function (only in the x86 instantiation
|  of the PAL ifc) is entirely in service to "Application portability."
|  The TCB cost is very low: We're providing access to a CPU feature which,
|  due to how all modern and plausible host OSes configure the CPU, has
|  no security impact.
|  The application benefit is enormous: by including this ABI, we enable
|  a bunch of code to run as raw binaries, without even requiring a
|  rebuild from source. In the case of the posix/linux toolchain,
|  everything except libc (on the bottom; requires syscall entry points
|  to be rewritten) and the app executable itself (on the top; must be
|  compiled with -pie flag) can run without recompilation.
|  We believe this tradeoff is a good one for facilitating the introduction
|  of new DPIs.
|
| Parameters: 
|   fs (IN) -- FS segment register value
|   gs (IN) -- GS segment register value
|
| Returns: none
----------------------------------------------------------------------*/
typedef void (*zf_zoog_x86_set_segments)(uint32_t fs, uint32_t gs);

/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_launch_application
|
| Purpose:  Launches an application.
|
| Parameters: 
|   image         (IN) -- Points to a position-independent 
|       (or self-relocating) block of code that will boot the new
|       application. It will begin execution with eip pointing at image 
|       and ebp pointing at a XaxDispatchTable. It is responsible to 
|       allocate any stack and thread contexts it needs.
|
|   image_len     (IN) -- Length of image, in bytes
|
|   signature     (IN) -- [will eventually] refer to a signature over the boot
|       block which will allow the monitor to assign responsibility for
|       the new process to a principal identifier (key).
|       For now, it's unimplemented and must be NULL.
|
|   signature_len (IN) -- Length of signature, in bytes
|
| Returns: none

TODO
There are three reasons code might want to make this call:
- It wants another process owned by the same vendor, which the vendor
may want to exit, fail, or otherwise account resources separately from
the first.  Clearly, a vendor can specify its own boot block in this
API to achieve this goal.
- It wants a 'sandbox' process in which to run untrusted code not
associated with any origin. A vendor could achieve this by rolling a new
process identity and signing a boot block with it, but we might want
to optimize those semantics down to something fast but equivalent.
- It wants to communicate with a different vendor, and needs to bootstrap
that vendor. This call is designed around this third case.

Rationale:
This call represents a minimal TCB primitive to enable one vendor to
launch another. Onto the app making the call, it pushes all the
responsibility for
	- resolving a name into vendor identity, and
	- fetching a bootblock for the launched vendor's app;
Onto the launched vendor, it pushesall the responsibility for 
	- bootstrapping the rest of a process,
	- doing so despite only having untrusted help from outside (hence
	the boot block needs to embed hashes or public keys to provide
	integrity checks), and
	- accepting further "parameters" from the app initiating the launch.

Reiterating, observe that there's no way to pass a parameter from
the calling app to the launched app. Pedagogically, we imagine every vendor
as being "in the cloud", "always on". So if app A wants to get app B to
do something (say, start talking to the user sitting at this screen), in
general A should simply find a way to get a message to B telling B what
it ought to do. That message-routing problem is left to the apps; zoog only
provides the network packet primitive. Now, if in practice B happens to
not be executing on the local machine, and A & B agree that it ought to
(to facilitate a fluid or offline UI experience -- that's all Zoog
ultimately does!) then B needs a way to get running on the Zoog host.
That means app A must provide some runtime resources to B, and then
the kernel must begin running an executable that B wants run, using
those resources. (Resource provisioning has not been specified yet,
however.)

This call only addresses the very last step: how the kernel
learns what executable B wants run. Rather that burden the TCB with
resolving names, doing HTTP lookups, and resolving MIME types, it's
A's job to find the boot block. The only thing the TCB does, behind this
call, is the minimal thing: verify that the boot block really does
represent B's wishes. We can circumscribe this specification very tightly,
needing only to incorporate the desired signature crypto tools.
----------------------------------------------------------------------*/
typedef void (*zf_zoog_launch_application)(
	SignedBinary *signed_binary);

