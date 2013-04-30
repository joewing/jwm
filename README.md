Amigo Desktop based on JWM (Joe's Window Manager)
==============================================================================

Amigo Desktop his an attempt and experiment at the same time, trying to make
some improvements in my view on top of JWM to making it more eye candy and
accessible in the user side point of view.

JWM is a light-weight window manager for the X11 Window System.

Requirements
------------------------------------------------------------------------------
To build JWM you will need a C compiler (gcc works), X11, and the
"development headers" for X11 and Xlib.
If available and not disabled at compile time, JWM will also use
the following libraries:

 - libcairo and librsvg for SVG icons and backgrounds.
 - libfribidi for bi-directional text support.
 - libjpeg for JPEG icons and backgrounds.
 - libpng for PNG icons and backgrounds.
 - libXext for the shape extension.
 - libXext for the render extension.
 - libXmu for rounded corners.
 - libxft for antialiased and true type fonts.
 - libXinerama for multiple head support.
 - libXpm for XPM icons and backgrounds.

Installation
------------------------------------------------------------------------------

 0. If building from the git repository, run autoreconf to generate configure.
 1. Run "./configure --help" for configuration options.
 2. Run "./configure [options]"
 3. Run "make" to build JWM.
 4. Run "make install" to install JWM.  Depending on where you are installing
    JWM, you may need to perform this step as root ("sudo make install").

License
------------------------------------------------------------------------------
See LICENSE for license information.

For more information see http://joewing.net/projects/jwm/

