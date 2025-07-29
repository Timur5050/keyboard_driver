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
    struct usb_endpoint_descriptor *ep_desc;
    unsigned char *data;
    struct urb *urb;
};

static void keyboard_interrupt_handler(struct urb *urb)
{
    struct keyboard_logger *dev = urb->context;
    
    if (urb->status == 0) {
        pr_info(DRV_NAME " : data received: %*ph\n", urb->actual_length, dev->data);
    } else {
        pr_err(DRV_NAME " : URB error : %d\n", urb->status);
    }
    
    usb_submit_urb(urb, GFP_ATOMIC);
}

static int keyboard_logger_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_device *udev = interface_to_usbdev(interface);
    struct keyboard_logger *dev;
    int retval = 0;  
    int i;

    pr_info(DRV_NAME " : probe called for device : %s:%s\n", 
            udev->manufacturer ? udev->manufacturer : "Unknown",
            udev->product ? udev->product : "Unknown");

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev) {
        return -ENOMEM;
    }

    dev->udev = usb_get_dev(udev);
    dev->interface = interface;

    for (i = 0; i < dev->interface->num_altsetting; i++) {
        pr_info(DRV_NAME " : alt set num : %d\n", i);
        struct usb_host_interface *temp = &dev->interface->altsetting[i];
        for (int j = 0; j < temp->desc.bNumEndpoints; j++) {
            struct usb_endpoint_descriptor *temp_desc = &temp->endpoint[j].desc; 
            pr_info(DRV_NAME " : j = %d, type: 0x%x, address: 0x%02x, maxsize: %d\n",
                    j,
                    temp_desc->bmAttributes,
                    temp_desc->bEndpointAddress,
                    le16_to_cpu(temp_desc->wMaxPacketSize));
        }
    }

    struct usb_host_interface *iface_desc = dev->interface->cur_altsetting;
    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        struct usb_endpoint_descriptor *endpoint = &iface_desc->endpoint[i].desc;
        if (usb_endpoint_is_int_in(endpoint)) {
            dev->ep_desc = endpoint;
            break;
        }
    }

    if (!dev->ep_desc) {
        pr_err(DRV_NAME " : Could not find interrupt IN endpoint\n");
        retval = -ENODEV;
        goto error_no_endpoint;
    }

    dev->data = kmalloc(usb_endpoint_maxp(dev->ep_desc), GFP_KERNEL);
    if (!dev->data) {
        retval = -ENOMEM;
        goto error_alloc_buff;
    }

    dev->urb = usb_alloc_urb(0, GFP_KERNEL);
    if (!dev->urb) {
        retval = -ENOMEM;
        goto error_alloc_urb;
    }

    usb_fill_int_urb(  
        dev->urb,
        dev->udev,
        usb_rcvintpipe(dev->udev, dev->ep_desc->bEndpointAddress),
        dev->data,
        usb_endpoint_maxp(dev->ep_desc),
        keyboard_interrupt_handler,
        dev,
        dev->ep_desc->bInterval
    );

    retval = usb_submit_urb(dev->urb, GFP_KERNEL);
    if (retval) {
        pr_err(DRV_NAME " : Failed to submit URB: %d\n", retval);
        goto error_submit_urb;
    }

    usb_set_intfdata(interface, dev);  
    pr_info(DRV_NAME " : device connected successfully!\n");
    return 0;

error_submit_urb:
    usb_free_urb(dev->urb);
error_alloc_urb:
    kfree(dev->data);
error_alloc_buff:
error_no_endpoint:
    usb_put_dev(dev->udev);
    kfree(dev);
    return retval;
}

static void keyboard_logger_disconnect(struct usb_interface *interface)
{
    struct keyboard_logger *dev = usb_get_intfdata(interface);
    
    if (!dev) {
        pr_err(DRV_NAME " : disconnect called with NULL device\n");
        return;
    }

    pr_info(DRV_NAME " : device disconnecting...\n");

    usb_set_intfdata(interface, NULL);

    if (dev->urb) {
        usb_kill_urb(dev->urb); 
        usb_free_urb(dev->urb);
    }

    if (dev->data) {
        kfree(dev->data);
    }

    if (dev->udev) {
        usb_put_dev(dev->udev);
    }

    kfree(dev);
    
    pr_info(DRV_NAME " : device successfully disconnected\n");
}

static struct usb_driver keyboard_logger_driver = {
    .name = DRV_NAME,
    .id_table = keyboard_logger_table,
    .probe = keyboard_logger_probe,
    .disconnect = keyboard_logger_disconnect,
};

module_usb_driver(keyboard_logger_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Timur5050");
MODULE_DESCRIPTION("Simple USB keyboard logger driver");