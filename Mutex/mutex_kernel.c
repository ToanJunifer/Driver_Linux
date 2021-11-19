#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#define mem_size 1024
dev_t dev = 0;
static struct class * dev_class;
static struct cdev my_cdev;
uint8_t * kernel_buffer;

struct mutex chr_mutex;
unsigned long chr_mutex_var = 0;

static struct task_struct * chr_thread1;
static struct task_struct * chr_thread2;
int thrd_func1(void *p);
int thrd_func2(void *p);

static int  __init    chr_driver_init(void);
static void __exit    chr_driver_exit(void);
static int      chr_open      (struct inode * inode, struct file * file);
static int      chr_release   (struct inode * inode, struct file * file);
static ssize_t  chr_read  (struct file *filp, char __user *buf       , size_t len, loff_t *off);
static ssize_t  chr_write (struct file *filp, const char __user *buf , size_t len, loff_t *off);

static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .read           = chr_read,
    .write          = chr_write,
    .open           = chr_open,
    .release        = chr_release,
};

int thrd_func1(void *p){
    while(!kthread_should_stop()){
        mutex_lock(&chr_mutex);
        chr_mutex_var++;
        printk(KERN_INFO "In thread function 1: %lu\n",chr_mutex_var);
        mutex_unlock(&chr_mutex);
        msleep(1000);
    }
    return 0;
}

int thrd_func2(void *p){
    while(!kthread_should_stop()){
        mutex_lock(&chr_mutex);
        chr_mutex_var++;
        printk(KERN_INFO "In thread function 2: %lu\n",chr_mutex_var);
        mutex_unlock(&chr_mutex);
        msleep(1000);
    }
    return 0;
}

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
    //copy_to_user(buf,kernel_buffer,mem_size);
    printk(KERN_INFO "Data read: \"%s\"\n",kernel_buffer);
    return mem_size;
}

static ssize_t chr_write (struct file *filp, const char __user *buf, size_t len, loff_t *off){
    //copy_from_user(kernel_buffer,buf,len);
    printk(KERN_INFO "Data is written successfully: %s",buf);
    return len;
}


static int __init chr_driver_init(void){
    // Allocting major number
    if  ((alloc_chrdev_region(&dev,0,1,"my_dev_region")) < 0){
        printk(KERN_INFO "Can not allocate the major number.\n");
        return -1;
    }
    printk(KERN_INFO "Major = %d Minor = %d\n",MAJOR(dev),MINOR(dev));
    
    // creating cdev structure
    cdev_init(&my_cdev,&fops);
    
    // adding character device to the system
    if  ((cdev_add(&my_cdev,dev,1)) < 0){
        printk(KERN_INFO "Can not add device to the system.\n");
        goto r_class;
    }

    // creating struct class
    if  ((dev_class = class_create(THIS_MODULE,"my_class")) == NULL){
        printk(KERN_INFO "Can not create the structure class.\n");
        goto r_class;
    }

    // creating device
    if  ((device_create(dev_class,NULL,dev,NULL,"my_device")) == NULL){
        printk(KERN_INFO "Can not create the device.\n");
        goto r_device;
    }

    // Initiliazing the mutex
    mutex_init(&chr_mutex);

    // Creating the first thread
    chr_thread1  = kthread_run(thrd_func1,NULL,"mutex_thread_1");
    if (chr_thread1){
        printk(KERN_INFO "kernel thread 1 created successfully.\n");
    }
    else{
        printk(KERN_ERR "kernel thread 1 created fail.\n");
        goto r_device;
    }

    // Creating the second thread
    chr_thread2  = kthread_run(thrd_func2,NULL,"mutex_thread_2");
    if (chr_thread2){
        printk(KERN_INFO "kernel thread 2 created successfully.\n");
    }
    else{
        printk(KERN_ERR "kernel thread 2 created fail.\n");
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
    kthread_stop(chr_thread1);
    kthread_stop(chr_thread2);
    device_destroy(dev_class,dev);
    class_destroy(dev_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev,1);
    printk(KERN_INFO "Device driver is removed successfully.\n");
}

module_init(chr_driver_init);
module_exit(chr_driver_exit);

MODULE_LICENSE      ("GPL");
MODULE_AUTHOR       ("ToanJunifer");
MODULE_DESCRIPTION  ("The charater deivce driver");