/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __ADAPTAION_H__
#define __ADAPTAION_H__


#ifdef  __cplusplus
extern "C" {
#endif

#define __AVR__ 0
#define asm_clear 0
#define asm_isZero 0
#define asm_testBit 0
#define asm_numBits 0
#define asm_cmp 0
#define asm_set 0
#define asm_rshift1 0
#define asm_add 0
#define asm_sub 0
#define asm_mult 0
#define asm_square 0
#define asm_modAdd 0
#define asm_modSub 0
#define asm_modSub_fast 0
#define asm_mmod_fast 0
#define asm_modInv 0

#define __clang__ 1
#define __clang_major__ 5
#define __clang_minor__ 1

#ifdef __cplusplus
}
#endif

#endif
