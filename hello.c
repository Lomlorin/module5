#include <linux/module.h> // включено для всех модулей ядра
#include <linux/kernel.h> // включено для KERN_INFO
#include <linux/init.h> // включено для макросов init и exit

MODULE_LICENSE("GPL");
MODULE_AUTHOR("astashova");
MODULE_DESCRIPTION("Simple module Hello World");

static int __init hello_init(void)
{
 printk(KERN_INFO "polinka super\n");
 return 0; // Не нулевой результат означает, что модуль не удалось загрузить.
}

static void __exit hello_cleanup(void)
{
 printk(KERN_INFO "clean.\n");
}

module_init(hello_init);
module_exit(hello_cleanup);
