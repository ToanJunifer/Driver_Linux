#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/proc_fs.h>
#include <linux/kobject.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#define IRQ_NO 1

unsigned int i = 0;
#define mem_size 1024
dev_t dev = 0;

static struct class * dev_class;
static struct cdev chr_cdev;
uint8_t * kernel_buffer;

DEFINE_SPINLOCK(chr_spinlock);
unsigned long chr_spinlock_variable = 0;

static int  __init    chr_driver_init(void);
static void __exit    chr_driver_exit(void);
static int      chr_open      (struct inode * inode, struct file * file);
static int      chr_release   (struct inode * inode, struct file * file);
static ssize_t  chr_read  (struct file *filp, char __user *buf       , size_t len, loff_t *off);
static ssize_t  chr_write (struct file *filp, const char __user *buf , size_t len, loff_t *off);

int thread_func1(void *p);
int thread_func2(void *p);

static struct task_struct *chr_thread1;
static struct task_struct *chr_thread2;

struct tasklet_struct * tasklet;
void tasklet_func(unsigned long data);


static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .read           = chr_read,
    .write          = chr_write,
    .open           = chr_open,
    .release        = chr_release,
};

void tasklet_func(unsigned long data){
    spin_lock_irq(&chr_spinlock);
    chr_spinlock_variable++;
    printk(KERN_INFO "Executing the tasklet function, data=%lu\n",chr_spinlock_variable);
    spin_unlock_irq(&chr_spinlock);
}

// interrupt handler for interrupt 1
static irqreturn_t irq_handler(int irq, void * dev_id){
    spin_lock_irq(&chr_spinlock);
    chr_spinlock_variable++;
    printk(KERN_INFO "Interrupt occured and executing ISR, value: %lu\n",chr_spinlock_variable);
    spin_unlock_irq(&chr_spinlock);
    // Schedule the tasklet
    tasklet_schedule(tasklet);
    return IRQ_HANDLED;
}

int thread_func1(void *p){
    while(!kthread_should_stop()){
        if (!spin_is_locked(&chr_spinlock)){
            printk(KERN_INFO "Spinlock is not locked in thread function 1.\n");
        }
        spin_lock(&chr_spinlock);
        if (spin_is_locked(&chr_spinlock)){
            printk(KERN_INFO "Spinlock is locked in thread function 1.\n");
        }
        chr_spinlock_variable++;
        printk(KERN_INFO "In thread function 1, value %lu.\n",chr_spinlock_variable);
        spin_unlock(&chr_spinlock);
        msleep(1000);
    }
    return 0;
}
int thread_func2(void *p){
    while(!kthread_should_stop()){
        if (!spin_is_locked(&chr_spinlock)){
            printk(KERN_INFO "Spinlock is not locked in thread function 2.\n");
        }
        spin_lock(&chr_spinlock);
        if (spin_is_locked(&chr_spinlock)){
            printk(KERN_INFO "Spinlock is locked in thread function 2.\n");
        }
        chr_spinlock_variable++;
        printk(KERN_INFO "In thread function 2, value %lu.\n",chr_spinlock_variable);
        spin_unlock(&chr_spinlock);
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
    copy_to_user(buf,kernel_buffer,mem_size);
    printk(KERN_INFO "Data read: \"%s\"\n",kernel_buffer);
    return mem_size;
}

static ssize_t chr_write (struct file *filp, const char  *buf, size_t len, loff_t *off){
    copy_from_user(kernel_buffer,buf,len);
    printk(KERN_INFO "Data is written successfully: %s",buf);
    return len;
}



static int __init chr_driver_init(void){
    // Allocting major number
    if  ((alloc_chrdev_region(&dev,0,1,"chr_dev")) < 0){
        printk(KERN_INFO "Can not allocate the major number.\n");
        return -1;
    }
    printk(KERN_INFO "Major = %d Minor = %d\n",MAJOR(dev),MINOR(dev));
    
    // creating cdev structure
    cdev_init(&chr_cdev,&fops);
    
    // adding character device to the system
    if  ((cdev_add(&chr_cdev,dev,1)) < 0){
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

    tasklet = kmalloc(sizeof(struct tasklet_struct),GFP_KERNEL);    // DYNAMIC Method
    if(tasklet == NULL){
        printk(KERN_INFO "Can't allocate the memory");
        goto irq;
    }
    tasklet_init(tasklet,tasklet_func,0);                           // DYNAMIC Method

    chr_thread1 = kthread_create(thread_func1,NULL,"chr_thread_1");
    if (chr_thread1){
        wake_up_process(chr_thread1);
    }
    else{
        printk(KERN_INFO "Unable to create the thread.\n");
        goto r_device;
    }

    chr_thread2 = kthread_run(thread_func2,NULL,"chr_thread_2");
    if (chr_thread2){
        printk(KERN_INFO "Run the thread successfully.\n");
    }
    else{
        printk(KERN_INFO "Unable to create the thread.\n");
        goto r_device;
    }


    printk(KERN_INFO "Device driver insert...done\n");
    return 0;

irq:
    free_irq(IRQ_NO,(void *)(irq_handler));

r_device:
    class_destroy(dev_class);

r_class:
    unregister_chrdev_region(dev,1);
    cdev_del(&chr_cdev);
    return -1;

}

static void __exit chr_driver_exit(void){
    kthread_stop(chr_thread1);
    kthread_stop(chr_thread2);
    free_irq(IRQ_NO,(void*)(irq_handler));
    tasklet_kill(tasklet);
    device_destroy(dev_class,dev);
    class_destroy(dev_class);
    cdev_del(&chr_cdev);
    unregister_chrdev_region(dev,1);
    printk(KERN_INFO "Device driver is removed successfully.\n");
}

module_init(chr_driver_init);
module_exit(chr_driver_exit);

MODULE_LICENSE      ("GPL");
MODULE_AUTHOR       ("ToanJunifer");
MODULE_DESCRIPTION  ("The charater deivce driver");