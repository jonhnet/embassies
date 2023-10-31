# Embassies: Rich Web Applications with Strong Isolation

Embassies was a research project that "radically refactored the web":
applications had much greater isolation, relying on only a minimal isolation
abstraction, and yet they could also be much richer than Web 2.0 JavaScript
apps, employing nearly any legacy library.

[The 10-Kilobyte Web Browser](https://www.usenix.org/system/files/login/articles/02_howell-online.pdf) provides a concise introduction and motivation.

## Deeper reading

[Embassies: Radically Refactoring the Web](https://www.usenix.org/conference/nsdi13/technical-sessions/presentation/howell)
is the best-paper award-winning paper and its associated talk.

[How to Run POSIX Apps in a Minimal Picoprocess](https://www.usenix.org/conference/atc13/technical-sessions/presentation/howell)
describes the tricks and techniques we used to persuade legacy application and
library software stacks to run in minimal containers.

[Missive: Fast Application Launch From an Untrusted Buffer Cache](https://www.usenix.org/conference/atc14/technical-sessions/presentation/howell)
explains the performance story: how we can deliver isolated applications,
each with its huge set of library dependencies, while keeping interaction
snappy.

[Leveraging Legacy Code to Deploy Desktop Applications on the Web](http://www.usenix.org/conference/osdi-08/leveraging-legacy-code-deploy-desktop-applications-web) is the paper that launched this line of research.
It validated the basic hypothesis on programs with modest dependencies,
but couldn't yet show how one might replace the entire web browser.

[Rethinking the Library OS from the Top Down](https://www.microsoft.com/en-us/research/publication/rethinking-the-library-os-from-the-top-down/) [(ACM paywall version)](https://dl.acm.org/doi/10.1145/1950365.1950399)
The Drawbridge project was a fork of Xax applied to Windows guest applications.
This paper won the [ASPLOS Influential Paper Award in 2022](https://www.sigarch.org/benefit/awards/acm-sigarch-sigplan-sigops-asplos-influential-paper-award/).

# Using the demo image

The code for embassies in this image was built against a 32-bit build of
Debian GNU/Linux 6.0.10 (squeeze), a 2014-era release. Much has changed
since then. The easiest way to see the pieces working in action is to
launch a VM-packaged demo that brings all of its dependencies with it.

[Demo Instructions](demo-instructions.md)

# Other navigational aids

[build-demo.md](build-demo.md) records the steps I took to build the demo
VM from the Debian distribution and sources in this repo.

[apologies.md](apologies.md) mentions some practical mechanics of how the demos
are wired together that would be useful to someone trying to port these demos
into the modern world.
