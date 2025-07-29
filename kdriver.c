#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/slab.h>

#define VENDOR_ID 0x0416
#define PRODUCT_ID 0xa0f8

static struct usb_device_id keyboard_logger_table[] = {
  { USB_DEVICE_AND_INTERFACE_INFO(VENDOR_ID, DEVICE_ID, 3, 1, 1) },
  {}
};
MODULE_DEVICE_TABLE(usb, keyboard_logger_table);

struct keyboard_logger {
  struct usb_device *udev;
  struct usb_interface *interface;
  unsigned char *data;
  struct urb *urb;
};


