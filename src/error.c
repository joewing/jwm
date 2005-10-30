/****************************************************************************
 * Functions to handle error events in JWM.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "main.h"

/****************************************************************************
 ****************************************************************************/
void FatalError(const char *str, ...) {
	va_list ap;
	va_start(ap, str);

	fprintf(stderr, "JWM: error: ");
	vfprintf(stderr, str, ap);
	fprintf(stderr, "\n");

	va_end(ap);

	exit(1);

}

/****************************************************************************
 ****************************************************************************/
void Warning(const char *str, ...) {
	va_list ap;
	va_start(ap, str);

	fprintf(stderr, "JWM: warning: ");
	vfprintf(stderr, str, ap);
	fprintf(stderr, "\n");

	va_end(ap);
}

/****************************************************************************
 ****************************************************************************/
void WarningVA(const char *part, const char *str, va_list ap) {
	fprintf(stderr, "JWM: warning: %s: ", part);
	vfprintf(stderr, str, ap);
	fprintf(stderr, "\n");
}

/****************************************************************************
 ****************************************************************************/
int ErrorHandler(Display *d, XErrorEvent *e) {
	char message[80], code[10], request[80];

	if(initializing) {
		if(e->request_code == X_ChangeWindowAttributes
			&& e->error_code == BadAccess) {
			FatalError("display is already managed");
		}
	}

	snprintf(code, sizeof(code), "%d", e->request_code);
	XGetErrorDatabaseText(display, "XRequest", code, "", request,
		sizeof(request));
	if(!request[0]) {
		snprintf(request, sizeof(request), "[request_code=%d]",
			e->request_code);
	}
	if(XGetErrorText(display, e->error_code, message, sizeof(message))
		!= Success) {
		snprintf(message, sizeof(message), "[error_code=%d]",
			e->error_code);
	}

	ShowCheckpoint();
	Warning("XError: %s[%d]: %s", request, e->minor_code, message);

	return 0;
}

