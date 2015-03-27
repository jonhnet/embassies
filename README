Here are a couple bits of deep magic embedded in the current prototype
code, which I'm explaining to head off your having to discover them
unpleasantly yourself.

(1) In various places, we bake ZOOG_ROOT, the path to the root of your
zoog source tree, into binaries. We do this so that two people can build
and test on the same machine, and because often there's no clean way
to parameterize this stuff through the Zoog process-start interface.
(We don't pass in parameters or environment or anything.)

It's conceivable that one could improve this situation by adding a debug
zoogcall that means the only place this information gets passed is into
the (debug) monitor itself. In any case, I want to be sure this dev expedient
doesn't mislead someone into mis-structure app code to expect a dependency
on *client-side* state.


(2) Various chunks of code use ~/.zoog_tunid, small integer, to rendezvous.
This number is used to assign your client a unique IP range, a unique 'tunN'
kernel network interface, and (less importantly) for app pals to rendezvous
with their coordinator via a named pipe in /tmp. Again, this is because
two users might want to use the same machine.

This could be improved by
(a) having the coordinator allocate a tunnel dynamically every time you
start it,
(b) having the coordinator name the IP range after the allocated tunnel,
(c) having the coordinator communicate the IP address to the monitor/pal
(that may already be working correctly), and
(d) having zftp_backend (the developer's simulated origin server) coordinate
via a well-known user-specific name with the coordinator to discover which
gateway IP address it should listen on.
