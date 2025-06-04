#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/jiffies.h>

#define HEX_BASE_PHYS 0xFF236000  // = 0xFF203000 + 0x33000
#define HEX_SPAN      0x20

static struct timer_list hour_timer;
static void __iomem *hex_base = NULL;

static const u8 hex_map[16] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F, // 9
    0x77, // A
    0x7C, // b
    0x39, // C
    0x5E, // d
    0x79, // E
    0x71  // F
};

static void update_display(struct timer_list *t)
{
    struct timespec ts;
    struct tm tm;
    int h, m, s;

    getnstimeofday(&ts);
    time64_to_tm(ts.tv_sec, 0, &tm);

    h = tm.tm_hour;
    m = tm.tm_min;
    s = tm.tm_sec;

    if (hex_base) {
        iowrite32(hex_map[h / 10], hex_base + 4 * 5); // HEX5
        iowrite32(hex_map[h % 10], hex_base + 4 * 4); // HEX4
        iowrite32(hex_map[m / 10], hex_base + 4 * 3); // HEX3
        iowrite32(hex_map[m % 10], hex_base + 4 * 2); // HEX2
        iowrite32(hex_map[s / 10], hex_base + 4 * 1); // HEX1
        iowrite32(hex_map[s % 10], hex_base + 4 * 0); // HEX0
    }

    mod_timer(&hour_timer, jiffies + HZ); // toutes les 1 sec
}

static int __init led_hour_init(void)
{
    pr_info("led_hour: module init\n");

    hex_base = ioremap(HEX_BASE_PHYS, HEX_SPAN);
    if (!hex_base) {
        pr_err("led_hour: ioremap failed\n");
        return -ENOMEM;
    }

    timer_setup(&hour_timer, update_display, 0);
    mod_timer(&hour_timer, jiffies + HZ);

    return 0;
}

static void __exit led_hour_exit(void)
{
    del_timer_sync(&hour_timer);

    if (hex_base)
        iounmap(hex_base);

    pr_info("led_hour: module exit\n");
}

module_init(led_hour_init);
module_exit(led_hour_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ENSEA - VEEK MT2S");
MODULE_DESCRIPTION("Module kernel pour afficher l'heure HHMMSS sur les HEX");
