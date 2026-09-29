#include <windows.h>
#include "tinylayout.h"

enum {
    FIELD_NAME,
    FIELD_EMAIL,
    FIELD_PHONE,
    FIELD_ADDRESS,
    FIELD_NOTES,
    FIELD_COUNT
};

enum { ID_SAVE = 100, ID_DISCARD };

typedef struct {
    TL_Node layout;
    HWND hwnd; /* NULL for layout-only containers. */
} Item;

typedef struct {
    Item root;
    Item rows[FIELD_COUNT];
    Item labels[FIELD_COUNT];
    Item edits[FIELD_COUNT];
    Item actions;
    Item save;
    Item discard;
} Form;

static Form form;

static HWND make_control(HWND parent, DWORD ex_style, const char *class_name,
                         const char *caption, DWORD style, int id, HINSTANCE instance)
{
    HWND control = CreateWindowExA(ex_style, class_name, caption,
                                   WS_CHILD | WS_VISIBLE | style,
                                   0, 0, 0, 0, parent, (HMENU)(INT_PTR)id,
                                   instance, NULL);
    if (control != NULL) {
        SendMessageA(control, WM_SETFONT,
                     (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    }
    return control;
}

static int make_form(HWND window, HINSTANCE instance)
{
    static const char *labels[FIELD_COUNT] = {
        "Name", "Email", "Phone", "Address", "Notes"
    };
    int i;

    tl_node_init(&form.root.layout);
    form.root.layout.style.direction = TL_COLUMN;
    form.root.layout.style.row_gap = 10;
    for (i = 0; i < 4; ++i) form.root.layout.style.padding[i] = 16;

    for (i = 0; i < FIELD_COUNT; ++i) {
        DWORD edit_style = WS_TABSTOP;
        int multiline = i == FIELD_ADDRESS || i == FIELD_NOTES;

        tl_node_init(&form.rows[i].layout);
        tl_node_init(&form.labels[i].layout);
        tl_node_init(&form.edits[i].layout);

        form.rows[i].layout.style.direction = TL_ROW;
        form.rows[i].layout.style.column_gap = 10;
        form.rows[i].layout.style.height = tl_point(
            i == FIELD_ADDRESS ? 76 : i == FIELD_NOTES ? 80 : 26);
        if (i == FIELD_NOTES) form.rows[i].layout.style.grow = 1;

        form.labels[i].layout.style.width = tl_point(90);
        if (multiline) {
            form.labels[i].layout.style.height = tl_point(20);
            form.labels[i].layout.style.align_self = TL_ALIGN_START;
            edit_style |= ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL;
        } else {
            edit_style |= ES_AUTOHSCROLL;
        }
        form.edits[i].layout.style.grow = 1;
        form.edits[i].layout.style.shrink = 1;

        tl_node_append(&form.root.layout, &form.rows[i].layout);
        tl_node_append(&form.rows[i].layout, &form.labels[i].layout);
        tl_node_append(&form.rows[i].layout, &form.edits[i].layout);

        form.labels[i].hwnd = make_control(window, 0, "STATIC", labels[i],
            multiline ? SS_LEFT : SS_LEFT | SS_CENTERIMAGE, 0, instance);
        form.edits[i].hwnd = make_control(window, WS_EX_CLIENTEDGE, "EDIT", "",
            edit_style, 10 + i, instance);
        if (form.labels[i].hwnd == NULL || form.edits[i].hwnd == NULL) return 0;
    }

    tl_node_init(&form.actions.layout);
    tl_node_init(&form.save.layout);
    tl_node_init(&form.discard.layout);
    form.actions.layout.style.direction = TL_ROW;
    form.actions.layout.style.justify_content = TL_FLEX_END;
    form.actions.layout.style.column_gap = 8;
    form.actions.layout.style.height = tl_point(30);
    form.save.layout.style.width = tl_point(86);
    form.discard.layout.style.width = tl_point(86);
    tl_node_append(&form.root.layout, &form.actions.layout);
    tl_node_append(&form.actions.layout, &form.save.layout);
    tl_node_append(&form.actions.layout, &form.discard.layout);

    form.save.hwnd = make_control(window, 0, "BUTTON", "Save",
                                  WS_TABSTOP | BS_PUSHBUTTON, ID_SAVE, instance);
    form.discard.hwnd = make_control(window, 0, "BUTTON", "Discard",
                                     WS_TABSTOP | BS_PUSHBUTTON, ID_DISCARD, instance);
    return form.save.hwnd != NULL && form.discard.hwnd != NULL;
}

static int pixel(float value) { return (int)(value + 0.5f); }

static void place_children(TL_Node *parent, float parent_x, float parent_y)
{
    TL_Node *node;
    for (node = parent->first_child; node != NULL; node = node->next) {
        Item *item = (Item *)node; /* TL_Node is the first member of Item. */
        float x = parent_x + node->x;
        float y = parent_y + node->y;
        if (item->hwnd != NULL) {
            MoveWindow(item->hwnd, pixel(x), pixel(y),
                       pixel(node->width), pixel(node->height), TRUE);
        }
        place_children(node, x, y);
    }
}

static void layout_form(HWND window)
{
    RECT client;
    GetClientRect(window, &client);
    tl_layout(&form.root.layout, (float)(client.right - client.left),
              (float)(client.bottom - client.top));
    place_children(&form.root.layout, 0, 0);
}

static LRESULT CALLBACK window_proc(HWND window, UINT message,
                                    WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_CREATE:
        if (!make_form(window, ((CREATESTRUCTA *)lparam)->hInstance)) return -1;
        layout_form(window);
        return 0;
    case WM_SIZE:
        if (form.root.layout.first_child != NULL) layout_form(window);
        return 0;
    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lparam)->ptMinTrackSize.x = 440;
        ((MINMAXINFO *)lparam)->ptMinTrackSize.y = 420;
        return 0;
    case WM_COMMAND:
        if (LOWORD(wparam) == ID_SAVE || LOWORD(wparam) == ID_DISCARD) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(window, message, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous,
                   LPSTR command_line, int show_command)
{
    const char class_name[] = "TinyLayoutSimpleForm";
    WNDCLASSEXA window_class = {0};
    HWND window;
    MSG message;
    (void)previous;
    (void)command_line;

    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    window_class.lpszClassName = class_name;
    if (RegisterClassExA(&window_class) == 0) return 1;

    window = CreateWindowExA(0, class_name, "TinyLayout Data Entry Form",
                           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                           640, 500, NULL, NULL, instance, NULL);
    if (window == NULL) return 1;
    ShowWindow(window, show_command);
    UpdateWindow(window);
    while (GetMessageA(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return (int)message.wParam;
}
