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

#include <stdlib.h>
#include <string.h>
#include "countdown_to_time_face.h"
#include "watch.h"
#include "watch_utility.h"

#define CD_SELECTIONS 3
static bool quick_ticks_running;

static void abort_quick_ticks(countdown_to_time_state_t *state) {
    if (quick_ticks_running) {
        quick_ticks_running = false;
        if (state->mode == cdtt_setting)
            movement_request_tick_frequency(4);
        else
            movement_request_tick_frequency(1);
    }
}

static inline void store_countdown(countdown_to_time_state_t *state) {
    state->set_hours = state->hours;
    state->set_minutes = state->minutes;
    state->set_seconds = state->seconds;
    state->target_ts = state->hours * 60 * 60 + state->minutes * 60 + state->seconds;
}

uint32_t get_time_in_seconds(watch_date_time_t time) {
    return time.unit.hour * 3600 + time.unit.minute * 60 + time.unit.second;
}

void countdown_to_time_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(countdown_to_time_state_t));
        countdown_to_time_state_t *state = (countdown_to_time_state_t *)*context_ptr;
        memset(*context_ptr, 0, sizeof(countdown_to_time_state_t));
        state->hours = 0;
        state->minutes = 0;
        state->seconds = 0;
        state->mode = cdtt_running;
        state->watch_face_index = watch_face_index;
        watch_date_time_t now = movement_get_local_date_time();
        state->now_ts = get_time_in_seconds(now);
        store_countdown(state);
    }

}

void countdown_to_time_face_activate(void *context) {
    countdown_to_time_state_t *state = (countdown_to_time_state_t *)context;
    state->mode = cdtt_running;
    watch_date_time_t now = movement_get_local_date_time();
    state->now_ts = get_time_in_seconds(now);
    watch_set_colon();
    movement_request_tick_frequency(1);
    quick_ticks_running = false;
}

static void draw(countdown_to_time_state_t *state, uint8_t subsecond) {
    char buf[16];

    int32_t delta;
    div_t result;

    switch (state->mode) {
        case cdtt_running:
            delta = (int32_t)state->target_ts - (int32_t)state->now_ts;
            if (delta == 0) {
                movement_play_alarm();
            } else if (delta < 0) {
                delta += 86400;
            }
            result = div(delta, 60);
            state->seconds = result.rem;
            result = div(result.quot, 60);
            state->hours = result.quot;
            state->minutes = result.rem;
            sprintf(buf, "%2d%02d%02d", state->hours, state->minutes, state->seconds);
            break;
        case cdtt_setting:
            sprintf(buf, "%2d%02d%02d", state->hours, state->minutes, state->seconds);
            if (!quick_ticks_running && subsecond % 2) {
                switch(state->selection) {
                    case 0:
                        buf[0] = buf[1] = ' ';
                        break;
                    case 1:
                        buf[2] = buf[3] = ' ';
                        break;
                    case 2:
                        buf[4] = buf[5] = ' ';
                        break;
                    default:
                        break;
                }
            }
            break;
    }

    watch_display_text(WATCH_POSITION_BOTTOM, buf);

}

static void settings_increment(countdown_to_time_state_t *state) {
    switch(state->selection) {
        case 0:
            state->hours = (state->hours + 1) % 24;
            break;
        case 1:
            state->minutes = (state->minutes + 1) % 60;
            break;
        case 2:
            state->seconds = (state->seconds + 1) % 60;
            break;
        default:
            // should never happen
            break;
    }
    return;
}

static inline void button_beep() {
    // play a beep as confirmation for a button press (if applicable)
    if (movement_button_should_sound()) watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
}

static inline void set_to_stored_time(countdown_to_time_state_t *state) {
    state->hours = state->set_hours;
    state->minutes = state->set_minutes;
    state->seconds = state->set_seconds;
}

bool countdown_to_time_face_loop(movement_event_t event, void *context) {
    countdown_to_time_state_t *state = (countdown_to_time_state_t *)context;
    watch_date_time_t now;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "CDTT", "CT");
            now = movement_get_local_date_time();
            state->now_ts = get_time_in_seconds(now);
            draw(state,event.subsecond);
            break;
        case EVENT_TICK:
            if (quick_ticks_running) {
                if (HAL_GPIO_BTN_ALARM_read())
                    settings_increment(state);
                else
                    abort_quick_ticks(state);
            }
            now = movement_get_local_date_time();
            state->now_ts = get_time_in_seconds(now);
            draw(state, event.subsecond);
            break;
        case EVENT_MODE_BUTTON_UP:
            abort_quick_ticks(state);
            movement_move_to_next_face();
            break;
        case EVENT_LIGHT_BUTTON_UP:
            switch(state->mode) {
                case cdtt_running:
                    movement_illuminate_led();
                    break;
                case cdtt_setting:
                    state->selection++;
                    if(state->selection >= CD_SELECTIONS) {
                        state->selection = 0;
                        state->mode = cdtt_running;
                        store_countdown(state);
                        movement_request_tick_frequency(1);
                        button_beep();
                    }
                    break;
            }
        
            draw(state, event.subsecond);
            break;
        case EVENT_ALARM_BUTTON_UP:
            switch(state->mode) {
                case cdtt_running:
                    break;
                
                case cdtt_setting:
                    settings_increment(state);
                    break;
            }
            draw(state, event.subsecond);
            break;
        case EVENT_ALARM_LONG_PRESS:
            switch(state->mode) {
                case cdtt_setting:
                    // long press in settings mode starts quick ticks for adjusting the time
                    quick_ticks_running = true;
                    movement_request_tick_frequency(8);
                    break;
                case cdtt_running:
                    set_to_stored_time(state);
                    state->mode = cdtt_setting;
                    movement_request_tick_frequency(4);
                    button_beep();
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (state->mode == cdtt_setting) {
                switch (state->selection) {
                    case 0:
                        state->hours = 0;
                        // intentional fallthrough
                    case 1:
                        state->minutes = 0;
                        // intentional fallthrough
                    case 2:
                        state->seconds = 0;
                        break;
                }
            }
            break;
        case EVENT_ALARM_LONG_UP:
            abort_quick_ticks(state);
            break;
        case EVENT_TIMEOUT:
            // Your watch face will receive this event after a period of inactivity. If it makes sense to resign,
            // you may uncomment this line to move back to the first watch face in the list:
            // movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            // If you did not resign in EVENT_TIMEOUT, you can use this event to update the display once a minute.
            // Avoid displaying fast-updating values like seconds, since the display won't update again for 60 seconds.
            // You should also consider starting the tick animation, to show the wearer that this is sleep mode:
            // watch_start_sleep_animation(500);
            break;
        default:
            // Movement's default loop handler will step in for any cases you don't handle above:
            // * EVENT_LIGHT_BUTTON_DOWN lights the LED
            // * EVENT_MODE_BUTTON_UP moves to the next watch face in the list
            // * EVENT_MODE_LONG_PRESS returns to the first watch face (or skips to the secondary watch face, if configured)
            // You can override any of these behaviors by adding a case for these events to this switch statement.
            return movement_default_loop_handler(event);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    return true;
}

static void display_time_left(countdown_to_time_state_t *state, watch_date_time_t current) {
    char buf[8];
    uint32_t now_ts = get_time_in_seconds(current);
    uint32_t delta = state->target_ts - now_ts;
    if (delta == 0) {
        movement_play_alarm();
    } else if (delta < 0) {
        delta = delta + 24 * 60 * 60;
    }
    snprintf(
        buf,
        sizeof(buf),
        "%2d%02d%02d",
        delta / 60 / 60,
        delta / 60 % 60,
        delta % 60
    );
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

void countdown_to_time_face_resign(void *context) {
    (void) context;

    // handle any cleanup before your watch face goes off-screen.
}

