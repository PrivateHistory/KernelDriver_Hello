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
////-------
#include <asm/io.h>
#include <mach/platform.h>

struct gpio_register_base{
uint32_t GPIOSEL[6];  //first 6 select register with 32 bits
uint32_t RESERVED;   //reserved register
uint32_t GPIOSET[2]; //2 next set registered
uint32_t RESERVED_1; //reserved register
uint32_t GPIOCLR[2]; //register to remove/clear gpio
};

//create a point to the structure pf registe
struct gpio_register_base* gpio_struct;

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


	//code for led

	//get the maping of real and virtual addressing
	gpio_struct = (struct gpio_register_base*)ioremap(GPIO_BASE, sizeof(struct clone*))

     //example
     //the set has 32 bits all of the belonging to one GPIO 1 means high and 0 means low
     //set GPIO 24 as output
    setGPIO(18, 0); //set GPIO 24 as input  ,GPIO 24 has the index 18 as in documnetation
    setGPIO(18, 1); //set GPIO 24 as input

	//set gpio 24 high(change the value in register to 1);
	gpio_struct->GPIOSET[18]=1<<18;

	//set gpio 23 as input
	setGPIO(16, 0);
	//get the value of the register
	if(gpio_struct->GPIOSET[16]){
	pr_info("The value of the GPIO 23 is high %i\n");
}
else
{
	pr_info("The value of the GPIO 23 is low %i\n");
}

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


static void SetGPIOOutputValue(int GPIO, bool outputValue)
{
    if (outputValue)
        s_pGpioRegisters->GPSET[GPIO / 32] = (1 << (GPIO % 32));
    else
        s_pGpioRegisters->GPCLR[GPIO / 32] = (1 << (GPIO % 32));
}






///set the gpio as output or input

static void setGPIO(int GPIO, int selectfunction){
//select function determines if the GPIO is output or input

	int rIndex = GPIO / 10;  //every select register has 10 GPIO on it so GPIO number div 10 will give the index of select register

	int bit_value = (GPIO % 10) * 3;   //every select register assigns 3 bits for every GPIO ,this will give the start index of the 3 bits


    ///-------------///
    //3 bits mentioned above can get different values(8 to be exact 2^3)
    //now to select gpio as input the 3 bits have to be 000
    // for output they have to be 001 ,and the rest are for alternative function
    ////----------////


    //select function 0 means input and 1 means output
    if(selectfunction==0){
  //set the GPIO to input
  gpio_struct->GPIOSEL[rIndex] &=~(7<<(bit_value));
  //this equations works like this.It was stated that for input you have to set the 3 bits belonging to GPIO to 000
  //moreover this bits have the starting index at bit_value ,so in order to achive this it is only needed
  //to get 7(which in power two is 111) to left shift by bit_value and to negate it so the three bits are 000
  //Because only the 3 bits have to be changed and the rest to remain 0 the and is used

}
else
{
  gpio_struct->GPIOSEL[rIndex]|=(1<<(bit_value));
   //this equations works like this.It was stated that for input you have to set the 3 bits belonging to GPIO to 001
  //moreover this bits have the starting index at bit_value ,so in order to achive this it is only needed
  //to get 1(which in power two is 001) to left shift by bit_value
  //Because only the third bit matter (1 has to be the third bit to be output) only or has to be used

}


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
