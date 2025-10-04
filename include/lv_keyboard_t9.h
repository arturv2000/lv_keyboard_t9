#ifndef LV_KEYBOARD_T9_H
#define LV_KEYBOARD_T9_H

#ifdef __cplusplus
extern "C" {
#endif


// Event types for T9 keyboard
typedef enum {
	LV_KEYBOARD_T9_EVENT_READY = 0,   // OK button pressed
	LV_KEYBOARD_T9_EVENT_CANCEL = 1   // Close button pressed
} lv_keyboard_t9_event_t;

// Callback type for T9 keyboard events
typedef void (*lv_keyboard_t9_event_cb_t)(lv_obj_t *keyboard, lv_keyboard_t9_event_t event);

// Register a callback for keyboard events
void lv_keyboard_t9_set_event_cb(lv_obj_t *keyboard, lv_keyboard_t9_event_cb_t cb);

// Create and link the T9 keyboard to a textarea. Returns the created keyboard object.
lv_obj_t* lv_keyboard_t9_init(lv_obj_t* parent, lv_obj_t* ta);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LV_KEYBOARD_T9_H
