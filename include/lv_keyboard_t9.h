#ifndef LV_KEYBOARD_T9_H
#define LV_KEYBOARD_T9_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create and initialize the T9 keyboard widget.
 * @param parent Parent LVGL object (container)
 * @param ta Linked LVGL textarea object
 * @return true on success, false on error
 */
bool lv_keyboard_t9_init(lv_obj_t *parent, lv_obj_t *ta);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LV_KEYBOARD_T9_H
