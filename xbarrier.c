#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xfixes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef VERSION
#define VERSION "dev"
#endif

int usage()
{
    return printf("usage: xbarrier [-h|-v] [-r T] [-d S] [-t T] X Y W [H]\n\
\n\
Creates a cross-shaped barrier in point (X, Y)\n\
with width W and height H (defaults to W).\n\
\n\
Hit and leave events, distance and time are written to stdout.\n\
\n\
OPTIONS:\n\
    -h      show this help\n\
    -v      show version\n\
    -r T    reset hit timer after T ms of inactivity (default 100)\n\
    -d D    print one event, after D travel distance\n\
    -t T    print one event, after T ms\n");
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IOLBF, 32);
    unsigned hit_reset_delay = 100;
    unsigned trigger_wait = 0;
    unsigned trigger_distance = 0;
    int opt;
    extern char *optarg;
    extern int optind;
    while ((opt = getopt(argc, argv, "hvr:t:d:")) != -1) {
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
            trigger_wait = atoi(optarg);
            break;
        case 'd':
            trigger_distance = atoi(optarg);
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

    Bool skip_until_leave = False;
    Time hit_start = 0;
    Time prev_hit = 0;
    Bool b1_hit = False;
    Bool b2_hit = False;
    double x_sum = 0;
    double y_sum = 0;
    double distance;

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

                x_sum += b->dx;
                y_sum += b->dy;
                distance = sqrt(x_sum * x_sum + y_sum * y_sum);

                unsigned t_since_start = b->time - hit_start;
                if (skip_until_leave || t_since_start < trigger_wait ||
                    distance < trigger_distance) {
                    break;
                }

                if (trigger_wait != 0 || trigger_distance != 0) {
                    skip_until_leave = True;
                }
                printf("h\t%.0f\t%u\n", distance, t_since_start);
                break;
            case XI_BarrierLeave:
                if (b->barrier == bar1) {
                    b1_hit = False;
                } else if (b->barrier == bar2) {
                    b2_hit = False;
                }
                if (b1_hit || b2_hit) {
                    break;
                }

                if (trigger_wait == 0 && trigger_distance == 0) {
                    printf("l\t%.0f\t%lu\n",
                           sqrt(x_sum * x_sum + y_sum * y_sum),
                           b->time - hit_start);
                }
                skip_until_leave = False;
                x_sum = 0;
                y_sum = 0;
                prev_hit = 0;
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
