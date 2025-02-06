#include "libusb-1.0/libusb.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// G502 specifc data retrieved via lsusb
#define G502_VID 0x046d // G502 Vender ID
#define G502_PID 0xc08b // G502 Product ID
#define G502_INTERFACE 0x01

// G502 magic numbers pulled via wireshark
#define G502_MAGIC_NUMBER_1 0x11
#define G502_MAGIC_NUMBER_2 0xff
#define G502_MAGIC_NUMBER_3 0x02
#define G502_MAGIC_NUMBER_4 0x3c

// SET_REPORT config pulled via wireshark, mouse config is sent via SET_REPORT
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

enum G502_LED_ID { LOGO_LED = 0x01, DPI_LED = 0x00 };

static int set_mouse_color(uint8_t interface, uint8_t red, uint8_t blue,
                           enum G502_LED_ID green) {
  int ret = -1;

  libusb_init_context(NULL, NULL, 0);

  libusb_device_handle *handle;

  LOG_DEBUG("Opening device with VID:PID %04x/%04x\n", G502_VID, G502_PID);

  handle = libusb_open_device_with_vid_pid(NULL, G502_VID, G502_PID);

  if (handle == NULL) {
    fprintf(stderr, "ERROR: Failed opening device");
    goto cleanup;
  }

  // check for kernel driver
  if (libusb_kernel_driver_active(handle, G502_INTERFACE)) {
    LOG_DEBUG("Found active kernel, deactivating...");

    int result = libusb_detach_kernel_driver(handle, G502_INTERFACE);

    if (result < 0) {
      const char *error_name = libusb_error_name(result);
      fprintf(stderr, "ERROR: %d %s while claiming interface", result,
              error_name);
      goto cleanup;
    }
  }

  int status = libusb_claim_interface(handle, G502_INTERFACE);

  if (status < 0) {
    const char *error_name = libusb_error_name(status);
    fprintf(stderr, "ERROR: %d %s while claiming interface", status,
            error_name);
    return -1;
  }

  unsigned char data[] = {
      G502_MAGIC_NUMBER_1,
      G502_MAGIC_NUMBER_2,
      G502_MAGIC_NUMBER_3,
      G502_MAGIC_NUMBER_4,
      interface, // interface: 0x01 for logo*/
      0x01,      // config: 0x00 off, 0x01 fixed, 0x02 breathing, 0x03 cycle*/
      red,       // red component*/
      blue,      // blue component*/
      green,     // green component*/
      0x02,
      0x00,
      0x00, // fixed config*/
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00};

  // at this point, we have the interface: let's add a solid color
  int result =
      libusb_control_transfer(handle, USB_REQUEST_TYPE, USB_REQUEST, USB_VALUE,
                              USB_INDEX, data, sizeof(data), USB_TIMEOUT);

  if (result < 0) {
    const char *error_name = libusb_error_name(result);
    fprintf(stderr, "ERROR: %d %s while sending packet", result, error_name);
    goto cleanup;
  }

  LOG_DEBUG("Closing device...\n");

cleanup:
  if (handle) {
    libusb_release_interface(handle, G502_INTERFACE);

    // re-enable the kernel--this is quite important, as I accidentally lost all
    // peripherals from forgetting this
    libusb_attach_kernel_driver(handle, G502_INTERFACE);

    libusb_close(handle);
  }

  libusb_exit(NULL);

  return ret;
}

void printUsage() {
  printf("USAGE:\n");
  printf("  g502-controller [command] <options>\n");
  printf("\n");
  printf("COMMANDS:\n");
  printf("  %-20s %s\n", "logo-light <hex>",
         "Sets the color of the Logitech LED Logo");
  printf("  %-20s %s\n", "dpi-light <hex>", "Sets the color of the DPI lights");
}

int validateHex(char *hex) {
  int strLen = strlen(hex);

  // valid formats: 0xRRGGBB, 000000
  if (strLen != 6 && strLen != 8) {
    return -1;
  }

  int start = 0;

  // check for valid prefix
  if (strLen == 8) {
    if (hex[0] != '0' || hex[1] != 'x') {
      return -1;
    }

    start = 2;
  }

  for (int i = start; i < strLen; i++) {
    // check each character for valid format
    if (!((hex[i] >= '0' && hex[i] <= '9') ||
          (hex[i] >= 'A' && hex[i] <= 'F') ||
          (hex[i] >= 'a' && hex[i] <= 'f'))) {
      return -1;
    }
  }

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printUsage();
    return 0;
  }

  char *input = argv[1];

  if (strcmp(input, "logo-light") == 0) {
    if (validateHex(argv[2]) == -1) {
      printf("ERROR: Invalid hexcode provided: %s", argv[2]);
      return 0;
    }

    int hex = (int)strtol(argv[2], NULL, 16);

    set_mouse_color(LOGO_LED, (hex & 0xFF0000) >> 16, (hex & 0xFF00) >> 8,
                    hex & 0xFF);
  } else if (strcmp(input, "dpi-light") == 0) {
    if (validateHex(argv[2]) == -1) {
      printf("ERROR: Invalid hexcode provided: %s", argv[2]);
      return 0;
    }

    int hex = (int)strtol(argv[2], NULL, 16);

    set_mouse_color(DPI_LED, (hex & 0xFF0000) >> 16, (hex & 0xFF00) >> 8,
                    hex & 0xFF);
  } else {
    printUsage();
    return -1;
  }

  return 0;
}
