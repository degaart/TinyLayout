#include "tinylayout.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

static float tl_max(float a, float b) { return a > b ? a : b; }
static float tl_min(float a, float b) { return a < b ? a : b; }
static int tl_known(float v) { return isfinite(v); }
static float tl_resolve(TL_Length v, float reference) {
    if (v.unit == TL_POINT) return v.value;
    if (v.unit == TL_PERCENT && tl_known(reference)) return reference * v.value / 100.0f;
    return NAN;
}
static float tl_margin(TL_Length v) { return v.unit == TL_POINT ? v.value : 0.0f; }
static float tl_bound(float value, TL_Length lo, TL_Length hi, float reference) {
    float a = tl_resolve(lo, reference), b = tl_resolve(hi, reference);
    if (tl_known(b)) value = tl_min(value, b);
    if (tl_known(a)) value = tl_max(value, a);
    return value;
}
static float tl_pad(const TL_Node *n, int horizontal) {
    return horizontal ? n->style.padding[TL_LEFT] + n->style.padding[TL_RIGHT]
                      : n->style.padding[TL_TOP] + n->style.padding[TL_BOTTOM];
}
static float tl_edge_margin(const TL_Node *n, int horizontal) {
    return horizontal ? tl_margin(n->style.margin[TL_LEFT]) + tl_margin(n->style.margin[TL_RIGHT])
                      : tl_margin(n->style.margin[TL_TOP]) + tl_margin(n->style.margin[TL_BOTTOM]);
}

TL_Length tl_auto(void) { TL_Length x = { TL_AUTO, 0 }; return x; }
TL_Length tl_point(float value) { TL_Length x = { TL_POINT, value }; return x; }
TL_Length tl_percent(float value) { TL_Length x = { TL_PERCENT, value }; return x; }

void tl_node_init(TL_Node *n) {
    int i;
    memset(n, 0, sizeof(*n));
    n->style.direction = TL_COLUMN;
    n->style.align_items = TL_ALIGN_STRETCH;
    n->style.align_content = TL_CONTENT_START;
    for (i = 0; i < 4; ++i) {
        n->style.margin[i] = tl_point(0);
        n->style.inset[i] = tl_auto();
    }
}
void tl_node_detach(TL_Node *n) {
    TL_Node *p = n->parent;
    if (!p) return;
    if (n->previous) n->previous->next = n->next; else p->first_child = n->next;
    if (n->next) n->next->previous = n->previous; else p->last_child = n->previous;
    n->parent = n->previous = n->next = NULL;
}
void tl_node_append(TL_Node *p, TL_Node *n) {
    TL_Node *a;
    if (!p || !n || p == n) return;
    for (a = p; a; a = a->parent) if (a == n) return;
    tl_node_detach(n);
    n->parent = p;
    n->previous = p->last_child;
    if (p->last_child) p->last_child->next = n; else p->first_child = n;
    p->last_child = n;
}

/* Intrinsic border-box size. A definite style size takes precedence over content. */
static TL_Size tl_preferred(TL_Node *n, float parent_w, float parent_h) {
    TL_Size out = {0, 0}, child;
    float w = tl_resolve(n->style.width, parent_w);
    float h = tl_resolve(n->style.height, parent_h);
    float inner_w = tl_known(w) ? tl_max(0, w - tl_pad(n, 1)) : NAN;
    float inner_h = tl_known(h) ? tl_max(0, h - tl_pad(n, 0)) : NAN;
    TL_Node *c;
    unsigned count = 0;
    if (n->measure) {
        TL_Size m = n->measure(n->measure_context,
            tl_known(inner_w) ? inner_w : NAN, tl_known(inner_w) ? TL_EXACTLY : TL_UNDEFINED,
            tl_known(inner_h) ? inner_h : NAN, tl_known(inner_h) ? TL_EXACTLY : TL_UNDEFINED);
        out.width = tl_max(0, m.width);
        out.height = tl_max(0, m.height);
    } else for (c = n->first_child; c; c = c->next) {
        float cm, cx;
        if (c->style.position == TL_ABSOLUTE) continue;
        child = tl_preferred(c, inner_w, inner_h);
        cm = (n->style.direction == TL_ROW ? child.width : child.height)
           + tl_edge_margin(c, n->style.direction == TL_ROW);
        cx = (n->style.direction == TL_ROW ? child.height : child.width)
           + tl_edge_margin(c, n->style.direction != TL_ROW);
        if (n->style.direction == TL_ROW) {
            out.width += cm;
            out.height = tl_max(out.height, cx);
        } else {
            out.height += cm;
            out.width = tl_max(out.width, cx);
        }
        ++count;
    }
    if (count > 1) {
        if (n->style.direction == TL_ROW) out.width += (count - 1) * n->style.column_gap;
        else out.height += (count - 1) * n->style.row_gap;
    }
    out.width += tl_pad(n, 1);
    out.height += tl_pad(n, 0);
    if (tl_known(w)) out.width = w;
    if (tl_known(h)) out.height = h;
    out.width = tl_bound(out.width, n->style.min_width, n->style.max_width, parent_w);
    out.height = tl_bound(out.height, n->style.min_height, n->style.max_height, parent_h);
    out.width = tl_max(out.width, tl_pad(n, 1));
    out.height = tl_max(out.height, tl_pad(n, 0));
    return out;
}

static float tl_axis_size(TL_Size s, int row) { return row ? s.width : s.height; }
static float tl_cross_size(TL_Size s, int row) { return row ? s.height : s.width; }
static TL_Length tl_cross_length(const TL_Node *n, int row) { return row ? n->style.height : n->style.width; }
static TL_Length tl_axis_min(const TL_Node *n, int row) { return row ? n->style.min_width : n->style.min_height; }
static TL_Length tl_axis_max(const TL_Node *n, int row) { return row ? n->style.max_width : n->style.max_height; }
static float tl_main_margin(const TL_Node *n, int row, int end) {
    return tl_margin(n->style.margin[row ? (end ? TL_RIGHT : TL_LEFT) : (end ? TL_BOTTOM : TL_TOP)]);
}
static float tl_cross_margin(const TL_Node *n, int row, int end) {
    return tl_margin(n->style.margin[row ? (end ? TL_BOTTOM : TL_TOP) : (end ? TL_RIGHT : TL_LEFT)]);
}
/* Cross size needed when a wrapping container receives a definite main size. */
static float tl_wrapped_cross(TL_Node *n, float main_size, float parent_w, float parent_h) {
    int row = n->style.direction == TL_ROW;
    float inner_main = tl_max(0, main_size - tl_pad(n, row));
    float gap_main = row ? n->style.column_gap : n->style.row_gap;
    float gap_cross = row ? n->style.row_gap : n->style.column_gap;
    float used = 0, line_cross = 0, total_cross = 0;
    unsigned count = 0;
    TL_Node *c;
    for (c = n->first_child; c; c = c->next) {
        TL_Size p;
        float main, cross;
        if (c->style.position == TL_ABSOLUTE) continue;
        p = tl_preferred(c, row ? inner_main : parent_w, row ? parent_h : inner_main);
        main = tl_resolve(c->style.basis, inner_main);
        if (!tl_known(main)) main = tl_axis_size(p, row);
        main = tl_bound(main, tl_axis_min(c, row), tl_axis_max(c, row), inner_main);
        main += tl_main_margin(c, row, 0) + tl_main_margin(c, row, 1);
        cross = tl_cross_size(p, row) + tl_cross_margin(c, row, 0) + tl_cross_margin(c, row, 1);
        if (count && used + gap_main + main > inner_main) {
            total_cross += line_cross + gap_cross;
            used = line_cross = 0;
            count = 0;
        }
        used += (count ? gap_main : 0) + main;
        line_cross = tl_max(line_cross, cross);
        ++count;
    }
    if (count) total_cross += line_cross;
    return total_cross + tl_pad(n, !row);
}
static int tl_auto_main(const TL_Node *n, int row, int end) {
    return n->style.margin[row ? (end ? TL_RIGHT : TL_LEFT) : (end ? TL_BOTTOM : TL_TOP)].unit == TL_AUTO;
}
static int tl_auto_cross(const TL_Node *n, int row, int end) {
    return n->style.margin[row ? (end ? TL_BOTTOM : TL_TOP) : (end ? TL_RIGHT : TL_LEFT)].unit == TL_AUTO;
}

static void tl_layout_node(TL_Node *n, float width, float height, float parent_w, float parent_h) {
    TL_Size pref;
    TL_Node *c;
    float inner_w, inner_h, inner_main, inner_cross, gap_main, gap_cross;
    float cross_total = 0, max_main = 0, cross_free, line_start, line_gap = 0;
    unsigned line_count = 0, line_id = 0;
    int row = n->style.direction == TL_ROW;
    if (!tl_known(width) || !tl_known(height)) {
        pref = tl_preferred(n, parent_w, parent_h);
        if (!tl_known(width)) width = pref.width;
        if (!tl_known(height)) height = pref.height;
    }
    width = tl_bound(width, n->style.min_width, n->style.max_width, parent_w);
    height = tl_bound(height, n->style.min_height, n->style.max_height, parent_h);
    n->width = tl_max(width, tl_pad(n, 1));
    n->height = tl_max(height, tl_pad(n, 0));
    inner_w = tl_max(0, n->width - tl_pad(n, 1));
    inner_h = tl_max(0, n->height - tl_pad(n, 0));
    inner_main = row ? inner_w : inner_h;
    inner_cross = row ? inner_h : inner_w;
    gap_main = row ? n->style.column_gap : n->style.row_gap;
    gap_cross = row ? n->style.row_gap : n->style.column_gap;

    /* Assign lines before distributing free space. */
    {
        float used = 0;
        unsigned count = 0;
        for (c = n->first_child; c; c = c->next) {
            float basis, outer, ref_main = row ? inner_w : inner_h;
            TL_Size p;
            if (c->style.position == TL_ABSOLUTE) continue;
            p = tl_preferred(c, inner_w, inner_h);
            basis = tl_resolve(c->style.basis, ref_main);
            if (!tl_known(basis)) {
                basis = tl_axis_size(p, row);
                if (c->style.wrap == TL_WRAP && c->style.direction != n->style.direction) {
                    TL_Align a = c->style.align_self == TL_ALIGN_AUTO ? n->style.align_items : c->style.align_self;
                    float wrap_main = tl_resolve(tl_cross_length(c, row), inner_cross);
                    if (!tl_known(wrap_main) && a == TL_ALIGN_STRETCH &&
                        !tl_auto_cross(c, row, 0) && !tl_auto_cross(c, row, 1))
                        wrap_main = tl_max(0, inner_cross - tl_cross_margin(c, row, 0) - tl_cross_margin(c, row, 1));
                    if (tl_known(wrap_main)) basis = tl_wrapped_cross(c, wrap_main, inner_w, inner_h);
                }
            }
            basis = tl_bound(basis, tl_axis_min(c, row), tl_axis_max(c, row), ref_main);
            basis = tl_max(basis, tl_pad(c, row));
            c->_basis = c->_main = basis;
            outer = basis + tl_main_margin(c, row, 0) + tl_main_margin(c, row, 1);
            if (n->style.wrap == TL_WRAP && count && used + gap_main + outer > inner_main) {
                ++line_id; used = 0; count = 0;
            }
            c->_line = line_id;
            used += (count ? gap_main : 0) + outer;
            ++count;
        }
        line_count = count ? line_id + 1 : 0;
    }

    for (line_id = 0; line_id < line_count; ++line_id) {
        float total = 0, line_cross = 0;
        unsigned count = 0, auto_count = 0;
        for (c = n->first_child; c; c = c->next) if (c->style.position != TL_ABSOLUTE && c->_line == line_id) {
            total += c->_main + tl_main_margin(c, row, 0) + tl_main_margin(c, row, 1);
            c->_outer_main = 0;
            auto_count += tl_auto_main(c, row, 0) + tl_auto_main(c, row, 1);
            ++count;
        }
        total += count > 1 ? (count - 1) * gap_main : 0;
        /* Freeze items that reach min/max, then redistribute the remainder. */
        {
            unsigned pass;
            for (pass = 0; pass <= count; ++pass) {
                float base = (count > 1 ? (count - 1) * gap_main : 0), weight = 0, free_space;
                int changed = 0;
                for (c = n->first_child; c; c = c->next) if (c->style.position != TL_ABSOLUTE && c->_line == line_id) {
                    base += tl_main_margin(c, row, 0) + tl_main_margin(c, row, 1);
                    base += c->_outer_main ? c->_main : c->_basis;
                }
                free_space = inner_main - base;
                for (c = n->first_child; c; c = c->next) if (c->style.position != TL_ABSOLUTE && c->_line == line_id && !c->_outer_main)
                    weight += free_space >= 0 ? tl_max(0, c->style.grow) : tl_max(0, c->style.shrink) * c->_basis;
                for (c = n->first_child; c; c = c->next) if (c->style.position != TL_ABSOLUTE && c->_line == line_id && !c->_outer_main) {
                    float factor = free_space >= 0 ? tl_max(0, c->style.grow) : tl_max(0, c->style.shrink) * c->_basis;
                    float proposed = c->_basis + (weight > 0 ? free_space * factor / weight : 0);
                    float bounded = tl_max(tl_pad(c, row), tl_bound(tl_max(0, proposed), tl_axis_min(c, row), tl_axis_max(c, row), inner_main));
                    c->_main = bounded;
                    if (fabsf(bounded - proposed) > 0.0001f) { c->_outer_main = 1; changed = 1; }
                }
                if (!changed) break;
            }
        }
        for (c = n->first_child; c; c = c->next) if (c->style.position != TL_ABSOLUTE && c->_line == line_id) {
            {
                TL_Size p = tl_preferred(c, inner_w, inner_h);
                float cross = tl_resolve(tl_cross_length(c, row), inner_cross);
                if (!tl_known(cross)) cross = tl_cross_size(p, row);
                cross = tl_bound(cross, tl_axis_min(c, !row), tl_axis_max(c, !row), inner_cross);
                c->_cross = tl_max(cross, tl_pad(c, !row));
            }
            line_cross = tl_max(line_cross, c->_cross + tl_cross_margin(c, row, 0) + tl_cross_margin(c, row, 1));
        }
        /* A single non-wrapped line fills the container's cross axis. */
        if (n->style.wrap == TL_NO_WRAP) line_cross = inner_cross;
        for (c = n->first_child; c; c = c->next) if (c->style.position != TL_ABSOLUTE && c->_line == line_id)
            c->_line_cross = line_cross;
        cross_total += line_cross;
        if (line_id) cross_total += gap_cross;
        max_main = tl_max(max_main, total);
    }
    (void)max_main;
    cross_free = inner_cross - cross_total;
    line_start = row ? n->style.padding[TL_TOP] : n->style.padding[TL_LEFT];
    if (cross_free > 0) {
        switch (n->style.align_content) {
        case TL_CONTENT_END: line_start += cross_free; break;
        case TL_CONTENT_CENTER: line_start += cross_free / 2; break;
        case TL_CONTENT_SPACE_BETWEEN: if (line_count > 1) line_gap = cross_free / (line_count - 1); break;
        case TL_CONTENT_SPACE_AROUND: if (line_count) { line_gap = cross_free / line_count; line_start += line_gap / 2; } break;
        default: break;
        }
    }
    for (line_id = 0; line_id < line_count; ++line_id) {
        float used = 0, free_main, cursor, between = gap_main, line_cross = 0;
        unsigned count = 0, auto_count = 0;
        for (c = n->first_child; c; c = c->next) if (c->style.position != TL_ABSOLUTE && c->_line == line_id) {
            used += c->_main + tl_main_margin(c, row, 0) + tl_main_margin(c, row, 1);
            line_cross = c->_line_cross;
            auto_count += tl_auto_main(c, row, 0) + tl_auto_main(c, row, 1);
            ++count;
        }
        used += count > 1 ? (count - 1) * gap_main : 0;
        if (n->style.align_content == TL_CONTENT_STRETCH && cross_free > 0 && line_count)
            line_cross += cross_free / line_count;
        free_main = tl_max(0, inner_main - used);
        cursor = row ? n->style.padding[TL_LEFT] : n->style.padding[TL_TOP];
        if (!auto_count) switch (n->style.justify_content) {
        case TL_FLEX_END: cursor += free_main; break;
        case TL_CENTER: cursor += free_main / 2; break;
        case TL_SPACE_BETWEEN: if (count > 1) between += free_main / (count - 1); break;
        case TL_SPACE_AROUND: if (count) { between += free_main / count; cursor += free_main / (2 * count); } break;
        case TL_SPACE_EVENLY: between += free_main / (count + 1); cursor += free_main / (count + 1); break;
        default: break;
        }
        for (c = n->first_child; c; c = c->next) if (c->style.position != TL_ABSOLUTE && c->_line == line_id) {
            TL_Align align = c->style.align_self == TL_ALIGN_AUTO ? n->style.align_items : c->style.align_self;
            float before = tl_cross_margin(c, row, 0), after = tl_cross_margin(c, row, 1);
            float cross = c->_cross, cross_pos, remaining;
            float lead = tl_main_margin(c, row, 0);
            float trail = tl_main_margin(c, row, 1);
            if (auto_count) {
                if (tl_auto_main(c, row, 0)) lead += free_main / auto_count;
                if (tl_auto_main(c, row, 1)) trail += free_main / auto_count;
            }
            cursor += lead;
            if (align == TL_ALIGN_STRETCH && tl_cross_length(c, row).unit == TL_AUTO &&
                !tl_auto_cross(c, row, 0) && !tl_auto_cross(c, row, 1))
                cross = tl_max(tl_pad(c, !row), tl_bound(tl_max(0, line_cross - before - after),
                    tl_axis_min(c, !row), tl_axis_max(c, !row), inner_cross));
            remaining = tl_max(0, line_cross - cross - before - after);
            cross_pos = line_start + before;
            if (tl_auto_cross(c, row, 0) && tl_auto_cross(c, row, 1)) cross_pos += remaining / 2;
            else if (tl_auto_cross(c, row, 0)) cross_pos += remaining;
            else if (!tl_auto_cross(c, row, 1)) {
                if (align == TL_ALIGN_END) cross_pos += remaining;
                if (align == TL_ALIGN_CENTER) cross_pos += remaining / 2;
            }
            c->x = row ? cursor : cross_pos;
            c->y = row ? cross_pos : cursor;
            tl_layout_node(c, row ? c->_main : cross, row ? cross : c->_main, inner_w, inner_h);
            cursor += c->_main + trail + between;
        }
        line_start += line_cross + gap_cross + line_gap;
    }
    /* Absolute children do not take part in flex lines. */
    for (c = n->first_child; c; c = c->next) if (c->style.position == TL_ABSOLUTE) {
        float left = tl_resolve(c->style.inset[TL_LEFT], inner_w);
        float right = tl_resolve(c->style.inset[TL_RIGHT], inner_w);
        float top = tl_resolve(c->style.inset[TL_TOP], inner_h);
        float bottom = tl_resolve(c->style.inset[TL_BOTTOM], inner_h);
        float cw = tl_resolve(c->style.width, inner_w), ch = tl_resolve(c->style.height, inner_h);
        TL_Size p = tl_preferred(c, inner_w, inner_h);
        if (!tl_known(cw)) cw = tl_known(left) && tl_known(right) ? tl_max(0, n->width-left-right) : p.width;
        if (!tl_known(ch)) ch = tl_known(top) && tl_known(bottom) ? tl_max(0, n->height-top-bottom) : p.height;
        cw = tl_bound(cw, c->style.min_width, c->style.max_width, inner_w);
        ch = tl_bound(ch, c->style.min_height, c->style.max_height, inner_h);
        c->x = tl_known(left) ? left : tl_known(right) ? n->width-right-cw : n->style.padding[TL_LEFT];
        c->y = tl_known(top) ? top : tl_known(bottom) ? n->height-bottom-ch : n->style.padding[TL_TOP];
        tl_layout_node(c, cw, ch, inner_w, inner_h);
    }
}

void tl_layout(TL_Node *root, float available_width, float available_height) {
    TL_Size p;
    float w, h;
    if (!root) return;
    p = tl_preferred(root, available_width, available_height);
    w = tl_known(available_width) ? available_width : p.width;
    h = tl_known(available_height) ? available_height : p.height;
    if (root->style.wrap == TL_WRAP && root->style.direction == TL_ROW &&
        tl_known(w) && !tl_known(available_height) && root->style.height.unit == TL_AUTO)
        h = tl_wrapped_cross(root, w, available_width, available_height);
    if (root->style.wrap == TL_WRAP && root->style.direction == TL_COLUMN &&
        tl_known(h) && !tl_known(available_width) && root->style.width.unit == TL_AUTO)
        w = tl_wrapped_cross(root, h, available_width, available_height);
    root->x = root->y = 0;
    tl_layout_node(root, w, h, available_width, available_height);
}
