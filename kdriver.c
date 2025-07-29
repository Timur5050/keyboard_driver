#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/slab.h>

#define VENDOR_ID 0x0416
#define DEVICE_ID 0xa0f8
#define DRV_NAME "keyboard_logger"

static struct usb_device_id keyboard_logger_table[] = {
  { USB_DEVICE_AND_INTERFACE_INFO(VENDOR_ID, DEVICE_ID, 3, 1, 1) },
  {}
};
MODULE_DEVICE_TABLE(usb, keyboard_logger_table);

struct keyboard_logger {
  struct usb_device *udev;
  struct usb_interface *interface;
  unsigned char *data;
};

static int keyboard_logger_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
  struct usb_device *udev = interface_to_usbdev(interface);
  struct usb_endpoint_descriptor *ep_desc;
  struct keyboard_logger *dev;
  
  pr_info(DRV_NAME " : probe, called for device : %s:%s\n", udev->manufacturer, udev->product);
  
  dev = kzalloc(sizeof(*dev), GFP_KERNEL);
  if(!dev)
  {
    return -ENOMEM;
  }
  
  usb_set_intfdata(interface, udev);
  
  pr_info(DRV_NAME " : device connected sucessfully!\n");
  return 0;
}


static void keyboard_logger_disconnect(struct usb_interface *interface)
{
  struct keyboard_logger *udev = usb_get_intfdata(interface);
  usb_set_intfdata(interface, NULL);
  kfree(udev);
  
  pr_info(DRV_NAME " : device successfully disconnected");
}


static struct usb_driver keyboard_logger_driver = {
  .name = DRV_NAME,
  .id_table = keyboard_logger_table,
  .probe = keyboard_logger_probe,
  .disconnect =  keyboard_logger_disconnect,
};

module_usb_driver(keyboard_logger_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Timur5050");
MODULE_DESCRIPTION("device driver for keyboard");

