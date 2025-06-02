#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>

#define PROC_NAME_DIR   "ensea"
#define PROC_NAME_SPEED "speed"
#define PROC_NAME_DIR "speed"

static struct proc_dir_entry *ensea_dir;    
static struct proc_dir_entry *speed_file;
static struct proc_dir_entry *dir_file;

#define DEVICE_NAME_LED "ensea-led"
#define DEVICE_NAME_DIR "ensea/dir"


static struct task_struct *chenillard_thread;
static uint8_t led_state = 0x01;
static int direction = 1; // 1 = gauche, 0 = droite
static int speed_ms = 500;

module_param(speed_ms, uint, 0644);
MODULE_PARM_DESC(speed_ms, "Vitesse du chenillard en millisecondes");

static DEFINE_MUTEX(lock);

static int chenillard_fn(void *data) {
    struct file *led_file;
    mm_segment_t oldfs;
    uint8_t value = 0x01;

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    led_file = filp_open(PATH_LED, O_WRONLY, 0);
    if (IS_ERR(led_file)) {
        pr_err("Impossible d'ouvrir %s\n", PATH_LED);
        set_fs(oldfs);
        return PTR_ERR(led_file);
    }

    while (!kthread_should_stop()) {
        mutex_lock(&lock);

        kernel_write(led_file, (char *)&value, 1, &led_file->f_pos);

        if (direction)
            value = (value == 0x80) ? 0x01 : (value << 1);
        else
            value = (value == 0x01) ? 0x80 : (value >> 1);

        mutex_unlock(&lock);
        msleep(speed_ms);
    }

    filp_close(led_file, NULL);
    set_fs(oldfs);

    return 0;
}

/* ----- /proc/ensea/speed ----- */
static ssize_t speed_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    char buffer[16];
    int len = snprintf(buffer, sizeof(buffer), "%d\n", speed_ms);
    return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static ssize_t speed_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    char buffer[16];
    if (copy_from_user(buffer, buf, min(count, sizeof(buffer) - 1)))
        return -EFAULT;

    buffer[min(count, sizeof(buffer) - 1)] = '\0';
    if (kstrtoint(buffer, 10, &speed_ms) < 0)
        return -EINVAL;

    return count;
}

static const struct proc_ops speed_fops = {
    .proc_read = speed_read,
    .proc_write = speed_write,
};

/* ----- /dev/ensea/dir ----- */
static ssize_t dir_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    char buffer[16];
    int len = snprintf(buffer, sizeof(buffer), "%d\n", direction);
    return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static ssize_t dir_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    char kbuf[4];
    if (copy_from_user(kbuf, buf, min(count, sizeof(kbuf) - 1)))
        return -EFAULT;

    kbuf[min(count, sizeof(kbuf) - 1)] = '\0';
    if (kstrtoint(kbuf, 10, &direction) < 0)
        return -EINVAL;

    direction = (direction != 0); // 0 ou 1
    return count;
}

static const struct proc_ops dir_fops = {
    .proc_write = dir_write,
    .proc_read = dir_read,
};

/* ----- Init / Exit ----- */
static int __init chenillard_init(void) {
    int ret;

    ensea_dir = proc_mkdir(PROC_NAME_DIR, NULL);
    speed_file = proc_create(PROC_NAME_SPEED, 0666, ensea_dir, &speed_fops);
    dir_file = proc_create(PROC_NAME_DIR, 0666, ensea_dir, &dir_fops);

    chenillard_thread = kthread_run(chenillard_fn, NULL, "chenillard_thread");
    if (IS_ERR(chenillard_thread)) {
        remove_proc_entry(speed_file, ensea_dir);
        remove_proc_entry(dir_file, ensea_dir);
        remove_proc_entry(ensea_dir, NULL);
        return PTR_ERR(chenillard_thread);
    }

    printk(KERN_INFO "Chenillard module loaded.\n");
    return 0;
}

static void __exit chenillard_exit(void) {
    kthread_stop(chenillard_thread);

    kemove_proc_entry(speed_file, ensea_dir);
    remove_proc_entry(dir_file, ensea_dir);
    remove_proc_entry(ensea_dir, NULL);
    printk(KERN_INFO "Chenillard module unloaded.\n");
}

module_init(chenillard_init);
module_exit(chenillard_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ENSEA");
MODULE_DESCRIPTION("Chenillard à thread noyau avec /dev et /proc");
