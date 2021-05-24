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
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <libusb.h>


#include "auto-config.h"


#define COND_OR_FAIL(COND, MSG)                                       \
    do {                                                              \
        const bool cond = (COND);                                     \
        const char *const msg = (MSG);                                \
        if (!cond) {                                                  \
            fprintf(stderr, "Fatal: %s: !(%s)\n", msg, #COND);        \
            exit(EXIT_FAILURE);                                       \
        }                                                             \
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


/* We do not care about padding and storage efficiency here */
typedef struct {
    uint16_t idProduct;
    const char *const name;
    const char *const source_descr;
    const char *const sources[4];
} notepad_device_T;


static
const notepad_device_T handled_devices[] = {
    { 0x0030,
      "NOTEPAD-5",
      "channels 1+2 of 2-channel audio capture device",
      {"CH 1+2", "ST 2+3", "ST 4+5", "MASTER L+R"}},

    { 0x0031,
      "NOTEPAD-8FX",
      "channels 1+2 of 2-channel audio capture device",
      {"CH 1+2", "ST 3+4", "ST 5+6", "MASTER L+R"}},

    { 0x0032,
      "NOTEPAD-12FX",
      "channels 3+4 of 4-channel audio capture device",
      {"CH 3+4", "ST 5+6", "ST 7+8", "MASTER L+R"}},

    /* The termination marker is .idProduct being 0 */
    { 0 }
};


static
void get_matching_devices(libusb_device ***device_list, size_t *device_count)
    __attribute__(( nonnull(1), nonnull(2) ));

static
void get_matching_devices(libusb_device ***device_list, size_t *device_count)
{
    // printf("get_matching_devices\n");

    libusb_device **ret_list = calloc(128, sizeof(libusb_device *));
    if (ret_list == NULL) {
        perror("calloc libusb_device list");
        exit(EXIT_FAILURE);
    }
    size_t ret_count = 0;

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

        for (int k=0; handled_devices[k].idProduct != 0; ++k) {
            const notepad_device_T *const np_dev = &handled_devices[k];
            if ((0x05fc == desc.idVendor) &&
                (np_dev->idProduct == desc.idProduct)) {

                const uint8_t busnum = libusb_get_bus_number(dev);
                const uint8_t devaddr = libusb_get_device_address(dev);

                libusb_device_handle *dev_handle;
                const int luret_open =
                    libusb_open(dev, &dev_handle);
                LIBUSB_OR_FAIL(luret_open, "libusb_open");

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

                printf("Bus %03d Device %03d: ID %04x:%04x %s %s\n",
                       busnum, devaddr, desc.idVendor, desc.idProduct,
                       buf_manufacturer, buf_product);

                libusb_close(dev_handle);

                libusb_ref_device(dev);
                ret_list[ret_count++] = dev;
            }
        }
    }
    libusb_free_device_list(devices, 1);

    *device_list = ret_list;
    *device_count = ret_count;
}


static
const notepad_device_T *notepad_device_from_idProduct(const uint16_t idProduct);

static
const notepad_device_T *notepad_device_from_idProduct(const uint16_t idProduct)
{
    for (size_t i=0; handled_devices[i].idProduct != 0; ++i) {
        if (idProduct == handled_devices[i].idProduct) {
            return &handled_devices[i];
        }
    }
    return NULL;
}


static
void device_set(libusb_device *dev, const uint8_t v);

static
void device_set(libusb_device *dev, const uint8_t v)
{
    // printf("device_set(%p, %u)\n", (void *)dev, v);

    struct libusb_device_descriptor desc;
    const int luret_get_dev_descr =
        libusb_get_device_descriptor(dev, &desc);
    LIBUSB_OR_FAIL(luret_get_dev_descr, "libusb_get_device_descriptor");

    libusb_device_handle *dev_handle;
    const int luret_open =
        libusb_open(dev, &dev_handle);
    LIBUSB_OR_FAIL(luret_open, "libusb_open");

    const notepad_device_T *const notepad_device =
        notepad_device_from_idProduct(desc.idProduct);
    COND_OR_FAIL(notepad_device != NULL, "unhandled idProduct");

    printf("Setting USB audio source to %d (%s) for device %s\n",
	   v, notepad_device->sources[v], notepad_device->name);

    uint8_t data[8];
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x04;
    data[3] = 0x00;
    data[4] = v;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;

    const int luret_ctrl_transfer =
        libusb_control_transfer(dev_handle,
                                0x40 /* bmRequestType */,
                                16 /* bReqest */,
                                0 /* wValue */,
                                0 /* wIndex */,
                                data, sizeof(data),
                                10000 /* timeout in ms */);
    LIBUSB_OR_FAIL(luret_ctrl_transfer, "libusb_control_transfer");
    COND_OR_FAIL(luret_ctrl_transfer == sizeof(data),
                 "libusb_control_transfer");

    libusb_close(dev_handle);
}


static
void command_set(const uint8_t v);

static
void command_set(const uint8_t v)
{
    // printf("command_set(%u)\n", v);

    const int luret_init =
        libusb_init(NULL);
    LIBUSB_OR_FAIL(luret_init, "libusb_init");

    libusb_device **dev_list;
    size_t dev_count;
    get_matching_devices(&dev_list, &dev_count);

    if (dev_count == 0) {
        fprintf(stderr, "No Notepad devices found\n");
        exit(EXIT_FAILURE);
    }
    if (dev_count > 1) {
        fprintf(stderr, "Cannot handle more than one Notepad device yet. Sorry.\n");
        exit(EXIT_FAILURE);
    }

    device_set(dev_list[0], v);

    for (size_t i=0; i<dev_count; ++i) {
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
    printf("%s (%s) %s\n", prog, PACKAGE_NAME, PACKAGE_VERSION);
}


static
void print_usage(const char *const prog);

static
void print_usage(const char *const prog)
{
    printf("Usage: %s <command>\n"
           "\n"
           "Select mixer channels to connect the to USB audio capture device\n"
           "of a Soundcraft Notepad series mixer.\n"
           "\n"
           "Commands:\n"
           "\n"
           "    --help     Print this message and exit.\n"
           "\n"
           "    --version  Print version message and exit.\n"
           "\n"
           "    set <NUM>  Find a supported device, and set its audio sources.\n"
           "               There must be exactly one supported device connected.\n"
           "               The valid source numbers are specific to the device:\n"
           "\n",
           prog);
    for (int i=0; handled_devices[i].idProduct != 0; ++i) {
        printf("               %s %s\n",
               handled_devices[i].name, handled_devices[i].source_descr);
        for (int k=0; k<4; k++) {
            printf("                 %d  %s\n",
                   k, handled_devices[i].sources[k]);
        }
        printf("\n");
    }
    printf("               Note that on the NOTEPAD-12FX 4-channel audio capture device,\n"
           "               capture device channels 1+2 are always fed from mixer CH 1+2.\n"
           "\n"
           "Example:\n"
           "\n"
           "    [user@host ~]$ scnp-cli set 2\n"
           "    Bus 006 Device 003: ID 05fc:0032 Soundcraft Notepad-12FX\n"
           "    Setting USB audio source to 2 (ST 7+8) for device NOTEPAD-12FX\n"
           "    [user@host ~]$ \n");
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
    COND_OR_RETURN(argc <= 3, "too man command line arguments");

    if (false) {
        /* nothing */
    } else if ((argc == 2) && (strcmp(argv[1], "--help") == 0)) {
        print_usage(prog);
        return EXIT_SUCCESS;
    } else if ((argc == 2) && (strcmp(argv[1], "--version") == 0)) {
        print_version(prog);
        return EXIT_SUCCESS;
    } else if ((argc == 3) && (strcmp(argv[1], "set") == 0)) {
        char *p = NULL;
        errno = 0;
        const long lval = strtol(argv[2], &p, 10);
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
        if (lval > 3) {
            fprintf(stderr, "Fatal: Error converting number: value range\n");
            return EXIT_FAILURE;
        }
        // printf("SET lval=%ld\n", lval);
        const uint8_t u8val = (uint8_t) lval;
        command_set(u8val);
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "Fatal: Unhandled command line argument(s)\n");
        return EXIT_FAILURE;
    }
}


int main(const int argc, const char *const argv[])
{
    return parse_cmdline(argc, argv);
}
