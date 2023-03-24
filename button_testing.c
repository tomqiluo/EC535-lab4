#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#define GPIO_BUTTON_PIN 26

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qi Luo");
MODULE_DESCRIPTION("Button Interrupt Module");

static irqreturn_t my_irq_handler(int irq, void *dev_id)
{
    printk(KERN_INFO "Button pressed!\n");

    return IRQ_HANDLED;
}

static int __init button_init(void)
{
    int result = 0;

    printk(KERN_INFO "Initializing button interrupt module\n");

    if (!gpio_is_valid(GPIO_BUTTON_PIN)) {
        printk(KERN_INFO "Invalid GPIO pin %d\n", GPIO_BUTTON_PIN);
        return -ENODEV;
    }

    result = gpio_request(GPIO_BUTTON_PIN, "button");
    if (result < 0) {
        printk(KERN_INFO "Failed to request GPIO pin %d\n", GPIO_BUTTON_PIN);
        return result;
    }

    result = gpio_direction_input(GPIO_BUTTON_PIN);
    if (result < 0) {
        printk(KERN_INFO "Failed to set GPIO pin %d as input\n", GPIO_BUTTON_PIN);
        gpio_free(GPIO_BUTTON_PIN);
        return result;
    }

    result = request_irq(gpio_to_irq(GPIO_BUTTON_PIN), (irq_handler_t)my_irq_handler, IRQF_TRIGGER_FALLING, "button_irq", NULL);
    if (result < 0) {
        printk(KERN_INFO "Failed to request interrupt for GPIO pin %d\n", GPIO_BUTTON_PIN);
        gpio_free(GPIO_BUTTON_PIN);
        return result;
    }


    return 0;
}

static void __exit button_exit(void)
{
    printk(KERN_INFO "Exiting button interrupt module\n");

    free_irq(gpio_to_irq(GPIO_BUTTON_PIN), NULL);
    gpio_free(GPIO_BUTTON_PIN);
}

module_init(button_init);
module_exit(button_exit);

