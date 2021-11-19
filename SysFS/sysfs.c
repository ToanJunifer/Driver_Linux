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
#include <linux/jiffies.h>
#include <linux/timer.h>


unsigned int sysfs_val = 0;
// Declare the kernel object
struct kobject * kobj_ref;

static int  __init    chr_driver_init(void);
static void __exit    chr_driver_exit(void);
// Sysfs function
static ssize_t sysfs_show(struct kobject * kobj,struct kobj_attribute * attr, char * buf);
static ssize_t sysfs_store(struct kobject * kobj, struct kobj_attribute * attr, const char * buf, size_t count);
// Create  a variable at: /sys/kernel/my_sysfs/sysfs_val
// also can read and write it
struct kobj_attribute sysfs_attr = __ATTR(sysfs_val,0660,sysfs_show,sysfs_store);


// Sysfs function defination
static ssize_t sysfs_show(struct kobject * kobj,struct kobj_attribute * attr, char * buf){
    printk(KERN_INFO "Reading in sysfs show function.\n");
    return sprintf(buf,"%d",sysfs_val);
}
static ssize_t sysfs_store(struct kobject * kobj, struct kobj_attribute * attr, const char * buf, size_t count){
    printk(KERN_INFO "Writing - sysfs store function.\n");
    sscanf(buf,"%d",&sysfs_val);
    return count;
}

static int __init chr_driver_init(void){
    // Creating the directory in /sys/kernel/my_sysfs
    kobj_ref = kobject_create_and_add("my_sysfs",kernel_kobj);

    // Creating the sysfs file
    if (sysfs_create_file(kobj_ref,&sysfs_attr.attr)){
        printk(KERN_INFO "Unable to create the sysfs file.\n");
        goto r_sysfs;
    }

    printk(KERN_INFO "Device driver insert...done\n");
    return 0;

r_sysfs:
    kobject_put(kobj_ref);
    sysfs_remove_file(kernel_kobj,&sysfs_attr.attr);
    return 0;
}

static void __exit chr_driver_exit(void){
    kobject_put(kobj_ref);
    sysfs_remove_file(kernel_kobj,&sysfs_attr.attr);
    printk(KERN_INFO "Device driver is removed successfully.\n");
}

module_init(chr_driver_init);
module_exit(chr_driver_exit);

MODULE_LICENSE      ("GPL");
MODULE_AUTHOR       ("ToanJunifer");
MODULE_DESCRIPTION  ("The charater deivce driver");