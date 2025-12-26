/*
 * MIT License
 *
 * Copyright (c) 2025 Ola Kåre Risa
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "movement.h"

/*
 * Set a time of day (hours, minutes, seconds) and this
 * watch face will show how much time it is left until
 * this time of day
 */

typedef enum {
    cdtt_running,
    cdtt_setting
} countdown_to_time_mode_t;

typedef struct {
    uint32_t target_ts;
    uint32_t now_ts;
    uint8_t set_hours;
    uint8_t set_minutes;
    uint8_t set_seconds;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t selection;
    countdown_to_time_mode_t mode;
    uint8_t watch_face_index;
} countdown_to_time_state_t;

void countdown_to_time_face_setup(uint8_t watch_face_index, void ** context_ptr);
void countdown_to_time_face_activate(void *context);
bool countdown_to_time_face_loop(movement_event_t event, void *context);
void countdown_to_time_face_resign(void *context);

#define countdown_to_time_face ((const watch_face_t){ \
    countdown_to_time_face_setup, \
    countdown_to_time_face_activate, \
    countdown_to_time_face_loop, \
    countdown_to_time_face_resign, \
    NULL, \
})
