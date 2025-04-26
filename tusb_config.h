/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#if MEDIUM == SD
#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#elif MEDIUM == USB
#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_HOST   | OPT_MODE_FULL_SPEED)
#endif

#define BOARD_TUD_MAX_SPEED     OPT_MODE_DEFAULT_SPEED
#define BOARD_TUH_MAX_SPEED     OPT_MODE_DEFAULT_SPEED

#define CFG_TUD_MAX_SPEED       BOARD_TUD_MAX_SPEED
#define CFG_TUH_MAX_SPEED       BOARD_TUH_MAX_SPEED

#define CFG_TUD_MSC             1
#define CFG_TUD_CDC             1

#define CFG_TUH_MSC             1

#define CFG_TUD_MSC_EP_BUFSIZE  8192

#define CFG_TUD_CDC_EP_BUFSIZE  64
#define CFG_TUD_CDC_RX_BUFSIZE  64
#define CFG_TUD_CDC_TX_BUFSIZE  64

#endif
