/**
 * board.h — the hardware contract.
 *
 * This is EVERYTHING that app/ code is allowed to ask the hardware for.
 * If a file in app/ needs something else, a function is missing here —
 * we don't add STM32/HAL includes directly into app/.
 *
 * Each board (stm32f769_disco/, simulator/) must implement these functions.
 * app/ doesn't know which one is used: that's chosen by the CMakePreset.
 *
 * See Documentation/Architecture.md section 5.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initializes clocks, pins, display, touch, network. Called once at startup. */
void board_init(void);

/** Milliseconds since startup (like HAL_GetTick(), but portable). */
uint32_t board_millis(void);

/** Debug trace (UART on the real board, stdout on the simulator). */
void board_log(const char* msg);

/** Called by LVGL to send a rectangle of pixels to the display. */
void board_display_flush(const lv_area_t* area, const uint8_t* pixels);

/** Called by LVGL to read the touch state. Returns false if nothing is pressed. */
bool board_touch_read(int16_t* x, int16_t* y);

#ifdef __cplusplus
}
#endif

/*
 * No dedicated network function: LwIP provides standard sockets once
 * board_init() has configured Ethernet, so app/comms/mqtt/ is already
 * portable without adding anything here.
 *
 * CAN stays managed by UOSM-Core, configured via config/UOSMCoreConfig.h —
 * no board_can_* function needed either.
 */
