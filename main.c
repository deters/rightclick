
/* gcc -o rightbutton `pkg-config --cflags --libs x11 xi` main.c */

/* gcc -o part5        `pkg-config --cflags --libs xi` part5.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/Xutil.h>

#include <time.h>

static int xi_opcode;

int BUTTON = 3;

Display *dpy;

int pending_click = False;

#include <X11/extensions/XTest.h> /* emulating device events */

static void mouse_click(Display *display, int button)
{

    // XTestFakeMotionEvent(display, DefaultScreen(dpy), x, y, 0);
    XTestFakeButtonEvent(display, button, True, CurrentTime);
    XTestFakeButtonEvent(display, button, False, CurrentTime);
    //XFlush(dpy);
}

void grab_pointer(Display *dpy)
{

    printf("grabbing\n");

    int count = XScreenCount(dpy);

    printf("%d screens\n", count);

    int screen;
    for (screen = 0; screen < count; screen++)
    {

        Window win = RootWindow(dpy, screen);

        int rc;
        XEvent ev;
        XIEventMask mask;

        mask.deviceid = XIAllDevices;
        mask.mask_len = XIMaskLen(XI_Motion);
        mask.mask = calloc(mask.mask_len, sizeof(char));
        XISetMask(mask.mask, XI_ButtonPress);
        //XISetMask(mask.mask, XI_Motion);
        XISetMask(mask.mask, XI_ButtonRelease);

        int nmods = 1;
        XIGrabModifiers mods = {
            XIAnyModifier};

        if ((rc = XIGrabButton(dpy, 2, BUTTON,
                               win, None,
                               GrabModeAsync, GrabModeAsync, False, &mask, nmods, &mods)) != GrabSuccess)
        {
            fprintf(stderr, "Grab failed with %d\n", rc);
            return;
        }
        free(mask.mask);

        //   grab_key(dpy, win);

        // grab_enter(dpy, win);
    }

    // XFlush(dpy);
}

void ungrab_pointer(Display *dpy)
{

    printf("ungrabbing\n");

    int count = XScreenCount(dpy);

    int screen;
    for (screen = 0; screen < count; screen++)
    {

        Window win = RootWindow(dpy, screen);

        XIGrabModifiers mods = {
            XIAnyModifier};
        XIUngrabButton(dpy, 2, BUTTON, win,
                       1, &mods);
    }
}

Bool XNextEventTimed(Display *display, XEvent *event_return, struct timeval *timeout)
{

    /*
     *  Usage:
     *    struct timeval = { .tv_sec = 5, .tv_usec = 0 };
     *    if (XNextEventTimed(display, &event, &timeout) == True) {
     *        //.. do your event processing switch
     *    } else {
     *        //.. do your timeout thing
     *    }
     *
     *  Return:  
     *    True if an event was selected, or False when no event
     *    was found prior to the timeout or when select returned
     *    an error (most likely EINTR).
    */

    if (timeout == NULL)
    {
        XNextEvent(display, event_return);
        return True;
    }

    if (XPending(display) == 0)
    {
        int fd = ConnectionNumber(display);
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(fd, &readset);
        if (select(fd + 1, &readset, NULL, NULL, timeout) > 0)
        {
            XNextEvent(display, event_return);
            return True;
        }
        else
        {
            return False;
        }
    }
    else
    {
        XNextEvent(display, event_return);
        return True;
    }
}

clock_t last;

int waiting = False;

int main(int argc, char **argv)
{

    last = 0;

    int event, error;
    Window win;
    XEvent ev;

    dpy = XOpenDisplay(NULL);

    if (!dpy)
    {
        fprintf(stderr, "Failed to open display.\n");
        return -1;
    }

    if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &event, &error))
    {
        printf("X Input extension not available.\n");
        return -1;
    }

    grab_pointer(dpy);

    int stop = False;

    printf("Grab on device 2, waiting for button release\n");

    int x11_fd;
    fd_set in_fds;

    struct timeval tv;

    int count = XScreenCount(dpy);

    XFlush(dpy);

    struct timeval interval = {.tv_sec = 0, .tv_usec = 900 * 1000};
    struct timeval next_tick = interval;

    while (!stop)
    {

        printf(".");

        if (XNextEventTimed(dpy, &ev, &next_tick))
        {
            //            dispatch(&cookie);

            XGenericEventCookie *cookie = &ev.xcookie;

            if (cookie->type != GenericEvent ||
                cookie->extension != xi_opcode ||
                !XGetEventData(dpy, cookie))
                continue;

            if (cookie->evtype == XI_Motion)
            {
                printf(".");
                fflush(stdout);
            }
            else if (cookie->evtype == XI_ButtonRelease)
            {

                XIDeviceEvent *data = (XIDeviceEvent *)ev.xcookie.data;

                printf("release\n");

                if (waiting)
                {
                    waiting = False;
                    printf("double click detected.\n");
                }
                else
                {

                    printf(" waiting secound click \n");
                    waiting = True;
                }
            }

            XFreeEventData(dpy, cookie);
        }
        else
        {

            if (waiting)
            {

                printf("emulating click\n");
                waiting = False;

                XFlush(dpy);

                while (XPending(dpy))
                {
                    XNextEvent(dpy, &ev);
                }

                ungrab_pointer(dpy);

                mouse_click(dpy, BUTTON);

                grab_pointer(dpy);
            }

            fprintf(stdout, "tick...\n");
            next_tick = interval;
        }
    }
    ungrab_pointer;

    XCloseDisplay(dpy);
    return 0;
}
