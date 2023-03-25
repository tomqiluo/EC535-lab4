#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#define GPIO_GREEN 44
#define GPIO_YELLOW 68
#define GPIO_RED 67

#define GPIO_BUTTON_ONE 26

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qi Luo");
MODULE_DESCRIPTION("A simple kernel module");

static int time_cycle = 1000; // default cycle time is 1 second
module_param(time_cycle, int, 0644); // allow time_cycle to be set as a module parameter

static struct timer_list mytimer;

static int cycle_count = 0;

static int MODE = 0;

static int flash_flag = 0;

static irqreturn_t button_one_handlr(int irq, void *dev_id)
{
    printk(KERN_INFO "Button pressed!\n");
    if (MODE < 2) {
        MODE = MODE + 1;
    } else {
        MODE = 0;
    }

    return IRQ_HANDLED;
}

void mytimer_callback(struct timer_list *timer)
{
    // Normal mode
    if (MODE == 0) {
        if (cycle_count == 0) {
            printk(KERN_INFO "Green: %u\n", jiffies_to_msecs(jiffies)/1000);
            gpio_set_value(GPIO_GREEN, 1);      // GREEN: ON
            gpio_set_value(GPIO_YELLOW, 0);     // YELLOW: OFF
            gpio_set_value(GPIO_RED, 0);        // RED: OFF
            mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle * 3));
            cycle_count = cycle_count + 1;
        } else if (cycle_count == 1) {
            printk(KERN_INFO "Yellow: %u\n", jiffies_to_msecs(jiffies)/1000);
            gpio_set_value(GPIO_GREEN, 0);      // GREEN: OFF
            gpio_set_value(GPIO_YELLOW, 1);     // YELLOW: ON
            gpio_set_value(GPIO_RED, 0);        // RED: OFF
            mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle));
            cycle_count = cycle_count + 1;
        } else if (cycle_count == 2) {
            printk(KERN_INFO "Red: %u\n", jiffies_to_msecs(jiffies)/1000);
            gpio_set_value(GPIO_GREEN, 0);      // GREEN: OFF
            gpio_set_value(GPIO_YELLOW, 0);     // YELLOW: OFF
            gpio_set_value(GPIO_RED, 1);        // RED: ON
            mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle * 2));
            cycle_count = 0;
        }
    } else if (MODE == 1) {       // Flashing-Red mode
        if (flash_flag == 0) {
            gpio_set_value(GPIO_GREEN, 0);      // GREEN: OFF
            gpio_set_value(GPIO_YELLOW, 0);     // YELLOW: OFF
            gpio_set_value(GPIO_RED, 1);        // RED: ON
            mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle));
            flash_flag = 1;
        } else {
            gpio_set_value(GPIO_GREEN, 0);      // GREEN: OFF
            gpio_set_value(GPIO_YELLOW, 0);     // YELLOW: OFF
            gpio_set_value(GPIO_RED, 0);        // RED: ON
            mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle));
            flash_flag = 0;
        }
    } else if (MODE == 2) {       // Flashing-Yellow mode
        if (flash_flag == 0) {
            gpio_set_value(GPIO_GREEN, 0);      // GREEN: OFF
            gpio_set_value(GPIO_YELLOW, 1);     // YELLOW: ON
            gpio_set_value(GPIO_RED, 0);        // RED: OFF
            mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle));
            flash_flag = 1;
        } else {
            gpio_set_value(GPIO_GREEN, 0);      // GREEN: OFF
            gpio_set_value(GPIO_YELLOW, 0);     // YELLOW: OFF
            gpio_set_value(GPIO_RED, 0);        // RED: ON
            mod_timer(&mytimer, jiffies + msecs_to_jiffies(time_cycle));
            flash_flag = 0;
        }
    }
}

static int __init mytraffic_init(void)
{
    int result;
    printk(KERN_INFO "My module loaded\n");

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
    timer_setup(&mytimer, mytimer_callback, 0);
    mod_timer(&mytimer, jiffies + msecs_to_jiffies(1));

    return 0;
}

static void __exit mytraffic_exit(void)
{   
    // Free timer
    del_timer(&mytimer);

    // Free GPIOs
    gpio_free(GPIO_GREEN);
    gpio_free(GPIO_YELLOW);
    gpio_free(GPIO_RED);

    // Free GPIOs button and interrupts
    free_irq(gpio_to_irq(GPIO_BUTTON_ONE), NULL);
    gpio_free(GPIO_BUTTON_ONE);

    printk(KERN_INFO "mytraffic module unloaded.\n");
}

module_init(mytraffic_init);
module_exit(mytraffic_exit);
