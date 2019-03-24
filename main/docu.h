/*!
    \file
    \brief GITTA Tschenggins Lämpli: system status (see \ref FF)

    - Copyright (c) 2019 Philippe Kehl & flipflip industries (flipflip at oinkzwurgl dot org),
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli

*/
#ifndef __DOCU_H__
#define __DOCU_H__
/*!

\mainpage
\anchor mainpage
\tableofcontents

\section P_INTRO Introduction

See https://oinkzwurgl.org/projeaggd/tschenggins-laempli for the project documentation.

See [source code](files.html) for copyrights, credits, sources and more.

\section P_MODULES Modules

- \ref FF_BACKEND
- \ref FF_CFG
- \ref FF_DEBUG
- \ref FF_HSV2RGB
- \ref FF_JENKINS
- \ref FF_JSON
- \ref FF_LEDS
- \ref FF_MON
- \ref FF_STATUS
- \ref FF_STDINC
- \ref FF_STUFF
- \ref FF_TONE
- \ref FF_WIFI

\section P_ARCH Architecture

\verbatim
+---------------------------------+               +---------------------------------+                    +----------------------------+
|         *** wifi.c ***          |               |       *** jenkins.c ***         |                    |     *** console.c ***      |
+---------------------------------+               +---------------------------------+          .---------| serial console task,       |
| wifi task, maintains network    |               | jenkins task, maintains jobs    |          |         | user commands and config   |
| connection to backend server,   |               | states and results, updates     |          |         +----------------------------+
| passes received data to         |               | LEDs and triggers tones,        |          |
| backend handler                 |               | melodies and effects            |          |
|                                 |               |                                 |          |
|                                 |               |                                 |          |         +----------------------------+
+---------------------------------+               +---------------------------------+          |         |       *** mon.c ***        |
               |  ^                                                  ^    |                    |         | system monitor task,       |
               |  |                                                  |    |                    |         | debug on serial output     |
               |  |                                                  |    |                    |         +----------------------------+
               |  |                                                  |    |                    |
               |  |       +---------------------------------+        |    |                    |
               |  |       |       *** backend.c ***         |        |    |                    |
               |  |       +---------------------------------+        |    |                    |         +----------------------------+
               |  |       | handles received backend data,  |        |    |                    |         |      *** leds.c ***        |
               |  '-------| triggers jenkins state and      |        |    +----------------------------->| RGB LEDs control and       |
               |          | result updates, re-connects,    |--------'    |                    |         | effects task               |
               '--------->| configuration changes, etc.     |             |                    |         +----------------------------+
                          |                                 |             |                    |
                          |                                 |             |                    |
                          |                                 |             |                    |
                          +---------------------------------+             |                    |         +----------------------------+
                                                                          +----------------------------->|         status.h           |
                                                                          |                    |         | status LED, tones and fx   |
                                                                          |                    |         +----------------------------+
                                                                          |                    |
                                                                          |                    |         +----------------------------+
                                                                          '----------------------------->|         tone.h             |
                                                                                               |         | tones and melodies         |
                                                                                               |         +----------------------------+
                                                                                               +----.
                                                                                               |    '----------.
                                                                                              \|/             \|/
+----------------------------+     +----------------------------+     +----------------------------+     +----------------------------+
|          stuff.h           |     |         debug.h            |     |            env.h           |     |           cfg.h            |
|    utilities, helpers      |     |      debug helpers         |     |        NVS env vars        |     |       configuration        |
+----------------------------+     +----------------------------+     +----------------------------+     +----------------------------+

. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

                               +-------------+  +-------------+  +-------------+  +-------------+
                               | libc        |  | freertos    |  | esp-idf     |  | components  |
                               +-------------+  +-------------+  +-------------+  +-------------+
                                      <----------- stdinc.h ------------>
\endverbatim

See also
- #WIFI_STATE_t

*/
#endif // __DOCU_H__
//@}
// eof
