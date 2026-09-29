#include "tinylayout.h"
#include <math.h>

int main(void) {
    TL_Node root, child;
    tl_node_init(&root);
    tl_node_init(&child);
    root.style.direction = TL_ROW;
    child.style.width = tl_point(25);
    child.style.height = tl_point(10);
    tl_node_append(&root, &child);
    tl_layout(&root, 100, 40);
    if (fabsf(child.width - 25) > 0.001f || fabsf(child.height - 10) > 0.001f) return 1;
    child.style.height = tl_auto();
    child.style.grow = 1;
    tl_layout(&root, 100, 40);
    if (fabsf(child.width - 100) > 0.001f || fabsf(child.height - 40) > 0.001f) return 2;
    tl_node_detach(&child);
    tl_layout(&root, NAN, NAN);
    return root.width == 0 && root.height == 0 ? 0 : 3;
}
