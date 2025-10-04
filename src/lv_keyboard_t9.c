/**
 * @file lv_keyboard_t9.c
 * @brief Custom LVGL T9 keyboard widget implementation.
 *
 * This widget provides a T9-style keyboard for LVGL, supporting cycling through characters,
 * symbol popovers on long-press, and helper buttons for space, backspace, OK, close, and mode switching.
 * Usage: Call lv_keyboard_t9_init(parent, ta) to create and link the keyboard to a textarea.
 */
#include <stddef.h>
#include "lvgl/lvgl.h"
#include "lv_keyboard_t9.h"

#define T9_KEYBOARD_COLS 4
#define T9_KEYBOARD_ROWS 4
#define T9_BUTTON_COUNT 10

static const char *t9_btn_labels[T9_KEYBOARD_ROWS][T9_KEYBOARD_COLS] = {
    {"1", "2", "3", LV_SYMBOL_BACKSPACE},
    {"4", "5", "6", LV_SYMBOL_OK},
    {"7", "8", "9", LV_SYMBOL_CLOSE},
    {"abc", "0", "space", LV_SYMBOL_NEW_LINE}};

static const char t9_btn_symbols_0[] = {'!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', 0};
static const char t9_btn_symbols_1[] = {':', ';', '<', '=', '>', '?', '@', '[', '\\', ']', '^', '_', '`', '{', '|', '}', '~', 0};

static const char *const t9_btn_chars_lower[T9_BUTTON_COUNT] = {
    NULL, "abc2", "def3", "ghi4", "jkl5", "mno6", "pqrs7", "tuv8", "wxyz9", NULL};
static const char *const t9_btn_chars_upper[T9_BUTTON_COUNT] = {
    NULL, "ABC2", "DEF3", "GHI4", "JKL5", "MNO6", "PQRS7", "TUV8", "WXYZ9", NULL};
static const char *const t9_btn_chars_numbers[T9_BUTTON_COUNT] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};

typedef enum
{
    T9_MODE_LOWER,
    T9_MODE_UPPER,
    T9_MODE_NUMBERS
} t9_mode_t;
static t9_mode_t t9_mode = T9_MODE_LOWER;

static uint8_t t9_btn_cycle_idx[T9_BUTTON_COUNT] = {0};
static uint32_t t9_btn_last_press_time[T9_BUTTON_COUNT] = {0};
static const uint32_t T9_CYCLE_TIMEOUT_MS = 1000;

static lv_obj_t *t9_btns[T9_KEYBOARD_ROWS][T9_KEYBOARD_COLS];
static lv_obj_t *linked_ta = NULL;

// Per-keyboard event callback storage
#define LV_KEYBOARD_T9_EVENT_CB_KEY "lv_keyboard_t9_event_cb"

// Helper to set callback as user data on parent keyboard object
void lv_keyboard_t9_set_event_cb(lv_obj_t *keyboard, lv_keyboard_t9_event_cb_t cb)
{
    lv_obj_set_user_data(keyboard, (void *)cb);
}

// Helper to get callback from parent keyboard object
static lv_keyboard_t9_event_cb_t t9_get_event_cb(lv_obj_t *keyboard)
{
    return (lv_keyboard_t9_event_cb_t)lv_obj_get_user_data(keyboard);
}

// --- Static function prototypes ---
static int get_btn_char_idx(int row, int col);
static void t9_show_symbol_popover(const char *symbols, int symbol_count, lv_obj_t *keyboard, int col_count);
static void t9_popover_close_btn_event_cb(lv_event_t *e);
static void t9_symbol_btn_event_cb(lv_event_t *e);
static void t9_btn_event_cb(lv_event_t *e);
static void t9_update_btn_label(lv_obj_t *btn, const char *label);

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
 * @brief Update the label of a button safely.
 *
 * This helper function updates the label of a button, performing NULL checks and using LVGL API.
 *
 * @param btn Pointer to the button object
 * @param label Text to set as the label
 */
static void t9_update_btn_label(lv_obj_t *btn, const char *label)
{
    if (btn == NULL)
    {
        LV_LOG_WARN("t9_update_btn_label: btn is NULL");
        return;
    }
    if (label == NULL)
    {
        LV_LOG_WARN("t9_update_btn_label: label is NULL");
        return;
    }
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    if (lbl)
    {
        lv_label_set_text(lbl, label);
    }
    else
    {
        LV_LOG_WARN("t9_update_btn_label: label object not found");
    }
}

/**
 * Show symbol popover for T9 keyboard (used for long-press).
 */
static void t9_show_symbol_popover(const char *symbols, int symbol_count, lv_obj_t *keyboard, int col_count)
{
    int row_count = (symbol_count + col_count - 1) / col_count;
    int popover_w = (lv_obj_get_width(keyboard) * 8) / 10;
    int btn_size = popover_w / col_count - 10;
    int popover_h = row_count * btn_size + 40;
    lv_obj_t *popover = lv_obj_create(lv_scr_act());
    lv_obj_set_size(popover, popover_w, popover_h);
    lv_obj_add_flag(popover, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(popover);
    lv_obj_set_style_bg_color(popover, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_radius(popover, 8, 0);
    lv_obj_set_style_pad_all(popover, 0, 0);
    lv_obj_set_style_pad_column(popover, 8, 0);
    lv_obj_set_style_pad_row(popover, 8, 0);
    // Add close button (top right)
    lv_obj_t *close_btn = lv_btn_create(popover);
    lv_obj_set_size(close_btn, btn_size * 0.8, btn_size * 0.8);
    lv_obj_set_grid_cell(close_btn, LV_GRID_ALIGN_END, col_count - 1, 1, LV_GRID_ALIGN_START, 0, 1);
    lv_obj_set_style_radius(close_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(close_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_bg_opa(close_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(close_btn, 0, 0);
    lv_obj_set_style_pad_all(close_btn, 4, 0);
    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, LV_SYMBOL_CLOSE);
    lv_obj_center(close_lbl);
    lv_obj_add_event_cb(close_btn, t9_popover_close_btn_event_cb, LV_EVENT_CLICKED, NULL);
    // Set grid columns and rows for popover IMMEDIATELY after creation
    lv_coord_t pop_col_dsc[5] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_coord_t pop_row_dsc[row_count + 2];
    pop_row_dsc[0] = LV_GRID_FR(1); // for close button
    for (int r = 1; r <= row_count; r++)
    {
        pop_row_dsc[r] = LV_GRID_FR(1);
    }
    pop_row_dsc[row_count + 1] = LV_GRID_TEMPLATE_LAST;
    lv_obj_set_grid_dsc_array(popover, pop_col_dsc, pop_row_dsc);
    lv_obj_set_layout(popover, LV_LAYOUT_GRID);
    for (int i = 0; i < symbol_count; i++)
    {
        char sym[2] = {symbols[i], 0};
        lv_obj_t *sym_btn = lv_btn_create(popover);
        lv_obj_set_style_pad_all(sym_btn, 0, 0);
        lv_obj_set_grid_cell(sym_btn, LV_GRID_ALIGN_STRETCH, i % col_count, 1, LV_GRID_ALIGN_STRETCH, 1 + i / col_count, 1);
        lv_obj_set_size(sym_btn, btn_size, btn_size);
        lv_obj_t *lbl = lv_label_create(sym_btn);
        lv_obj_center(lbl);
        lv_label_set_text(lbl, sym);
        lv_obj_set_user_data(sym_btn, (void *)(uintptr_t)symbols[i]);
        lv_obj_add_event_cb(sym_btn, t9_symbol_btn_event_cb, LV_EVENT_CLICKED, NULL);
    }
    lv_obj_update_layout(popover);
}
// Event callback for popover close button
/**
 * Event callback for popover close button.
 */
static void t9_popover_close_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    if (!target)
    {
        return;
    }
    lv_obj_t *parent = lv_obj_get_parent(target);
    if (!parent)
    {
        return;
    }
    lv_obj_del(parent);
}

/**
 * Event callback for symbol button in popover.
 */
static void t9_symbol_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    if (!btn)
    {
        return;
    }
    char sym = (char)(uintptr_t)lv_obj_get_user_data(btn);
    if (linked_ta)
    {
        char str[2] = {sym, 0};
        lv_textarea_add_text(linked_ta, str);
    }
    lv_obj_t *parent = lv_obj_get_parent(btn);
    if (parent)
    {
        lv_obj_del(parent);
    }
}

static void t9_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    if (!btn)
    {
        return;
    }
    int user_data = (int)(uintptr_t)lv_obj_get_user_data(btn);
    int row = user_data >> 8;
    int col = user_data & 0xFF;
    int char_idx = get_btn_char_idx(row, col);

    static int last_char_idx = -1;
    static uint32_t last_insert_pos = 0;
    if (char_idx >= 0)
    {
        // Handle long-press for key '1' (row 0, col 0)
        if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED && char_idx >= 0)
        {
            lv_obj_t *keyboard = lv_obj_get_parent(btn);
            if (char_idx == 0)
            {
                int sym_count = sizeof(t9_btn_symbols_1) - 1;
                t9_show_symbol_popover(t9_btn_symbols_1, sym_count, keyboard, 4);
                return;
            }
            else if (char_idx == 9)
            {
                int sym_count = sizeof(t9_btn_symbols_0) - 1;
                t9_show_symbol_popover(t9_btn_symbols_0, sym_count, keyboard, 4);
                return;
            }
            else
            {
                // For buttons 2–9, show their available characters in current mode
                const char *chars;
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
                int char_count = lv_strlen(chars);
                t9_show_symbol_popover(chars, char_count, keyboard, char_count > 4 ? 4 : char_count);
                return;
            }
        }
        // T9 button: cycle through chars with timeout
        uint32_t now = lv_tick_get();
        bool reset_cycle = false;
        if (char_idx != last_char_idx || now - t9_btn_last_press_time[char_idx] > T9_CYCLE_TIMEOUT_MS)
        {
            t9_btn_cycle_idx[char_idx] = 0;
            reset_cycle = true;
        }
        t9_btn_last_press_time[char_idx] = now;
        // Use symbol arrays for '1' and '0' keys, T9 for others
        const char *chars;
        if (char_idx == 0)
        {
            chars = t9_btn_symbols_1;
        }
        else if (char_idx == 9)
        {
            chars = t9_btn_symbols_0;
        }
        else
        {
            chars = (t9_mode == T9_MODE_NUMBERS) ? t9_btn_chars_numbers[char_idx] : (t9_mode == T9_MODE_UPPER) ? t9_btn_chars_upper[char_idx]
                                                                                                               : t9_btn_chars_lower[char_idx];
        }
        uint8_t *cycle = &t9_btn_cycle_idx[char_idx];
        char c = chars[*cycle];
        if (c == '\0')
        {
            *cycle = 0;
            c = chars[0];
        }
        if (linked_ta && c != ' ')
        {
            char str[2] = {c, 0};
            uint32_t txt_len = lv_strlen(lv_textarea_get_text(linked_ta));
            // Always move cursor to end before replacement or insertion
            lv_textarea_set_cursor_pos(linked_ta, txt_len);
            if (reset_cycle || char_idx != last_char_idx || txt_len == 0)
            {
                LV_LOG_USER("Reset Cycle Inserting char '%s' at end", str);
                lv_textarea_add_text(linked_ta, str);
                lv_textarea_set_cursor_pos(linked_ta, txt_len + 1);
            }
            else
            {
                // Replace last character at end
                lv_textarea_set_cursor_pos(linked_ta, txt_len);
                lv_textarea_delete_char(linked_ta);
                LV_LOG_USER("Inserting char '%s' at end (replacing)", str);
                lv_textarea_add_text(linked_ta, str);
                lv_textarea_set_cursor_pos(linked_ta, txt_len); // keep cursor at end
            }
        }
        last_char_idx = char_idx;
        // Advance cycle
        (*cycle)++;
    }
    else
    {
        // Helper buttons
        const char *label = t9_btn_labels[row][col];
        lv_obj_t *keyboard = lv_obj_get_parent(btn);
        lv_keyboard_t9_event_cb_t cb = t9_get_event_cb(keyboard);
        if (lv_strcmp(label, LV_SYMBOL_BACKSPACE) == 0)
        {
            if (linked_ta)
                lv_textarea_delete_char(linked_ta);
        }
        else if (lv_strcmp(label, LV_SYMBOL_OK) == 0)
        {
            // Call user callback for OK
            if (cb)
                cb(keyboard, LV_KEYBOARD_T9_EVENT_READY);
            // Hide the keyboard
            lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        }
        else if (lv_strcmp(label, LV_SYMBOL_CLOSE) == 0)
        {
            // Call user callback for Close
            if (cb)
                cb(keyboard, LV_KEYBOARD_T9_EVENT_CANCEL);
            // Hide the keyboard
            lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        }
        else if (lv_strcmp(label, "ABC") == 0 || lv_strcmp(label, "abc") == 0)
        {
            if (t9_mode == T9_MODE_NUMBERS)
                return;
            t9_mode = (t9_mode == T9_MODE_LOWER) ? T9_MODE_UPPER : T9_MODE_LOWER;
            for (int r = 0; r < T9_KEYBOARD_ROWS; r++)
                for (int c = 0; c < T9_KEYBOARD_COLS; c++)
                {
                    int idx = get_btn_char_idx(r, c);
                    if (idx >= 0)
                    {
                        char buf[16];
                        const char *chars;
                        if (idx == 0) {
                            chars = t9_btn_symbols_1;
                            lv_snprintf(buf, sizeof(buf), "1\n%c%c%c", chars[0], chars[1], chars[2]);
                        } else if (idx == 9) {
                            chars = t9_btn_symbols_0;
                            lv_snprintf(buf, sizeof(buf), "0\n%c%c%c", chars[0], chars[1], chars[2]);
                        } else {
                            chars = (t9_mode == T9_MODE_NUMBERS) ? t9_btn_chars_numbers[idx] : (t9_mode == T9_MODE_UPPER) ? t9_btn_chars_upper[idx]
                                                                                                   : t9_btn_chars_lower[idx];
                            lv_snprintf(buf, sizeof(buf), "%s\n%s", t9_btn_labels[r][c], chars);
                        }
                        t9_update_btn_label(t9_btns[r][c], buf);
                    }
                }
            t9_update_btn_label(t9_btns[3][0], (t9_mode == T9_MODE_LOWER) ? "abc" : "ABC");
        }
        else if (lv_strcmp(label, LV_SYMBOL_NEW_LINE) == 0)
        {
            t9_mode = (t9_mode == T9_MODE_NUMBERS) ? T9_MODE_LOWER : T9_MODE_NUMBERS;
            for (int r = 0; r < T9_KEYBOARD_ROWS; r++)
                for (int c = 0; c < T9_KEYBOARD_COLS; c++)
                {
                    int idx = get_btn_char_idx(r, c);
                    if (idx >= 0)
                    {
                        char buf[16];
                        const char *chars;
                        if (idx == 0) {
                            chars = t9_btn_symbols_1;
                            lv_snprintf(buf, sizeof(buf), "1\n%c%c%c", chars[0], chars[1], chars[2]);
                        } else if (idx == 9) {
                            chars = t9_btn_symbols_0;
                            lv_snprintf(buf, sizeof(buf), "0\n%c%c%c", chars[0], chars[1], chars[2]);
                        } else {
                            chars = (t9_mode == T9_MODE_NUMBERS) ? t9_btn_chars_numbers[idx] : (t9_mode == T9_MODE_UPPER) ? t9_btn_chars_upper[idx]
                                                                                                   : t9_btn_chars_lower[idx];
                            lv_snprintf(buf, sizeof(buf), "%s\n%s", t9_btn_labels[r][c], chars);
                        }
                        t9_update_btn_label(t9_btns[r][c], buf);
                    }
                }
            t9_update_btn_label(t9_btns[3][3], (t9_mode == T9_MODE_NUMBERS) ? "123" : "T9");
            t9_update_btn_label(t9_btns[3][0], (t9_mode == T9_MODE_UPPER) ? "ABC" : "abc");
        }
        else if (lv_strcmp(label, "space") == 0)
        {
            if (linked_ta)
                lv_textarea_add_text(linked_ta, " ");
        }
        else if (label == LV_SYMBOL_NEW_LINE)
        {
            if (linked_ta)
                lv_textarea_add_text(linked_ta, "\n");
        }
    }
}

lv_obj_t *lv_keyboard_t9_init(lv_obj_t *parent, lv_obj_t *ta)
{
    linked_ta = ta;

    lv_obj_t *keyboard = lv_obj_create(parent);

    // Fill parent size
    lv_obj_set_size(keyboard, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_set_style_pad_all(keyboard, 0, 0);
    // Optionally, arrange keyboard as grid
    static lv_coord_t col_dsc[5] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[5] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(keyboard, col_dsc, row_dsc);
    lv_obj_set_layout(keyboard, LV_LAYOUT_GRID);

    for (int row = 0; row < T9_KEYBOARD_ROWS; row++)
    {
        for (int col = 0; col < T9_KEYBOARD_COLS; col++)
        {
            t9_btns[row][col] = lv_btn_create(keyboard);
            lv_obj_set_size(t9_btns[row][col], 60, 60);
            lv_obj_set_grid_cell(t9_btns[row][col], LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
            lv_obj_t *label = lv_label_create(t9_btns[row][col]);
            int char_idx = get_btn_char_idx(row, col);
            char buf[32];
            if (char_idx >= 0)
            {
                if (char_idx == 0)
                {
                    lv_snprintf(buf, sizeof(buf), "1\n%c%c%c", t9_btn_symbols_1[0], t9_btn_symbols_1[1], t9_btn_symbols_1[2]);
                }
                else if (char_idx == 9)
                {
                    lv_snprintf(buf, sizeof(buf), "0\n%c%c%c", t9_btn_symbols_0[0], t9_btn_symbols_0[1], t9_btn_symbols_0[2]);
                }
                else
                {
                    const char *chars = (t9_mode == T9_MODE_NUMBERS) ? t9_btn_chars_numbers[char_idx] : (t9_mode == T9_MODE_UPPER) ? t9_btn_chars_upper[char_idx]
                                                                                                                                   : t9_btn_chars_lower[char_idx];
                    lv_snprintf(buf, sizeof(buf), "%s\n%s", t9_btn_labels[row][col], chars);
                }
                lv_label_set_text(label, buf);
            }
            else
            {
                if (row == 3 && col == 0)
                {
                    if (t9_mode == T9_MODE_LOWER)
                        lv_label_set_text(label, "abc");
                    else if (t9_mode == T9_MODE_UPPER)
                        lv_label_set_text(label, "ABC");
                    else
                        lv_label_set_text(label, "abc");
                }
                else if (row == 3 && col == 3)
                {
                    if (t9_mode == T9_MODE_NUMBERS)
                        lv_label_set_text(label, "123");
                    else
                        lv_label_set_text(label, "T9");
                }
                else
                {
                    lv_label_set_text(label, t9_btn_labels[row][col]);
                }
            }
            lv_obj_set_user_data(t9_btns[row][col], (void *)((row << 8) | col));
            lv_obj_add_event_cb(t9_btns[row][col], t9_btn_event_cb, LV_EVENT_CLICKED, NULL);
            int idx = get_btn_char_idx(row, col);
            if (idx >= 0)
                lv_obj_add_event_cb(t9_btns[row][col], t9_btn_event_cb, LV_EVENT_LONG_PRESSED, NULL);
        }
    }
    return keyboard;
}
