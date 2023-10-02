
# OpenEXRCore

This directory includes the sources for the OpenEXRCore 3.2.0 library, and a 
unity build file, openexr-c.c that compiles the sources into the Hio library.
The sources are concatenated into a unity build in order that all symbols are
available within a single compilation unit. All functions are adorned with 
either an EXR_EXPORT macro, or an EXR_INTERNAL macro, both of which are set to
static here to prevent symbols from leaking out of Hio. In this way, the 
interred OpenEXRCore library will not conflict with any user-preferred 
OpenEXR libraries that may be on the system and otherwise linking to an
application or plugin.

A few changes are still in progress to upstreamed to the OpenEXR project, but
these are minor, and otherwise, almost all differences between the interred 
OpenEXRCore and the official OpenEXR repo are consolidated to the
openexr_conf.h header. The remaining differences are removing an extern marking
from the few global data tables.

With regards to the maintainability of the unity concatenation, the OpenEXR team
has indicated a desire to have the concatenated build available as part of the
standard code distribution. As such, any maintenance with respect to that file
will be handled by CI on the EXR project itself. A unit test will be landed
upstream to ensure this is reliably in place, however the exact approach is 
still in discussion.
