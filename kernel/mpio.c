/* -*- linux-c -*- */

/* 
 * Driver for USB MPIO-DMG
 *
 * Yuji Touya (salmoon@users.sourceforge.net)
 *
 * small adaptions for Kernel 2.4.x support by
 * Markus Germeier (mager@tzi.de)
 * 
 * based on rio500.c by Cesar Miquel (miquel@df.uba.ar)
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * */
#include <linux/config.h>
#include <linux/version.h>
#include <linux/mm.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/random.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/usb.h>
#include <linux/smp_lock.h>

#ifdef CONFIG_USB_DEBUG
#	define DEBUG
#else
#	undef DEBUG
#endif

/*
 * Version Information
 */
#define DRIVER_VERSION "v0.1a-pre1"
#define DRIVER_AUTHOR "Yuji Touya <salmoon@users.sourceforge.net>"
#define DRIVER_DESC "USB MPIO-DMG driver"

#define MPIO_MINOR    70

/* stall/wait timeout for mpio */
#define NAK_TIMEOUT (HZ)

/* Size of the mpio buffer */
#define IBUF_SIZE 0x10000
#define OBUF_SIZE 0x10000

struct mpio_usb_data {
        struct usb_device *mpio_dev;    /* init: probe_mpio */
        unsigned int ifnum;             /* Interface number of the USB device */
        int isopen;                     /* nz if open */
        int present;                    /* Device is present on the bus */
        char *obuf, *ibuf;              /* transfer buffers */
        char bulk_in_ep, bulk_out_ep;   /* Endpoint assignments */
        wait_queue_head_t wait_q;       /* for timeouts */
	struct semaphore lock;          /* general race avoidance */
};

static struct mpio_usb_data mpio_instance;

static int open_mpio(struct inode *inode, struct file *file)
{
	struct mpio_usb_data *mpio = &mpio_instance;

	lock_kernel();

	if (mpio->isopen || !mpio->present) {
		unlock_kernel();
		return -EBUSY;
	}
	mpio->isopen = 1;

	init_waitqueue_head(&mpio->wait_q);

	MOD_INC_USE_COUNT;

	unlock_kernel();

	info("MPIO opened.");

	return 0;
}

static int close_mpio(struct inode *inode, struct file *file)
{
	struct mpio_usb_data *mpio = &mpio_instance;

	mpio->isopen = 0;

	MOD_DEC_USE_COUNT;

	info("MPIO closed.");
	return 0;
}

static ssize_t
write_mpio(struct file *file, const char *buffer, size_t count, loff_t * ppos)
{
	struct mpio_usb_data *mpio = &mpio_instance;

	unsigned long copy_size;
	unsigned long bytes_written = 0;
	unsigned int partial;

	int result = 0;
	int maxretry;
	int errn = 0;

        /* Sanity check to make sure mpio is connected, powered, etc */
        if ( mpio == NULL ||
             mpio->present == 0 ||
             mpio->mpio_dev == NULL )
          return -1;

	down(&(mpio->lock));

	do {
		unsigned long thistime;
		char *obuf = mpio->obuf;

		thistime = copy_size =
		    (count >= OBUF_SIZE) ? OBUF_SIZE : count;
		if (copy_from_user(mpio->obuf, buffer, copy_size)) {
			errn = -EFAULT;
			goto error;
		}
		maxretry = 5;
		while (thistime) {
			if (!mpio->mpio_dev) {
				errn = -ENODEV;
				goto error;
			}
			if (signal_pending(current)) {
			        errn = bytes_written ? bytes_written : -EINTR;
				goto error;
			}

			result =
			  usb_bulk_msg(mpio->mpio_dev,
				       usb_sndbulkpipe(mpio->mpio_dev, 1),
				       obuf, thistime, &partial, 5 * HZ);

			dbg("write stats: result:%d thistime:%lu partial:%u",
			     result, thistime, partial);

			/* NAK - so hold for a while */
			if (result == USB_ST_TIMEOUT) {
				if (!maxretry--) {
					errn = -ETIME;
					goto error;
				}
				interruptible_sleep_on_timeout(&mpio-> wait_q,
							       NAK_TIMEOUT);
				continue;
			} else if (!result & partial) {
				obuf += partial;
				thistime -= partial;
			} else
				break;
		};
		if (result) {
			err("Write Whoops - %x", result);
			errn = -EIO;
			goto error;
		}
		bytes_written += copy_size;
		count -= copy_size;
		buffer += copy_size;
	} while (count > 0);

	errn = bytes_written ? bytes_written : -EIO;

error:
	up(&(mpio->lock));
	return errn;
}

static ssize_t
read_mpio(struct file *file, char *buffer, size_t count, loff_t * ppos)
{
	struct mpio_usb_data *mpio = &mpio_instance;
	ssize_t read_count;
	unsigned int partial;
	int this_read;
	int result;
	int maxretry = 10;
	char *ibuf = mpio->ibuf;

        /* Sanity check to make sure mpio is connected, powered, etc */
        if ( mpio == NULL ||
             mpio->present == 0 ||
             mpio->mpio_dev == NULL )
          return -1;

	read_count = 0;

	down(&(mpio->lock));

	while (count > 0) {
		if (signal_pending(current)) {
			up(&(mpio->lock));
			return read_count ? read_count : -EINTR;
		}
		if (!mpio->mpio_dev) {
			up(&(mpio->lock));
			return -ENODEV;
		}
		this_read = (count >= IBUF_SIZE) ? IBUF_SIZE : count;

		result = usb_bulk_msg(mpio->mpio_dev,
				      usb_rcvbulkpipe(mpio->mpio_dev, 2),
				      ibuf, this_read, &partial,
				      (int) (HZ * 8));

		dbg(KERN_DEBUG "read stats: result:%d this_read:%u partial:%u",
		       result, this_read, partial);

		if (partial) {
			count = this_read = partial;
			
		} else if (result == USB_ST_TIMEOUT || result == 15) { /* FIXME: 15 ? */
			if (!maxretry--) {
				up(&(mpio->lock));
				err("read_mpio: maxretry timeout");
				return -ETIME;
			}
			interruptible_sleep_on_timeout(&mpio->wait_q,
						       NAK_TIMEOUT);
			continue;
		} else if (result != USB_ST_DATAUNDERRUN) {
			up(&(mpio->lock));
			err("Read Whoops - result:%u partial:%u this_read:%u",
			     result, partial, this_read);
			return -EIO;
		} else {
			up(&(mpio->lock));
			return (0);
		}

		if (this_read) {
			if (copy_to_user(buffer, ibuf, this_read)) {
				up(&(mpio->lock));
				return -EFAULT;
			}
			count -= this_read;
			read_count += this_read;
			buffer += this_read;
		}
	}
	up(&(mpio->lock));
	return read_count;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0) /* Hmm, .... */
static void *probe_mpio(struct usb_device *dev, unsigned int ifnum,
			const struct usb_device_id *id)
#else
static void *probe_mpio(struct usb_device *dev, unsigned int ifnum)
#endif
{
	struct mpio_usb_data *mpio = &mpio_instance;

	if (dev->descriptor.idVendor != 0x2735 /* Digit@lway */ ) {
		return NULL;
	}

	if (dev->descriptor.idProduct != 0x1 /* MPIO-DMG */ ) {
		warn(KERN_INFO "MPIO player model not supported/tested.");
		return NULL;
	}

	info("USB MPIO found at address %d", dev->devnum);
	usb_show_device(dev); /* Show device info */

	mpio->present = 1;
	mpio->mpio_dev = dev;

	if (!(mpio->obuf = (char *) kmalloc(OBUF_SIZE, GFP_KERNEL))) {
		err("probe_mpio: Not enough memory for the output buffer");
		return NULL;
	}
	dbg("probe_mpio: obuf address:%p", mpio->obuf);

	if (!(mpio->ibuf = (char *) kmalloc(IBUF_SIZE, GFP_KERNEL))) {
		err("probe_mpio: Not enough memory for the input buffer");
		kfree(mpio->obuf);
		return NULL;
	}
	dbg("probe_mpio: ibuf address:%p", mpio->ibuf);

	init_MUTEX(&(mpio->lock));

	return mpio;
}

static void disconnect_mpio(struct usb_device *dev, void *ptr)
{
	struct mpio_usb_data *mpio = (struct mpio_usb_data *) ptr;

	if (mpio->isopen) {
		mpio->isopen = 0;
		/* better let it finish - the release will do whats needed */
		mpio->mpio_dev = NULL;
		return;
	}
	kfree(mpio->ibuf);
	kfree(mpio->obuf);

	info("USB MPIO disconnected.");

	mpio->present = 0;
}

static struct
file_operations usb_mpio_fops = {
	read:		read_mpio,
	write:		write_mpio,
/*  	ioctl:		ioctl_mpio, */
	open:		open_mpio,
	release:	close_mpio,
};

static struct usb_device_id mpio_table [] = {
	{ USB_DEVICE(0x2735, 1) }, 		/* MPIO-* (all models?) */
	{ }					/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, mpio_table);

static struct usb_driver mpio_driver = {
	name:		"mpio",
	probe:		probe_mpio,
	disconnect:	disconnect_mpio,
	fops:		&usb_mpio_fops,
	minor:		MPIO_MINOR,
	id_table:	mpio_table,
};

int usb_mpio_init(void)
{
	if (usb_register(&mpio_driver) < 0)
		return -1;

	info("USB MPIO support registered.");
        info(DRIVER_VERSION ":" DRIVER_DESC);

	return 0;
}


void usb_mpio_cleanup(void)
{
	struct mpio_usb_data *mpio = &mpio_instance;

	mpio->present = 0;
	usb_deregister(&mpio_driver);


}


/* ---------- debugging helpers ----------- */

static void usb_show_endpoint(struct usb_endpoint_descriptor *endpoint)
{
	usb_show_endpoint_descriptor(endpoint);
}

static void usb_show_interface(struct usb_interface_descriptor *altsetting)
{
	int i;

	usb_show_interface_descriptor(altsetting);

	for (i = 0; i < altsetting->bNumEndpoints; i++)
		usb_show_endpoint(altsetting->endpoint + i);
}

static void usb_show_config(struct usb_config_descriptor *config)
{
	int i, j;
	struct usb_interface *ifp;

	usb_show_config_descriptor(config);
	for (i = 0; i < config->bNumInterfaces; i++) {
		ifp = config->interface + i;

		if (!ifp)
			break;

		printk("\n  Interface: %d\n", i);
		for (j = 0; j < ifp->num_altsetting; j++)
			usb_show_interface(ifp->altsetting + j);
	}
}

void usb_show_device(struct usb_device *dev)
{
	int i;

	usb_show_device_descriptor(&dev->descriptor);
	for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
		usb_show_config(dev->config + i);
}

/*
 * Parse and show the different USB descriptors.
 */
void usb_show_device_descriptor(struct usb_device_descriptor *desc)
{
	if (!desc)
	{
		printk("Invalid USB device descriptor (NULL POINTER)\n");
		return;
	}
	printk("  Length              = %2d%s\n", desc->bLength,
		desc->bLength == USB_DT_DEVICE_SIZE ? "" : " (!!!)");
	printk("  DescriptorType      = %02x\n", desc->bDescriptorType);

	printk("  USB version         = %x.%02x\n",
		desc->bcdUSB >> 8, desc->bcdUSB & 0xff);
	printk("  Vendor:Product      = %04x:%04x\n",
		desc->idVendor, desc->idProduct);
	printk("  MaxPacketSize0      = %d\n", desc->bMaxPacketSize0);
	printk("  NumConfigurations   = %d\n", desc->bNumConfigurations);
	printk("  Device version      = %x.%02x\n",
		desc->bcdDevice >> 8, desc->bcdDevice & 0xff);

	printk("  Device Class:SubClass:Protocol = %02x:%02x:%02x\n",
		desc->bDeviceClass, desc->bDeviceSubClass, desc->bDeviceProtocol);
	switch (desc->bDeviceClass) {
	case 0:
		printk("    Per-interface classes\n");
		break;
	case USB_CLASS_AUDIO:
		printk("    Audio device class\n");
		break;
	case USB_CLASS_COMM:
		printk("    Communications class\n");
		break;
	case USB_CLASS_HID:
		printk("    Human Interface Devices class\n");
		break;
	case USB_CLASS_PRINTER:
		printk("    Printer device class\n");
		break;
	case USB_CLASS_MASS_STORAGE:
		printk("    Mass Storage device class\n");
		break;
	case USB_CLASS_HUB:
		printk("    Hub device class\n");
		break;
	case USB_CLASS_VENDOR_SPEC:
		printk("    Vendor class\n");
		break;
	default:
		printk("    Unknown class\n");
	}
}

void usb_show_config_descriptor(struct usb_config_descriptor *desc)
{
	printk("Configuration:\n");
	printk("  bLength             = %4d%s\n", desc->bLength,
		desc->bLength == USB_DT_CONFIG_SIZE ? "" : " (!!!)");
	printk("  bDescriptorType     =   %02x\n", desc->bDescriptorType);
	printk("  wTotalLength        = %04x\n", desc->wTotalLength);
	printk("  bNumInterfaces      =   %02x\n", desc->bNumInterfaces);
	printk("  bConfigurationValue =   %02x\n", desc->bConfigurationValue);
	printk("  iConfiguration      =   %02x\n", desc->iConfiguration);
	printk("  bmAttributes        =   %02x\n", desc->bmAttributes);
	printk("  MaxPower            = %4dmA\n", desc->MaxPower * 2);
}

void usb_show_interface_descriptor(struct usb_interface_descriptor *desc)
{
	printk("  Alternate Setting: %2d\n", desc->bAlternateSetting);
	printk("    bLength             = %4d%s\n", desc->bLength,
		desc->bLength == USB_DT_INTERFACE_SIZE ? "" : " (!!!)");
	printk("    bDescriptorType     =   %02x\n", desc->bDescriptorType);
	printk("    bInterfaceNumber    =   %02x\n", desc->bInterfaceNumber);
	printk("    bAlternateSetting   =   %02x\n", desc->bAlternateSetting);
	printk("    bNumEndpoints       =   %02x\n", desc->bNumEndpoints);
	printk("    bInterface Class:SubClass:Protocol =   %02x:%02x:%02x\n",
		desc->bInterfaceClass, desc->bInterfaceSubClass, desc->bInterfaceProtocol);
	printk("    iInterface          =   %02x\n", desc->iInterface);
}

void usb_show_endpoint_descriptor(struct usb_endpoint_descriptor *desc)
{
	char *LengthCommentString = (desc->bLength ==
		USB_DT_ENDPOINT_AUDIO_SIZE) ? " (Audio)" : (desc->bLength ==
		USB_DT_ENDPOINT_SIZE) ? "" : " (!!!)";
	char *EndpointType[4] = { "Control", "Isochronous", "Bulk", "Interrupt" };

	printk("    Endpoint:\n");
	printk("      bLength             = %4d%s\n",
		desc->bLength, LengthCommentString);
	printk("      bDescriptorType     =   %02x\n", desc->bDescriptorType);
	printk("      bEndpointAddress    =   %02x (%s)\n", desc->bEndpointAddress,
		(desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
			USB_ENDPOINT_XFER_CONTROL ? "i/o" :
		(desc->bEndpointAddress & USB_ENDPOINT_DIR_MASK) ? "in" : "out");
	printk("      bmAttributes        =   %02x (%s)\n", desc->bmAttributes,
		EndpointType[USB_ENDPOINT_XFERTYPE_MASK & desc->bmAttributes]);
	printk("      wMaxPacketSize      = %04x\n", desc->wMaxPacketSize);
	printk("      bInterval           =   %02x\n", desc->bInterval);

	/* Audio extensions to the endpoint descriptor */
	if (desc->bLength == USB_DT_ENDPOINT_AUDIO_SIZE) {
		printk("      bRefresh            =   %02x\n", desc->bRefresh);
		printk("      bSynchAddress       =   %02x\n", desc->bSynchAddress);
	}
}

void usb_show_string(struct usb_device *dev, char *id, int index)
{
	char *buf;

	if (!index)
		return;
	if (!(buf = kmalloc(256, GFP_KERNEL)))
		return;
	if (usb_string(dev, index, buf, 256) > 0)
		printk(KERN_INFO "%s: %s\n", id, buf);
	kfree(buf);
}

void usb_dump_urb (purb_t purb)
{
	printk ("urb                   :%p\n", purb);
	printk ("next                  :%p\n", purb->next);
	printk ("dev                   :%p\n", purb->dev);
	printk ("pipe                  :%08X\n", purb->pipe);
	printk ("status                :%d\n", purb->status);
	printk ("transfer_flags        :%08X\n", purb->transfer_flags);
	printk ("transfer_buffer       :%p\n", purb->transfer_buffer);
	printk ("transfer_buffer_length:%d\n", purb->transfer_buffer_length);
	printk ("actual_length         :%d\n", purb->actual_length);
	printk ("setup_packet          :%p\n", purb->setup_packet);
	printk ("start_frame           :%d\n", purb->start_frame);
	printk ("number_of_packets     :%d\n", purb->number_of_packets);
	printk ("interval              :%d\n", purb->interval);
	printk ("error_count           :%d\n", purb->error_count);
	printk ("context               :%p\n", purb->context);
	printk ("complete              :%p\n", purb->complete);
}
/* --------- end of helpers ---------- */


module_init(usb_mpio_init);
module_exit(usb_mpio_cleanup);

MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0) /* Hmm, .... */
MODULE_LICENSE("GPL");
#endif
