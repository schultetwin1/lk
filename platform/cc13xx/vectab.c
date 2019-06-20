/*
 * Copyright (c) 2016 Brian Swetland
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

#include <lk/debug.h>
#include <lk/compiler.h>
#include <arch/arm/cm.h>

static void ti_cc_dummy_irq(void) {
    arm_cm_irq_entry();
    panic("unhandled irq");
}

#define DEFAULT_HANDLER(x) \
    void ti_cc_##x##_irq(void) __WEAK_ALIAS("ti_cc_dummy_irq")

#define DEFIRQ(n) DEFAULT_HANDLER(n);
#include <platform/defirq.h>
#undef DEFIRQ

#define VECTAB_ENTRY(x) ti_cc_##x##_irq

const void *const __SECTION(".text.boot.vectab2") vectab2[] = {
#define DEFIRQ(n) VECTAB_ENTRY(n),
#include <platform/defirq.h>
#undef DEFIRQ
};
