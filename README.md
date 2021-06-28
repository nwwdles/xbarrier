# xbarrier

```txt
usage: xbarrier [-h|-v] [-d S] [-t T] [-r T] X Y W [H]

Creates a cross-shaped barrier in point (X, Y)
with width W and height H (defaults to W).

Hit and leave events, distance and time are written to stdout.

OPTIONS:
    -h      show this help
    -v      show version
    -d D    print one event, after D travel distance
    -t T    print one event, after T ms
    -r T    reset hit timer after T ms of inactivity (default 100)
```

## Example hot corner

```sh
#!/bin/sh
xbarrier -d 500 1024 0 10 | while read -r; do notify-send 'Hello!'; done
```
