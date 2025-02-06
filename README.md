# G502 Light Controller 

An LED controller for the G502 Hero Mouse to programatically change its colors.

## Requirements
- libusb-devel
- G502 mouse (or a similar logitech mouse with the vendor/product ID changed)

## Building
```
make
```

## Usage

Sending information to your G502 mouse requires sudo permissions by default. If you would rather run this from a user, follow these steps:

1. Create a new file `/etc/udev/rules.d/usb.rules`
2. Paste the following: `SUBSYSTEM=="usb", ATTRS{idVendor}="046d", ATTRS{idProduct}=="c08b", MODE="0666"`
3. Disconnect and reconnect the mouse

```
USAGE:
  g502-controller [command] <options>

COMMANDS:
  logo-light <hex>     Sets the color of the Logitech LED Logo
  dpi-light <hex>      Sets the color of the DPI lights
```
