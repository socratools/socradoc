/* scnp-cli main program
 *
 * MIT License
 *
 * Copyright (c) 2020 Jim Ramsay
 * Copyright (c) 2021 Hans Ulrich Niedermann
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


#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <signal.h>
#include <unistd.h>


#include <libusb.h>


#include "auto-config.h"


static
bool dry_run = false;


static
uint32_t dry_run_value = 0x00001000;


#define COND_OR_FAIL(COND, MSG)                                       \
    do {                                                              \
        const bool cond = (COND);                                     \
        const char *const msg = (MSG);                                \
        if (!cond) {                                                  \
            fprintf(stderr, "Fatal: %s: !(%s)\n", msg, #COND);        \
            exit(EXIT_FAILURE);                                       \
        }                                                             \
    } while (0)


#define BE_LONG_OR_FAIL(SMALLER, BIGGER)                                \
    do {                                                                \
        const long smaller = (SMALLER);                                 \
        const long bigger = (BIGGER);                                   \
        if (smaller > bigger) {                                         \
            fprintf(stderr,                                             \
                    "Fatal: %s=%ld=0x%08lx above %s=%ld=0x%08lx\n",     \
                    #SMALLER, smaller, smaller,                         \
                    #BIGGER, bigger, bigger);                           \
            exit(EXIT_FAILURE);                                         \
        }                                                               \
    } while (0)


#define BE_UINT32_OR_FAIL(SMALLER, BIGGER)                              \
    do {                                                                \
        const uint32_t smaller = (SMALLER);                             \
        const uint32_t bigger = (BIGGER);                               \
        if (smaller > bigger) {                                         \
            fprintf(stderr,                                             \
                    "Fatal: %s=%d=0x%08x above %s=%d=0x%08x\n",         \
                    #SMALLER, smaller, smaller,                         \
                    #BIGGER, bigger, bigger);                           \
            exit(EXIT_FAILURE);                                         \
        }                                                               \
    } while (0)


#define COND_OR_RETURN(COND, MSG)                                     \
    do {                                                              \
        const bool cond = (COND);                                     \
        const char *const msg = (MSG);                                \
        if (!cond) {                                                  \
            fprintf(stderr, "Fatal: %s: !(%s)\n", msg, #COND);        \
            return EXIT_FAILURE;                                      \
        }                                                             \
    } while (0)


#define LIBUSB_OR_FAIL(LIBUSB_RETVAL, MSG)                            \
    do {                                                              \
        const int retval = (LIBUSB_RETVAL);                           \
        const char *const msg = (MSG);                                \
        if (retval < 0) {                                             \
            fprintf(stderr, "Fatal: %s: %s\n", msg,                   \
                    libusb_strerror(retval));                         \
            exit(EXIT_FAILURE);                                       \
        }                                                             \
    } while (0)


#ifndef HAVE_EXP10_FUNCTION
inline static
double exp10(double x);

inline static
double exp10(double x)
{
    /* Just hoping that the pre-2.28 glibc pow() >10000 times slowness
     * bug does not appear to the values we are using. */
    return pow(10.0, x);
}
#endif


static
uint32_t dB_to_uint(const uint32_t ref_value, const double dB_value)
{
    const double d_ref_value = ref_value;

    /* If your compilation fails here due to the exp10(3) function not
     * being available, uncomment the above reimplementation of
     * exp10(3) in terms of log(3) and exp(3) for a quick fix and drop
     * us a hint in the issue tracker. Thank you!
     */
    const double d_uint_value = d_ref_value * exp10(dB_value/20.0);

    const long long_uint_value = lround(d_uint_value);
    BE_LONG_OR_FAIL(0L, long_uint_value);
    BE_LONG_OR_FAIL(long_uint_value, ref_value);
    const uint32_t retval = (uint32_t) long_uint_value;
    return retval;
}


static
double uint_to_dB(const uint32_t ref_value, const uint32_t uint_value)
{
    const double d_ref = ref_value;
    const double d_uint = uint_value;
    const double d_dB = 20.0 * log10(d_uint/d_ref);
    return d_dB;
}


#define REF_VALUE_RANGE     0x1fffffffUL
#define REF_VALUE_THRESHOLD 0x007fffffUL
#define REF_VALUE_METER     0x00ffffffUL


static
uint32_t dB_to_uint_range(const double range_dB)
{
    /* note the tiny "negative" sign */
    return dB_to_uint(REF_VALUE_RANGE, -range_dB);
}


static
uint32_t dB_to_uint_threshold(const double thresh_dB)
{
    return dB_to_uint(REF_VALUE_THRESHOLD, thresh_dB);
}


static
uint32_t dB_to_uint_meter(const double range_dB)
{
    return dB_to_uint(REF_VALUE_METER, range_dB);
}


static
double uint_to_dB_meter(const uint32_t uint_value)
{
    return uint_to_dB(REF_VALUE_METER, uint_value);
}


#define NOTEPAD_SOURCES_MAX 4


/* We do not care about padding and storage efficiency here */
typedef struct {
    uint16_t idProduct;
    const char *const name;
    const char *const source_descr;
    const char *const sources[NOTEPAD_SOURCES_MAX];
} notepad_device_T;


static
const notepad_device_T supported_devices[] = {
    { 0x0030,
      "NOTEPAD-5",
      "channels 1+2 of 2-channel audio capture device",
      {"MIC+LINE 1+2", "LINE 2+3", "LINE 4+5", "MASTER L+R"}},

    { 0x0031,
      "NOTEPAD-8FX",
      "channels 1+2 of 2-channel audio capture device",
      {"MIC 1+2", "LINE 3+4", "LINE 5+6", "MASTER L+R"}},

    { 0x0032,
      "NOTEPAD-12FX",
      "channels 3+4 of 4-channel audio capture device",
      {"MIC 3+4", "LINE 5+6", "LINE 7+8", "MASTER L+R"}},

    /* The termination marker is .idProduct being 0 */
    { 0 }
};


static
ssize_t supported_device_list(libusb_device ***device_list)
    __attribute__(( nonnull(1) ));

static
ssize_t supported_device_list(libusb_device ***device_list)
{
    // printf("get_supported_device_list\n");

    libusb_device **ret_list = calloc(128, sizeof(libusb_device *));
    if (ret_list == NULL) {
        perror("calloc libusb_device list");
        exit(EXIT_FAILURE);
    }
    ssize_t ret_count = 0;

    libusb_device **devices = NULL;
    const ssize_t luret_get_device_list =
        libusb_get_device_list(NULL, &devices);
    if (luret_get_device_list < 0) {
        COND_OR_FAIL(luret_get_device_list >= INT_MIN,
                     "ssize_t value out of int range");
        const int luret_get_device_list_as_int = (int) luret_get_device_list;
        LIBUSB_OR_FAIL(luret_get_device_list_as_int, "libusb_get_device_list");
    }

    for (int i=0; devices[i] != NULL; ++i) {
        libusb_device *dev = devices[i];
        struct libusb_device_descriptor desc;
        const int luret_get_dev_descr =
            libusb_get_device_descriptor(dev, &desc);
        LIBUSB_OR_FAIL(luret_get_dev_descr, "libusb_get_device_descriptor");

        for (int k=0; supported_devices[k].idProduct != 0; ++k) {
            const notepad_device_T *const np_dev = &supported_devices[k];
            if ((0x05fc == desc.idVendor) &&
                (np_dev->idProduct == desc.idProduct)) {

                const uint8_t busnum = libusb_get_bus_number(dev);
                const uint8_t devaddr = libusb_get_device_address(dev);

                libusb_device_handle *dev_handle;
                const int luret_open =
                    libusb_open(dev, &dev_handle);
                LIBUSB_OR_FAIL(luret_open, "libusb_open");

                const uint8_t firmware_hi = (desc.bcdDevice >> 8) & 0xff;
                const uint8_t firmware_lo = (desc.bcdDevice >> 0) & 0xff;

                unsigned char buf_manufacturer[1024];
                const int luret_get_sd_ascii_manuf =
                    libusb_get_string_descriptor_ascii(dev_handle,
                                                       desc.iManufacturer,
                                                       buf_manufacturer,
                                                       sizeof(buf_manufacturer));
                LIBUSB_OR_FAIL(luret_get_sd_ascii_manuf,
                               "libusb_get_string_descriptor_ascii iManufacturer");

                unsigned char buf_product[1024];
                const int luret_get_sd_ascii_prod =
                    libusb_get_string_descriptor_ascii(dev_handle,
                                                       desc.iProduct,
                                                       buf_product,
                                                       sizeof(buf_product));
                LIBUSB_OR_FAIL(luret_get_sd_ascii_prod,
                               "libusb_get_string_descriptor_ascii iProduct");

                printf("Bus %03d Device %03d: ID %04x:%04x %s %s (firmware %d.%02d)\n",
                       busnum, devaddr, desc.idVendor, desc.idProduct,
                       buf_manufacturer, buf_product,
                       firmware_hi, firmware_lo);

                libusb_close(dev_handle);

                libusb_ref_device(dev);
                ret_list[ret_count++] = dev;
            }
        }
    }
    libusb_free_device_list(devices, 1);

    COND_OR_FAIL(ret_count >= 0, "device number overflow... uhm what?");

    /* NULL terminate list like the libusb_get_device_list retval */
    ret_list[ret_count] = NULL;

    /* reduce the memory size to what we actually use */
    ret_list = reallocarray(ret_list, ((size_t)ret_count)+1,
                            sizeof(libusb_device *));
    COND_OR_FAIL(ret_list != NULL, "reallocarray");

    *device_list = ret_list;
    return ret_count;
}


static
const notepad_device_T *notepad_device_from_idProduct(const uint16_t idProduct);

static
const notepad_device_T *notepad_device_from_idProduct(const uint16_t idProduct)
{
    for (size_t i=0; supported_devices[i].idProduct != 0; ++i) {
        if (idProduct == supported_devices[i].idProduct) {
            return &supported_devices[i];
        }
    }
    return NULL;
}



static
void device_recv_ctrl_message(libusb_device_handle *device_handle,
                              uint8_t *data, const size_t data_size)
    __attribute__(( nonnull(1), nonnull(2) ));

static
void device_recv_ctrl_message(libusb_device_handle *device_handle,
                              uint8_t *data, const size_t data_size)
{
    COND_OR_FAIL(data_size < UINT16_MAX, "data_size exceeds uint16_t range");
    const uint16_t u16_data_size = (uint16_t) data_size;

    COND_OR_FAIL(data_size == 8, "all known notepad messages are 8 bytes");

    if (dry_run) {
        data[0] = (dry_run_value >>  0) & 0xff;
        data[1] = (dry_run_value >>  8) & 0xff;
        data[2] = (dry_run_value >> 16) & 0xff;
        data[3] = (dry_run_value >> 24) & 0xff;
        data[4] = 0x00;
        data[5] = 0x00;
        data[6] = 0x00;
        data[7] = 0x00;
        return;
    } else {
        const int luret_ctrl_transfer =
            libusb_control_transfer(device_handle,
                                    0xc0 /* bmRequestType */,
                                    16 /* bRequest */,
                                    0 /* wValue */,
                                    0 /* wIndex */,
                                    data, u16_data_size,
                                    10000 /* timeout in ms */);
        LIBUSB_OR_FAIL(luret_ctrl_transfer, "libusb_control_transfer");
        COND_OR_FAIL(luret_ctrl_transfer == sizeof(data),
                     "libusb_control_transfer");
    }

#if 0
    printf("device_recv_ctrl_message got"
           " {%02x %02x %02x %02x %02x %02x %02x %02x}%s\n",
           data[0], data[1], data[2], data[3],
           data[4], data[5], data[6], data[7],
           dry_run?" (dry-run)":"");
#endif
}


static
void device_send_ctrl_message(libusb_device_handle *device_handle,
                              uint8_t *data, const size_t data_size)
    __attribute__(( nonnull(1), nonnull(2) ));

static
void device_send_ctrl_message(libusb_device_handle *device_handle,
                              uint8_t *data, const size_t data_size)
{
    COND_OR_FAIL(data_size < UINT16_MAX, "data_size exceeds uint16_t range");
    const uint16_t u16_data_size = (uint16_t) data_size;

    COND_OR_FAIL(data_size == 8, "all known notepad messages are 8 bytes");
    printf("device_send_ctrl_message(device,"
           " {%02x %02x %02x %02x %02x %02x %02x %02x})%s\n",
           data[0], data[1], data[2], data[3],
           data[4], data[5], data[6], data[7],
           dry_run?" (dry-run)":"");

    if (dry_run) {
        return;
    }

    const int luret_ctrl_transfer =
        libusb_control_transfer(device_handle,
                                0x40 /* bmRequestType */,
                                16 /* bRequest */,
                                0 /* wValue */,
                                0 /* wIndex */,
                                data, u16_data_size,
                                10000 /* timeout in ms */);
    LIBUSB_OR_FAIL(luret_ctrl_transfer, "libusb_control_transfer");
    COND_OR_FAIL(luret_ctrl_transfer == sizeof(data),
                 "libusb_control_transfer");
}


typedef struct {
    libusb_device *device;
    struct libusb_device_descriptor *descriptor;
    libusb_device_handle *device_handle;
    const notepad_device_T *const notepad_device;
} usbdev_T;


static
void usbdev_audio_routing(usbdev_T *usbdev, const uint8_t src_idx)
    __attribute__(( nonnull(1) ));

static
void usbdev_audio_routing(usbdev_T *usbdev, const uint8_t src_idx)
{
    printf("Setting USB audio source to %d (%s) for device %s\n",
	   src_idx,
           usbdev->notepad_device->sources[src_idx],
           usbdev->notepad_device->name);

    uint8_t data[8];
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x04;
    data[3] = 0x00;
    data[4] = src_idx;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;

    device_send_ctrl_message(usbdev->device_handle, data, sizeof(data));
}


static
void usbdev_ducker_off(usbdev_T *usbdev)
    __attribute__(( nonnull(1) ));

static
void usbdev_ducker_off(usbdev_T *usbdev)
{
    printf("ducker-off %s\n", usbdev->notepad_device->name);

    uint8_t data[8];
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x02;
    data[3] = 0x80;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;

    device_send_ctrl_message(usbdev->device_handle, data, sizeof(data));
}


static
void usbdev_ducker_on(usbdev_T *usbdev,
                      const uint8_t inputs, const uint16_t release_ms)
    __attribute__(( nonnull(1) ));

static
void usbdev_ducker_on(usbdev_T *usbdev,
                      const uint8_t inputs, const uint16_t release_ms)
{
    COND_OR_FAIL(inputs < 16, "inputs bitmap out of range (0b0000 to 0b1111)");
    COND_OR_FAIL(release_ms <= 5000, "release_ms out of range (0 to 5000ms)");

    printf("ducker-on inputs=%u release_ms=%u %s\n",
           inputs, release_ms, usbdev->notepad_device->name);

    uint8_t data[8];
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x02;
    data[3] = 0x80;
    data[4] = 0x01;
    data[5] = inputs;
    data[6] = ((release_ms>>8) & 0xff);
    data[7] = ((release_ms>>0) & 0xff);

    device_send_ctrl_message(usbdev->device_handle, data, sizeof(data));
}


static
void usbdev_ducker_range(usbdev_T *usbdev,
                         const uint32_t range_value)
    __attribute__(( nonnull(1) ));

static
void usbdev_ducker_range(usbdev_T *usbdev,
                         const uint32_t range_value)
{
    BE_UINT32_OR_FAIL(range_value, 0x1fffffff);

    printf("ducker-range range=0x%x=%u %s\n",
           range_value, range_value, usbdev->notepad_device->name);

    uint8_t data[8];
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x02;
    data[3] = 0x81;
    data[4] = ((range_value>>24) & 0xff);
    data[5] = ((range_value>>16) & 0xff);
    data[6] = ((range_value>> 8) & 0xff);
    data[7] = ((range_value>> 0) & 0xff);

    device_send_ctrl_message(usbdev->device_handle, data, sizeof(data));
}


static
void usbdev_ducker_threshold(usbdev_T *usbdev,
                             const uint32_t thresh_value)
    __attribute__(( nonnull(1) ));

static
void usbdev_ducker_threshold(usbdev_T *usbdev,
                             const uint32_t thresh_value)
{
    BE_UINT32_OR_FAIL(thresh_value, 0x007fffff);

    printf("ducker-threshold thresh=0x%x=%u %s\n",
           thresh_value, thresh_value, usbdev->notepad_device->name);

    uint8_t data[8];
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x02;
    data[3] = 0x82;
    data[4] = ((thresh_value>>24) & 0xff);
    data[5] = ((thresh_value>>16) & 0xff);
    data[6] = ((thresh_value>> 8) & 0xff);
    data[7] = ((thresh_value>> 0) & 0xff);

    device_send_ctrl_message(usbdev->device_handle, data, sizeof(data));
}


static
volatile sig_atomic_t global_abort = false;


extern
void handle_signal(int unused_signum);

void handle_signal(int unused_signum __attribute__(( unused )))
{
    global_abort = true;
}


static
void usbdev_meter(usbdev_T *usbdev)
    __attribute__(( nonnull(1) ));

static
void usbdev_meter(usbdev_T *usbdev)
{
    const int stdout_fileno = fileno(stdout);
    COND_OR_FAIL(isatty(stdout_fileno),
                 "The interactive meter only works inside a TTY");

    printf("meter for %s. Press Ctrl-C to quit.\n",
           usbdev->notepad_device->name);

    uint8_t data[8];

    uint32_t min_value = 0xffffffff;
    uint32_t max_value = 0x00000000;

    double min_double = +DBL_MAX;
    double max_double = -DBL_MAX;

    /* We could use unicode block characters. We could use ANSI
     * colors. But unicode block characters might not be
     * available. ANSI colors might not be. termcap is complex. And we
     * are lazy. */

#define METER_WIDTH 63UL

    static char meterbuf[76];
    for (size_t i=1; i<1+METER_WIDTH; ++i) {
        meterbuf[i] = '-';
    }
    meterbuf[0] = '[';
    meterbuf[1+METER_WIDTH] = ']';
    meterbuf[2+METER_WIDTH] = '\0';

    signal(SIGINT, handle_signal);

    printf("uintval   dB    bar graph\n");

    while (true) {
        device_recv_ctrl_message(usbdev->device_handle, data, sizeof(data));
        const uint32_t cur_value =
            (((uint32_t)data[0])<< 0) |
            (((uint32_t)data[1])<< 8) |
            (((uint32_t)data[2])<<16) |
            (((uint32_t)data[3])<<24);
        if (cur_value < min_value) {
            min_value = cur_value;
        }
        if (cur_value > max_value) {
            max_value = cur_value;
        }


        /* original dB value can be slightly outside the -100.0 .. 0.0 range */
        const double raw_dB = uint_to_dB_meter(cur_value);
        const double raw_dB1 = (raw_dB < -100.0) ? -100.0 : raw_dB;
        /* dB value constrained into -100.0 to 0.0 interval */
        const double dB = (raw_dB1 > 0.0) ? 0.0 : raw_dB1;

        if (raw_dB < min_double) {
            min_double = raw_dB;
        }
        if (raw_dB > max_double) {
            max_double = raw_dB;
        }

        const double d_idx = ((100.0 + dB) * METER_WIDTH) * 0.01;
        const uint32_t idx = (uint32_t) d_idx;

        COND_OR_FAIL(idx <= METER_WIDTH, "value range exceeded");

        for (size_t i=1; i<1+idx; ++i) {
            meterbuf[i] = '#';
        }
        for (size_t i=1+idx; i<1+METER_WIDTH; ++i) {
            meterbuf[i] = '-';
        }
        printf("%07x %6.1f %s\r", cur_value, dB, meterbuf);
        fflush(stdout);

        const struct timespec req = { 0L, 100L*1000L*1000L };
        struct timespec rem;
        (void) nanosleep(&req, &rem);

        if (global_abort) {
            /* When Ctrl-C has been pressed, re-print the meter line
             * to overwrite the "^C" shown at the beginning of the
             * line before leaving the loop.
             */
            printf("\r%07x %6.1f %s  \r", cur_value, dB, meterbuf);
            fflush(stdout);
            break;
        }
    }

    printf("\n");
    printf("meter summary:\n"
           "  %s  %9u = 0x%08x  %6.1fdB\n"
           "  %s  %9u = 0x%08x  %6.1fdB\n"
           "",
           "minimum", min_value, min_value, min_double,
           "maximum", max_value, max_value, max_double);
}

typedef union {
    struct {
        uint8_t source_index;
    } audio_routing;

    struct {
        uint8_t inputs;
        uint16_t release_ms;
    } ducker_on;

    struct {
        uint32_t range;
    } ducker_range;

    struct {
        uint32_t thresh;
    } ducker_threshold;
} command_params_T;


typedef void (*command_func_T)(usbdev_T *usbdev, command_params_T *params);


static
void commandfunc_audio_routing(usbdev_T *usbdev,
                               command_params_T *params)
{
    usbdev_audio_routing(usbdev,
                         params->audio_routing.source_index);
}


static
void commandfunc_ducker_off(usbdev_T *usbdev,
                            command_params_T *params __attribute__(( unused )) )
{
    usbdev_ducker_off(usbdev);
}


static
void commandfunc_ducker_on(usbdev_T *usbdev,
                           command_params_T *params)
{
    usbdev_ducker_on(usbdev,
                     params->ducker_on.inputs,
                     params->ducker_on.release_ms);
}


static
void commandfunc_ducker_range(usbdev_T *usbdev,
                              command_params_T *params)
{
    usbdev_ducker_range(usbdev,
                        params->ducker_range.range);
}


static
void commandfunc_ducker_threshold(usbdev_T *usbdev,
                                  command_params_T *params)
{
    usbdev_ducker_threshold(usbdev,
                            params->ducker_threshold.thresh);
}


static
void commandfunc_meter(usbdev_T *usbdev,
                       command_params_T *params __attribute__(( unused )) )
    __attribute__(( nonnull(1) ));

static
void commandfunc_meter(usbdev_T *usbdev,
                       command_params_T *params __attribute__(( unused )) )
{
    usbdev_meter(usbdev);
}


static
void run_command(command_func_T command_func,
                 command_params_T *command_params)
    __attribute__(( nonnull(1), nonnull(2) ));

static
void run_command(command_func_T command_func,
                 command_params_T *command_params)
{
    const int luret_init =
        libusb_init(NULL);
    LIBUSB_OR_FAIL(luret_init, "libusb_init");

    libusb_device **dev_list;
    ssize_t dev_count = supported_device_list(&dev_list);

    if (dev_count == 0) {
        fprintf(stderr, "No Notepad device found\n");
        exit(EXIT_FAILURE);
    }
    if (dev_count > 1) {
        fprintf(stderr, "Cannot handle more than one Notepad device yet. Sorry.\n");
        exit(EXIT_FAILURE);
    }

    libusb_device *device = dev_list[0];

    struct libusb_device_descriptor descriptor;
    const int luret_get_dev_descr =
        libusb_get_device_descriptor(device, &descriptor);
    LIBUSB_OR_FAIL(luret_get_dev_descr, "libusb_get_device_descriptor");

    const notepad_device_T *const notepad_device =
        notepad_device_from_idProduct(descriptor.idProduct);
    COND_OR_FAIL(notepad_device != NULL, "unhandled idProduct");

    libusb_device_handle *device_handle;
    const int luret_open =
        libusb_open(device, &device_handle);
    LIBUSB_OR_FAIL(luret_open, "libusb_open");

    usbdev_T usbdev = {
        device,
        &descriptor,
        device_handle,
        notepad_device
    };

    command_func(&usbdev, command_params);

    libusb_close(device_handle);

    for (ssize_t i=0; i<dev_count; ++i) {
        libusb_unref_device(dev_list[i]);
    }

    free(dev_list);

    libusb_exit(NULL);
}


static
void print_version(const char *const prog);

static
void print_version(const char *const prog)
{
    printf("%s (%s) %s\n"
           "Copyright (C) 2021 Jim Ramsay\n"
           "Copyright (C) 2021 Hans Ulrich Niedermann\n"
           "\n"
           "Permission is hereby granted, free of charge, to any person obtaining a copy\n"
           "of this software and associated documentation files (the \"Software\"), to deal\n"
           "in the Software without restriction, including without limitation the rights\n"
           "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
"opies of the Software, and to permit persons to whom the Software is\n"
           "furnished to do so, subject to the following conditions:\n"
           "\n"
           "The above copyright notice and this permission notice shall be included in all\n"
           "copies or substantial portions of the Software.\n"
           "\n"
           "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
           "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
           "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
           "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
           "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
           "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
           "SOFTWARE.\n"
           "",
           prog, PACKAGE_NAME, PACKAGE_VERSION);
}


static
void print_usage(const char *const prog);

static
void print_usage(const char *const prog)
{
    printf("Usage: %s <command> <command_params...>\n"
           "\n"
           "Gives command line access to the USB control commands for the Soundcraft\n"
           "Notepad series of mixers to help verify the USB protocol description document.\n"
           "\n"
           "Commands:\n"
           "\n"
           "    --help     Print this usage message and exit.\n"
           "\n"
           "    --version  Print version message and exit.\n"
           "\n"
           "    audio-routing <NUM>\n"
           "               Find a supported device, and set its audio sources.\n"
           "               There must be exactly one supported device connected.\n"
           "               The valid source numbers are specific to the device:\n"
           "\n",
           prog);
    for (int i=0; supported_devices[i].idProduct != 0; ++i) {
        printf("               %s %s\n",
               supported_devices[i].name, supported_devices[i].source_descr);
        for (int k=0; k<NOTEPAD_SOURCES_MAX; k++) {
            printf("                 %d  %s\n",
                   k, supported_devices[i].sources[k]);
        }
        printf("\n");
    }
    printf("               Note that on the NOTEPAD-12FX 4-channel audio capture device,\n"
           "               capture device channels 1+2 are always fed from mixer CH 1+2.\n"
           "\n"
           "    ducker-off\n"
           "               Turn the ducker off.\n"
           "\n"
           "    ducker-on <INPUTS> <RELEASE>ms\n"
           "               Turn the ducker on and set the following values:\n"
           "                 INPUTS (range 0..15) is a bitmask for the input \"selection\"\n"
           "                         0b0000 =  0 = no inputs watched\n"
           "                         0b1111 = 15 = all four inputs watched\n"
           "                 RELEASE (range 0..5000) is a the \"release\" time in ms\n"
           "                         0 = release time 0, 5000 = release time 5000ms=5s\n"
           "\n"
           "    ducker-range <HEX_VALUE>|<RANGE>dB\n"
           "               Set the \"duck range\" to the given value.\n"
           "               Valid range is 0dB to 90dB, or 0 to 0x1fffffff.\n"
           "               It may be best to only use this while ducker is on.\n"
           "\n"
           "    ducker-threshold <HEX_VALUE>|<THRESH>dB\n"
           "               Set the \"threshold\" to the given value.\n"
           "               Valid range is -60dB to 0dB, or 0x000000 to 0x7fffff.\n"
           "               It may be best to only use this while ducker is on.\n"
           "\n"
           "    meter\n"
           "               Show the meter until you press Ctrl-C\n"
           "               It may be best to only use this while ducker is on.\n"
           );
}


static
int parse_command_audio_routing(const char *const param_sources)
    __attribute__(( nonnull(1) ));

static
int parse_command_audio_routing(const char *const param_sources)
{
    char *p = NULL;
    errno = 0;
    if (*(param_sources) == '\0') {
        fprintf(stderr, "Fatal: Looking for number, got empty string.\n");
        return EXIT_FAILURE;
    }
    const long lval = strtol(param_sources, &p, 10);
    if ((p == NULL) || (*p != '\0')) {
        fprintf(stderr, "Fatal: Error converting number\n");
        return EXIT_FAILURE;
    }
    if ((lval == LONG_MIN) || (lval == LONG_MAX)) {
        fprintf(stderr, "Fatal: Error converting number: number range\n");
        return EXIT_FAILURE;
    }
    if (lval < 0) {
        fprintf(stderr, "Fatal: Error converting number: negative\n");
        return EXIT_FAILURE;
    }
    if (lval > UINT8_MAX) {
        fprintf(stderr, "Fatal: Error converting number: too large\n");
        return EXIT_FAILURE;
    }

    // printf("audio-routing lval=%ld\n", lval);
    const uint8_t source_index = (uint8_t) lval;
    COND_OR_RETURN(source_index < NOTEPAD_SOURCES_MAX,
                   "sources index must be less than 4");

    command_params_T params;
    params.audio_routing.source_index = source_index;

    run_command(commandfunc_audio_routing, &params);
    return EXIT_SUCCESS;
}


static
int parse_command_ducker_on(const char *const param_inputs,
                            const char *const param_release_ms)
    __attribute__(( nonnull(1), nonnull(2) ));

static
int parse_command_ducker_on(const char *const param_inputs,
                            const char *const param_release)
{
    command_params_T params;

    if (true) {
        char *p = NULL;
        errno = 0;
        if (*(param_inputs) == '\0') {
            fprintf(stderr, "Fatal: Looking for number, got empty string.\n");
            return EXIT_FAILURE;
        }
        long lval;
        if (strncmp("0b", param_inputs, 2) == 0) {
            lval = strtol(&param_inputs[2], &p, 2);
        } else {
            lval = strtol(param_inputs, &p, 0);
        }
        // fprintf(stderr, "value conversion: %s to %ld\n", param_inputs, lval);
        if ((p == NULL) || (*p != '\0')) {
            fprintf(stderr, "Fatal: Error converting number\n");
            return EXIT_FAILURE;
        }
        if ((lval == LONG_MIN) || (lval == LONG_MAX)) {
            fprintf(stderr, "Fatal: Error converting number: number range\n");
            return EXIT_FAILURE;
        }
        if (lval < 0) {
            fprintf(stderr, "Fatal: Error converting number: negative\n");
            return EXIT_FAILURE;
        }
        if (lval > UINT8_MAX) {
            fprintf(stderr, "Fatal: Error converting number: too large\n");
            return EXIT_FAILURE;
        }
        /* range check input_set */
        if (lval > 15) {
            fprintf(stderr, "Fatal: Error converting number: outside valid range\n");
            return EXIT_FAILURE;
        }
        params.ducker_on.inputs = (uint8_t) lval;
    }

    if (true) {
        char *p = NULL;
        errno = 0;
        if (*(param_release) == '\0') {
            fprintf(stderr, "Fatal: Looking for number, got empty string.\n");
            return EXIT_FAILURE;
        }
        const long lval = strtol(param_release, &p, 10);
        if (p == NULL) {
            fprintf(stderr, "Fatal: Error converting number\n");
            return EXIT_FAILURE;
        }
        if (strcmp(p, "ms") != 0) {
            fprintf(stderr, "Fatal: Missing unit (ms)\n");
            return EXIT_FAILURE;
        }
        if ((lval == LONG_MIN) || (lval == LONG_MAX)) {
            fprintf(stderr, "Fatal: Error converting number: number range\n");
            return EXIT_FAILURE;
        }
        if (lval < 0) {
            fprintf(stderr, "Fatal: Error converting number: negative\n");
            return EXIT_FAILURE;
        }
        if (lval > UINT16_MAX) {
            fprintf(stderr, "Fatal: Error converting number: too large\n");
            return EXIT_FAILURE;
        }
        if (lval > 5000L) {
            fprintf(stderr, "Fatal: Error converting number: outside valid range\n");
            return EXIT_FAILURE;
        }
        params.ducker_on.release_ms = (uint16_t) lval;
    }

    run_command(commandfunc_ducker_on, &params);
    return EXIT_SUCCESS;
}


static
int parse_command_ducker_range(const char *const param_range)
    __attribute__(( nonnull(1) ));

static
int parse_command_ducker_range(const char *const param_range)
{
    command_params_T params;

    char *p = NULL;
    errno = 0;
    if (*(param_range) == '\0') {
        fprintf(stderr, "Fatal: Looking for number, got empty string.\n");
        return EXIT_FAILURE;
    }
    const long lval = strtol(param_range, &p, 0);
    if (p == NULL) {
        fprintf(stderr, "Fatal: Error converting number\n");
        return EXIT_FAILURE;
    }
    if ((lval == LONG_MIN) || (lval == LONG_MAX)) {
        fprintf(stderr, "Fatal: Error converting number: number range\n");
        return EXIT_FAILURE;
    }

    if (strcmp(p, "dB") == 0) { /* integer ending with unit "dB" */
        if (lval < 0) {
            fprintf(stderr, "Fatal: Error converting number: below 0dB\n");
            return EXIT_FAILURE;
        }
        if (lval > UINT32_MAX) {
            fprintf(stderr, "Fatal: Error converting number: too large\n");
            return EXIT_FAILURE;
        }
        if (lval > 90) {
            fprintf(stderr, "Fatal: Error converting number: above 90dB\n");
            return EXIT_FAILURE;
        }
        /* value range is now 0 .. 90 including */
        const double dval   = (double) lval;
        const uint32_t ui = dB_to_uint_range(dval);
        params.ducker_range.range = ui;
    } else if (*p == '\0') { /* integer without a unit */
        if (lval < 0) {
            fprintf(stderr, "Fatal: Error converting number: negative\n");
            return EXIT_FAILURE;
        }
        if (lval > UINT32_MAX) {
            fprintf(stderr, "Fatal: Error converting number: too large\n");
            return EXIT_FAILURE;
        }
        if (lval > 0x1fffffff) {
            fprintf(stderr, "Fatal: Error converting number: outside valid range\n");
            return EXIT_FAILURE;
        }
        params.ducker_range.range = (uint32_t) lval;
    } else {
        fprintf(stderr, "Fatal: Invalid unit (must be integer or integer with dB)\n");
        return EXIT_FAILURE;
    }

    run_command(commandfunc_ducker_range, &params);
    return EXIT_SUCCESS;
}


static
int parse_command_ducker_threshold(const char *const param_threshold)
    __attribute__(( nonnull(1) ));

static
int parse_command_ducker_threshold(const char *const param_threshold)
{
    command_params_T params;

    char *p = NULL;
    errno = 0;
    if (*(param_threshold) == '\0') {
        fprintf(stderr, "Fatal: Looking for number, got empty string.\n");
        return EXIT_FAILURE;
    }
    const long lval = strtol(param_threshold, &p, 0);
    if (p == NULL) {
        fprintf(stderr, "Fatal: Error converting number\n");
        return EXIT_FAILURE;
    }
    if ((lval == LONG_MIN) || (lval == LONG_MAX)) {
        fprintf(stderr, "Fatal: Error converting number: number range\n");
        return EXIT_FAILURE;
    }

    if (strcmp(p, "dB") == 0) { /* integer ending with unit "dB" */
        if (lval < -60) {
            fprintf(stderr, "Fatal: Error converting number: below -60dB\n");
            return EXIT_FAILURE;
        }
        if (lval > 0) {
            fprintf(stderr, "Fatal: Error converting number: above 0dB\n");
            return EXIT_FAILURE;
        }
        /* value range is now -60 .. 0 including */
        const double dval = (double) lval;
        const uint32_t ui = dB_to_uint_threshold(dval);
        params.ducker_threshold.thresh = ui;
    } else if (*p == '\0') { /* integer without a unit */
        if (lval < 0) {
            fprintf(stderr, "Fatal: Error converting number: negative\n");
            return EXIT_FAILURE;
        }
        if (lval > UINT32_MAX) {
            fprintf(stderr, "Fatal: Error converting number: too large\n");
            return EXIT_FAILURE;
        }
        if (lval > 0x1fffffff) {
            fprintf(stderr, "Fatal: Error converting number: outside valid range\n");
            return EXIT_FAILURE;
        }
        params.ducker_threshold.thresh = (uint32_t) lval;
    } else {
        fprintf(stderr, "Fatal: Invalid unit (must be integer or integer with dB)\n");
        return EXIT_FAILURE;
    }

    run_command(commandfunc_ducker_threshold, &params);
    return EXIT_SUCCESS;
}


/* undocumented/unsupported command */
static
int parse_command_dump_tables(void)
{
    printf("\n");
    printf("### value table for %s ###\n", "ducker range");
    printf("\n");
    printf("    %-6s  %-10s  %-10s\n", "__dB__", "_uint32_t_", "_uint32_t_");
    for (int dB=0; dB<=90; dB+=10) {
        const uint32_t u32 = dB_to_uint_range(dB);
        printf("    %4ddB  0x%08x  %10u\n", dB, u32, u32);
    }

    printf("\n");
    printf("### value table for %s ###\n", "ducker threshold");
    printf("\n");
    printf("    %-6s  %-10s  %-10s\n", "__dB__", "_uint32_t_", "_uint32_t_");
    for (int dB=-60; dB<=0; dB+=10) {
        const uint32_t u32 = dB_to_uint_threshold(dB);
        printf("    %4ddB  0x%08x  %10u\n", dB, u32, u32);
    }

    printf("\n");
    printf("### value table for %s ###\n", "ducker meter");
    printf("\n");
    printf("    %-6s  %-10s  %-10s\n", "__dB__", "_uint32_t_", "_uint32_t_");
    for (int dB=-100; dB<=0; dB+=10) {
        const uint32_t u32 = dB_to_uint_meter(dB);
        printf("    %4ddB  0x%08x  %10u\n", dB, u32, u32);
    }
    return EXIT_SUCCESS;
}


static
const char *arg0_to_prog(const char *const arg0);

static
const char *arg0_to_prog(const char *const arg0)
{
    const char *const last_slash = strrchr(arg0, '/');
    const char *const after_last_slash = last_slash?(last_slash+1):arg0;
    const char *const prog =
        (*after_last_slash == '\0')?arg0:after_last_slash;

#if 0
    fprintf(stderr, "argv[0] %s\n", arg0);
    fprintf(stderr, "last_slash %s\n", last_slash);
    fprintf(stderr, "after_last_slash %s\n", after_last_slash);
    fprintf(stderr, "prog %s\n", prog);
#endif

    return prog;
}


static
int parse_cmdline(const int argc, const char *const argv[]);

static
int parse_cmdline(const int argc, const char *const argv[])
{
#if 0
    for (int i=0; i<argc; i++) {
        printf("%3d. %s\n", i, argv[i]);
    }
#endif

    COND_OR_RETURN(argc > 0, "program invocations without arg0?!");

    const char *const prog = arg0_to_prog(argv[0]);

    COND_OR_RETURN(argc >= 2, "too few command line arguments");
    COND_OR_RETURN(argc <= 4, "too many command line arguments");

    if (false) {
        /* nothing */
    } else if ((argc == 2) && (strcmp(argv[1], "--help") == 0)) {
        print_usage(prog);
        return EXIT_SUCCESS;
    } else if ((argc == 2) && (strcmp(argv[1], "--version") == 0)) {
        print_version(prog);
        return EXIT_SUCCESS;
    } else if ((argc == 3) && (strcmp(argv[1], "audio-routing") == 0)) {
        return parse_command_audio_routing(argv[2]);
    } else if ((argc == 2) && (strcmp(argv[1], "dump-tables") == 0)) {
        /* undocumented/unsupported command */
        return parse_command_dump_tables();
    } else if ((argc == 2) && (strcmp(argv[1], "meter") == 0)) {
        command_params_T params;
        /* no params needed to run the meter */
        run_command(commandfunc_meter, &params);
        return EXIT_SUCCESS;
    } else if ((argc == 2) && (strcmp(argv[1], "ducker-off") == 0)) {
        command_params_T params;
        /* no params needed to turn off ducker  */
        run_command(commandfunc_ducker_off, &params);
        return EXIT_SUCCESS;
    } else if ((argc == 4) && (strcmp(argv[1], "ducker-on") == 0)) {
        return parse_command_ducker_on(argv[2], argv[3]);
    } else if ((argc == 3) && (strcmp(argv[1], "ducker-range") == 0)) {
        return parse_command_ducker_range(argv[2]);
    } else if ((argc == 3) && (strcmp(argv[1], "ducker-threshold") == 0)) {
        return parse_command_ducker_threshold(argv[2]);
    } else {
        fprintf(stderr, "Fatal: Unhandled command line argument(s)\n");
        return EXIT_FAILURE;
    }
}


int main(const int argc, const char *const argv[])
{
    const char *const env_scnp_cli_dry_run = getenv("SCNP_CLI_DRY_RUN");
    if (env_scnp_cli_dry_run && (*env_scnp_cli_dry_run != '\0')) {
        dry_run = true;

        char *endp = NULL;
        const uintmax_t uim = strtoumax(env_scnp_cli_dry_run, &endp, 0);
        if ((endp != NULL) && (endp != env_scnp_cli_dry_run) && (*endp == '\0')) {
            if (uim != UINTMAX_MAX) {
                dry_run_value = (uint32_t) uim;
            }
        }
    }

    return parse_cmdline(argc, argv);
}
