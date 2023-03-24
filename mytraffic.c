#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qi Luo");
MODULE_DESCRIPTION("A simple kernel module");

static int time_cycle = 1000; // default cycle time is 1 second
module_param(time_cycle, int, 0644); // allow time_cycle to be set as a module parameter

static struct timer_list my_timer;

static int cycle_count = 0;

void my_timer_callback(struct timer_list *timer)
{
    if (cycle_count == 0) {
        printk(KERN_INFO "Green: %u\n", jiffies_to_msecs(jiffies)/1000);
        mod_timer(&my_timer, jiffies + msecs_to_jiffies(time_cycle * 3));
        cycle_count = cycle_count + 1;
    } else if (cycle_count == 1) {
        printk(KERN_INFO "Yellow: %u\n", jiffies_to_msecs(jiffies)/1000);
        mod_timer(&my_timer, jiffies + msecs_to_jiffies(time_cycle));
        cycle_count = cycle_count + 1;
    } else if (cycle_count == 2) {
        printk(KERN_INFO "Red: %u\n", jiffies_to_msecs(jiffies)/1000);
        mod_timer(&my_timer, jiffies + msecs_to_jiffies(time_cycle * 2));
        cycle_count = 0;
    }
}

static int __init my_module_init(void)
{
    printk(KERN_INFO "My module loaded\n");
    
    timer_setup(&my_timer, my_timer_callback, 0);
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(1));
    
    return 0;
}

static void __exit my_module_exit(void)
{
    del_timer(&my_timer);
    printk(KERN_INFO "My module unloaded\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
