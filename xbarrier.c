#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xfixes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef VERSION
#define VERSION "dev"
#endif

int usage()
{
    return printf("usage: xbarrier [-h|-v] [-r T] [-t T] X Y W [H]\n\
\n\
Creates a cross-shaped barrier in point (X, Y)\n\
with width W and height H (defaults to W).\n\
If W or H are zero, the corresponding cross side\n\
is not created.\n\
\n\
Hit and leave events are written to stdout.\n\
\n\
OPTIONS:\n\
    -h      show this help\n\
    -v      show version\n\
    -r T    reset hit timer after T msec without events\n\
    -t T    only print one event, after T msec\n");
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IOLBF, 32);
    unsigned hit_reset_delay = 100;
    unsigned trigger_delay = 0;
    int opt;
    extern char *optarg;
    extern int optind;
    while ((opt = getopt(argc, argv, "hvr:t:")) != -1) {
        switch (opt) {
        case 'h':
            usage();
            exit(0);
            break;
        case 'v':
            printf("%s\n", VERSION);
            exit(0);
            break;
        case 'r':
            hit_reset_delay = atoi(optarg);
            break;
        case 't':
            trigger_delay = atoi(optarg);
            break;
        }
    }
    if (argc - optind < 3) {
        fprintf(stderr, "Arguments must be: x, y, width, (height).\n");
        exit(1);
    }
    int x = atoi(argv[optind]);
    int y = atoi(argv[optind + 1]);
    int w = atoi(argv[optind + 2]);
    int h = w;
    if (argc - optind > 3) {
        h = atoi(argv[optind + 3]);
    }

    Display *d = XOpenDisplay(NULL);
    if (d == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
    int s = DefaultScreen(d);
    Window root = RootWindow(d, s);

    int x1 = x - w;
    int y1 = y - h;
    int x2 = x + w;
    int y2 = y + h;
    int direction = 0;
    int num_devices = 0;
    int *devices = NULL;
    PointerBarrier bar1, bar2;
    if (w > 0) {
        bar1 = XFixesCreatePointerBarrier(d, root, x1, y, x2, y, direction,
                                          num_devices, devices);
    }
    if (h > 0) {
        bar2 = XFixesCreatePointerBarrier(d, root, x, y1, x, y2, direction,
                                          num_devices, devices);
    }

    XIEventMask mask;
    unsigned char m[XIMaskLen(XI_BarrierLeave)] = {0};
    mask.mask = m;
    mask.deviceid = XIAllMasterDevices;
    mask.mask_len = XIMaskLen(XI_BarrierLeave);
    XISetMask(mask.mask, XI_BarrierHit);
    XISetMask(mask.mask, XI_BarrierLeave);

    XISelectEvents(d, root, &mask, 1);

    Bool triggered = False;
    Time hit_start = 0;
    Time prev_hit = 0;
    Bool b1_hit = False;
    Bool b2_hit = False;
    while (1) {
        XEvent ev;
        XNextEvent(d, &ev);
        switch (ev.type) {
        case GenericEvent:
            XGetEventData(d, &ev.xcookie);
            XIBarrierEvent *b = (XIBarrierEvent *)(ev.xcookie.data);

            switch (b->evtype) {
            case XI_BarrierHit:
                if (b->barrier == bar1) {
                    b1_hit = True;
                } else if (b->barrier == bar2) {
                    b2_hit = True;
                }

                unsigned dif = b->time - prev_hit;
                prev_hit = b->time;

                if (dif > hit_reset_delay) {
                    hit_start = b->time;
                }

                unsigned t_since_start = b->time - hit_start;
                if (t_since_start > trigger_delay && !triggered) {
                    if (trigger_delay == 0) {
                        printf("hit\t%u\n", t_since_start);
                    } else {
                        printf("hit\n");
                        triggered = True;
                    }
                }

                break;
            case XI_BarrierLeave:
                if (b->barrier == bar1) {
                    b1_hit = False;
                } else if (b->barrier == bar2) {
                    b2_hit = False;
                }

                if (!b1_hit && !b2_hit) {
                    triggered = False;
                    if (trigger_delay == 0) {
                        printf("leave\t%lu\n", b->time - hit_start);
                    }
                }
                break;
            }

            XFreeEventData(d, &ev.xcookie);
            break;
        }
    }

    XFixesDestroyPointerBarrier(d, bar1);
    XFixesDestroyPointerBarrier(d, bar2);
    XCloseDisplay(d);

    return 0;
}
