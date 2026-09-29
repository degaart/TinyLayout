#include "tinylayout.h"
#include <yoga/Yoga.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static YGEdge edge(int i) { static const YGEdge edges[]={YGEdgeLeft,YGEdgeTop,YGEdgeRight,YGEdgeBottom}; return edges[i]; }
static YGJustify justify(TL_Justify a) {
    switch (a) {
    case TL_FLEX_END: return YGJustifyFlexEnd;
    case TL_CENTER: return YGJustifyCenter;
    case TL_SPACE_BETWEEN: return YGJustifySpaceBetween;
    case TL_SPACE_AROUND: return YGJustifySpaceAround;
    case TL_SPACE_EVENLY: return YGJustifySpaceEvenly;
    default: return YGJustifyFlexStart;
    }
}
static YGAlign align(TL_Align a) {
    switch (a) {
    case TL_ALIGN_END: return YGAlignFlexEnd;
    case TL_ALIGN_CENTER: return YGAlignCenter;
    case TL_ALIGN_STRETCH: return YGAlignStretch;
    case TL_ALIGN_AUTO: return YGAlignAuto;
    default: return YGAlignFlexStart;
    }
}
static YGAlign content(TL_AlignContent a) {
    switch (a) {
    case TL_CONTENT_END: return YGAlignFlexEnd;
    case TL_CONTENT_CENTER: return YGAlignCenter;
    case TL_CONTENT_STRETCH: return YGAlignStretch;
    case TL_CONTENT_SPACE_BETWEEN: return YGAlignSpaceBetween;
    case TL_CONTENT_SPACE_AROUND: return YGAlignSpaceAround;
    default: return YGAlignFlexStart;
    }
}
static void dim(YGNodeRef y, TL_Length l, void (*point)(YGNodeRef,float), void (*percent)(YGNodeRef,float)) {
    if (l.unit == TL_POINT) point(y,l.value);
    if (l.unit == TL_PERCENT) percent(y,l.value);
}
static YGSize measure(YGNodeConstRef y, float w, YGMeasureMode wm, float h, YGMeasureMode hm) {
    TL_Node *n = (TL_Node *)YGNodeGetContext(y);
    TL_Size s = n->measure(n->measure_context,w,wm == YGMeasureModeExactly ? TL_EXACTLY : wm == YGMeasureModeAtMost ? TL_AT_MOST : TL_UNDEFINED,
        h,hm == YGMeasureModeExactly ? TL_EXACTLY : hm == YGMeasureModeAtMost ? TL_AT_MOST : TL_UNDEFINED);
    return {s.width,s.height};
}
static YGNodeRef make_yoga(TL_Node *n, YGConfigRef config) {
    YGNodeRef y = YGNodeNewWithConfig(config);
    YGNodeStyleSetFlexDirection(y,n->style.direction == TL_ROW ? YGFlexDirectionRow : YGFlexDirectionColumn);
    YGNodeStyleSetFlexWrap(y,n->style.wrap == TL_WRAP ? YGWrapWrap : YGWrapNoWrap);
    YGNodeStyleSetJustifyContent(y,justify(n->style.justify_content));
    YGNodeStyleSetAlignItems(y,align(n->style.align_items));
    YGNodeStyleSetAlignSelf(y,align(n->style.align_self));
    YGNodeStyleSetAlignContent(y,content(n->style.align_content));
    YGNodeStyleSetPositionType(y,n->style.position == TL_ABSOLUTE ? YGPositionTypeAbsolute : YGPositionTypeRelative);
    YGNodeStyleSetFlexGrow(y,n->style.grow);
    YGNodeStyleSetFlexShrink(y,n->style.shrink);
    if (n->style.basis.unit == TL_POINT) YGNodeStyleSetFlexBasis(y,n->style.basis.value);
    if (n->style.basis.unit == TL_PERCENT) YGNodeStyleSetFlexBasisPercent(y,n->style.basis.value);
    dim(y,n->style.width,YGNodeStyleSetWidth,YGNodeStyleSetWidthPercent);
    dim(y,n->style.height,YGNodeStyleSetHeight,YGNodeStyleSetHeightPercent);
    dim(y,n->style.min_width,YGNodeStyleSetMinWidth,YGNodeStyleSetMinWidthPercent);
    dim(y,n->style.min_height,YGNodeStyleSetMinHeight,YGNodeStyleSetMinHeightPercent);
    dim(y,n->style.max_width,YGNodeStyleSetMaxWidth,YGNodeStyleSetMaxWidthPercent);
    dim(y,n->style.max_height,YGNodeStyleSetMaxHeight,YGNodeStyleSetMaxHeightPercent);
    for (int i=0;i<4;i++) {
        YGNodeStyleSetPadding(y,edge(i),n->style.padding[i]);
        if (n->style.margin[i].unit == TL_AUTO) YGNodeStyleSetMarginAuto(y,edge(i));
        else YGNodeStyleSetMargin(y,edge(i),n->style.margin[i].value);
        if (n->style.inset[i].unit == TL_POINT) YGNodeStyleSetPosition(y,edge(i),n->style.inset[i].value);
    }
    YGNodeStyleSetGap(y,YGGutterColumn,n->style.column_gap);
    YGNodeStyleSetGap(y,YGGutterRow,n->style.row_gap);
    YGNodeSetContext(y,n);
    if (n->measure) YGNodeSetMeasureFunc(y,measure);
    int index=0;
    for (TL_Node *c=n->first_child;c;c=c->next) YGNodeInsertChild(y,make_yoga(c,config),index++);
    return y;
}
static int compare(TL_Node *n, YGNodeRef y, const char *name, int *index) {
    float expected[4]={YGNodeLayoutGetLeft(y),YGNodeLayoutGetTop(y),YGNodeLayoutGetWidth(y),YGNodeLayoutGetHeight(y)};
    float actual[4]={n->x,n->y,n->width,n->height};
    const char *fields[4]={"x","y","width","height"};
    for (int i=0;i<4;i++) if (std::fabs(actual[i]-expected[i]) > 0.02f) {
        std::fprintf(stderr,"%s node %d %s: TinyLayout %.3f, Yoga %.3f\n",name,*index,fields[i],actual[i],expected[i]);
        return 1;
    }
    ++*index;
    int j=0;
    for (TL_Node *c=n->first_child;c;c=c->next) if (compare(c,YGNodeGetChild(y,j++),name,index)) return 1;
    return 0;
}
static void init(TL_Node *n, int count) { for(int i=0;i<count;i++) tl_node_init(n+i); }
static TL_Size measured(void *,float,TL_MeasureMode,float,TL_MeasureMode) { return {43,17}; }
static TL_Size responsive(void *,float w,TL_MeasureMode wm,float,TL_MeasureMode) {
    float width=wm == TL_UNDEFINED ? 80 : std::fmin(80.0f,w);
    return {width,width < 60 ? 30.0f : 15.0f};
}
static unsigned rng=0x1394ab71;
static unsigned random32() { rng ^= rng<<13; rng ^= rng>>17; rng ^= rng<<5; return rng; }
static int run(const char *name,TL_Node *n,float w,float h,YGConfigRef config) {
    YGNodeRef y=make_yoga(n,config);
    tl_layout(n,w,h);
    YGNodeCalculateLayout(y,w,h,YGDirectionLTR);
    int index=0,failed=compare(n,y,name,&index);
    YGNodeFreeRecursive(y);
    return failed;
}
int main() {
    YGConfigRef config=YGConfigNew();
    YGConfigSetPointScaleFactor(config,0);
    TL_Node n[8];
    init(n,8);
    n[0].style.direction=TL_ROW;
    n[0].style.column_gap=7;
    n[0].style.padding[TL_LEFT]=9;
    n[0].style.padding[TL_TOP]=5;
    n[1].style.width=tl_point(30); n[1].style.height=tl_point(20);
    n[2].style.grow=1; n[2].style.min_width=tl_point(20);
    n[3].style.width=tl_percent(25); n[3].style.height=tl_point(35);
    for(int i=1;i<4;i++) tl_node_append(&n[0],&n[i]);
    if(run("row grow percent",n,200,90,config)) return 1;
    init(n,8);
    n[0].style.direction=TL_ROW; n[0].style.wrap=TL_WRAP;
    n[0].style.column_gap=5; n[0].style.row_gap=8;
    for(int i=1;i<5;i++) { n[i].style.width=tl_point(45); n[i].style.height=tl_point(20); tl_node_append(&n[0],&n[i]); }
    if(run("wrap",n,105,100,config)) return 1;
    init(n,8);
    n[0].style.direction=TL_ROW; n[0].style.justify_content=TL_CENTER;
    n[1].style.width=tl_point(20); n[1].style.height=tl_point(20);
    n[2].style.width=tl_point(20); n[2].style.height=tl_point(20);
    n[2].style.margin[TL_LEFT]=tl_auto();
    tl_node_append(&n[0],&n[1]); tl_node_append(&n[0],&n[2]);
    if(run("auto margin",n,150,60,config)) return 1;
    init(n,8);
    n[0].style.padding[TL_LEFT]=4; n[0].style.padding[TL_TOP]=6;
    n[1].style.position=TL_ABSOLUTE; n[1].style.inset[TL_RIGHT]=tl_point(7); n[1].style.inset[TL_BOTTOM]=tl_point(9);
    n[1].style.width=tl_point(20); n[1].style.height=tl_point(15);
    tl_node_append(&n[0],&n[1]);
    if(run("absolute",n,120,80,config)) return 1;
    init(n,8);
    n[0].style.direction=TL_ROW; n[1].measure=measured; n[2].style.grow=1;
    tl_node_append(&n[0],&n[1]); tl_node_append(&n[0],&n[2]);
    if(run("measured",n,160,40,config)) return 1;
    init(n,8);
    n[0].style.direction=TL_COLUMN; n[0].style.padding[TL_LEFT]=7;
    n[0].style.row_gap=6;
    n[1].style.direction=TL_ROW; n[1].style.height=tl_point(35);
    n[2].style.width=tl_point(30); n[2].style.height=tl_point(12);
    n[3].style.grow=1;
    n[4].style.height=tl_point(18);
    tl_node_append(&n[0],&n[1]); tl_node_append(&n[1],&n[2]); tl_node_append(&n[1],&n[3]); tl_node_append(&n[0],&n[4]);
    if(run("nested",n,180,100,config)) return 1;
    n[4].style.height=tl_point(25);
    tl_node_detach(&n[3]); tl_node_append(&n[0],&n[3]);
    if(run("edited reparented",n,180,100,config)) return 1;
    init(n,8);
    n[0].style.direction=TL_ROW;
    for(int i=1;i<4;i++) { n[i].style.width=tl_point(60); n[i].style.shrink=1; n[i].style.height=tl_point(20); tl_node_append(&n[0],&n[i]); }
    n[1].style.min_width=tl_point(55);
    if(run("shrink minimum",n,120,60,config)) return 1;
    init(n,8);
    n[0].style.direction=TL_ROW; n[0].style.wrap=TL_WRAP; n[0].style.align_content=TL_CONTENT_SPACE_AROUND;
    n[0].style.align_items=TL_ALIGN_CENTER;
    for(int i=1;i<5;i++) { n[i].style.width=tl_point(45); n[i].style.height=tl_point(20); tl_node_append(&n[0],&n[i]); }
    if(run("wrapped alignment",n,100,100,config)) return 1;
    init(n,8);
    n[1].style.direction=TL_ROW; n[1].style.wrap=TL_WRAP;
    for(int i=2;i<6;i++) { n[i].style.width=tl_point(45); n[i].style.height=tl_point(20); tl_node_append(&n[1],&n[i]); }
    tl_node_append(&n[0],&n[1]);
    if(run("nested wrap",n,100,100,config)) return 1;
    init(n,8);
    n[0].style.direction=TL_ROW; n[0].style.align_items=TL_ALIGN_END;
    n[1].style.basis=tl_percent(40); n[1].style.grow=1;
    n[1].style.min_width=tl_percent(45); n[1].style.max_width=tl_point(120);
    n[1].style.height=tl_point(15); n[1].style.align_self=TL_ALIGN_CENTER;
    n[2].style.basis=tl_point(30); n[2].style.grow=1;
    n[2].style.height=tl_point(20);
    tl_node_append(&n[0],&n[1]); tl_node_append(&n[0],&n[2]);
    if(run("basis limits align self",n,200,80,config)) return 1;
    init(n,8);
    n[0].style.padding[TL_LEFT]=10; n[0].style.padding[TL_RIGHT]=6;
    n[1].style.position=TL_ABSOLUTE;
    n[1].style.inset[TL_LEFT]=tl_point(8); n[1].style.inset[TL_RIGHT]=tl_point(12);
    n[1].style.inset[TL_TOP]=tl_point(4); n[1].style.inset[TL_BOTTOM]=tl_point(5);
    tl_node_append(&n[0],&n[1]);
    if(run("absolute opposing offsets",n,100,70,config)) return 1;
    init(n,8);
    n[0].style.direction=TL_ROW;
    n[1].measure=responsive; n[1].style.shrink=1;
    tl_node_append(&n[0],&n[1]);
    if(run("responsive measure",n,45,70,config)) return 1;
    for(int t=0;t<100;t++) {
        init(n,8); n[0].style.direction=TL_ROW;
        n[0].style.justify_content=(TL_Justify)(random32()%6);
        n[0].style.align_items=(TL_Align)(1+random32()%4);
        n[0].style.column_gap=(float)(random32()%8);
        for(int i=1;i<5;i++) {
            n[i].style.width=tl_point((float)(10+random32()%40));
            n[i].style.height=tl_point((float)(10+random32()%30));
            n[i].style.grow=(float)(random32()%3);
            tl_node_append(&n[0],&n[i]);
        }
        char label[40]; std::snprintf(label,sizeof(label),"generated seed 1394ab71 case %d",t);
        if(run(label,n,180,80,config)) return 1;
    }
    YGConfigFree(config);
    std::puts("Yoga comparison passed");
}
