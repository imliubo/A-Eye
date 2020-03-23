/**
Copyright (c) 2018 Gombe.

This software is released under the MIT License.
http://opensource.org/licenses/mit-license.php
*/

#ifndef _3DCONFIG
#define _3DCONFIG

const static int window_width = 240;
const static int window_height = 135;

//#define DISABLE_ANIMATION
//#define DISABLE_OUTPUT

#define DRAW_NLINES (window_height)
#define MAXPROC_POLYNUM (300)

#define USE_K210

#define OMIT_ZBUFFER_CONFLICT
#define PROCESSNUM 2

#ifdef OUTPUTTERMINAL
#undef ENDIAN_LITTLE
#endif
#endif
