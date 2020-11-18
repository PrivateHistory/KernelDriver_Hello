///////////////////////boilerplate.c//////////////////////////////
// This file contains the basic infrastructure for a LKM		//
// There is no 'main' function. When the LKM is loaded, 		//
// functions such as READ and WRITE are only activated 			//
// when the LKM is accessed. functions such as INIT and EXIT	//
// are activated when the LKM is loaded and removed.			//
//////////////////////CPS-ECSI 2018-19////////////////////////////



#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
//#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/errno.h>

#define DEVICE_NAME		"boilerplate"

struct boilerplate_dev  {
	struct cdev cdev;
};

// declaration of entry points
static int boilerplate_open(struct inode *inode, struct file *filep);
static ssize_t boilerplate_read(struct file *filep, char *buf, size_t count, loff_t *f_pos);
static ssize_t boilerplate_write(struct file *filep, const char *bur, size_t count, loff_t *f_pos);
static int boilerplate_release(struct inode *inode, struct file *filep);

// file operations structure
static struct file_operations boilerplate_fops = {
	.owner		= THIS_MODULE,
	.open 		= boilerplate_open,
	.release 	= boilerplate_release,
	.read 		= boilerplate_read,
	.write 		= boilerplate_write,
};

// prototypes
static int boilerplate_init(void);
static void boilerplate_exit(void);

// global variables
struct boilerplate_dev *boilerplate_devp;
static dev_t first;
static struct class *boilerplate_class;
//message received from userspace
static char message_received_from_userspace[256] = {0};
//size of received message
static int received_message_length=0;

// OPEN function
static int boilerplate_open(struct inode *inode, struct file *filep){

	return 0;
}

// RELEASE function
static int boilerplate_release(struct inode *inode, struct file *filep){

	return 0;
}

// READ function
static ssize_t boilerplate_read(struct file *filep, char *buf, size_t count, loff_t *f_pos){
	ssize_t retval =received_message_length; //message sent to userspace length
   int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error= copy_to_user(buf, message_received_from_userspace,received_message_length);

   if (error==0){            // if true then there are no errors
      printk(KERN_INFO "Sent %d characters to the user\n",received_message_length); //for debuging
      return (retval=0);  // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "Failed to send %d characters to the user\n", error);
      return -EFAULT;
   }
   //return the message length
	return retval;
}

// WRITE function

//Assignmnet3
static ssize_t boilerplate_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos){
   sprintf(message_received_from_userspace, "%s(%zu letters)", buf, count);   //append the received message from buffer to global varibale message_received_from_userspace

   //store the size of message received from userspace
   received_message_length=strlen(message_received_from_userspace);

   printk(KERN_INFO "Received %zu characters from the user\n", count); //debuging check that the string is received
   return count;
}

// INIT function
static int __init boilerplate_init(void)
{
	int ret = 0;

	if(alloc_chrdev_region(&first, 0, 1, DEVICE_NAME) < 0){
		pr_info("Cannot register device\n");
		return -1;
	}

	if((boilerplate_class = class_create(THIS_MODULE, DEVICE_NAME)) == NULL){
		pr_info("Cannot create class %s\n", DEVICE_NAME);
		unregister_chrdev_region(first, 1);
		return -EINVAL;
	}

	boilerplate_devp = kmalloc(sizeof(struct boilerplate_dev), GFP_KERNEL);

	if(!boilerplate_devp){
		pr_info("Bad kmalloc\n");
		return -ENOMEM;
	}

	boilerplate_devp->cdev.owner = THIS_MODULE;

	cdev_init(&boilerplate_devp->cdev, &boilerplate_fops);

	if((ret = cdev_add(&boilerplate_devp->cdev, first, 1))){
		pr_alert("Error %d adding cdev\n", ret);
		device_destroy(boilerplate_class, first);
		class_destroy(boilerplate_class);
		unregister_chrdev_region(first, 1);

		return ret;
	}

	if(device_create(boilerplate_class, NULL, first, NULL, DEVICE_NAME) == NULL){
		class_destroy(boilerplate_class);
		unregister_chrdev_region(first, 1);

		return -1;
	}

	pr_info("Boilerplate driver initialized\n");

	return 0;
}

// EXIT function
static void __exit boilerplate_exit(void)
{
	unregister_chrdev_region(first, 1);

	device_destroy(boilerplate_class, first);

	class_destroy(boilerplate_class);

	pr_info("Boilerplate driver removed\n");
}

module_init(boilerplate_init);
module_exit(boilerplate_exit);

MODULE_LICENSE("GPL");
