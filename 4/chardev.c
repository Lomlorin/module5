#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h> 

#define DEVICE_NAME "chardev"
#define BUF_LEN 256 

static int major;
static char msg[BUF_LEN]; // Буфер 
static size_t msg_len;    // Длина актуального сообщения
static struct class *cls;

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);

// Операции символьного устройства
static struct file_operations chardev_fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
};

static int __init chardev_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &chardev_fops);
    if (major < 0) {
        pr_alert("Registering char device failed with %d\n", major);
        return major;
    }
    pr_info("I was assigned major number %d.\n", major);

    cls = class_create(DEVICE_NAME);
    if (IS_ERR(cls)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(cls);
    }

    if (IS_ERR(device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME))) {
        class_destroy(cls);
        unregister_chrdev(major, DEVICE_NAME);
        return -1;
    }

    pr_info("Device created on /dev/%s\n", DEVICE_NAME);
    return 0;
}
static void __exit chardev_exit(void) {
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("Char device unregistered\n");
}

static int device_open(struct inode *inode, struct file *file) {
    pr_info("Device opened\n");
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    pr_info("Device closed\n");
    return 0;
}

static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset) {
    ssize_t bytes_read = 0;

    if (*offset >= msg_len) // Если все данные прочитаны
        return 0;

    if (length > msg_len - *offset) // Ограничиваем количество байт для чтения
        length = msg_len - *offset;

    if (copy_to_user(buffer, msg + *offset, length)) {
        pr_alert("Failed to send data to userspace\n");
        return -EFAULT;
    }

    *offset += length;
    bytes_read = length;
    pr_info("Sent %zu bytes to userspace\n", bytes_read);
    return bytes_read;
}

static ssize_t device_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset) {
    if (length >= BUF_LEN) {
        pr_alert("Input too large\n");
        return -EINVAL;
    }

    if (copy_from_user(msg, buffer, length)) {
        pr_alert("Failed to receive data from userspace\n");
        return -EFAULT;
    }

    msg[length] = '\0'; // Завершаем строку
    msg_len = length;
    pr_info("Received %zu bytes from userspace: %s\n", length, msg);
    return length;
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("astashova");
MODULE_DESCRIPTION("A simple chardev module for kernel-userspace communication");

