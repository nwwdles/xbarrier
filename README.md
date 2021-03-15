# xbarrier

```txt
usage: xbarrier [-h|-v] [-r T] [-t T] X Y W [H]

Creates a cross-shaped barrier in point (X, Y)
with width W and height H (defaults to W).
If W or H are zero, the corresponding cross side
is not created.

Hit and leave events are written to stdout.

OPTIONS:
    -h      show this help
    -v      show version
    -r T    reset hit timer after T msec without events
    -t T    only print one event, after T msec
```

## Example hot corner

```sh
#!/bin/sh
xbarrier 1024 0 10 | while read -r ev dur; do
    case "$ev" in
    l*) can_trigger=true ;;
    h*) if "${can_trigger:-true}" && [ "$dur" -gt 200 ]; then
        notify-send 'Hello!'
        can_trigger=false
    fi ;;
    esac
done
```

Or, simpler

```sh
#!/bin/sh
xbarrier -t 200 1030 0 10 | while read -r; do notify-send 'Hello!'; done
```
