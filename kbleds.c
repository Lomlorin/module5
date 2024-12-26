#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/console_struct.h>
#include <linux/vt_kern.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/console_struct.h>
#include <linux/vt_kern.h>
#include <linux/timer.h>

MODULE_DESCRIPTION("Keyboard LED control via sysfs.");
MODULE_LICENSE("GPL");

struct timer_list my_timer;
struct tty_driver *my_driver;
static int led_status = 0;
static int blink_pattern = 3; // LED pattern mask (binary)

#define BLINK_DELAY   HZ / 5
#define RESTORE_LEDS  0xFF
#define ALL_LEDS_ON   0x07

static struct kobject *example_kobject;

static ssize_t led_mask_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", led_status);
}

static ssize_t led_mask_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int new_mask;
    if (kstrtoint(buf, 10, &new_mask) == 0 && new_mask >= 0 && new_mask <= 7) {
        blink_pattern = new_mask;
    } else {
        pr_err("Invalid value. Enter a number from 0 to 7.\n");
    }
    return count;
}

static struct kobj_attribute led_mask_attribute = __ATTR(led_mask, 0664, led_mask_show, led_mask_store);

/*
 * Function my_timer_func blinks the keyboard LEDs periodically
 * by invoking the KDSETLED ioctl command on the keyboard driver.
 */
static void my_timer_func(struct timer_list *ptr)
{
    if (led_status == blink_pattern)
        led_status = RESTORE_LEDS;
    else
        led_status = blink_pattern;
    
    (my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, led_status);
    
    my_timer.expires = jiffies + BLINK_DELAY;
    add_timer(&my_timer);
}

static int __init kbleds_init(void)
{
    int error = 0;

    printk(KERN_INFO "kbleds: loading\n");

    example_kobject = kobject_create_and_add("kbleds", kernel_kobj);
    if (!example_kobject)
        return -ENOMEM;

    error = sysfs_create_file(example_kobject, &led_mask_attribute.attr);
    if (error) {
        pr_debug("Failed to create led_mask file in /sys/kernel/kbleds\n");
        return error;
    }

    my_driver = vc_cons[fg_console].d->port.tty->driver;

    /* Set up the LED blink timer the first time */
    timer_setup(&my_timer, my_timer_func, 0);
    my_timer.expires = jiffies + BLINK_DELAY;
    add_timer(&my_timer);
    
    return 0;
}

static void __exit kbleds_cleanup(void)
{
    printk(KERN_INFO "kbleds: unloading\n");
    del_timer(&my_timer);
    (my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, RESTORE_LEDS);
    kobject_put(example_kobject);
}

module_init(kbleds_init);
module_exit(kbleds_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("astashova");