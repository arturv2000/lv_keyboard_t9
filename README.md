# LVGL T9 Keyboard Component

A custom T9-style keyboard widget for LVGL, supporting character cycling, symbol popovers, and helper buttons (space, backspace, OK, close, mode switching).

## Features
- T9-style input for touchscreens
- Long-press popover for symbols
- Helper buttons for space, backspace, OK, close, mode toggle
- Easy integration with LVGL textareas

## Usage
1. Add this repo as a submodule or external component:
   ```sh
git submodule add git@github.com:arturv2000/lv_keyboard_t9.git external/lv_keyboard_t9
```
2. Add `src/` and `include/` to your build system (CMake, PlatformIO, etc).
3. Include the header:
   ```c
#include "lv_keyboard_t9.h"
```
4. Create and link the keyboard to a textarea:
   ```c
lv_keyboard_t9_init(parent, ta);
```

## Example
See [`example/main.c`](example/main.c) for a minimal usage example.

## License
MIT (see LICENSE)

## Compatibility
- LVGL v9+
- C99 or later

## Contributing
Pull requests and issues welcome!
