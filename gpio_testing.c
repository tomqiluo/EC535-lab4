#include <linux/module.h>
#include <linux/gpio.h>

#define GPIO_PIN_NUMBER 67

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qi Luo");
MODULE_DESCRIPTION("[Testing] GPIO control module");

static int __init mymodule_init(void)
{
    int result;

    result = gpio_request(GPIO_PIN_NUMBER, "my-gpio");
    if (result < 0) {
        printk(KERN_ERR "Failed to request GPIO %d\n", GPIO_PIN_NUMBER);
        return result;
    }

    result = gpio_direction_output(GPIO_PIN_NUMBER, 1);
    if (result < 0) {
        printk(KERN_ERR "Failed to set GPIO %d direction\n", GPIO_PIN_NUMBER);
        gpio_free(GPIO_PIN_NUMBER);
        return result;
    }

    gpio_set_value(GPIO_PIN_NUMBER, 1);

    return 0;
}

static void __exit mymodule_exit(void)
{
    gpio_free(GPIO_PIN_NUMBER);
}

module_init(mymodule_init);
module_exit(mymodule_exit);
