#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/netlink.h>
#include <net/sock.h>

#define NETLINK_USER 31
#define MAX_PAYLOAD 1024
#define PROC_FILENAME "netlink_proc"

static struct sock *nl_sk = NULL;
static struct proc_dir_entry *proc_file;

// Буфер для обработки сообщений через procfs
static char proc_buffer[MAX_PAYLOAD];
static size_t proc_buffer_len;

// Обработчик получения сообщений через Netlink
static void nl_recv_msg(struct sk_buff *skb) {
    struct nlmsghdr *nlh;
    char *msg;

    nlh = (struct nlmsghdr *)skb->data;
    msg = (char *)nlmsg_data(nlh);

    pr_info("Netlink: Received message from user: %s\n", msg);
}

// Обработчик записи в файл /proc/netlink_proc
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos) {
    struct nlmsghdr *nlh;
    struct sk_buff *skb_out;
    char kernel_reply[MAX_PAYLOAD];

    if (count > MAX_PAYLOAD - 1)
        return -EINVAL;

    if (copy_from_user(proc_buffer, buffer, count))
        return -EFAULT;

    proc_buffer[count] = '\0'; // Завершаем строку
    proc_buffer_len = count;

    pr_info("Procfs: Received command: %s\n", proc_buffer);

    // Формируем Netlink-сообщение
    snprintf(kernel_reply, MAX_PAYLOAD, "Kernel received: %s", proc_buffer);

    skb_out = nlmsg_new(strlen(kernel_reply), GFP_KERNEL);
    if (!skb_out) {
        pr_err("Netlink: Failed to allocate skb\n");
        return -ENOMEM;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, strlen(kernel_reply), 0);
    strncpy(nlmsg_data(nlh), kernel_reply, strlen(kernel_reply));

    // Отправляем сообщение ядру (PID ядра всегда 0)
    if (nlmsg_unicast(nl_sk, skb_out, 0) < 0)
        pr_err("Netlink: Failed to send message\n");

    return count;
}

// Обработчик чтения файла /proc/netlink_proc
static ssize_t proc_read(struct file *file, char __user *buffer, size_t count, loff_t *pos) {
    if (*pos > 0 || proc_buffer_len == 0)
        return 0;

    if (copy_to_user(buffer, proc_buffer, proc_buffer_len))
        return -EFAULT;

    *pos = proc_buffer_len;
    return proc_buffer_len;
}

static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

// Инициализация модуля
static int __init netlink_init(void) {
    struct netlink_kernel_cfg cfg = {
        .input = nl_recv_msg,
    };

    pr_info("Netlink: Initializing module\n");

    // Создаём Netlink-сокет
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sk) {
        pr_err("Netlink: Failed to create socket\n");
        return -ENOMEM;
    }

    // Создаём файл в /proc
    proc_file = proc_create(PROC_FILENAME, 0666, NULL, &proc_fops);
    if (!proc_file) {
        pr_err("Procfs: Failed to create file\n");
        netlink_kernel_release(nl_sk);
        return -ENOMEM;
    }

    pr_info("Netlink: Module loaded successfully\n");
    return 0;
}

// Завершение работы модуля
static void __exit netlink_exit(void) {
    pr_info("Netlink: Exiting module\n");

    if (proc_file)
        proc_remove(proc_file);

    if (nl_sk)
        netlink_kernel_release(nl_sk);
}

module_init(netlink_init);
module_exit(netlink_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("astashova");
MODULE_DESCRIPTION("Netlink Kernel Module with Procfs Interface");
