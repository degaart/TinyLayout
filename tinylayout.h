#ifndef TINYLAYOUT_H
#define TINYLAYOUT_H

/* TinyLayout: copy this file and tinylayout.c into a C99 project.
   All coordinates are relative to the parent. NAN means an unconstrained
   root dimension. Only left-to-right row/column layout is supported. */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { TL_AUTO, TL_POINT, TL_PERCENT } TL_Unit;
typedef struct { TL_Unit unit; float value; } TL_Length;
typedef enum { TL_ROW, TL_COLUMN } TL_Direction;
typedef enum { TL_NO_WRAP, TL_WRAP } TL_Wrap;
typedef enum {
    TL_FLEX_START, TL_FLEX_END, TL_CENTER, TL_SPACE_BETWEEN,
    TL_SPACE_AROUND, TL_SPACE_EVENLY
} TL_Justify;
typedef enum { TL_ALIGN_AUTO, TL_ALIGN_START, TL_ALIGN_END, TL_ALIGN_CENTER,
               TL_ALIGN_STRETCH } TL_Align;
typedef enum { TL_CONTENT_START, TL_CONTENT_END, TL_CONTENT_CENTER,
               TL_CONTENT_STRETCH, TL_CONTENT_SPACE_BETWEEN,
               TL_CONTENT_SPACE_AROUND } TL_AlignContent;
typedef enum { TL_RELATIVE, TL_ABSOLUTE } TL_Position;
typedef enum { TL_LEFT, TL_TOP, TL_RIGHT, TL_BOTTOM } TL_Edge;
typedef enum { TL_EXACTLY, TL_AT_MOST, TL_UNDEFINED } TL_MeasureMode;

typedef TL_Length TL_Edges[4];
typedef struct { float width, height; } TL_Size;
typedef TL_Size (*TL_Measure)(void *context, float width, TL_MeasureMode width_mode,
                              float height, TL_MeasureMode height_mode);

typedef struct {
    TL_Direction direction;
    TL_Wrap wrap;
    TL_Justify justify_content;
    TL_Align align_items, align_self;
    TL_AlignContent align_content;
    TL_Position position;
    float grow, shrink;
    TL_Length basis, width, height, min_width, min_height, max_width, max_height;
    float padding[4];                 /* points; left, top, right, bottom */
    TL_Edges margin;                  /* points or auto */
    TL_Edges inset;                   /* points or auto */
    float column_gap, row_gap;        /* points */
} TL_Style;

typedef struct TL_Node TL_Node;
struct TL_Node {
    TL_Style style;
    TL_Node *parent, *first_child, *last_child, *previous, *next;
    TL_Measure measure;
    void *measure_context;
    float x, y, width, height;
    /* Layout scratch: callers should not change these fields. */
    float _basis, _main, _cross, _outer_main, _line_cross;
    unsigned _line;
};

TL_Length tl_auto(void);
TL_Length tl_point(float value);
TL_Length tl_percent(float value); /* 100 means 100 percent */
void tl_node_init(TL_Node *node);
/* A child can belong to only one parent. Detach before attaching elsewhere. */
void tl_node_append(TL_Node *parent, TL_Node *child);
void tl_node_detach(TL_Node *child);
/* Available root width/height are exact outer sizes when finite. */
void tl_layout(TL_Node *root, float available_width, float available_height);

#ifdef __cplusplus
}
#endif
#endif
