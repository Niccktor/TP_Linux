#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_AUTHOR("Quelqu'un");
MODULE_DESCRIPTION("Exemple de module");
MODULE_SUPPORTED_DEVICE("Tous");
MODULE_LICENSE("GPL");

static int __init le_module_init(void) {
    printk(KERN_INFO "Hello world!\n");
    return 0;
}

static void __exit le_module_exit(void) {
    printk(KERN_ALERT "Bye bye...\n");
}
    module_init(le_module_init);
    module_exit(le_module_exit);