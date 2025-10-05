/**
 * @file lv_keyboard_t9.c
 * @brief Custom LVGL T9 keyboard widget implementation.
 *
 * This widget provides a T9-style keyboard for LVGL, supporting cycling through characters,
 * symbol popovers on long-press, and helper buttons for space, backspace, OK, close, and mode switching.
 * Usage: Call lv_keyboard_t9_init(parent, ta) to create and link the keyboard to a textarea.
 */
#include "lvgl.h"
#include "lv_keyboard_t9.h"

// Buttonmatrix definitions
#define T9_KEYBOARD_COLS 4
#define T9_KEYBOARD_ROWS 4
#define T9_BUTTON_COUNT 10

static const char *t9_btn_labels[] = {
    "1", "2", "3", LV_SYMBOL_BACKSPACE,
    "4", "5", "6", LV_SYMBOL_OK,
    "7", "8", "9", LV_SYMBOL_CLOSE,
    "abc", "0", "space", LV_SYMBOL_NEW_LINE,
    "" // End marker for buttonmatrix
};

static const char t9_btn_symbols_0[] = {'0', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', 0};
static const char t9_btn_symbols_1[] = {'1', ':', ';', '<', '=', '>', '?', '@', '[', '\\', ']', '^', '_', '`', '{', '|', '}', '~', 0};

static const char *const t9_btn_chars_lower[T9_BUTTON_COUNT] = {
    NULL, "abc2", "def3", "ghi4", "jkl5", "mno6", "pqrs7", "tuv8", "wxyz9", NULL};
static const char *const t9_btn_chars_upper[T9_BUTTON_COUNT] = {
    NULL, "ABC2", "DEF3", "GHI4", "JKL5", "MNO6", "PQRS7", "TUV8", "WXYZ9", NULL};
static const char *const t9_btn_chars_numbers[T9_BUTTON_COUNT] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};

static t9_mode_t t9_mode = T9_MODE_LOWER;

static uint8_t t9_btn_cycle_idx[T9_BUTTON_COUNT] = {0};
static uint32_t t9_btn_last_press_time[T9_BUTTON_COUNT] = {0};
static uint32_t t9_cycle_timeout_ms = 1000;

static lv_obj_t *t9_btnmatrix = NULL;
static lv_obj_t *linked_ta = NULL;

// --- Static function prototypes ---
static int get_btn_char_idx(int row, int col);
static void t9_update_btnmatrix_labels(void);
static void t9_btnmatrix_event_cb(lv_event_t *e);
static void t9_btnmatrix_longpress_cb(lv_event_t *e);
static void t9_btnmatrix_drawtask_cb(lv_event_t *e);

/**
 * Initialize and create a T9 keyboard linked to a given textarea.
 *
 * @param parent Pointer to the parent LVGL object (e.g., a screen or container)
 * @param ta Pointer to the LVGL textarea object to link for input
 * @return Pointer to the created T9 keyboard object, or NULL on failure
 */
lv_obj_t *lv_keyboard_t9_init(lv_obj_t *parent, lv_obj_t *ta)
{
    if (ta == NULL)
    {
        LV_LOG_WARN("lv_keyboard_t9_init: ta is NULL");
        return NULL;
    }

    linked_ta = ta;

    lv_obj_t *keyboard = lv_obj_create(parent);
    lv_obj_set_size(keyboard, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_set_style_pad_all(keyboard, 0, 0);
    lv_obj_set_flag(keyboard, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_update_layout(keyboard);

    // Create buttonmatrix and add to keyboard
    t9_btnmatrix = lv_buttonmatrix_create(keyboard);
    lv_obj_set_size(t9_btnmatrix, lv_obj_get_width(keyboard), lv_obj_get_height(keyboard));
    lv_obj_center(t9_btnmatrix);
    //  Make main keyboard buttons bigger
    lv_obj_set_style_pad_all(t9_btnmatrix, 0, 0);
    lv_obj_set_style_pad_row(t9_btnmatrix, 4, 0);
    lv_obj_set_style_pad_column(t9_btnmatrix, 4, 0);
    t9_update_btnmatrix_labels();
    lv_obj_add_event_cb(t9_btnmatrix, t9_btnmatrix_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(t9_btnmatrix, t9_btnmatrix_longpress_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(t9_btnmatrix, t9_btnmatrix_drawtask_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(t9_btnmatrix, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

    return keyboard;
}

/**
 * Set or change the linked textarea for the T9 keyboard.
 *
 * @param keyboard Pointer to the T9 keyboard object.
 * @param ta Pointer to the textarea object to link.
 */
void lv_keyboard_t9_set_textarea(lv_obj_t *keyboard, lv_obj_t *ta)
{
    if (keyboard == NULL)
    {
        LV_LOG_WARN("lv_keyboard_t9_set_textarea: keyboard is NULL");
        return;
    }
    if (ta == NULL)
    {
        LV_LOG_WARN("lv_keyboard_t9_set_textarea: ta is NULL");
        return;
    }
    linked_ta = ta;
}

/**
 * @brief Set the T9 keyboard mode (lower, upper, numbers).
 * @param keyboard Pointer to the T9 keyboard object
 * @param mode T9 mode to set
 */
void lv_keyboard_t9_set_mode(lv_obj_t *keyboard, t9_mode_t mode)
{
    LV_UNUSED(keyboard); // For now, only one global mode
    t9_mode = mode;
    t9_update_btnmatrix_labels();
}

/**
 * @brief Get the current T9 keyboard mode.
 * @param keyboard Pointer to the T9 keyboard object
 * @return Current T9 mode
 */
t9_mode_t lv_keyboard_t9_get_mode(lv_obj_t *keyboard)
{
    LV_UNUSED(keyboard); // For now, only one global mode
    return t9_mode;
}

/**
 * @brief Set the T9 cycling timeout in milliseconds.
 * @param ms Timeout in milliseconds
 */
void lv_keyboard_t9_set_cycle_timeout(uint32_t ms)
{
    t9_cycle_timeout_ms = ms;
}

/**
 * @brief Get the T9 cycling timeout in milliseconds.
 * @return Timeout in milliseconds
 */
uint32_t lv_keyboard_t9_get_cycle_timeout(void)
{
    return t9_cycle_timeout_ms;
}

// Helper to set callback as user data on parent keyboard object
void lv_keyboard_t9_set_event_cb(lv_obj_t *keyboard, lv_keyboard_t9_event_cb_t cb)
{
    lv_obj_set_user_data(keyboard, (void *)cb);
}

static void t9_btnmatrix_drawtask_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target_obj(e);
    lv_draw_task_t *draw_task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t *base_dsc = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(draw_task);

    if (base_dsc->part == LV_PART_ITEMS)
    {
        if (base_dsc->id1 == 12 || base_dsc->id1 == 14 || base_dsc->id1 == 15)
        {
            lv_draw_label_dsc_t *label_draw_dsc = lv_draw_task_get_label_dsc(draw_task);
            if (label_draw_dsc)
            {
                label_draw_dsc->decor |= LV_TEXT_DECOR_UNDERLINE;
            }
        }
    }
}

// Helper to get callback from parent keyboard object
static lv_keyboard_t9_event_cb_t t9_get_event_cb(lv_obj_t *keyboard)
{
    return (lv_keyboard_t9_event_cb_t)lv_obj_get_user_data(keyboard);
}

/**
 * Get the T9 button index (0-9) for a given grid row/col.
 * Returns -1 for helper buttons (non-T9).
 */
static int get_btn_char_idx(int row, int col)
{
    if (row == 0 && col < 3)
    {
        return col; // 1,2,3
    }
    if (row == 1 && col < 3)
    {
        return col + 3; // 4,5,6
    }
    if (row == 2 && col < 3)
    {
        return col + 6; // 7,8,9
    }
    if (row == 3 && col == 1)
    {
        return 9; // 0
    }
    // Defensive: log warning if invalid grid position
    if (row < 0 || row >= T9_KEYBOARD_ROWS || col < 0 || col >= T9_KEYBOARD_COLS)
    {
        LV_LOG_WARN("get_btn_char_idx: invalid row/col (%d,%d)", row, col);
    }
    return -1; // helper buttons or invalid
}

/**
 * Get the currently linked textarea of the T9 keyboard.
 *
 * @param keyboard Pointer to the T9 keyboard object.
 * @return Pointer to the linked textarea object, or NULL if none is linked.
 */
// Update buttonmatrix labels by setting a new map
static void t9_update_btnmatrix_labels(void)
{
    // Rebuild the buttonmatrix map based on current mode
    // 4 rows * 4 cols + 4 newlines + 1 NULL = 21
    static const char *map[(T9_KEYBOARD_ROWS * T9_KEYBOARD_COLS) + T9_KEYBOARD_ROWS + 1];
    static char buf[T9_KEYBOARD_ROWS * T9_KEYBOARD_COLS][8];
    int idx = 0;
    for (int row = 0; row < T9_KEYBOARD_ROWS; row++)
    {
        for (int col = 0; col < T9_KEYBOARD_COLS; col++)
        {
            int char_idx = get_btn_char_idx(row, col);
            int buf_idx = row * T9_KEYBOARD_COLS + col;
            if (char_idx >= 0)
            {
                if (t9_mode == T9_MODE_NUMBERS)
                {
                    map[idx++] = t9_btn_chars_numbers[char_idx];
                }
                else if (char_idx == 0)
                {
                    lv_snprintf(buf[buf_idx], sizeof(buf[buf_idx]), "%c%c%c...", t9_btn_symbols_1[0], t9_btn_symbols_1[1], t9_btn_symbols_1[2]);
                    map[idx++] = buf[buf_idx];
                }
                else if (char_idx == 9)
                {
                    lv_snprintf(buf[buf_idx], sizeof(buf[buf_idx]), "%c%c%c...", t9_btn_symbols_0[0], t9_btn_symbols_0[1], t9_btn_symbols_0[2]);
                    map[idx++] = buf[buf_idx];
                }
                else
                {
                    map[idx++] = (t9_mode == T9_MODE_UPPER) ? t9_btn_chars_upper[char_idx] : t9_btn_chars_lower[char_idx];
                }
            }
            else
            {
                // Helper buttons
                if (row == 3 && col == 0)
                {
                    map[idx++] = (t9_mode == T9_MODE_LOWER) ? "abc" : (t9_mode == T9_MODE_UPPER) ? "ABC"
                                                                                                 : "abc";
                }
                else if (row == 3 && col == 2)
                {
                    map[idx++] = "space";
                }
                else if (row == 3 && col == 3)
                {
                    map[idx++] = (t9_mode == T9_MODE_NUMBERS) ? "123" : "T9";
                }
                else
                {
                    map[idx++] = t9_btn_labels[row * T9_KEYBOARD_COLS + col];
                }
            }
        }
        map[idx++] = "\n"; // Add newline after each row
    }
    // Remove the last newline
    map[idx - 1] = NULL; // End marker
    map[idx] = NULL;     // End marker
    lv_buttonmatrix_set_map(t9_btnmatrix, map);
}

/**
 * Event callback for buttonmatrix value changes (button presses).
 * Handles character cycling, helper buttons, and mode switching.
 *
 * @param e Pointer to the LVGL event
 */
static void t9_btnmatrix_event_cb(lv_event_t *e)
{
    lv_obj_t *btnmatrix = lv_event_get_target(e);
    uint16_t btn_id = lv_buttonmatrix_get_selected_button(btnmatrix);
    const char *txt = lv_buttonmatrix_get_button_text(btnmatrix, btn_id);
    if (!txt || !linked_ta)
        return;

    // Helper buttons
    if (lv_strcmp(txt, LV_SYMBOL_BACKSPACE) == 0)
    {
        lv_textarea_delete_char(linked_ta);
        return;
    }
    if (lv_strcmp(txt, "space") == 0)
    {
        lv_textarea_add_text(linked_ta, " ");
        return;
    }
    if (lv_strcmp(txt, LV_SYMBOL_OK) == 0)
    {
        lv_keyboard_t9_event_cb_t cb = t9_get_event_cb(btnmatrix);
        if (cb)
            cb(btnmatrix, LV_KEYBOARD_T9_EVENT_READY);
        return;
    }
    if (lv_strcmp(txt, LV_SYMBOL_CLOSE) == 0)
    {
        lv_keyboard_t9_event_cb_t cb = t9_get_event_cb(btnmatrix);
        if (cb)
            cb(btnmatrix, LV_KEYBOARD_T9_EVENT_CANCEL);
        return;
    }
    if (lv_strcmp(txt, "T9") == 0 || lv_strcmp(txt, "123") == 0)
    {
        t9_mode = (t9_mode == T9_MODE_NUMBERS) ? T9_MODE_LOWER : T9_MODE_NUMBERS;
        t9_update_btnmatrix_labels();
        return;
    }
    if (lv_strcmp(txt, "abc") == 0 || lv_strcmp(txt, "ABC") == 0)
    {
        t9_mode = (t9_mode == T9_MODE_LOWER) ? T9_MODE_UPPER : T9_MODE_LOWER;
        t9_update_btnmatrix_labels();
        return;
    }

    // T9 cycling logic
    int char_idx = get_btn_char_idx(btn_id / T9_KEYBOARD_COLS, btn_id % T9_KEYBOARD_COLS);
    if (char_idx < 0 || char_idx >= T9_BUTTON_COUNT)
        return;

    const char *chars = NULL;
    if (t9_mode == T9_MODE_NUMBERS)
    {
        chars = t9_btn_chars_numbers[char_idx];
    }
    else if (t9_mode == T9_MODE_UPPER)
    {
        chars = t9_btn_chars_upper[char_idx];
    }
    else
    {
        chars = t9_btn_chars_lower[char_idx];
    }
    if (!chars)
        return;

    uint32_t now = lv_tick_get();
    if (now - t9_btn_last_press_time[char_idx] > t9_cycle_timeout_ms)
    {
        t9_btn_cycle_idx[char_idx] = 0;
    }
    else
    {
        t9_btn_cycle_idx[char_idx]++;
        if (chars[t9_btn_cycle_idx[char_idx]] == '\0')
            t9_btn_cycle_idx[char_idx] = 0;
        // Remove last char if cycling
        lv_textarea_delete_char(linked_ta);
    }
    char out[2] = {chars[t9_btn_cycle_idx[char_idx]], '\0'};
    lv_textarea_add_text(linked_ta, out);
    t9_btn_last_press_time[char_idx] = now;
}

// --- Popover logic ---
static lv_obj_t *t9_popover = NULL;
// Use max symbol count for buffer size
#define T9_POPOVER_MAX_SYMBOLS 40
static const char *t9_popover_map[T9_POPOVER_MAX_SYMBOLS + 2]; // max symbols + \n + NULL
static char t9_popover_buf[T9_POPOVER_MAX_SYMBOLS][2];

/**
 * Event callback for popover buttonmatrix selection.
 * Inserts the selected character into the linked textarea and closes the popover.
 */
static void t9_popover_event_cb(lv_event_t *e)
{
    lv_obj_t *popover = lv_event_get_target(e);
    uint16_t btn_id = lv_buttonmatrix_get_selected_button(popover);
    const char *txt = lv_buttonmatrix_get_button_text(popover, btn_id);
    if (!txt || !linked_ta)
        return;
    if (lv_strcmp(txt, "\n") == 0)
        return;
    lv_textarea_add_text(linked_ta, txt);
    lv_obj_del(popover);
    t9_popover = NULL;
}

/**
 * Show a popover with given characters for selection.
 * It is called when a long-press is detected on a T9 button.
 *
 * @param event LVGL event pointer
 */
static void t9_btnmatrix_longpress_cb(lv_event_t *e)
{
    lv_obj_t *btnmatrix = lv_event_get_target(e);
    uint16_t btn_id = lv_buttonmatrix_get_selected_button(btnmatrix);

    int char_idx = get_btn_char_idx(btn_id / T9_KEYBOARD_COLS, btn_id % T9_KEYBOARD_COLS);

    // Disable popover in Number mode
    if (t9_mode == T9_MODE_NUMBERS)
    {
        LV_LOG_INFO("Long-press: popover disabled in Number mode");
        return;
    }

    bool is_symbol_btn = (char_idx == 0 || char_idx == 9); // 1 or 0

    const char *chars = NULL;
    if (char_idx == 0)
    {
        chars = t9_btn_symbols_1;
    }
    else if (char_idx == 9)
    {
        chars = t9_btn_symbols_0;
    }
    else if (t9_mode == T9_MODE_UPPER)
    {
        chars = t9_btn_chars_upper[char_idx];
    }
    else
    {
        chars = t9_btn_chars_lower[char_idx];
    }
    LV_LOG_INFO("Long-press: chars='%s'", chars ? chars : "NULL");
    if (!chars || chars[0] == '\0')
    {
        LV_LOG_INFO("Long-press: chars is NULL or empty");
        return;
    }
    // Clear buffer
    for (int i = 0; i < T9_POPOVER_MAX_SYMBOLS; i++)
    {
        t9_popover_buf[i][0] = '\0';
        t9_popover_buf[i][1] = '\0';
        t9_popover_map[i] = NULL;
    }
    // Build popover map with 4 elements per row, no trailing linebreak
    int idx = 0;
    int col_count = 0;
    int last_newline_idx = -1;
    for (int i = 0; chars[i] != '\0' && idx < T9_POPOVER_MAX_SYMBOLS; i++)
    {
        t9_popover_buf[i][0] = chars[i];
        t9_popover_buf[i][1] = '\0';
        t9_popover_map[idx++] = t9_popover_buf[i];
        col_count++;
        if (col_count == 4)
        {
            t9_popover_map[idx++] = "\n";
            last_newline_idx = idx - 1;
            col_count = 0;
        }
        LV_LOG_INFO("Long-press: popover label[%d]='%s'", i, t9_popover_buf[i]);
    }
    // Remove trailing linebreak if present
    if (col_count == 0 && last_newline_idx == idx - 1)
    {
        idx--; // Remove last newline
    }
    t9_popover_map[idx] = NULL;
    LV_LOG_INFO("Long-press: popover map built, count=%d", idx);
    // Create popover as child of keyboard object
    lv_obj_t *keyboard = lv_obj_get_parent(btnmatrix);
    lv_obj_update_layout(keyboard); // Ensure parent size is up-to-date
    int popover_w = lv_obj_get_width(keyboard) * 90 / 100;
    int popover_h = lv_obj_get_height(keyboard) * 90 / 100;
    if (is_symbol_btn == false)
    {
        // change the height to something smaller
        popover_h = lv_obj_get_height(keyboard) * 33 / 100;
    }
    t9_popover = lv_buttonmatrix_create(keyboard);
    lv_obj_set_size(t9_popover, popover_w, popover_h); // Fill most of parent
    lv_obj_center(t9_popover);
    // lv_obj_set_style_bg_color(t9_popover, lv_color_hex(0x222222), 0);
    lv_obj_set_style_border_color(t9_popover, lv_color_hex(0x8888ff), 0);
    lv_obj_set_style_border_width(t9_popover, 2, 0);
    lv_obj_set_style_pad_all(t9_popover, 6, 0);
    lv_obj_set_style_pad_row(t9_popover, 8, 0);    // Comfortable row spacing
    lv_obj_set_style_pad_column(t9_popover, 8, 0); // Comfortable column spacing

    lv_buttonmatrix_set_map(t9_popover, t9_popover_map);
    lv_obj_add_event_cb(t9_popover, t9_popover_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
