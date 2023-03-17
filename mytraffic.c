/* Necessary includes for device drivers */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <asm/system_misc.h> /* cli(), *_flags */
#include <linux/uaccess.h>
#include <asm/uaccess.h> /* copy_from/to_user */
#include <linux/timer.h>

MODULE_LICENSE("Dual BSD/GPL");

#DEFINE RED_GPIO 1
#DEFINE YELLOW_GPIO 2
#DEFINE GREEN_GPIO 3

#DEFINE CYCLE_RATE 1

// STATES  0		1			2
//	 normal      flashing-red 	flashing-yellow

int state;
int red_light_flag,
yellow light_flag,
green_light_flag;

void main(){
state = 0;


if(state = 0){




}
else if (state =1) {





}
else if (state =2) {







}


}


