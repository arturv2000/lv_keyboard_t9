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

typedef enum
{
    T9_MODE_LOWER,
    T9_MODE_UPPER,
    T9_MODE_NUMBERS
} t9_mode_t;

// Callback type for T9 keyboard events
typedef void (*lv_keyboard_t9_event_cb_t)(lv_obj_t *keyboard, lv_keyboard_t9_event_t event);

// Register a callback for keyboard events
void lv_keyboard_t9_set_event_cb(lv_obj_t *keyboard, lv_keyboard_t9_event_cb_t cb);

// Create and link the T9 keyboard to a textarea. Returns the created keyboard object.
lv_obj_t* lv_keyboard_t9_init(lv_obj_t* parent, lv_obj_t* ta);

// Set or change the linked textarea for the T9 keyboard.
void lv_keyboard_t9_set_textarea(lv_obj_t *keyboard, lv_obj_t *ta);

// Get the currently linked textarea of the T9 keyboard.
lv_obj_t *lv_keyboard_t9_get_textarea(lv_obj_t *keyboard);

// Set the input mode of the T9 keyboard (lowercase, uppercase, numbers).
void lv_keyboard_t9_set_mode(lv_obj_t *keyboard, t9_mode_t mode);

// Get the current input mode of the T9 keyboard.
t9_mode_t lv_keyboard_t9_get_mode(lv_obj_t *keyboard);

// Set the T9 key cycle timeout in milliseconds (default: 1000 ms)
void lv_keyboard_t9_set_cycle_timeout(uint32_t ms);

// Get the current T9 key cycle timeout in milliseconds
uint32_t lv_keyboard_t9_get_cycle_timeout(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LV_KEYBOARD_T9_H
