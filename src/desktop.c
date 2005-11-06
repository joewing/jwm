/***************************************************************************
 ***************************************************************************/

#include "jwm.h"
#include "desktop.h"
#include "main.h"
#include "client.h"
#include "hint.h"
#include "pager.h"
#include "taskbar.h"
#include "error.h"

/***************************************************************************
 ***************************************************************************/
void NextDesktop()
{
	ChangeDesktop((currentDesktop + 1) % desktopCount);
}

/***************************************************************************
 ***************************************************************************/
void PreviousDesktop()
{
	if(currentDesktop > 0) {
		ChangeDesktop(currentDesktop - 1);
	} else {
		ChangeDesktop(desktopCount - 1);
	}
}

/***************************************************************************
 ***************************************************************************/
void ChangeDesktop(int desktop)
{

	ClientNode *np;
	int x;

	if(desktop >= desktopCount || desktop < 0) {
		return;
	}

	if(currentDesktop == desktop && !initializing) {
		return;
	}

	for(x = 0; x < LAYER_COUNT; x++) {
		for(np = nodes[x]; np; np = np->next) {
			if(np->state.status & STAT_STICKY) {
				continue;
			}
			if(np->state.desktop == desktop) {
				ShowClient(np);
			} else if(np->state.desktop == currentDesktop) {
				HideClient(np);
			}
		}
	}

	currentDesktop = desktop;

	SetCardinalAtom(rootWindow, ATOM_NET_CURRENT_DESKTOP, currentDesktop);
	SetCardinalAtom(rootWindow, ATOM_WIN_WORKSPACE, currentDesktop);

	RestackClients();

	UpdatePager();
	UpdateTaskBar();

}

/***************************************************************************
 ***************************************************************************/
void SetDesktopCount(const char *str)
{

	Assert(str);

	desktopCount = atoi(str);
	if(desktopCount <= 0 || desktopCount > MAX_DESKTOP_COUNT) {
		Warning("invalid desktop count: \"%s\"", str);
		desktopCount = DEFAULT_DESKTOP_COUNT;
	}

}


