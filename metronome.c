#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/sysrq.h>
#include <linux/tty.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pablo Garaizar");

#define METRO_DELAY	1E9L
#define METRO_KEY	KEY_SPACE
#define METRO_SYSRQ	0x64
#define METRO_HELP_MSG  "metronome(d)"
#define METRO_STATUS	0

static long metronome_delay = METRO_DELAY;
static unsigned int metronome_key = METRO_KEY;
static long metronome_delay2 = 0;
static unsigned int metronome_key2 = METRO_KEY;
static long metronome_delay3 = 0;
static unsigned int metronome_key3 = METRO_KEY;
static int metronome_sysrq = METRO_SYSRQ;
static int metronome_status = METRO_STATUS;

module_param(metronome_delay, long, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(metronome_delay, "delay of the high-resolution timer in ns for the first key (default = 1E+9, 1000 ms)");
module_param(metronome_key, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(metronome_key, "first key (default = KEY_SPACE)");
module_param(metronome_delay2, long, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(metronome_delay2, "delay of the high-resolution timer in ns for the second key (default = undefined)");
module_param(metronome_key2, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(metronome_key2, "second key (default = KEY_SPACE)");
module_param(metronome_delay3, long, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(metronome_delay3, "delay of the high-resolution timer in ns for the third key (default = undefined)");
module_param(metronome_key3, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(metronome_key3, "third key (default = KEY_SPACE)");
module_param(metronome_sysrq, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(metronome_sysrq, "SysRq key (default = 'd')");
module_param(metronome_status, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(metronome_status, "status (default = 0, off)");

static struct hrtimer hrt, hrt2, hrt3;
static struct input_dev *dev;

static void sysrq_handle_metronome(int key, struct tty_struct *tty)
{
	metronome_status = (metronome_status) ? 0 : 1;
}

static struct sysrq_key_op sysrq_metronome_op = {
	.handler	= (void *)sysrq_handle_metronome,
	.help_msg	= METRO_HELP_MSG,
	.action_msg	= "Changing metronome state",
	.enable_mask	= SYSRQ_ENABLE_KEYBOARD,
};

enum hrtimer_restart metronome_hrt_callback(struct hrtimer *timer)
{
	ktime_t now, t;
	unsigned long missed;

	now = ktime_get();
	t = ktime_set(0, metronome_delay);
	missed = hrtimer_forward(&hrt, now, t);

	if (missed > 1)
	{
		printk(KERN_INFO "metronome: Missed ticks: %ld.\n", missed - 1);
	}

	if (metronome_status)
	{
		input_report_key(dev, metronome_key, 1);
		input_sync(dev);
		input_report_key(dev, metronome_key, 0);
		input_sync(dev);
		printk(KERN_INFO "metronome: Key event (%d, %lluns).\n", metronome_key, ktime_to_ns(now));
	}

	return HRTIMER_RESTART;
}

enum hrtimer_restart metronome_hrt_callback2(struct hrtimer *timer)
{
	ktime_t now, t;
	unsigned long missed;

	now = ktime_get();
	t = ktime_set(0, metronome_delay2);
	missed = hrtimer_forward(&hrt2, now, t);

	if (missed > 1)
	{
		printk(KERN_INFO "metronome: Missed ticks 2: %ld.\n", missed - 1);
	}

	if (metronome_status)
	{
		input_report_key(dev, metronome_key2, 1);
		input_sync(dev);
		input_report_key(dev, metronome_key2, 0);
		input_sync(dev);
		printk(KERN_INFO "metronome: Key event 2 (%d, %lluns).\n", metronome_key2, ktime_to_ns(now));
	}

	return HRTIMER_RESTART;
}

enum hrtimer_restart metronome_hrt_callback3(struct hrtimer *timer)
{
	ktime_t now, t;
	unsigned long missed;

	now = ktime_get();
	t = ktime_set(0, metronome_delay3);
	missed = hrtimer_forward(&hrt3, now, t);

	if (missed > 1)
	{
		printk(KERN_INFO "metronome: Missed ticks 3: %ld.\n", missed - 1);
	}

	if (metronome_status)
	{
		input_report_key(dev, metronome_key3, 1);
		input_sync(dev);
		input_report_key(dev, metronome_key3, 0);
		input_sync(dev);
		printk(KERN_INFO "metronome: Key event 3 (%d, %lluns).\n", metronome_key3, ktime_to_ns(now));
	}

	return HRTIMER_RESTART;
}

int __init metronome_init(void)
{
	ktime_t t, t2, t3;
	int ret = 0;

	printk(KERN_NOTICE "metronome: Loaded.\n");

	printk(KERN_INFO "metronome: Registering device...\n");
	dev = input_allocate_device();

	if (dev) 
	{
		dev->name = "Generic device";
		dev->evbit[0] = BIT (EV_KEY) | BIT (EV_REP); 
		set_bit(metronome_key, dev->keybit); 
		set_bit(metronome_key2, dev->keybit); 
		set_bit(metronome_key3, dev->keybit); 
		ret = input_register_device(dev);
		if (ret) 
		{
			printk(KERN_ERR "metronome: Failed to register device.\n");
			input_free_device(dev);
			ret = -1;
		} 
		else
		{
			printk(KERN_INFO "metronome: Registering SysRq key (%c)...\n", (char)metronome_sysrq);
			register_sysrq_key(metronome_sysrq, &sysrq_metronome_op);
			t = ktime_set(0, metronome_delay);
			printk(KERN_INFO "metronome: Starting high-resolution timer (%lluns)...\n", ktime_to_ns(t));
			hrtimer_init(&hrt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
			hrt.function = &metronome_hrt_callback;
			hrtimer_start(&hrt, t, HRTIMER_MODE_REL);
			if (metronome_delay2 > 0)
			{
				t2 = ktime_set(0, metronome_delay2);
				printk(KERN_INFO "metronome: Starting high-resolution timer (%lluns)...\n", ktime_to_ns(t2));
				hrtimer_init(&hrt2, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
				hrt2.function = &metronome_hrt_callback2;
				hrtimer_start(&hrt2, t2, HRTIMER_MODE_REL);
			}
			if (metronome_delay3 > 0)
			{
				t3 = ktime_set(0, metronome_delay3);
				printk(KERN_INFO "metronome: Starting high-resolution timer (%lluns)...\n", ktime_to_ns(t3));
				hrtimer_init(&hrt3, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
				hrt3.function = &metronome_hrt_callback3;
				hrtimer_start(&hrt3, t3, HRTIMER_MODE_REL);
			}
		}
	} 
	else
	{
		printk(KERN_ERR "metronome: Failed to register device.\n");
		ret = -1;
	}

	return ret;
}

static void __exit metronome_exit(void)
{
	printk(KERN_INFO "metronome: Unregistering SysRq key (%c)...\n", (char)metronome_sysrq);
	unregister_sysrq_key(metronome_sysrq, &sysrq_metronome_op);
	printk(KERN_INFO "metronome: Stopping high-resolution timer...\n");
	hrtimer_cancel(&hrt);
	if (metronome_delay2 > 0) 
	{
		hrtimer_cancel(&hrt2);
	}
	if (metronome_delay3 > 0)
	{
		hrtimer_cancel(&hrt3);
	}
	printk(KERN_INFO "metronome: Unregistering device...\n");
	input_unregister_device(dev);
	printk(KERN_NOTICE "metronome: Unloaded.\n");
}

module_init(metronome_init);
module_exit(metronome_exit);
