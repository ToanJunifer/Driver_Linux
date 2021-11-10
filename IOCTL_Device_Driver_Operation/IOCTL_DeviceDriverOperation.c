#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#define mem_size 1024

// Define the ioctl code
#define WR_DATA _IOW('a','a',int32_t*)
#define RD_DATA _IOR('a','b',int32_t*)

int32_t val = 0;

dev_t dev = 0;
static struct class * dev_class;
static struct cdev etx_cdev;
uint8_t * kernel_buffer;

static int  __init    chr_driver_init(void);
static void __exit    chr_driver_exit(void);
static int      chr_open      (struct inode * inode, struct file * file);
static int      chr_release   (struct inode * inode, struct file * file);
static ssize_t  chr_read  (struct file *filp, char __user *buf       , size_t len, loff_t *off);
static ssize_t  chr_write (struct file *filp, const char __user *buf , size_t len, loff_t *off);
static long chr_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .read           = chr_read,
    .write          = chr_write,
    .open           = chr_open,
    .unlocked_ioctl = chr_ioctl,
    .release        = chr_release,
};

static int chr_open(struct inode * inode, struct file * file){
    // Creating physical memory
    if ((kernel_buffer = kmalloc(mem_size,GFP_KERNEL)) == 0){
        printk(KERN_INFO "Can not allocate the memory to the kernel.\n");
        return -1;
    }
    printk(KERN_INFO "Device file opened.\n");
    return 0;
}

static int chr_release (struct inode * inode, struct file * file){
    kfree(kernel_buffer);
    printk(KERN_INFO "Device file closed.\n");
    return 0;
}

static ssize_t chr_read  (struct file *filp, char __user *buf, size_t len, loff_t *off){
    copy_to_user(buf,kernel_buffer,mem_size);
    printk(KERN_INFO "Data read: \"%s\"\n",kernel_buffer);
    return mem_size;
}

static ssize_t chr_write (struct file *filp, const char __user *buf, size_t len, loff_t *off){
    copy_from_user(kernel_buffer,buf,len);
    printk(KERN_INFO "Data is written successfully: %s",buf);
    return len;
}

static long chr_ioctl(struct file * file, unsigned int cmd, unsigned long arg){
    switch (cmd)
    {
    case WR_DATA:
        copy_from_user(&val,(int32_t*)arg,sizeof(val));
        printk(KERN_INFO "val = %d\n",val);
        break;
    case RD_DATA:
        copy_to_user((int32_t*)arg,&val,sizeof(val));
        break;
    default:
        break;
    }
    return 0;
}

static int __init chr_driver_init(void){
    // Allocting major number
    if  ((alloc_chrdev_region(&dev,0,1,"etx_Dev")) < 0){
        printk(KERN_INFO "Can not allocate the major number.\n");
        return -1;
    }
    printk(KERN_INFO "Major = %d Minor = %d\n",MAJOR(dev),MINOR(dev));
    
    // creating cdev structure
    cdev_init(&etx_cdev,&fops);
    
    // adding character device to the system
    if  ((cdev_add(&etx_cdev,dev,1)) < 0){
        printk(KERN_INFO "Can not add device to the system.\n");
        goto r_class;
    }

    // creating struct class
    if  ((dev_class = class_create(THIS_MODULE,"chr_class")) == NULL){
        printk(KERN_INFO "Can not create the structure class.\n");
        goto r_class;
    }

    // creating device
    if  ((device_create(dev_class,NULL,dev,NULL,"chr_device")) == NULL){
        printk(KERN_INFO "Can not create the device.\n");
        goto r_device;
    }

    printk(KERN_INFO "Device driver insert...done\n");
    return 0;

r_device:
    class_destroy(dev_class);

r_class:
    unregister_chrdev_region(dev,1);
    return -1;

}

static void __exit chr_driver_exit(void){
    device_destroy(dev_class,dev);
    class_destroy(dev_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(dev,1);
    printk(KERN_INFO "Device driver is removed successfully.\n");
}

module_init(chr_driver_init);
module_exit(chr_driver_exit);

MODULE_LICENSE      ("GPL");
MODULE_AUTHOR       ("ToanJunifer");
MODULE_DESCRIPTION  ("The charater deivce driver");