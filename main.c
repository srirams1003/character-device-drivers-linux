#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#define MAX_DEV 3
#define MAX_DATA_LEN 30
#define BUFFER_SIZE 256

static int mychardev_open(struct inode *inode, struct file *file);
static int mychardev_release(struct inode *inode, struct file *file);
static long mychardev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t mychardev_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t mychardev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

static const struct file_operations mychardev_fops = {
    .owner      = THIS_MODULE,
    .open       = mychardev_open,
    .release    = mychardev_release,
    .unlocked_ioctl = mychardev_ioctl,
    .read       = mychardev_read,
    .write       = mychardev_write
};

struct mychar_device_data {
    struct cdev cdev;
    char buffer[BUFFER_SIZE];
    size_t size;
};

static int dev_major = 0;
static struct class *mychardev_class = NULL;
// static struct mychar_device_data mychardev_data[MAX_DEV];
static struct mychar_device_data *mychardev_data[MAX_DEV];


static int mychardev_uevent(const struct device *dev, struct kobj_uevent_env *env)
// static int mychardev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static int __init mychardev_init(void)
{
    int err, i;
    dev_t dev;

    err = alloc_chrdev_region(&dev, 0, MAX_DEV, "mychardev");

    dev_major = MAJOR(dev);

    // mychardev_class = class_create(THIS_MODULE, "mychardev");
    mychardev_class = class_create("mychardev");

    printk("mychardev: Module init\n");

    mychardev_class->dev_uevent = mychardev_uevent;

    for (i = 0; i < MAX_DEV; i++) {
        mychardev_data[i] = kmalloc(sizeof(struct mychar_device_data), GFP_KERNEL);
        if (!mychardev_data[i]) {
            // Handle error
            printk("mychardev: Error\n");
        }
        cdev_init(&mychardev_data[i]->cdev, &mychardev_fops);
        mychardev_data[i]->cdev.owner = THIS_MODULE;
        mychardev_data[i]->size = 0;

        cdev_add(&mychardev_data[i]->cdev, MKDEV(dev_major, i), 1);

        device_create(mychardev_class, NULL, MKDEV(dev_major, i), NULL, "mychardev-%d", i);
    }

    return 0;
}

static void __exit mychardev_exit(void)
{
    int i;

    for (i = 0; i < MAX_DEV; i++) {
        if (mychardev_data[i]) {
            device_destroy(mychardev_class, MKDEV(dev_major, i));
            kfree(mychardev_data[i]);
        }
    }
    printk("mychardev: Module exit\n");

    class_unregister(mychardev_class);
    class_destroy(mychardev_class);

    unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);
}

static int mychardev_open(struct inode *inode, struct file *file)
{
    file->private_data = container_of(inode->i_cdev, struct mychar_device_data, cdev);
    printk("MYCHARDEV: Device open\n");
    return 0;
}

static int mychardev_release(struct inode *inode, struct file *file)
{
    printk("MYCHARDEV: Device close\n");
    return 0;
}

static long mychardev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    printk("MYCHARDEV: Device ioctl\n");
    return 0;
}

static ssize_t mychardev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    size_t bytes_to_read;
    struct mychar_device_data *dev_data = file->private_data;
    int device_id = MINOR(file->f_path.dentry->d_inode->i_rdev);
    printk(KERN_INFO "mychardev: Reading from device %d\n", device_id);
    printk(KERN_INFO "mychardev output for device %d: %s\n", device_id, dev_data->buffer);

    if (*offset >= dev_data->size) {
        return 0;  // EOF
    }

    bytes_to_read = min(count, dev_data->size - *offset);

    if (copy_to_user(buf, dev_data->buffer + *offset, bytes_to_read)) {
        return -EFAULT;
    }

    *offset += bytes_to_read;

    return bytes_to_read;
}

static ssize_t mychardev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    size_t bytes_to_write = min(count, (size_t)(BUFFER_SIZE - 1));
    struct mychar_device_data *dev_data = file->private_data;

    printk(KERN_INFO "mychardev: Writing to device %d\n", MINOR(file->f_path.dentry->d_inode->i_rdev));

    if (copy_from_user(dev_data->buffer, buf, bytes_to_write)) {
        return -EFAULT;
    }

    dev_data->buffer[bytes_to_write] = '\0';  // Null-terminate the string
    dev_data->size = bytes_to_write;

    printk(KERN_INFO "mychardev: Wrote %zu bytes: %s\n", dev_data->size, dev_data->buffer);

    return bytes_to_write;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleg Kutkov <elenbert@gmail.com>");

module_init(mychardev_init);
module_exit(mychardev_exit);
