
/* gcc  -g -O2   -o rightbutton main.o -lX11 -lXi -lXtst -lpthread  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h> /* emulating device events */
#include <X11/Xutil.h>

#include <sys/types.h>

static int xi_opcode;

int BUTTON = 3;

Display *dpy;

int pending_click = False;

static void mouse_click(Display *display, int button)
{
    XTestFakeButtonEvent(display, button, True, CurrentTime);
    XTestFakeButtonEvent(display, button, False, CurrentTime);
}

void grab_pointer(Display *dpy)
{

    int count = XScreenCount(dpy);

    int screen;
    for (screen = 0; screen < count; screen++)
    {

        Window win = RootWindow(dpy, screen);

        int rc;
        XEvent ev;
        XIEventMask mask;

        mask.deviceid = XIAllDevices;
        mask.mask_len = XIMaskLen(XI_Motion);
        mask.mask = calloc(mask.mask_len, sizeof(unsigned char));
        XISetMask(mask.mask, XI_ButtonPress);
        XISetMask(mask.mask, XI_Motion);
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
    }
}

void ungrab_pointer(Display *dpy)
{

    //printf("ungrabbing\n");

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

int waiting = 0;

int main(int argc, char **argv)
{

    if (argc > 2)
    {
        fprintf(stderr, "usage: rightbutton \" command run when 2 clicks \" \n");
    }

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
        fprintf(stderr, "X Input extension not available.\n");
        return -1;
    }

    grab_pointer(dpy);

    int stop = False;

    int x11_fd;
    fd_set in_fds;

    struct timeval tv;

    XFlush(dpy);

    struct timeval *next_tick;

    while (!stop)
    {

        struct timeval interval = {.tv_sec = 0, .tv_usec = 900 * 1000};

        if (waiting)
        {
            next_tick = &interval;
        }
        else
        {
            next_tick = NULL;
        }

        if (XNextEventTimed(dpy, &ev, next_tick))
        {

            XGenericEventCookie *cookie = &ev.xcookie;

            if (cookie->type != GenericEvent ||
                cookie->extension != xi_opcode ||
                !XGetEventData(dpy, cookie))
                continue;

            if (cookie->evtype == XI_Motion)
            {
                //printf(".");
                //fflush(stdout);
            }
            else if (cookie->evtype == XI_ButtonRelease)
            {

                //XIDeviceEvent *data = (XIDeviceEvent *)ev.xcookie.data;

                waiting++;
            }

            XFreeEventData(dpy, cookie);
        }
        else
        {

            if (waiting == 1)
            {

                printf("emulating click\n");

                XFlush(dpy);

                ungrab_pointer(dpy);
                mouse_click(dpy, BUTTON);
                grab_pointer(dpy);
            }

            if (waiting > 1)
            {
                printf("%d clicks detected\n", waiting);

                if (argc == 2)
                {

                    int result = system(argv[1]);
                    if (result)
                    {
                        printf("error %d running command\n", result);
                    }
                }
                else
                {
                    printf("no command provided\n");
                }
            }

            waiting = 0;
        }
    }
    ungrab_pointer;

    XCloseDisplay(dpy);
    return 0;
}
