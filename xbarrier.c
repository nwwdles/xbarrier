#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrandr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef VERSION
#define VERSION "dev"
#endif

const int NO_HIT = 0;
const int HIT = 1;

int main(int argc, char **argv)
{
    unsigned hit_reset_delay = 100;

    int opt;
    extern char *optarg;
    while ((opt = getopt(argc, argv, "hvd:")) != -1) {
        switch (opt) {
        case 'd':
            hit_reset_delay = atoi(optarg);
            break;
        case 'v':
            printf("%s\n", VERSION);
            exit(0);
            break;
        case 'h':
            printf("usage: xbarrier [-h|-v|-d DEL] X Y W [H]\n\
\n\
Creates a cross-shaped barrier in point (X, Y) \n\
with width W and height H (defaults to W).\n\
If W or H are zero, the corresponding cross side\n\
is not created.\n\
\n\
Hit and leave events are written to stdout.\n\
\n\
OPTIONS:\n\
    -h      show this help\n\
    -v      show version\n\
    -d DEL  set delay before hit timer reset\n\
");
            exit(0);
            break;
        }
    }
    if (argc < 4) {
        fprintf(stderr, "Arguments must be: x, y, width, (height).\n");
        exit(1);
    }

    Display *d = XOpenDisplay(NULL);
    if (d == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
    int s = DefaultScreen(d);
    Window root = RootWindow(d, s);

    int x = atoi(argv[1]);
    int y = atoi(argv[2]);
    int w = atoi(argv[3]);
    int h = w;
    if (argc >= 5) {
        h = atoi(argv[4]);
    }

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

    Time hit_start = 0;
    Time prev_hit = 0;
    int b1_hit = 0;
    int b2_hit = 0;
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
                    b1_hit = HIT;
                } else if (b->barrier == bar2) {
                    b2_hit = HIT;
                }

                unsigned dif = b->time - prev_hit;
                prev_hit = b->time;

                if (dif > hit_reset_delay) {
                    hit_start = b->time;
                }

                printf("hit\t%lu\n", b->time - hit_start);

                break;
            case XI_BarrierLeave:
                if (b->barrier == bar1) {
                    b1_hit = NO_HIT;
                } else if (b->barrier == bar2) {
                    b2_hit = NO_HIT;
                }
                if (b1_hit == NO_HIT && b2_hit == NO_HIT) {
                    printf("leave\t%lu\n", b->time - hit_start);
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
