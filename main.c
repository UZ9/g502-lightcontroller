#include "libusb-1.0/libusb.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// G502 ID retrieved via lsusb
#define VID 0x046d // G502 Vender ID
#define PID 0xc08b // G502 Product ID

#define G502_LOGO_INTERFACE 0x01

// Captured via wireshark, mouse config is sent via SET_REPORT
// note: endian is reversed
#define USB_REQUEST_TYPE 0x21
#define USB_REQUEST 0x09
#define USB_VALUE 0x0211
#define USB_INDEX 0x0001
#define USB_TIMEOUT 5000 // ms

#define DEBUG 0
#define LOG_DEBUG(fmt, ...)                                                    \
  do {                                                                         \
    if (DEBUG)                                                                 \
      printf(fmt, ##__VA_ARGS__);                                              \
  } while (0)

static int set_mouse_color(unsigned char *color) {
  if (!color) {
    fprintf(stderr, "ERROR: invalid color provided  to set_mouse_color");
    return -1;
  }

  libusb_device_handle *handle;
  libusb_device *device;
  uint8_t bus;

  LOG_DEBUG("Opening device with VID:PID %04x/%04x\n", VID, PID);

  handle = libusb_open_device_with_vid_pid(NULL, VID, PID);

  if (handle == NULL) {
    fprintf(stderr, "ERROR: Failed opening device");
    return -1;
  }

  // check for kernel driver
  if (libusb_kernel_driver_active(handle, G502_LOGO_INTERFACE)) {
    LOG_DEBUG("Found active kernel, deactivating...");

    int result = libusb_detach_kernel_driver(handle, G502_LOGO_INTERFACE);

    if (result < 0) {
      const char *error_name = libusb_error_name(result);
      fprintf(stderr, "ERROR: %d %s while claiming interface", result,
              error_name);
      return -1;
    }
  }

  int status = libusb_claim_interface(handle, G502_LOGO_INTERFACE);

  if (status < 0) {
    const char *error_name = libusb_error_name(status);
    fprintf(stderr, "ERROR: %d %s while claiming interface", status,
            error_name);
    return -1;
  }

  unsigned char data[] = {
      0x11,     0xff, 0x02, 0x3c, // g502 magic numbers*/
      0x01,                       // interface: 0x01 for logo*/
      0x01,     // config: 0x00 off, 0x01 fixed, 0x02 breathing, 0x03 cycle*/
      color[0], // red component*/
      color[1], // blue component*/
      color[2], // green component*/
      0x02,     0x00, 0x00, // fixed config*/
      0x00,     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  // at this point, we have the interface: let's add a solid color
  int result =
      libusb_control_transfer(handle, USB_REQUEST_TYPE, USB_REQUEST, USB_VALUE,
                              USB_INDEX, data, sizeof(data), USB_TIMEOUT);

  if (result < 0) {
    const char *error_name = libusb_error_name(result);
    fprintf(stderr, "ERROR: %d %s while sending packet", result, error_name);
    libusb_release_interface(handle, G502_LOGO_INTERFACE);
    libusb_attach_kernel_driver(handle, G502_LOGO_INTERFACE);
    libusb_close(handle);
    libusb_exit(NULL);

    return -1;
  }

  LOG_DEBUG("Closing device...\n");

  libusb_release_interface(handle, G502_LOGO_INTERFACE);

  // re-enable the kernel--this is quite important, as I accidentally lost all
  // peripherals from forgetting this
  libusb_attach_kernel_driver(handle, G502_LOGO_INTERFACE);

  libusb_close(handle);
  libusb_exit(NULL);

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "USAGE: g502-light-controller <failed|passed>\n");
    return 0;
  }

  char *input = argv[1];
  int passed = 0;

  if (strcmp(input, "failed") == 0) {
    passed = 0;
  } else if (strcmp(input, "passed") == 0) {
    passed = 1;
  } else {
    fprintf(stderr, "USAGE: g502-light-controller <failed|passed>\n");
    return -1;
  }

  libusb_init_context(NULL, NULL, 0);

  unsigned char passedColor[] = {0x00, 0xFF, 0x00};
  unsigned char failedColor[] = {0xFF, 0x00, 0x00};

  return set_mouse_color(passed ? passedColor : failedColor);
}
