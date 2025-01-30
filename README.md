# G502 Light Controller 

An LED controller for the G502 Hero Mouse to programatically change its logo color.

## Requirements:
- libusb-devel
- G502 mouse (or a similar logitech mouse with the vendor/product ID changed)

## Building:
```
make clean && make
```

## Running 

- `sudo g502-lightcontroller <failed|passed>` will change the logo to red (failed) or green (passed)

For libusb to access your G502 device without sudo permissions, you can do the following:

1. Create a new file `/etc/udev/rules.d/usb.rules`
2. Paste the following: `SUBSYSTEM=="usb", ATTRS{idVendor}="046d", ATTRS{idProduct}=="c08b", MODE="0666"`
3. Disconnect and reconnect the mouse
