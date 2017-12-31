cmake_minimum_required(VERSION 2.8)
project(jwm C)
set (PACKAGE_NAME "${PROJECT_NAME}")
set(DATE "")
string(TIMESTAMP DATE)
set(VERSION_MAJOR 2)
set(VERSION_MINOR 4)
set(VERSION_REV 0)
set(SYSCONF "${CMAKE_INSTALL_PREFIX}/etc/") 
set(PACKAGE_BUGREPORT "joewing@joewing.net")
set(AUTHOR "Joe Wingbermuehle")
set(PACKAGE_URL "http://joewing.net/projects/jwm")
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REV}")
set(PACKAGE_VERSION "${VERSION}")
set(PACKAGE_STRING "Joe's Window Manager ${VERSION}")
set(PACKAGE_TARNAME "${PROJECT_NAME}")
set(LOCALE_DIR "/locale")
set(LOCALEDIR "${CMAKE_INSTALL_PREFIX}${LOCALE_DIR}")
set(ARCH_INDEP_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")
## TODO... make configure script that does: mkdir bin && cd bin && cmake ..
set(USE_ICONS true)
## path for custom cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

## DON'T build in here!!!! OK?!?!?!
if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
     message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt." )
endif()

## first line of our generated config.h file
set(SYSTEM_CONFIG "${SYSCONF}system.jwmrc")

string(COMPARE EQUAL "${CMAKE_BUILD_TYPE}" "Debug" D_RES)
set(DEBUG "${D_RES}")
######## FIND STUFF ####################

##### LIBRARIES #####
## We need X11
find_package(X11 REQUIRED)
string(COMPARE EQUAL "${X11_INCLUDE_DIR}" "" x11_result)
if( x11_result )
    message(FATAL_ERROR "X11 headers are not found")
endif()
set(ALL_LIBS "${X11_LIBRARIES}" "${X11_X11_LIB}")
set(JWM_INCLUDES "${X11_X11_INCLUDE_PATH}")
#libXpm
set(USE_XPM ${X11_Xpm_FOUND})
if(X11_Xpm_FOUND)
	## ALL_LIBS is the variable to store all the libraries we find
	set(ALL_LIBS "${ALL_LIBS}" "${X11_Xpm_LIB}")
    ## JWM_INCLUDES is the variable to contain all the includes we find
	set(JWM_INCLUDES "${JWM_INCLUDES}" "${X11_Xpm_INCLUDE_PATH}")
endif()

#libXinerama
set(USE_XINERAMA ${X11_Xinerama_FOUND})
if(X11_Xinerama_FOUND)
	set(ALL_LIBS "${ALL_LIBS}" "${X11_Xinerama_LIB}")
	set(JWM_INCLUDES "${JWM_INCLUDES}" "${X11_Xinerama_INCLUDE_PATH}")
endif()

#libXft
find_package(FreeType QUIET)
if(FREETYPE_FOUND)
	message("FREETYPE found")
	set(ALL_LIBS "${ALL_LIBS}" "${X11_Xft_LIB}" "${FREETYPE_LIBRARIES}")
	set(JWM_INCLUDES "${JWM_INCLUDES}" "${X11_Xft_INCLUDE_PATH}" "${FREETYPE_INCLUDE_DIRS}")
endif()
set(USE_XFT ${FREETYPE_FOUND})

#libXmu
set(USE_XMU ${X11_Xmu_FOUND})
if(X11_Xmu_FOUND)
	message("Found X11 Xmu")
	set(ALL_LIBS "${ALL_LIBS}" "${X11_Xmu_LIB}")
	set(JWM_INCLUDES "${JWM_INCLUDES}" "${X11_Xmu_INCLUDE_PATH}")
endif()

#libXrender
set(USE_XRENDER ${X11_Xrender_FOUND})
if(X11_Xrender_FOUND)
	set(ALL_LIBS "${ALL_LIBS}" "${X11_Xrender_LIB}")
	set(JWM_INCLUDES "${JWM_INCLUDES}" "${X11_Xrender_INCLUDE_PATH}")
endif()

#libXext
if(X11_Xext_FOUND)
	set(ALL_LIBS "${ALL_LIBS}" "${X11_Xext_LIB}")
	#set(JWM_INCLUDES "${JWM_INCLUDES}" "${}")
endif()

#libpng
find_package(PNG QUIET)
set(USE_PNG ${PNG_FOUND})
if(PNG_FOUND)
	set(ALL_LIBS "${ALL_LIBS}" "${PNG_LIBRARIES}")
	set(JWM_INCLUDES "${JWM_INCLUDES}" "${PNG_INCLUDE_DIRS}")
endif()

#libjpeg
find_package(JPEG QUIET)
set(USE_JPEG ${JPEG_FOUND})
if(PNG_FOUND)
	set(ALL_LIBS "${ALL_LIBS}" "${JPEG_LIBRARIES}")
	set(JWM_INCLUDES "${JWM_INCLUDES}" "${JPEG_INCLUDE_DIRS}")
endif()
#fribidi
find_package(Fribidi QUIET)
set(USE_FRIBIDI ${FRIBIDI_FOUND})
if(FRIBIDI_FOUND)
	set(ALL_LIBS "${ALL_LIBS}" "${FRIBIDI_LIBRARIES}")
	set(JWM_INCLUDES "${JWM_INCLUDES}" "${FRIBIDI_INCLUDE_DIR}")
endif()

#Cairo
find_package(Cairo)
set(USE_CAIRO ${CAIRO_FOUND})
if(CAIRO_FOUND)
	set(ALL_LIBS "${ALL_LIBS}" "${CAIRO_LIBRARIES}")
	set(JWM_INCLUDES "${JWM_INCLUDES}" "${CAIRO_INCLUDE_DIRS}")
endif()

#librsvg2
find_package(RSVG)
set(USE_RSVG ${RSVG_FOUND})
if(RSVG_FOUND)
	message("Found RSVG")
	set(ALL_LIBS "${ALL_LIBS}" "${RSVG_LIBRARIES}")
	set(JWM_INCLUDES "${JWM_INCLUDES}" "${RSVG_INCLUDE_DIRS}")
endif()

#nanosvg
#find_package( QUIET)
#set(USE_ ${})
#if(_FOUND)
#	set(ALL_LIBS "${ALL_LIBS}" "${_LIBRARIES}")
#	set(JWM_INCLUDES "${JWM_INCLUDES}" "${_INCLUDE_DIR}")
#endif()

##### HEADERS #####
include (CheckIncludeFiles)
function(CheckHeader header_filename)
	string(TOUPPER ${header_filename} OUTVAR)
	string(REPLACE . _ DEFINE_NAME ${OUTVAR})
	string(REPLACE / _ DEFINE_NAME ${DEFINE_NAME})
	string(REPLACE / _ DEFINE_NAME ${DEFINE_NAME})
	set(NORMAL_PLACES_TO_LOOK "/usr/include/;/usr/local/include/")
	check_include_files("${header_filename}" "HAVE_${DEFINE_NAME}")
	if( NOT HAVE_${DEFINE_NAME})
		message("Trying something else to find ${header_filename}")
		find_file("CHECK_${DEFINE_NAME}" "${header_filename}")
		if( CHECK_${DEFINE_NAME})
			set(HAVE_${DEFINE_NAME} true)
		endif()
	endif()
endfunction()

find_path(ALLOCA alloca.c)
set(C_ALLOCA "${ALLOCA}")

## check list of header files
set(HEADERS_TO_CHECK "alloca.h;ctype.h;iconv.h;inttypes.h;jpeglib.h;langinfo.h;libintl.h;locale.h;memory.h;signal.h;stdarg.h;stdint.h;stdio.h;stdlib.h;strings.h;string.h;sys/select.h;sys/stat.h;sys/time.h;sys/types.h;sys/wait.h;time.h;unistd.h;X11/cursorfont.h;X11/extensions/Xrender.h;X11/keysym.h;X11/Xatom.h;X11/Xlib.h;X11/xpm.h;X11/Xproto.h;X11/Xresource.h;X11/Xutil.h;")
message("Checking for header files")
foreach(loop_var ${HEADERS_TO_CHECK})
	CheckHeader("${loop_var}")
endforeach()

##### FUNCTIONS #####
include (CheckFunctionExists)
## FUNCTION CHECKER
function (CheckFunctions FunName)
	string(TOUPPER ${FunName} OUTVAR)
	check_function_exists(${FunName} "HAVE_${OUTVAR}")
endfunction()

set(FUNCTIONS_TO_CHECK
"CFLocaleCopyCurrent;CFPreferencesCopyAppValue;alloca;dcgettext;gettext;iconv;putenv;setlocale;unsetenv")
message("Checking for functions")
foreach(loop_var ${FUNCTIONS_TO_CHECK})
	CheckFunctions("${loop_var}")
endforeach()

###### LIBS ############
#check_library_exists(X11 XOpenDisplay "" USE_X11)
check_library_exists(Xrender XRenderComposite "" USE_XRENDER)
check_library_exists(fribidi fribidi_charset_to_unicode "" USE_FRIBIDI)
check_library_exists(X11 XReadBitmapFileData "" USE_XBM)
check_library_exists(X11 Xutf8TextPropertyToTextList "" USE_XUTF8)
check_library_exists(Xext XShapeCombineRectangles "" USE_SHAPE)
check_library_exists(Xmu XmuDrawRoundedRectangle "" USE_XMU)
check_library_exists(Xft XftFontOpenName "" USE_XFT)
check_include_files(ft2build.h USE_XFT)
###### Symbols  ############
include(CheckSymbolExists)
check_symbol_exists(XpmAllocColor X11/xpm.h USE_XPM)
check_symbol_exists(alloca alloca.h HAVE_ALLOCA)
##### compiles or not? ############
set(iconvsourcetest "")

include (CheckCSourceCompiles)
check_c_source_compiles("${iconvsourcetest}" ICONV_CONST)
##TODO: doesn't work right
check_c_source_compiles("#define _XOPEN_SOURCE 600L
#include <unistd.h>" _XOPEN_SOURCE_EXTENDED)
set(__EXTENSIONS__ "${_XOPEN_SOURCE_EXTENDED}")
if(_XOPEN_SOURCE_EXTENDED)
	set(_XOPEN_SOURCE 600L)
endif()
##TODO:
#AC_ARG_ENABLE(confirm,
#   AC_HELP_STRING([--disable-confirm], [disable confirm dialogs]) )
#if test "$enable_confirm" = "no" ; then
#   AC_DEFINE(DISABLE_CONFIRM, 1, [Define to disable confirm dialogs])
#else
#   enable_confirm="yes"
#fi

## gettext
# https://cmake.org/cmake/help/v2.8.12/cmake.html#module:FindGettext
find_package(Gettext)
if(GETTEXT_FOUND)
	file (GLOB PO_FILES ${CMAKE_SOURCE_DIR}/po/*.po)
	message( "msgfmt: ${GETTEXT_MSGFMT_EXECUTABLE}")
	message( "msgmerge: ${GETTEXT_MSGMERGE_EXECUTABLE}")
    GETTEXT_CREATE_TRANSLATIONS(po/jwm.pot ALL ${PO_FILES})
endif()

########### Source #####################
include_directories(
	"${JWM_INCLUDES}"
	"${X11_INCLUDE_DIR}"
	"${CMAKE_SOURCE_DIR}"
	"${CMAKE_SOURCE_DIR}/src"
	"${CMAKE_BINARY_DIR}/src"
)

# all the source files minus main.c
# for i in **.c; do [ "$i" != main.c ] && echo -e "\tsrc/$i"; done
set(JWM_SRCS
	src/action.c
	src/background.c
	src/binding.c
	src/border.c
	src/button.c
	src/client.c
	src/clientlist.c
	src/clock.c
	src/color.c
	src/command.c
	src/confirm.c
	src/cursor.c
	src/debug.c
	src/desktop.c
	src/dock.c
	src/error.c
	src/event.c
	src/font.c
	src/grab.c
	src/gradient.c
	src/group.c
	src/help.c
	src/hint.c
	src/icon.c
	src/image.c
	src/lex.c
	src/match.c
	src/menu.c
	src/misc.c
	src/move.c
	src/outline.c
	src/pager.c
	src/parse.c
	src/place.c
	src/popup.c
	src/render.c
	src/resize.c
	src/root.c
	src/screen.c
	src/settings.c
	src/spacer.c
	src/status.c
	src/swallow.c
	src/taskbar.c
	src/timing.c
	src/traybutton.c
	src/tray.c
	src/winmenu.c
)

######### Build ###############
## add the library
#add_library(JWM STATIC ${JWM_SRCS})
## make the executable
add_executable(${PROJECT_NAME} src/main.c ${JWM_SRCS})
## link the libs to the executable
target_link_libraries(${PROJECT_NAME} "${ALL_LIBS}")

## install jwm and libJWM
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

## install icons
file(GLOB ICONS "contrib/*.svg")
set(ICONS "${ICONS}" contrib/${PROJECT_NAME}.xbm)
install(FILES ${ICONS} DESTINATION "${ARCH_INDEP_INSTALL_DIR}/share/pixmaps")

##install man page
message("creating ${PROJECT_NAME}.1")
configure_file("${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.1.in" "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.1")
install(FILES "${PROJECT_NAME}.1" DESTINATION "${ARCH_INDEP_INSTALL_DIR}/share/man/man1/")

## configure header
configure_file("${CMAKE_SOURCE_DIR}/cmake/config.h.in" ../config.h)
message("Compiler:  ${CMAKE_C_COMPILER}\nCompiler Flags: ${CMAKE_C_FLAGS_RELEASE}")
install(FILES example.jwmrc DESTINATION "${SYSCONF}" RENAME system.jwmrc)
