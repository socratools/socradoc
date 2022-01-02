/* milli_sleep.c - implement the milli_sleep() function
 *
 * MIT License
 *
 * Copyright (c) 2022 Hans Ulrich Niedermann
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


#include "milli_sleep.h"

#include "auto-config.h"

#if   defined(HAVE_WINDOWS_H)
# include <windows.h>
#elif defined(HAVE_TIME_H)
# include <time.h>
#endif


void milli_sleep(unsigned int milliseconds)
{
#if   defined(HAVE_WINDOWS_H)
    Sleep(milliseconds);
#elif defined(HAVE_TIME_H)
    const struct timespec req = { 0L, milliseconds*1000L*1000L };
    struct timespec rem;
    (void) nanosleep(&req, &rem);
#else
# error Requires POSIX nanosleep() or Windows Sleep() at this time.
#endif
}
