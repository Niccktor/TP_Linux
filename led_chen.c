#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <linux/slab.h>

#define PROC_DIR       "ensea"
#define PROC_SPEED     "speed"
#define PROC_DIRECTION "dir"

static int speed = 500; // millisecondes
static int direction = 1;
static u8 pattern = 0x01;

module_param(speed, int, 0644);
MODULE_PARM_DESC(speed, "Vitesse du chenillard (ms)");

struct ensea_leds_dev {
    struct miscdevice miscdev;
    void __iomem *regs;
};

static struct ensea_leds_dev *leds_dev;
static struct timer_list led_timer;
static struct proc_dir_entry *proc_dir, *proc_speed, *proc_dir_file;

// === Timer ===
static void led_timer_fn(unsigned long data) {
    if (!leds_dev || !leds_dev->regs) return;

    iowrite32(pattern, leds_dev->regs);

    if (direction == 1) {
        pattern <<= 1;
        if (pattern == 0) pattern = 0x01;
    } else {
        pattern >>= 1;
        if (pattern == 0) pattern = 0x80;
    }

    mod_timer(&led_timer, jiffies + msecs_to_jiffies(speed));
}

// === /dev/ensea_leds ===
static ssize_t leds_read(struct file *file, char __user *buf, size_t len, loff_t *off) {
    return copy_to_user(buf, &pattern, sizeof(pattern)) ? -EFAULT : sizeof(pattern);
}

static ssize_t leds_write(struct file *file, const char __user *buf, size_t len, loff_t *off) {
    if (copy_from_user(&pattern, buf, sizeof(pattern)))
        return -EFAULT;
    return sizeof(pattern);
}

static const struct file_operations leds_fops = {
    .owner = THIS_MODULE,
    .read = leds_read,
    .write = leds_write
};

// === /proc/ensea/speed ===
static ssize_t speed_read(struct file *file, char __user *buf, size_t len, loff_t *off) {
    char tmp[16];
    int l = snprintf(tmp, sizeof(tmp), "%d\n", speed);
    return simple_read_from_buffer(buf, len, off, tmp, l);
}

static ssize_t speed_write(struct file *file, const char __user *buf, size_t len, loff_t *off) {
    char tmp[16];
    if (copy_from_user(tmp, buf, min(len, sizeof(tmp) - 1))) return -EFAULT;
    tmp[min(len, sizeof(tmp) - 1)] = '\0';
    kstrtoint(tmp, 10, &speed);
    return len;
}

static const struct file_operations speed_ops = {
    .owner = THIS_MODULE,
    .read = speed_read,
    .write = speed_write
};

// === /proc/ensea/dir ===
static ssize_t dir_read(struct file *file, char __user *buf, size_t len, loff_t *off) {
    char tmp[8];
    int l = snprintf(tmp, sizeof(tmp), "%d\n", direction);
    return simple_read_from_buffer(buf, len, off, tmp, l);
}

static ssize_t dir_write(struct file *file, const char __user *buf, size_t len, loff_t *off) {
    char tmp[8];
    if (copy_from_user(tmp, buf, min(len, sizeof(tmp) - 1))) return -EFAULT;
    tmp[min(len, sizeof(tmp) - 1)] = '\0';
    kstrtoint(tmp, 10, &direction);
    return len;
}

static const struct file_operations dir_ops = {
    .owner = THIS_MODULE,
    .read = dir_read,
    .write = dir_write
};

// === platform driver ===
static int leds_probe(struct platform_device *pdev) {
    struct resource *res;

    leds_dev = devm_kzalloc(&pdev->dev, sizeof(*leds_dev), GFP_KERNEL);
    if (!leds_dev) return -ENOMEM;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) return -ENODEV;

    leds_dev->regs = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(leds_dev->regs)) return PTR_ERR(leds_dev->regs);

    leds_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
    leds_dev->miscdev.name = "ensea_leds";
    leds_dev->miscdev.fops = &leds_fops;
    if (misc_register(&leds_dev->miscdev)) return -ENODEV;

    init_timer(&led_timer);
    led_timer.function = led_timer_fn;
    mod_timer(&led_timer, jiffies + msecs_to_jiffies(speed));

    proc_dir = proc_mkdir(PROC_DIR, NULL);
    proc_speed = proc_create(PROC_SPEED, 0666, proc_dir, &speed_ops);
    proc_dir_file = proc_create(PROC_DIRECTION, 0666, proc_dir, &dir_ops);

    platform_set_drvdata(pdev, leds_dev);
    return 0;
}

static int leds_remove(struct platform_device *pdev) {
    del_timer_sync(&led_timer);
    misc_deregister(&leds_dev->miscdev);
    remove_proc_entry(PROC_SPEED, proc_dir);
    remove_proc_entry(PROC_DIRECTION, proc_dir);
    remove_proc_entry(PROC_DIR, NULL);
    return 0;
}

static const struct of_device_id ensea_leds_dt_ids[] = {
    { .compatible = "dev,ensea" },
    { }
};

MODULE_DEVICE_TABLE(of, ensea_leds_dt_ids);

static struct platform_driver leds_platform_driver = {
    .driver = {
        .name = "led_chen_driver",
        .of_match_table = ensea_leds_dt_ids,
    },
    .probe = leds_probe,
    .remove = leds_remove
};

module_platform_driver(leds_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ENSEA");
MODULE_DESCRIPTION("Chenillard compatible kernel 4.5 /dev + /proc");
