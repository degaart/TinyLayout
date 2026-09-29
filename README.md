# TinyLayout

TinyLayout is a C99 flexbox library distributed as **`tinylayout.h` and `tinylayout.c`**. Copy those two files into a project and compile `tinylayout.c` with a C99 compiler. The library uses caller-owned nodes and allocates no memory.

# Warning - Vibe-coded AI slop

# Example use

This library has been entirely written by an LLM. The test suite passes but I didn't review it. Use at your own risk.

```c
#include "tinylayout.h"

TL_Node root, child;
tl_node_init(&root);
tl_node_init(&child);
root.style.direction = TL_ROW;
child.style.grow = 1;
tl_node_append(&root, &child);
tl_layout(&root, 320, 120);
/* child.x, child.y, child.width, child.height are now available. */
```

Initialize each node before use. Edit styles freely, then call `tl_layout` again. Detach a child before attaching it to another parent, or call `tl_node_append`, which detaches it for you. `tl_layout(root, NAN, NAN)` computes an unconstrained root size. Coordinates are relative to the parent; dimensions include padding.

The supported style set includes row and column, grow and shrink, point or percent basis, fixed/auto/percent dimensions, point or percent min/max dimensions, point padding and gaps, point or auto margins, normal wrap, main and cross alignment, align-self, wrapped-line alignment, basic absolute positioning with point offsets, and measured leaves. Defaults follow Yoga's native defaults: column, `align-content: flex-start`, `flex-shrink: 0`, and relative positioning. The implementation uses left-to-right layout only. Reverse axes, wrap-reverse, baseline alignment, percentage spacing and offsets, borders, aspect ratio, and other display modes are outside the API.

For a strict standalone C99 check:

```sh
cc -std=c99 -Wall -Wextra -Werror -pedantic -I. tinylayout.c tests/consumer.c -lm -o /tmp/tinylayout-consumer
/tmp/tinylayout-consumer
```

The development-only Yoga comparison requires CMake and a C++20 compiler. Pass a local Yoga v3.2.1 checkout to `sh tests/run_yoga.sh /path/to/yoga`, or explicitly download it with `sh tests/run_yoga.sh --fetch-yoga`. Yoga and the harness are not needed by consumers.
