/****************************************************************************
 * The main JWM header file.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef JWM_H
#define JWM_H

#define MAX_DESKTOP_COUNT 8

#include "../config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#ifdef USE_XPM
#include <X11/xpm.h>
#endif
#ifdef USE_PNG
#include <png.h>
#endif
#ifdef USE_SHAPE
#include <X11/extensions/shape.h>
#endif
#ifdef USE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#ifdef USE_XFT
#include <X11/Xft/Xft.h>
#endif
#ifdef USE_XRENDER
#include <X11/extensions/Xrender.h>
#endif

#include "enum.h"
#include "struct.h"

#include "border.h"
#include "button.h"
#include "client.h"
#include "color.h"
#include "command.h"
#include "confirm.h"
#include "const.h"
#include "cursor.h"
#include "event.h"
#include "debug.h"
#include "error.h"
#include "font.h"
#include "global.h"
#include "group.h"
#include "help.h"
#include "hint.h"
#include "icon.h"
#include "image.h"
#include "jxlib.h"
#include "key.h"
#include "lex.h" 
#include "load.h"
#include "main.h"
#include "match.h"
#include "menu.h"
#include "misc.h"
#include "move.h"
#include "os/os.h"
#include "outline.h"
#include "pager.h"
#include "parse.h"
#include "popup.h"
#include "render.h"
#include "resize.h"
#include "root.h"
#include "screen.h"
#include "status.h"
#include "timing.h"
#include "tray.h"
#include "winmenu.h"

#endif

