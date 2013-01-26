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

/** Last event time. */
extern Time eventTime;

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

/** Discard excess key events.
 * @param event The event to return.
 * @param w The window whose events to discard.
 */
void DiscardKeyEvents(XEvent *event, Window w);

/** Update the last event time.
 * @param event The event containing the time to use.
 */
void UpdateTime(const XEvent *event);

#endif /* EVENT_H */

