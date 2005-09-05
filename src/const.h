/****************************************************************************
 * Global constants.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef CONST_H
#define CONST_H

#define MAX_INCLUDE_DEPTH 16

#define MAX_BORDER_WIDTH 32
#define MIN_BORDER_WIDTH 3
#define DEFAULT_BORDER_WIDTH 5

#define MAX_TITLE_HEIGHT 64
#define MIN_TITLE_HEIGHT 2
#define DEFAULT_TITLE_HEIGHT 21

#define MAX_DOUBLE_CLICK_DELTA 32
#define MIN_DOUBLE_CLICK_DELTA 0
#define DEFAULT_DOUBLE_CLICK_DELTA 2

#define MAX_DOUBLE_CLICK_SPEED 2000
#define MIN_DOUBLE_CLICK_SPEED 1
#define DEFAULT_DOUBLE_CLICK_SPEED 400

#define MAX_TRAY_HEIGHT 128
#define MIN_TRAY_HEIGHT 8
#define DEFAULT_TRAY_HEIGHT 28

#define MIN_TRAY_WIDTH 320
#define MIN_MAX_TRAY_ITEM_WIDTH 32

#define MAX_SNAP_DISTANCE 32
#define MIN_SNAP_DISTANCE 1
#define DEFAULT_SNAP_DISTANCE 5

#define MOVE_DELTA 3

#define SHELL_NAME "/bin/sh"

#define DEFAULT_MENU_TITLE "JWM"

#define DEFAULT_CLOCK_FORMAT "%I:%M %p"
#define MAX_CLOCK_LENGTH 32

/* MWM Defines */
#define MWM_HINTS_FUNCTIONS   (1L << 0)
#define MWM_HINTS_DECORATIONS (1L << 1)
#define MWM_HINTS_INPUT_MODE  (1L << 2)
#define MWM_HINTS_STATUS      (1L << 3)

#define MWM_FUNC_ALL      (1L << 0)
#define MWM_FUNC_RESIZE   (1L << 1)
#define MWM_FUNC_MOVE     (1L << 2)
#define MWM_FUNC_MINIMIZE (1L << 3)
#define MWM_FUNC_MAXIMIZE (1L << 4)
#define MWM_FUNC_CLOSE    (1L << 5)

#define MWM_DECOR_ALL      (1L << 0)
#define MWM_DECOR_BORDER   (1L << 1)
#define MWM_DECOR_RESIZEH  (1L << 2)
#define MWM_DECOR_TITLE    (1L << 3)
#define MWM_DECOR_MENU     (1L << 4)
#define MWM_DECOR_MINIMIZE (1L << 5)
#define MWM_DECOR_MAXIMIZE (1L << 6)

#define MWM_INPUT_MODELESS                  0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL              2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3

#define MWM_TEAROFF_WINDOW (1L << 0)

#endif

