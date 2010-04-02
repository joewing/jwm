/**
 * @file jxlib.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Macros to wrap X calls for debugging.
 *
 */

#ifndef JXLIB_H
#define JXLIB_H

#define JXAddToSaveSet( a, b ) \
   ( SetCheckpoint(), XAddToSaveSet( a, b ) )

#define JXAllocColor( a, b, c ) \
   ( SetCheckpoint(), XAllocColor( a, b, c ) )

#define JXGetRGBColormaps( a, b, c, d, e ) \
   ( SetCheckpoint(), XGetRGBColormaps( a, b, c, d, e ) )

#define JXQueryColor( a, b, c ) \
   ( SetCheckpoint(), XQueryColor( a, b, c ) )

#define JXAllowEvents( a, b, c ) \
   ( SetCheckpoint(), XAllowEvents( a, b, c ) )

#define JXChangeProperty( a, b, c, d, e, f, g, h ) \
   ( SetCheckpoint(), XChangeProperty( a, b, c, d, e, f, g, h ) )

#define JXDeleteProperty( a, b, c ) \
   ( SetCheckpoint(), XDeleteProperty( a, b, c ) )

#define JXChangeWindowAttributes( a, b, c, d ) \
   ( SetCheckpoint(), XChangeWindowAttributes( a, b, c, d ) )

#define JXCheckTypedEvent( a, b, c ) \
   ( SetCheckpoint(), XCheckTypedEvent( a, b, c ) )

#define JXCheckTypedWindowEvent( a, b, c, d ) \
   ( SetCheckpoint(), XCheckTypedWindowEvent( a, b, c, d ) )

#define JXClearWindow( a, b ) \
   ( SetCheckpoint(), XClearWindow( a, b ) )

#define JXCloseDisplay( a ) \
   ( SetCheckpoint(), XCloseDisplay( a ) )

#define JXConfigureWindow( a, b, c, d ) \
   ( SetCheckpoint(), XConfigureWindow( a, b, c, d ) )

#define JXConnectionNumber( a ) \
   ( SetCheckpoint(), XConnectionNumber( a ) )

#define JXCopyArea( a, b, c, d, e, f, g, h, i, j ) \
   ( SetCheckpoint(), XCopyArea( a, b, c, d, e, f, g, h, i, j ) )

#define JXCopyPlane( a, b, c, d, e, f, g, h, i, j, k ) \
   ( SetCheckpoint(), XCopyPlane( a, b, c, d, e, f, g, h, i, j, k ) )

#define JXCreateFontCursor( a, b ) \
   ( SetCheckpoint(), XCreateFontCursor( a, b ) )

#define JXCreateGC( a, b, c, d ) \
   ( SetCheckpoint(), XCreateGC( a, b, c, d ) )

#define JXCreateImage( a, b, c, d, e, f, g, h, i, j ) \
   ( \
      SetCheckpoint(), \
      XCreateImage( a, b, c, d, e, f, g, h, i, j ) \
   )

#define JXCreatePixmap( a, b, c, d, e ) \
   ( SetCheckpoint(), XCreatePixmap( a, b, c, d, e ) )

#define JXCreatePixmapFromBitmapData( a, b, c, d, e, f, g, h ) \
   ( \
      SetCheckpoint(), \
      XCreatePixmapFromBitmapData( a, b, c, d, e, f, g, h ) \
   )

#define JXCreateBitmapFromData( a, b, c, d, e ) \
   ( SetCheckpoint(), XCreateBitmapFromData( a, b, c, d, e ) )

#define JXCreateSimpleWindow( a, b, c, d, e, f, g, h, i ) \
   ( \
      SetCheckpoint(), \
      XCreateSimpleWindow( a, b, c, d, e, f, g, h, i ) \
   )

#define JXCreateWindow( a, b, c, d, e, f, g, h, i, j, k, l ) \
   ( \
      SetCheckpoint(), \
      XCreateWindow( a, b, c, d, e, f, g, h, i, j, k, l ) \
   )

#define JXDefineCursor( a, b, c ) \
   ( SetCheckpoint(), XDefineCursor( a, b, c ) )

#define JXDestroyImage( a ) \
   ( SetCheckpoint(), XDestroyImage( a ) )

#define JXDestroyWindow( a, b ) \
   ( SetCheckpoint(), XDestroyWindow( a, b ) )

#define JXDrawPoint( a, b, c, d, e ) \
   ( SetCheckpoint(), XDrawPoint( a, b, c, d, e ) )

#define JXDrawLine( a, b, c, d, e, f, g ) \
   ( SetCheckpoint(), XDrawLine( a, b, c, d, e, f, g ) )

#define JXDrawRectangle( a, b, c, d, e, f, g ) \
   ( SetCheckpoint(), XDrawRectangle( a, b, c, d, e, f, g ) )

#define JXDrawString( a, b, c, d, e, f, g ) \
   ( SetCheckpoint(), XDrawString( a, b, c, d, e, f, g ) )

#define JXFetchName( a, b, c ) \
   ( SetCheckpoint(), XFetchName( a, b, c ) )

#define JXFillRectangle( a, b, c, d, e, f, g ) \
   ( SetCheckpoint(), XFillRectangle( a, b, c, d, e, f, g ) )

#define JXFlush( a ) \
   ( SetCheckpoint(), XFlush( a ) )

#define JXFree( a ) \
   ( SetCheckpoint(), XFree( a ) )

#define JXFreeColors( a, b, c, d, e ) \
   ( SetCheckpoint(), XFreeColors( a, b, c, d, e ) )

#define JXFreeCursor( a, b ) \
   ( SetCheckpoint(), XFreeCursor( a, b ) )

#define JXFreeFont( a, b ) \
   ( SetCheckpoint(), XFreeFont( a, b ) )

#define JXFreeGC( a, b ) \
   ( SetCheckpoint(), XFreeGC( a, b ) )

#define JXFreeModifiermap( a ) \
   ( SetCheckpoint(), XFreeModifiermap( a ) )

#define JXFreePixmap( a, b ) \
   ( SetCheckpoint(), XFreePixmap( a, b ) )

#define JXGetAtomName( a, b ) \
   ( SetCheckpoint(), XGetAtomName( a, b ) )

#define JXGetModifierMapping( a ) \
   ( SetCheckpoint(), XGetModifierMapping( a ) )

#define JXGetSubImage( a, b, c, d, e, f, g, h, i, j, k ) \
   ( SetCheckpoint(), XGetSubImage( a, b, c, d, e, f, g, h, i, j, k ) )

#define JXGetTransientForHint( a, b, c ) \
   ( SetCheckpoint(), XGetTransientForHint( a, b, c ) )

#define JXGetClassHint( a, b, c ) \
   ( SetCheckpoint(), XGetClassHint( a, b, c ) )

#define JXGetWindowAttributes( a, b, c ) \
   ( SetCheckpoint(), XGetWindowAttributes( a, b, c ) )

#define JXGetWindowProperty( a, b, c, d, e, f, g, h, i, j, k, l ) \
   ( SetCheckpoint(), \
   XGetWindowProperty( a, b, c, d, e, f, g, h, i, j, k, l ) )

#define JXGetWMColormapWindows( a, b, c, d ) \
   ( SetCheckpoint(), XGetWMColormapWindows( a, b, c, d ) )

#define JXGetWMNormalHints( a, b, c, d ) \
   ( SetCheckpoint(), XGetWMNormalHints( a, b, c, d ) )

#define JXSetIconSizes( a, b, c, d ) \
   ( SetCheckpoint(), XSetIconSizes( a, b, c, d ) )

#define JXSetWindowBorder( a, b, c ) \
   ( SetCheckpoint(), XSetWindowBorder( a, b, c ) )

#define JXGetWMHints( a, b ) \
   ( SetCheckpoint(), XGetWMHints( a, b ) )

#define JXGrabButton( a, b, c, d, e, f, g, h, i, j ) \
   ( SetCheckpoint(), XGrabButton( a, b, c, d, e, f, g, h, i, j ) )

#define JXKeycodeToKeysym( a, b, c ) \
   ( SetCheckpoint(), XKeycodeToKeysym( a, b, c ) )

#define JXGrabKey( a, b, c, d, e, f, g ) \
   ( SetCheckpoint(), XGrabKey( a, b, c, d, e, f, g ) )

#define JXUngrabKey( a, b, c, d ) \
   ( SetCheckpoint(), XUngrabKey( a, b, c, d ) )

#define JXGrabKeyboard( a, b, c, d, e, f ) \
   ( SetCheckpoint(), XGrabKeyboard( a, b, c, d, e, f ) )

#define JXGrabPointer( a, b, c, d, e, f, g, h, i ) \
   ( SetCheckpoint(), XGrabPointer( a, b, c, d, e, f, g, h, i ) )

#define JXGrabServer( a ) \
   ( SetCheckpoint(), XGrabServer( a ) )

#define JXInstallColormap( a, b ) \
   ( SetCheckpoint(), XInstallColormap( a, b ) )

#define JXInternAtom( a, b, c ) \
   ( SetCheckpoint(), XInternAtom( a, b, c ) )

#define JXKeysymToKeycode( a, b ) \
   ( SetCheckpoint(), XKeysymToKeycode( a, b ) )

#define JXKillClient( a, b ) \
   ( SetCheckpoint(), XKillClient( a, b ) )

#define JXLoadQueryFont( a, b ) \
   ( SetCheckpoint(), XLoadQueryFont( a, b ) )

#define JXMapRaised( a, b ) \
   ( SetCheckpoint(), XMapRaised( a, b ) )

#define JXMapWindow( a, b ) \
   ( SetCheckpoint(), XMapWindow( a, b ) )

#define JXMoveResizeWindow( a, b, c, d, e, f ) \
   ( SetCheckpoint(), XMoveResizeWindow( a, b, c, d, e, f ) )

#define JXMoveWindow( a, b, c, d ) \
   ( SetCheckpoint(), XMoveWindow( a, b, c, d ) )

#define JXNextEvent( a, b ) \
   ( SetCheckpoint(), XNextEvent( a, b ) )

#define JXMaskEvent( a, b, c ) \
   ( SetCheckpoint(), XMaskEvent( a, b, c ) )

#define JXCheckMaskEvent( a, b, c ) \
   ( SetCheckpoint(), XCheckMaskEvent( a, b, c ) )

#define JXOpenDisplay( a ) \
   ( SetCheckpoint(), XOpenDisplay( a ) )

#define JXParseColor( a, b, c, d ) \
   ( SetCheckpoint(), XParseColor( a, b, c, d ) )

#define JXPending( a ) \
   ( SetCheckpoint(), XPending( a ) )

#define JXPutBackEvent( a, b ) \
   ( SetCheckpoint(), XPutBackEvent( a, b ) )

#define JXGetImage( a, b, c, d, e, f, g, h ) \
   ( SetCheckpoint(), XGetImage( a, b, c, d, e, f, g, h ) )

#define JXPutImage( a, b, c, d, e, f, g, h, i, j ) \
   ( SetCheckpoint(), XPutImage( a, b, c, d, e, f, g, h, i, j ) )

#define JXQueryPointer( a, b, c, d, e, f, g, h, i ) \
   ( SetCheckpoint(), XQueryPointer( a, b, c, d, e, f, g, h, i ) )

#define JXQueryTree( a, b, c, d, e, f ) \
   ( SetCheckpoint(), XQueryTree( a, b, c, d, e, f ) )

#define JXReparentWindow( a, b, c, d, e ) \
   ( SetCheckpoint(), XReparentWindow( a, b, c, d, e ) )

#define JXRemoveFromSaveSet( a, b ) \
   ( SetCheckpoint(), XRemoveFromSaveSet( a, b ) )

#define JXResizeWindow( a, b, c, d ) \
   ( SetCheckpoint(), XResizeWindow( a, b, c, d ) )

#define JXRestackWindows( a, b, c ) \
   ( SetCheckpoint(), XRestackWindows( a, b, c ) )

#define JXSelectInput( a, b, c ) \
   ( SetCheckpoint(), XSelectInput( a, b, c ) )

#define JXSendEvent( a, b, c, d, e ) \
   ( SetCheckpoint(), XSendEvent( a, b, c, d, e ) )

#define JXSetBackground( a, b, c ) \
   ( SetCheckpoint(), XSetBackground( a, b, c ) )

#define JXSetClipMask( a, b, c ) \
   ( SetCheckpoint(), XSetClipMask( a, b, c ) )

#define JXSetClipOrigin( a, b, c, d ) \
   ( SetCheckpoint(), XSetClipOrigin( a, b, c, d) )

#define JXSetClipRectangles( a, b, c, d, e, f, g ) \
   ( SetCheckpoint(), XSetClipRectangles( a, b, c, d, e, f, g ) )

#define JXSetErrorHandler( a ) \
   ( SetCheckpoint(), XSetErrorHandler( a ) )

#define JXSetFont( a, b, c ) \
   ( SetCheckpoint(), XSetFont( a, b, c ) )

#define JXSetForeground( a, b, c ) \
   ( SetCheckpoint(), XSetForeground( a, b, c ) )

#define JXSetInputFocus( a, b, c, d ) \
   ( SetCheckpoint(), XSetInputFocus( a, b, c, d ) )

#define JXSetWindowBackground( a, b, c ) \
   ( SetCheckpoint(), XSetWindowBackground( a, b, c ) )

#define JXSetWindowBorderWidth( a, b, c ) \
   ( SetCheckpoint(), XSetWindowBorderWidth( a, b, c ) )

#define JXSetWMNormalHints( a, b, c ) \
   ( SetCheckpoint(), XSetWMNormalHints( a, b, c ) )

#define JXShapeCombineRectangles( a, b, c, d, e, f, g, h, i ) \
   ( SetCheckpoint(), XShapeCombineRectangles( a, b, c, d, e, f, g, h, i ) )

#define JXShapeCombineShape( a, b, c, d, e, f, g, h ) \
   ( SetCheckpoint(), XShapeCombineShape( a, b, c, d, e, f, g, h ) )

#define JXShapeCombineMask( a, b, c, d, e, f, g ) \
   ( SetCheckpoint(), XShapeCombineMask( a, b, c, d, e, f, g ) )

#define JXShapeQueryExtension( a, b, c ) \
   ( SetCheckpoint(), XShapeQueryExtension( a, b, c ) )

#define JXQueryExtension( a, b, c, d, e ) \
   ( SetCheckpoint(), XQueryExtension( a, b, c, d, e ) )

#define JXShapeQueryExtents( a, b, c, d, e, f, g, h, i, j, k, l ) \
   ( SetCheckpoint(), \
   XShapeQueryExtents( a, b, c, d, e, f, g, h, i, j, k, l ) )

#define JXShapeSelectInput( a, b, c ) \
   ( SetCheckpoint(), XShapeSelectInput( a, b, c ) )

#define JXStoreName( a, b, c ) \
   ( SetCheckpoint(), XStoreName( a, b, c ) )

#define JXStringToKeysym( a ) \
   ( SetCheckpoint(), XStringToKeysym( a ) )

#define JXSync( a, b ) \
   ( SetCheckpoint(), XSync( a, b ) )

#define JXTextWidth( a, b, c ) \
   ( SetCheckpoint(), XTextWidth( a, b, c ) )

#define JXUngrabButton( a, b, c, d ) \
   ( SetCheckpoint(), XUngrabButton( a, b, c, d ) )

#define JXUngrabKeyboard( a, b ) \
   ( SetCheckpoint(), XUngrabKeyboard( a, b ) )

#define JXUngrabPointer( a, b ) \
   ( SetCheckpoint(), XUngrabPointer( a, b ) )

#define JXUngrabServer( a ) \
   ( SetCheckpoint(), XUngrabServer( a ) )

#define JXUnmapWindow( a, b ) \
   ( SetCheckpoint(), XUnmapWindow( a, b ) )

#define JXWarpPointer( a, b, c, d, e, f, g, h, i ) \
   ( SetCheckpoint(), XWarpPointer( a, b, c, d, e, f, g, h, i ) )

#define JXSetSelectionOwner( a, b, c, d ) \
   ( SetCheckpoint(), XSetSelectionOwner( a, b, c, d ) )

#define JXGetSelectionOwner( a, b ) \
   ( SetCheckpoint(), XGetSelectionOwner( a, b ) )

/* XFT */

#define JXftFontOpenName( a, b, c ) \
   ( SetCheckpoint(), XftFontOpenName( a, b, c ) )

#define JXftFontOpenXlfd( a, b, c ) \
   ( SetCheckpoint(), XftFontOpenXlfd( a, b, c ) )

#define JXftDrawCreate( a, b, c, d ) \
   ( SetCheckpoint(), XftDrawCreate( a, b, c, d ) )

#define JXftDrawDestroy( a ) \
   ( SetCheckpoint(), XftDrawDestroy( a ) )

#define JXftTextExtentsUtf8( a, b, c, d, e ) \
   ( SetCheckpoint(), XftTextExtentsUtf8( a, b, c, d, e ) )

#define JXftDrawChange( a, b ) \
   ( SetCheckpoint(), XftDrawChange( a, b ) )

#define JXftDrawSetClipRectangles( a, b, c, d, e ) \
   ( SetCheckpoint(), XftDrawSetClipRectangles( a, b, c, d, e ) )

#define JXftDrawStringUtf8( a, b, c, d, e, f, g ) \
   ( SetCheckpoint(), XftDrawStringUtf8( a, b, c, d, e, f, g ) )

#define JXftColorFree( a, b, c, d ) \
   ( SetCheckpoint(), XftColorFree( a, b, c, d ) )

#define JXftColorAllocValue( a, b, c, d, e ) \
   ( SetCheckpoint(), XftColorAllocValue( a, b, c, d, e ) )

#define JXftFontClose( a, b ) \
   ( SetCheckpoint(), XftFontClose( a, b ) )

/* Xrender */

#define JXRenderQueryExtension( a, b, c ) \
   ( SetCheckpoint(), XRenderQueryExtension( a, b, c ) )

#define JXRenderFindVisualFormat( a, b ) \
   ( SetCheckpoint(), XRenderFindVisualFormat( a, b ) )

#define JXRenderFindFormat( a, b, c, d ) \
   ( SetCheckpoint(), XRenderFindFormat( a, b, c, d ) )

#define JXRenderFindStandardFormat( a, b ) \
   ( SetCheckpoint(), XRenderFindStandardFormat( a, b ) )

#define JXRenderCreatePicture( a, b, c, d, e ) \
   ( SetCheckpoint(), XRenderCreatePicture( a, b, c, d, e ) )

#define JXRenderFreePicture( a, b ) \
   ( SetCheckpoint(), XRenderFreePicture( a, b ) )

#define JXRenderComposite( a, b, c, d, e, f, g, h, i, j, k, l, m ) \
   ( SetCheckpoint(), \
     XRenderComposite( a, b, c, d, e, f, g, h, i, j, k, l, m) )

#endif /* JXLIB_H */

