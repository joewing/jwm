/**
 * @file event.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the event functions.
 *
 */

#ifndef EVENT_H
#define EVENT_H

/** Wait for an event and process it. */
void WaitForEvent();

/** Process an event.
 * @param event The event to process.
 */
void ProcessEvent(XEvent *event);

/** Discard excess motion events.
 * @param event The event to return.
 * @param w The window whose events to discard.
 */
void DiscardMotionEvents(XEvent *event, Window w);

#endif /* EVENT_H */

