#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/system_misc.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/jiffies.h>

#define GPIO_GREEN 44
#define GPIO_YELLOW 68
#define GPIO_RED 67

#define GPIO_BUTTON_ONE 26
#define GPIO_BUTTON_TWO 46

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qi Luo");
MODULE_DESCRIPTION("A simple kernel module");

static int mytraffic_init(void);
static void mytraffic_exit(void);
static ssize_t mytraffic_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t mytraffic_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static int mytraffic_open(struct inode *inode, struct file *filp);
static int mytraffic_release(struct inode *inode, struct file *filp);

struct file_operations mytraffic_fops = {
	read: mytraffic_read,
	write: mytraffic_write,
	open: mytraffic_open,
	release: mytraffic_release
};

static int time_cycle = 1000; // default cycle time is 1 second

const unsigned user_input_capacity = 256;
const unsigned user_output_capacity = 1024;
static unsigned bite = 256;

static const int mytraffic_major = 61;

static struct timer_list mytimer;
static int cycle_count = 0;
static int MODE = 0;
static int flash_flag = 0;
static int ped_flag = 0;

// Buffer to store data from user
static char *user_input;

// Buffer to store data to send to user
static char *user_output;

static irqreturn_t button_one_handlr(int irq, void *dev_id)
{
    printk(KERN_INFO "Button one pressed!\n");
    if (MODE < 2) {
        MODE = MODE + 1;
    } else {
        MODE = 0;
    }

    return IRQ_HANDLED;
}

static irqreturn_t button_two_handlr(int irq, void *dev_id)
{
    printk(KERN_INFO "Button two pressed!\n");

    // Ped Xing, set peg_flag to 1
    ped_flag = 1;

    return IRQ_HANDLED;
}

void mytraffic_callback(struct timer_list *timer)
{
	char * temp = user_output;

       
    // Normal mode
    if (MODE == 0) {
        if (cycle_count == 0) {
            // printk(KERN_INFO "Green: %u\n", jiffies_to_msecs(jiffies)/1000);
            gpio_set_value(GPIO_GREEN, 1);      // GREEN: ON
            gpio_set_value(GPIO_YELLOW, 0);     // YELLOW: OFF
            gpio_set_value(GPIO_RED, 0);        // RED: OFF
            temp += sprintf(temp, "Mode: Normal\nCycle Rate: %d HZ\nred off, yellow off, green on\n", 1000/time_cycle);
            mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle * 3));
            cycle_count = cycle_count + 1;
        } else if (cycle_count == 1) {
            // printk(KERN_INFO "Yellow: %u\n", jiffies_to_msecs(jiffies)/1000);
            gpio_set_value(GPIO_GREEN, 0);      // GREEN: OFF
            gpio_set_value(GPIO_YELLOW, 1);     // YELLOW: ON
            gpio_set_value(GPIO_RED, 0);        // RED: OFF
            temp += sprintf(temp, "Mode: Normal\nCycle Rate: %d HZ\nred off, yellow on, green off\n", 1000/time_cycle);
            mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle));
            cycle_count = cycle_count + 1;
        } else if (cycle_count == 2) {
            if (ped_flag == 1) {
                printk(KERN_INFO "Ped Xing\n");
                gpio_set_value(GPIO_GREEN, 0);      // GREEN: OFF
                gpio_set_value(GPIO_YELLOW, 1);     // YELLOW: ON
                gpio_set_value(GPIO_RED, 1);        // RED: ON
                temp += sprintf(temp, "Mode: Normal\nCycle Rate: %d HZ\nred on, yellow on, green off\n", 1000/time_cycle);
                mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle * 5));
                ped_flag = 0;
                cycle_count = 0;
            } else {
                printk(KERN_INFO "Ped BuXing\n");
                gpio_set_value(GPIO_GREEN, 0);      // GREEN: OFF
                gpio_set_value(GPIO_YELLOW, 0);     // YELLOW: OFF
                gpio_set_value(GPIO_RED, 1);        // RED: ON
                temp += sprintf(temp, "Mode: Normal\nCycle Rate: %d HZ\nred on, yellow off, green off\n", 1000/time_cycle);
                mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle * 2));
                cycle_count = 0;
            }
        }
    } else if (MODE == 1) {       // Flashing-Red mode
        if (flash_flag == 0) {
            gpio_set_value(GPIO_GREEN, 0);      // GREEN: OFF
            gpio_set_value(GPIO_YELLOW, 0);     // YELLOW: OFF
            gpio_set_value(GPIO_RED, 1);        // RED: ON
            temp += sprintf(temp, "Mode: Flashing-Red\nCycle Rate: %d HZ\nred on, yellow off, green off\n", 1000/time_cycle);
            mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle));
            flash_flag = 1;
        } else {
            gpio_set_value(GPIO_GREEN, 0);      // GREEN: OFF
            gpio_set_value(GPIO_YELLOW, 0);     // YELLOW: OFF
            gpio_set_value(GPIO_RED, 0);        // RED: ON
            temp += sprintf(temp, "Mode: Flashing-Red\nCycle Rate: %d HZ\nred off, yellow off, green off\n", 1000/time_cycle);
            mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle));
            flash_flag = 0;
        }
        ped_flag = 0;
    } else if (MODE == 2) {       // Flashing-Yellow mode
        if (flash_flag == 0) {

            gpio_set_value(GPIO_GREEN, 0);      // GREEN: OFF
            gpio_set_value(GPIO_YELLOW, 1);     // YELLOW: ON
            gpio_set_value(GPIO_RED, 0);        // RED: OFF
            temp += sprintf(temp, "Mode: Flashing-Yellow\nCycle Rate: %d HZ\nred off, yellow on, green off\n", 1000/time_cycle);
            mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle));
            flash_flag = 1;
        } else {
            gpio_set_value(GPIO_GREEN, 0);      // GREEN: OFF
            gpio_set_value(GPIO_YELLOW, 0);     // YELLOW: OFF
            gpio_set_value(GPIO_RED, 0);        // RED: ON
            temp += sprintf(temp, "Mode: Flashing-Yellow\nCycle Rate: %d HZ\nred ff, yellow off, green off\n", 1000/time_cycle);
            mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle));
            flash_flag = 0;
        }
        ped_flag = 0;
    }
}

static int __init mytraffic_init(void)
{
    int result;
    printk(KERN_INFO "My module loaded\n");

    result = register_chrdev(mytraffic_major, "mytraffic", &mytraffic_fops);
	if (result < 0)
	{
		printk(KERN_ALERT
			"mytraffic: cannot obtain major number %d\n", mytraffic_major);
		return result;
	}
	/* Allocating mytraffic for the buffer */
	user_input = kmalloc(user_input_capacity, GFP_KERNEL); 
	if (!user_input)
	{ 
		printk(KERN_ALERT "Insufficient kernel memory\n"); 
		result = -ENOMEM;
		goto fail; 
	} 
	memset(user_input, 0, user_input_capacity);
	/* Allocating mytraffic for the buffer */
	user_output = kmalloc(user_output_capacity, GFP_KERNEL); 
	if (!user_output)
	{ 
		printk(KERN_ALERT "Insufficient kernel memory\n"); 
		result = -ENOMEM;
		goto fail; 
	} 
	memset(user_output, 0, user_output_capacity);

    // Allocate GPIOs
    result = gpio_request(GPIO_GREEN, "gpio-green");
    if (result < 0) {
        printk(KERN_ERR "Failed to request GPIO %d\n", GPIO_GREEN);
        return result;
    }
    result = gpio_direction_output(GPIO_GREEN, 1);
    if (result < 0) {
        printk(KERN_ERR "Failed to set GPIO %d direction\n", GPIO_GREEN);
        gpio_free(GPIO_GREEN);
        return result;
    } // GREEN done
    result = gpio_request(GPIO_YELLOW, "gpio-yellow");
    if (result < 0) {
        printk(KERN_ERR "Failed to request GPIO %d\n", GPIO_YELLOW);
        return result;
    }
    result = gpio_direction_output(GPIO_YELLOW, 1);
    if (result < 0) {
        printk(KERN_ERR "Failed to set GPIO %d direction\n", GPIO_YELLOW);
        gpio_free(GPIO_YELLOW);
        return result;
    } // YELLOW done
    result = gpio_request(GPIO_RED, "gpio-red");
    if (result < 0) {
        printk(KERN_ERR "Failed to request GPIO %d\n", GPIO_RED);
        return result;
    }
    result = gpio_direction_output(GPIO_RED, 1);
    if (result < 0) {
        printk(KERN_ERR "Failed to set GPIO %d direction\n", GPIO_RED);
        gpio_free(GPIO_RED);
        return result;
    } // RED done

    // Allocate GPIO button and interrupts
    if (!gpio_is_valid(GPIO_BUTTON_TWO)) {
        printk(KERN_INFO "Invalid GPIO pin %d\n", GPIO_BUTTON_TWO);
        return -ENODEV;
    }
    result = gpio_request(GPIO_BUTTON_TWO, "button");
    if (result < 0) {
        printk(KERN_INFO "Failed to request GPIO pin %d\n", GPIO_BUTTON_TWO);
        return result;
    }
    result = gpio_direction_input(GPIO_BUTTON_TWO);
    if (result < 0) {
        printk(KERN_INFO "Failed to set GPIO pin %d as input\n", GPIO_BUTTON_TWO);
        gpio_free(GPIO_BUTTON_TWO);
        return result;
    }
    result = request_irq(gpio_to_irq(GPIO_BUTTON_TWO), (irq_handler_t)button_two_handlr, IRQF_TRIGGER_FALLING, "button_two_irq", NULL);
    if (result < 0) {
        printk(KERN_INFO "Failed to request interrupt for GPIO pin %d\n", GPIO_BUTTON_TWO);
        gpio_free(GPIO_BUTTON_TWO);
        return result;
    }

    if (!gpio_is_valid(GPIO_BUTTON_ONE)) {
        printk(KERN_INFO "Invalid GPIO pin %d\n", GPIO_BUTTON_ONE);
        return -ENODEV;
    }
    result = gpio_request(GPIO_BUTTON_ONE, "button");
    if (result < 0) {
        printk(KERN_INFO "Failed to request GPIO pin %d\n", GPIO_BUTTON_ONE);
        return result;
    }
    result = gpio_direction_input(GPIO_BUTTON_ONE);
    if (result < 0) {
        printk(KERN_INFO "Failed to set GPIO pin %d as input\n", GPIO_BUTTON_ONE);
        gpio_free(GPIO_BUTTON_ONE);
        return result;
    } // Button done
    result = request_irq(gpio_to_irq(GPIO_BUTTON_ONE), (irq_handler_t)button_one_handlr, IRQF_TRIGGER_FALLING, "button_one_irq", NULL);
    if (result < 0) {
        printk(KERN_INFO "Failed to request interrupt for GPIO pin %d\n", GPIO_BUTTON_ONE);
        gpio_free(GPIO_BUTTON_ONE);
        return result;
    } // Interupts done
    
    // Setup Timer
    timer_setup(&mytimer, mytraffic_callback, 0);
    mod_timer(&mytimer, jiffies + msecs_to_jiffies(1));

    return 0;

fail: 
	mytraffic_exit(); 
	return result;
}

static void __exit mytraffic_exit(void)
{   
    unregister_chrdev(mytraffic_major, "mytraffic");
	/* Freeing buffer memory */
	if (user_input)
	{
		kfree(user_input);
	}

	if(user_output) {
		kfree(user_output);
	}

    // Free timer
    del_timer(&mytimer);

    // Free GPIOs
    gpio_free(GPIO_GREEN);
    gpio_free(GPIO_YELLOW);
    gpio_free(GPIO_RED);

    // Free GPIOs button and interrupts
    free_irq(gpio_to_irq(GPIO_BUTTON_ONE), NULL);
    gpio_free(GPIO_BUTTON_ONE);
    free_irq(gpio_to_irq(GPIO_BUTTON_TWO), NULL);
    gpio_free(GPIO_BUTTON_TWO);

    printk(KERN_INFO "mytraffic module unloaded.\n");
}

static int mytraffic_open(struct inode *inode, struct file *filp)
{
	/* Success */
	return 0;
}

static int mytraffic_release(struct inode *inode, struct file *filp)
{
	/* Success */
	return 0;
}

static ssize_t mytraffic_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{ 
	int temp;
	char tbuf[256], *tbptr = tbuf;
	unsigned int mytraffic_len = strlen(user_output);

	/* end of buffer reached */
	if (*f_pos >= mytraffic_len)
	{
		return 0;
	}
	/* do not go over then end */
	if (count > mytraffic_len - *f_pos)
		count = mytraffic_len - *f_pos;
	/* Transfering data to user space */ 
	if (copy_to_user(buf, user_output + *f_pos, count))
	{
		return -EFAULT;
	}
	tbptr += sprintf(tbptr,								   
		"read called: process id %d, command %s, count %d, offest %lld chars ",
		current->pid, current->comm, count, *f_pos);
	for (temp = *f_pos; temp < count + *f_pos; temp++)					  
		tbptr += sprintf(tbptr, "%c", user_output[temp]);
	// Reset output buffer
	strcpy(user_output, "");
	return count; 
}

static ssize_t mytraffic_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    uint temp;
    char tbuf[256], *tbptr = tbuf;
    // char registerTimerBuffer[256];
    unsigned int num;
    
    // end of buffer reached
    if (*f_pos >= user_input_capacity) {
        return -ENOSPC;
    }
    
    // do not eat more than a bite
    if (count > bite) {
        count = bite;
    }
    
    // do not go over the end
    if (count > user_input_capacity - *f_pos) {
        count = user_input_capacity - *f_pos;
    }
    
    // copy data from user
    memset(user_input, 0, user_input_capacity);
    if (copy_from_user(user_input + *f_pos, buf, count)) {
        return -EFAULT;
    }
    
    // print debug information
    tbptr += sprintf(tbptr, "write called: process id %d, command %s, count %d, offset %lld, chars ",
                     current->pid, current->comm, count, *f_pos);
    for (temp = *f_pos; temp < count + *f_pos; temp++) {
        tbptr += sprintf(tbptr, "%c", user_input[temp]);
    }
    
    // extract integer value
    if (kstrtouint(user_input, 10, &num) < 0) {
        printk(KERN_ERR "Failed to convert input to integer\n");
        return -EINVAL;
    }
    
    // change cycle length
    printk(KERN_INFO "Converted to %u\n", num);
    time_cycle = 1000/num;
    
    return count;
}

module_init(mytraffic_init);
module_exit(mytraffic_exit);
