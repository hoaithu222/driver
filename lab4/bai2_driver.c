/*
 * Bai 2: Character Driver - Cap phat device number TINH
 * Ten driver: lab4.2_thu
 *
 * Su dung register_chrdev_region() de cap phat Major number tinh.
 * Can chon Major number chua bi su dung (xem /proc/devices).
 * Ho tro cac thao tac: open, release, read, write.
 */
 #include <linux/module.h>
 #include <linux/kernel.h>
 #include <linux/init.h>
 #include <linux/fs.h>
 #include <linux/cdev.h>
 #include <linux/device.h>
 #include <linux/uaccess.h>
 #include <linux/string.h>
 
 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("Thu");
 MODULE_DESCRIPTION("Lab4 Bai2: Character driver - cap phat tinh");
 
 #define DEVICE_NAME "lab4.2_thu"
 #define CLASS_NAME  "lab4_2_class"
 #define BUF_SIZE    1024
 
 /* Major number tinh (chon so chua bi su dung) */
 #define MY_MAJOR 235
 #define MY_MINOR 0
 
 /* Bien toan cuc */
 static dev_t dev_num;
 static struct cdev my_cdev;
 static struct class *my_class;
 static struct device *my_device;
 
 static char kernel_buf[BUF_SIZE];
 static int buf_len = 0;
 static int open_count = 0;
 
 /* Tham so module cho phep thay doi Major number */
 static int major = MY_MAJOR;
 module_param(major, int, 0644);
 MODULE_PARM_DESC(major, "Major number (mac dinh: 240)");
 
 /* ===== File Operations ===== */
 
 static int dev_open(struct inode *inodep, struct file *filep)
 {
     open_count++;
     pr_info("[%s] Device opened (lan thu %d)\n", DEVICE_NAME, open_count);
     return 0;
 }
 
 static int dev_release(struct inode *inodep, struct file *filep)
 {
     pr_info("[%s] Device closed\n", DEVICE_NAME);
     return 0;
 }
 
 static ssize_t dev_read(struct file *filep, char __user *buf,
                         size_t len, loff_t *offset)
 {
     int bytes_to_read;
 
     if (*offset >= buf_len)
         return 0;
 
     bytes_to_read = min((int)(buf_len - *offset), (int)len);
 
     if (copy_to_user(buf, kernel_buf + *offset, bytes_to_read)) {
         pr_err("[%s] Loi copy_to_user\n", DEVICE_NAME);
         return -EFAULT;
     }
 
     *offset += bytes_to_read;
     pr_info("[%s] Read %d bytes, offset = %lld\n",
             DEVICE_NAME, bytes_to_read, *offset);
     return bytes_to_read;
 }
 
 static ssize_t dev_write(struct file *filep, const char __user *buf,
                          size_t len, loff_t *offset)
 {
     int bytes_to_write;
 
     bytes_to_write = min((int)len, (int)(BUF_SIZE - 1));
 
     if (copy_from_user(kernel_buf, buf, bytes_to_write)) {
         pr_err("[%s] Loi copy_from_user\n", DEVICE_NAME);
         return -EFAULT;
     }
 
     kernel_buf[bytes_to_write] = '\0';
     buf_len = bytes_to_write;
     pr_info("[%s] Write %d bytes: \"%s\"\n",
             DEVICE_NAME, bytes_to_write, kernel_buf);
     return bytes_to_write;
 }
 
 /* File operations structure */
 static const struct file_operations fops = {
     .owner   = THIS_MODULE,
     .open    = dev_open,
     .release = dev_release,
     .read    = dev_read,
     .write   = dev_write,
     .llseek  = default_llseek,
 };
 
 /* ===== Module Init ===== */
 static int __init bai2_init(void)
 {
     int ret;
 
     pr_info("[%s] Khoi tao module...\n", DEVICE_NAME);
 
     /* 1. Cap phat device number TINH */
     dev_num = MKDEV(major, MY_MINOR);
     ret = register_chrdev_region(dev_num, 1, DEVICE_NAME);
     if (ret < 0) {
         pr_err("[%s] Loi register_chrdev_region (Major=%d): %d\n",
                DEVICE_NAME, major, ret);
         pr_err("[%s] Thu dung Major khac: sudo insmod bai2_driver.ko major=XXX\n",
                DEVICE_NAME);
         return ret;
     }
     pr_info("[%s] Cap phat tinh: Major = %d, Minor = %d\n",
             DEVICE_NAME, MAJOR(dev_num), MINOR(dev_num));
 
     /* 2. Khoi tao cdev */
     cdev_init(&my_cdev, &fops);
     my_cdev.owner = THIS_MODULE;
 
     ret = cdev_add(&my_cdev, dev_num, 1);
     if (ret < 0) {
         pr_err("[%s] Loi cdev_add: %d\n", DEVICE_NAME, ret);
         goto err_cdev;
     }
 
     /* 3. Tao device class */
     my_class = class_create(CLASS_NAME);
     if (IS_ERR(my_class)) {
         pr_err("[%s] Loi class_create\n", DEVICE_NAME);
         ret = PTR_ERR(my_class);
         goto err_class;
     }
 
     /* 4. Tao device file /dev/lab4.2_thu */
     my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
     if (IS_ERR(my_device)) {
         pr_err("[%s] Loi device_create\n", DEVICE_NAME);
         ret = PTR_ERR(my_device);
         goto err_device;
     }
 
     /* Khoi tao buffer */
     strscpy(kernel_buf, "Hello from lab4.2_thu driver (static)!\n", BUF_SIZE);
     buf_len = strlen(kernel_buf);
 
     pr_info("[%s] Module nap thanh cong! Device: /dev/%s (Major=%d)\n",
             DEVICE_NAME, DEVICE_NAME, major);
     return 0;
 
 err_device:
     class_destroy(my_class);
 err_class:
     cdev_del(&my_cdev);
 err_cdev:
     unregister_chrdev_region(dev_num, 1);
     return ret;
 }
 
 /* ===== Module Exit ===== */
 static void __exit bai2_exit(void)
 {
     device_destroy(my_class, dev_num);
     class_destroy(my_class);
     cdev_del(&my_cdev);
     unregister_chrdev_region(dev_num, 1);
     pr_info("[%s] Module da duoc go\n", DEVICE_NAME);
 }
 
 module_init(bai2_init);
 module_exit(bai2_exit);
 