#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEAF_CONFIG_MAX_STACK 1024
#define LEAF_IMPLEMENTATION
#include "leaf/leaf.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb/stb_truetype.h"

#include "clap/clap.h"

#include "upl.h"
#include "upl_internal.h"
#include "upl_default_font.h"

#if defined(_WIN32)
	#include "renderer/upl_renderer_d2d.c"
	#include "platform/upl_platform_win32.c"
#elif defined(__APPLE__)
	#include "renderer/upl_renderer_cg.c"
	#include "platform/upl_platform_cocoa.m"
#else
	#include "renderer/upl_renderer_gl.c"
	#include "platform/upl_platform_x11.c"
#endif

#include "upl_renderer.c"
#include "upl_gui.c"
#include "upl_clap.c"
