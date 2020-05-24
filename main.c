
/* gcc -o rightbutton `pkg-config --cflags --libs x11 xi` main.c */

/* gcc -o part5        `pkg-config --cflags --libs xi` part5.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>

static int xi_opcode;

int BUTTON = 3;

static Window create_win(Display *dpy)
{
    Window win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 200,
                                     200, 0, 0, WhitePixel(dpy, 0));
    Window subwindow = XCreateSimpleWindow(dpy, win, 50, 50, 50, 50, 0, 0,
                                           BlackPixel(dpy, 0));

    XSelectInput(dpy, win, ExposureMask | StructureNotifyMask);

    XMapWindow(dpy, subwindow);
    XMapWindow(dpy, win);
    XFlush(dpy);

    while (1)
    {
        XEvent ev;
        XNextEvent(dpy, &ev);
        if (ev.type == MapNotify)
            break;
    }

    return win;
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
        mask.mask = calloc(mask.mask_len, sizeof(char));
        XISetMask(mask.mask, XI_ButtonPress);
        //XISetMask(mask.mask, XI_Motion);
        XISetMask(mask.mask, XI_ButtonRelease);

        int nmods = 4;
        XIGrabModifiers mods[4] = {
            {0, 0},                  // no modifiers
            {LockMask, 0},           // Caps lock
            {Mod2Mask, 0},           // Num lock
            {LockMask | Mod2Mask, 0} // Caps & Num lock
        };

        nmods = 1;
        mods[0].modifiers = XIAnyModifier;

        if ((rc = XIGrabButton(dpy, 2, BUTTON,
                               win, None,
                               GrabModeAsync, GrabModeAsync, False, &mask, nmods, mods)) != GrabSuccess)
        {
            fprintf(stderr, "Grab failed with %d\n", rc);
            return;
        }
        free(mask.mask);

        //   grab_key(dpy, win);

        // grab_enter(dpy, win);
    }

    XFlush(dpy);
}

void ungrab_pointer(Display *dpy)
{

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

void grab_key(Display *dpy, Window win)
{
    int stop = 0;
    int nmodifiers = 1, failed_nmodifiers;
    XIGrabModifiers modifiers[nmodifiers], failed_modifiers[nmodifiers];
    XIEventMask mask;

    printf("Grabbing keycode 38 (usually 'a') with any modifier\n");

    /* Only listen for XI_KeyRelease */
    mask.mask_len = XIMaskLen(XI_KeyRelease);
    mask.mask = calloc(0, mask.mask_len);
    XISetMask(mask.mask, XI_KeyRelease);

    modifiers[0].modifiers = XIAnyModifier;

    memcpy(failed_modifiers, modifiers, sizeof(modifiers));

    failed_nmodifiers = XIGrabKeycode(dpy, 3, 38, win, GrabModeAsync,
                                      GrabModeAsync, False, &mask,
                                      nmodifiers, failed_modifiers);

    free(mask.mask);

    if (failed_nmodifiers)
    {
        int i;
        for (i = 0; i < failed_nmodifiers; i++)
            printf("Modifier %x failed with error %d\n",
                   failed_modifiers[i].modifiers, failed_modifiers[i].status);
    }

    printf("Waiting for grab to activate now. Press a key.\n");

    while (!stop)
    {
        XEvent ev;
        XGenericEventCookie *cookie = &ev.xcookie;
        XNextEvent(dpy, &ev);
        if (cookie->type != GenericEvent ||
            cookie->extension != xi_opcode ||
            !XGetEventData(dpy, cookie))
            continue;

        if (cookie->evtype == XI_KeyPress)
            printf("Oops, we didn't register for this event.\n");
        else if (cookie->evtype == XI_KeyRelease)
        {
            printf("Release, grab will deactivate now\n");
            stop = 1;
        }

        XFreeEventData(dpy, cookie);
    }

    XIUngrabKeycode(dpy, 3, 38, win, nmodifiers, modifiers);
}

int main(int argc, char **argv)
{
    Display *dpy;
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

    while (!stop)
    {
        XGenericEventCookie *cookie = &ev.xcookie;
        XNextEvent(dpy, &ev);
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
            printf("release. Ungrabbing.\n");
            //stop = True;
        }

        XFreeEventData(dpy, cookie);
    }

    ungrab_pointer;

    XCloseDisplay(dpy);
    return 0;
}
