/*
 * Copyright (c) 2015 Eric Holland
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <assert.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <platform.h>
#include <arch/arm/cm.h>
#include <platform/nrf52.h>
#include <platform/system_nrf52.h>
#include <platform/timer.h>
#include <sys/types.h>

//base counter is total number of clock cycles elapsed
static volatile uint64_t base_counter = 0;

static uint32_t clock_rate = 0;

//cycles of our clock per tick interval
static uint32_t cycles_per_tick = 0;

static platform_timer_callback cb;
static void *cb_args;

typedef enum handler_return (*platform_timer_callback)(void *arg, lk_time_t now);

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    ASSERT(clock_rate > 0);

    cb = callback;
    cb_args = arg;

    cycles_per_tick = clock_rate * interval / 1000 ;

    NRF_CLOCK->LFCLKSRC =  CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos;
    NRF_CLOCK->TASKS_LFCLKSTART = 1;

    NRF_RTC1->CC[0] = 0x00ffffff & (base_counter + cycles_per_tick);

    NRF_RTC1->PRESCALER = 0;

    NRF_RTC1->INTENSET  = RTC_INTENSET_COMPARE0_Enabled << RTC_INTENSET_COMPARE0_Pos;

    NRF_RTC1->EVENTS_TICK = 0;
    NRF_RTC1->EVENTS_COMPARE[0] = 0;
    NRF_RTC1->TASKS_START = 1;
    NVIC_EnableIRQ(RTC1_IRQn);

    return NO_ERROR;
}

// time in msec
lk_time_t current_time(void) {
    uint64_t t;

    do {
        t = base_counter;
    } while (base_counter != t);

    return t * 1000 / clock_rate;
}

// time in usec
lk_bigtime_t current_time_hires(void) {
    uint64_t t;

    do {
        t = base_counter;
    } while (base_counter != t);

    // base_counter only gets updated once every tick, regain extra
    //  precision by adding in elapsed counts since last tick interrupt
    //  Note: counter is only 24 bits
    t = t + ((NRF_RTC1->COUNTER - (t & 0x00ffffff)) & 0x00ffffff);
    return (t * 1000000 / clock_rate);
}

void nrf52_RTC1_IRQ(void) {
    // update to this point in time
    base_counter += cycles_per_tick;
    arm_cm_irq_entry();

    NRF_RTC1->EVENTS_COMPARE[0] = 0;
    // calculate time of next interrupt
    NRF_RTC1->CC[0] = 0x00ffffff & (base_counter + cycles_per_tick);

    bool resched = false;
    if (cb) {
        lk_time_t now = current_time();
        if (cb(cb_args, now) == INT_RESCHEDULE)
            resched = true;
    }
    arm_cm_irq_exit(resched);
}

void arm_cm_systick_init(uint32_t hz) {
    clock_rate = hz;
}

