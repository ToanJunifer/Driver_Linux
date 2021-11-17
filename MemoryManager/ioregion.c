#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/init.h>

#define IOSTART     0x200
#define IOEXTEND    0x40

static unsigned long iostart = IOSTART, ioextend=IOEXTEND,ioend;

static int __init my_init(void){
    unsigned long ultest = (unsigned long)100;
    ioend = iostart + ioextend;
    printk(KERN_INFO "requesting the IO region 0x%lx to 0x%lx.\n",iostart,ioend);

    if(!request_region(iostart,ioextend,"my_ioport")){
        printk(KERN_INFO "The IO region is busy. Quitting\n");
        return -EBUSY;
    }

    printk(KERN_INFO "Writing a long data: %ld\n",ultest);
    outl(ultest,iostart);

    ultest = inl(iostart);
    printk(KERN_INFO "Reading a long data: %ld\n",ultest);
    return 0;
}

static void __exit my_exit(void){
    printk(KERN_INFO "releasing the IO region 0x%lx to 0x%lx.\n",iostart,ioend);
    release_region(iostart,ioextend);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE      ("GPL");
MODULE_AUTHOR       ("ToanJunifer");
MODULE_DESCRIPTION  ("Requesting the I/O port");