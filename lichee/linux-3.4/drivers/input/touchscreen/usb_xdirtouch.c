#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

#define DRIVER_VERSION		"v0.1"
#define DRIVER_AUTHOR		"tony.luo"
#define DRIVER_DESC		"USB IrTouchscreen Driver"

#define ANDROID_4_X             1
#define MAX_PTS_NUM		16

static int swap_xy;
module_param(swap_xy, bool, 0644);
MODULE_PARM_DESC(swap_xy, "If set X and Y axes are swapped.");

static LIST_HEAD(xdtouch_points_list);

struct xdtouch_point {
        unsigned char id;
        int x, y;
        int touch;
        struct list_head node;
};

/* device specifc data/functions */
struct usb_xdirtouch_usb;
struct usb_xdirtouch_device_info {
	int min_xc, max_xc;
	int min_yc, max_yc;
	int min_press, max_press;
	int rept_size;

	void (*process_pkt) (struct usb_xdirtouch_usb *usb_xdirtouch, unsigned char *pkt, int len);

	/*
	 * used to get the packet len. possible return values:
	 * > 0: packet len
	 * = 0: skip one byte
	 * < 0: -return value more bytes needed
	 */
	int  (*get_pkt_len) (unsigned char *pkt, int len);

	int  (*read_data)   (struct usb_xdirtouch_usb *usb_xdirtouch, unsigned char *pkt);
	int  (*init)        (struct usb_xdirtouch_usb *usb_xdirtouch);
};

/* a usb_xdirtouch device */
struct usb_xdirtouch_usb {
	unsigned char *data;
	dma_addr_t data_dma;
	unsigned char *buffer;
	int buf_len;
	struct urb *irq;
	struct usb_device *udev;
	struct input_dev *input;
	struct usb_xdirtouch_device_info *type;
	char name[128];
	char phys[64];

	int x, y;
	int touch, press;
	int track_id;
//	int new;
};


/* device types */
enum {
	DEVTYPE_XDIRTOUCH,
};

#define USB_DEVICE_HID_CLASS(vend, prod) \
	.match_flags = USB_DEVICE_ID_MATCH_INT_CLASS \
		| USB_DEVICE_ID_MATCH_INT_PROTOCOL \
		| USB_DEVICE_ID_MATCH_DEVICE, \
	.idVendor = (vend), \
	.idProduct = (prod), \
	.bInterfaceClass = USB_INTERFACE_CLASS_HID, \
	.bInterfaceProtocol = USB_INTERFACE_PROTOCOL_MOUSE

static struct usb_device_id usb_xdirtouch_devices[] = {
	{USB_DEVICE(0x8883, 0x0002), .driver_info = DEVTYPE_XDIRTOUCH},
	{}
};

static unsigned char HostToDevDatas[4] = {0x35, 0x78, 0xa2, 0x96};
static unsigned char HostKey[3] = {0xbb, 0x0a, 0xb6};
static unsigned char DevKey;
static unsigned char MainKey[3];

/*****************************************************************************
 * General Touch Part
 */
static int xdtouch_read_data(struct usb_xdirtouch_usb *dev, unsigned char *pkt)
{
	struct xdtouch_point *xdtouch_point, *new_point;
	int found = 0;

	DevKey = pkt[4] + pkt[6];
	MainKey[0] = HostKey[0] + DevKey;
	MainKey[1] = HostKey[1] + DevKey;
	MainKey[2] = HostKey[2] + DevKey;

	dev->x = (((pkt[1] ^ MainKey[0]) & 0x0f) << 8) | (pkt[2] ^ MainKey[1]);
	dev->y = (((pkt[1] ^ MainKey[0]) & 0xf0) << 4) | (pkt[3] ^ MainKey[2]);

	switch(pkt[5] & 0x0f) {
	case 2:
	case 3:
		dev->touch = 1;	
		break;
	case 0:
		dev->touch = 0;
		break;
	default:
		;
	}

	dev->track_id = (pkt[5] & 0xf0) >> 4;

        list_for_each_entry(xdtouch_point, &xdtouch_points_list, node) {
                if (xdtouch_point->id == dev->track_id) {
			xdtouch_point->x = dev->x;
			xdtouch_point->y = dev->y;
			xdtouch_point->touch = dev->touch;
                        found = 1;
                        break;
                }
        }

	if (!found && dev->touch) {
        	new_point = kzalloc(sizeof(struct xdtouch_point), GFP_ATOMIC);
        	if (!new_point) {
                	printk(KERN_ERR "Memory allocation error\n");
                	return -ENOMEM;
        	}		
		new_point->id = dev->track_id;
		new_point->x = dev->x;
		new_point->y = dev->y;
		new_point->touch = dev->touch;
		list_add_tail(&new_point->node, &xdtouch_points_list);
	} 

        return 1;
}

static struct usb_xdirtouch_device_info usb_xdirtouch_dev_info[] = {
        [DEVTYPE_XDIRTOUCH] = {
                .min_xc         = 0x0,
                .max_xc         = 0xfff,
                .min_yc         = 0x0,
                .max_yc         = 0xfff,
                .rept_size      = 8,
                .read_data      = xdtouch_read_data,
        },
};


/*****************************************************************************
 * Generic Part
 */
static void usb_xdirtouch_process_pkt(struct usb_xdirtouch_usb *usb_xdirtouch,
                                 unsigned char *pkt, int len)
{
	struct usb_xdirtouch_device_info *type = usb_xdirtouch->type;
	struct xdtouch_point *xdtouch_point, *next;

	if (!type->read_data(usb_xdirtouch, pkt))
		return;

	if (list_empty(&xdtouch_points_list))
		return;

        list_for_each_entry_safe(xdtouch_point, next, &xdtouch_points_list, node) {
//		printk("--------id= %d, x= %d, y= %d, touch= %d\n",
//                	xdtouch_point->id, xdtouch_point->x, xdtouch_point->y, xdtouch_point->touch);

#ifdef ANDROID_4_X
                input_mt_slot(usb_xdirtouch->input, xdtouch_point->id);
                input_mt_report_slot_state(usb_xdirtouch->input, MT_TOOL_FINGER, xdtouch_point->touch ? true: false);
		if(xdtouch_point->touch) {
			if (swap_xy) {
				input_report_abs(usb_xdirtouch->input, ABS_MT_POSITION_X, xdtouch_point->y);
				input_report_abs(usb_xdirtouch->input, ABS_MT_POSITION_Y, xdtouch_point->x);
			} else {
                		input_report_abs(usb_xdirtouch->input, ABS_MT_POSITION_X, xdtouch_point->x);
                		input_report_abs(usb_xdirtouch->input, ABS_MT_POSITION_Y, xdtouch_point->y);
			}
                	input_report_abs(usb_xdirtouch->input, ABS_MT_TOUCH_MAJOR, xdtouch_point->touch);
//                	input_report_abs(usb_xdirtouch->input, ABS_MT_WIDTH_MAJOR, 128);
		}
#else
                input_report_abs(usb_xdirtouch->input, ABS_MT_TOUCH_MAJOR, xdtouch_point->touch);
//              input_report_abs(usb_xdirtouch->input, ABS_MT_TRACKING_ID, xdtouch_point->id);
                if (swap_xy) {
                        input_report_abs(usb_xdirtouch->input, ABS_MT_POSITION_X, xdtouch_point->y);
                        input_report_abs(usb_xdirtouch->input, ABS_MT_POSITION_Y, xdtouch_point->x);
                } else {
                        input_report_abs(usb_xdirtouch->input, ABS_MT_POSITION_X, xdtouch_point->x);
                        input_report_abs(usb_xdirtouch->input, ABS_MT_POSITION_Y, xdtouch_point->y);
                }
                input_mt_sync(usb_xdirtouch->input);
#endif
		if (!xdtouch_point->touch) {
			list_del(&xdtouch_point->node);
			kfree(xdtouch_point);
		}
	}
	input_sync(usb_xdirtouch->input);
}

static void usb_xdirtouch_irq(struct urb *urb)
{
	struct usb_xdirtouch_usb *usb_xdirtouch = urb->context;
	int retval;

	switch (urb->status) {
	case 0:
		/* success */
		break;
	case -ETIME:
		/* this urb is timing out */
		dbg("%s - urb timed out - was the device unplugged?",
		    __func__);
		return;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		/* this urb is terminated, clean up */
		dbg("%s - urb shutting down with status: %d",
		    __func__, urb->status);
		return;
	default:
		dbg("%s - nonzero urb status received: %d",
		    __func__, urb->status);
		goto exit;
	}

	usb_xdirtouch->type->process_pkt(usb_xdirtouch, usb_xdirtouch->data, urb->actual_length);

exit:
	retval = usb_submit_urb(urb, GFP_ATOMIC);
	if (retval)
		err("%s - usb_submit_urb failed with result: %d",
		    __func__, retval);
}

static int usb_xdirtouch_open(struct input_dev *input)
{
	struct usb_xdirtouch_usb *usb_xdirtouch = input_get_drvdata(input);

	usb_xdirtouch->irq->dev = usb_xdirtouch->udev;

	if (usb_submit_urb(usb_xdirtouch->irq, GFP_KERNEL))
		return -EIO;

	return 0;
}

static void usb_xdirtouch_close(struct input_dev *input)
{
	struct usb_xdirtouch_usb *usb_xdirtouch = input_get_drvdata(input);

	usb_kill_urb(usb_xdirtouch->irq);
}


static void usb_xdirtouch_free_buffers(struct usb_device *udev,
				  struct usb_xdirtouch_usb *usb_xdirtouch)
{
	//usb_buffer_free(udev, usb_xdirtouch->type->rept_size,
	//                usb_xdirtouch->data, usb_xdirtouch->data_dma);
	usb_free_coherent(udev, usb_xdirtouch->type->rept_size,
	                usb_xdirtouch->data, usb_xdirtouch->data_dma);
	kfree(usb_xdirtouch->buffer);
}


static int usb_xdirtouch_probe(struct usb_interface *intf,
			  const struct usb_device_id *id)
{
	struct usb_xdirtouch_usb *usb_xdirtouch;
	struct input_dev *input_dev;
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_xdirtouch_device_info *type;
	int err = -ENOMEM;

	interface = intf->cur_altsetting;
	endpoint = &interface->endpoint[0].desc;

	usb_xdirtouch = kzalloc(sizeof(struct usb_xdirtouch_usb), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!usb_xdirtouch || !input_dev)
		goto out_free;

	type = &usb_xdirtouch_dev_info[id->driver_info];
	usb_xdirtouch->type = type;
	if (!type->process_pkt)
		type->process_pkt = usb_xdirtouch_process_pkt;

	usb_xdirtouch->data = usb_alloc_coherent(udev, type->rept_size,
	                                  GFP_KERNEL, &usb_xdirtouch->data_dma);
	if (!usb_xdirtouch->data)
		goto out_free;

	if (type->get_pkt_len) {
		usb_xdirtouch->buffer = kmalloc(type->rept_size, GFP_KERNEL);
		if (!usb_xdirtouch->buffer)
			goto out_free_buffers;
	}

	usb_xdirtouch->irq = usb_alloc_urb(0, GFP_KERNEL);
	if (!usb_xdirtouch->irq) {
		dbg("%s - usb_alloc_urb failed: usb_xdirtouch->irq", __func__);
		goto out_free_buffers;
	}

	usb_xdirtouch->udev = udev;
	usb_xdirtouch->input = input_dev;

	if (udev->manufacturer)
		strlcpy(usb_xdirtouch->name, udev->manufacturer, sizeof(usb_xdirtouch->name));

	if (udev->product) {
		if (udev->manufacturer)
			strlcat(usb_xdirtouch->name, " ", sizeof(usb_xdirtouch->name));
		strlcat(usb_xdirtouch->name, udev->product, sizeof(usb_xdirtouch->name));
	}

	if (!strlen(usb_xdirtouch->name))
		snprintf(usb_xdirtouch->name, sizeof(usb_xdirtouch->name),
			"USB Touchscreen %04x:%04x",
			 le16_to_cpu(udev->descriptor.idVendor),
			 le16_to_cpu(udev->descriptor.idProduct));

	usb_make_path(udev, usb_xdirtouch->phys, sizeof(usb_xdirtouch->phys));
	strlcat(usb_xdirtouch->phys, "/input0", sizeof(usb_xdirtouch->phys));

	input_dev->name = usb_xdirtouch->name;
	input_dev->phys = usb_xdirtouch->phys;
	usb_to_input_id(udev, &input_dev->id);
	input_dev->dev.parent = &intf->dev;

	input_set_drvdata(input_dev, usb_xdirtouch);

	input_dev->open = usb_xdirtouch_open;
	input_dev->close = usb_xdirtouch_close;

#ifdef ANDROID_4_X
	__set_bit(EV_KEY, input_dev->evbit);
        __set_bit(EV_ABS, input_dev->evbit);
        __set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_mt_init_slots(input_dev, MAX_PTS_NUM);
        input_set_abs_params(input_dev, ABS_MT_POSITION_X, type->min_xc, type->max_xc, 0, 0);
        input_set_abs_params(input_dev, ABS_MT_POSITION_Y, type->min_yc, type->max_yc, 0, 0);
        input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
        input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 15, 0, 0);	
#else
        input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
        input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

//      input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 15, 0, 0);
        input_set_abs_params(input_dev, ABS_MT_POSITION_X, type->min_xc, type->max_xc, 0, 0);
        input_set_abs_params(input_dev, ABS_MT_POSITION_Y, type->min_yc, type->max_yc, 0, 0);
        input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
        input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 15, 0, 0);
#endif
	usb_fill_int_urb(usb_xdirtouch->irq, usb_xdirtouch->udev,
			 usb_rcvintpipe(usb_xdirtouch->udev, endpoint->bEndpointAddress),
			 usb_xdirtouch->data, type->rept_size,
			 usb_xdirtouch_irq, usb_xdirtouch, endpoint->bInterval);

	usb_xdirtouch->irq->dev = usb_xdirtouch->udev;
	usb_xdirtouch->irq->transfer_dma = usb_xdirtouch->data_dma;
	usb_xdirtouch->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	/* device specific init */
	if (type->init) {
		err = type->init(usb_xdirtouch);
		if (err) {
			dbg("%s - type->init() failed, err: %d", __func__, err);
			goto out_free_buffers;
		}
	}

	err = input_register_device(usb_xdirtouch->input);
	if (err) {
		dbg("%s - input_register_device failed, err: %d", __func__, err);
		goto out_free_buffers;
	}

	usb_set_intfdata(intf, usb_xdirtouch);

	return 0;

out_free_buffers:
	usb_xdirtouch_free_buffers(udev, usb_xdirtouch);
out_free:
	input_free_device(input_dev);
	kfree(usb_xdirtouch);
	return err;
}

static void usb_xdirtouch_disconnect(struct usb_interface *intf)
{
	struct usb_xdirtouch_usb *usb_xdirtouch = usb_get_intfdata(intf);

	dbg("%s - called", __func__);

	if (!usb_xdirtouch)
		return;

	dbg("%s - usb_xdirtouch is initialized, cleaning up", __func__);
	usb_set_intfdata(intf, NULL);
	usb_kill_urb(usb_xdirtouch->irq);
	input_unregister_device(usb_xdirtouch->input);
	usb_free_urb(usb_xdirtouch->irq);
	usb_xdirtouch_free_buffers(interface_to_usbdev(intf), usb_xdirtouch);
	kfree(usb_xdirtouch);
}

MODULE_DEVICE_TABLE(usb, usb_xdirtouch_devices);

static struct usb_driver usb_xdirtouch_driver = {
	.name		= "usb_xdirtouchscreen",
	.probe		= usb_xdirtouch_probe,
	.disconnect	= usb_xdirtouch_disconnect,
	.id_table	= usb_xdirtouch_devices,
};

static int __init usb_xdirtouch_init(void)
{
	return usb_register(&usb_xdirtouch_driver);
}

static void __exit usb_xdirtouch_cleanup(void)
{
	usb_deregister(&usb_xdirtouch_driver);
}

module_init(usb_xdirtouch_init);
module_exit(usb_xdirtouch_cleanup);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
