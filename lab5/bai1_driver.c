/*
 * Bai 5.1: Character Driver - Chuyen doi he so
 * Ten driver: lab5.1_thu
 * Cap phat device number dong (alloc_chrdev_region)
 *
 * Chuc nang:
 *   - Nhan 1 so he 10 tu userspace
 *   - Chuyen sang he 2, 8, 16 va luu trong kernel
 *   - Cho phep doc cac so he 2, 8, 16 tuong ung
 */
 #include <linux/module.h>
 #include <linux/kernel.h>
 #include <linux/init.h>
 #include <linux/fs.h>
 #include <linux/cdev.h>
 #include <linux/device.h>
 #include <linux/uaccess.h>
 #include <linux/string.h>
 
 #include "lab5_bai1_ioctl.h"
 
 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("Thu");
 MODULE_DESCRIPTION("Lab5 Bai1: Character driver chuyen doi he so");
 
 #define CLASS_NAME "lab5_1_class"
 
 /* Bien toan cuc */
 static dev_t dev_num;
 static struct cdev my_cdev;
 static struct class *my_class;
 static struct device *my_device;
 
 /* Du lieu luu tru */
 static struct base_data current_data;
 static int data_ready = 0;
 
 /* ===== Ham chuyen doi ===== */
 
 /* Dao nguoc chuoi */
 static void reverse_str(char *s, int len)
 {
     int i, j;
     char tmp;
     for (i = 0, j = len - 1; i < j; i++, j--) {
         tmp = s[i]; s[i] = s[j]; s[j] = tmp;
     }
 }
 
 /* He 10 -> He 2 */
 static void dec_to_bin(long val, char *buf, int size)
 {
     int idx = 0;
     int neg = 0;
     unsigned long uval;
 
     if (val == 0) {
         buf[0] = '0'; buf[1] = '\0';
         return;
     }
     if (val < 0) {
         neg = 1;
         uval = (unsigned long)(-val);
     } else {
         uval = (unsigned long)val;
     }
 
     while (uval > 0 && idx < size - 2) {
         buf[idx++] = (uval % 2) + '0';
         uval /= 2;
     }
     if (neg && idx < size - 1)
         buf[idx++] = '-';
     buf[idx] = '\0';
     reverse_str(buf, idx);
 }
 
 /* He 10 -> He 8 */
 static void dec_to_oct(long val, char *buf, int size)
 {
     int idx = 0;
     int neg = 0;
     unsigned long uval;
 
     if (val == 0) {
         buf[0] = '0'; buf[1] = '\0';
         return;
     }
     if (val < 0) {
         neg = 1;
         uval = (unsigned long)(-val);
     } else {
         uval = (unsigned long)val;
     }
 
     while (uval > 0 && idx < size - 2) {
         buf[idx++] = (uval % 8) + '0';
         uval /= 8;
     }
     if (neg && idx < size - 1)
         buf[idx++] = '-';
     buf[idx] = '\0';
     reverse_str(buf, idx);
 }
 
 /* He 10 -> He 16 */
 static void dec_to_hex(long val, char *buf, int size)
 {
     static const char hex_chars[] = "0123456789ABCDEF";
     int idx = 0;
     int neg = 0;
     unsigned long uval;
 
     if (val == 0) {
         buf[0] = '0'; buf[1] = '\0';
         return;
     }
     if (val < 0) {
         neg = 1;
         uval = (unsigned long)(-val);
     } else {
         uval = (unsigned long)val;
     }
 
     while (uval > 0 && idx < size - 2) {
         buf[idx++] = hex_chars[uval % 16];
         uval /= 16;
     }
     if (neg && idx < size - 1)
         buf[idx++] = '-';
     buf[idx] = '\0';
     reverse_str(buf, idx);
 }
 
 /* Thuc hien chuyen doi tat ca he so */
 static void convert_all(long dec_val)
 {
     current_data.dec_val = dec_val;
     dec_to_bin(dec_val, current_data.bin_str, sizeof(current_data.bin_str));
     dec_to_oct(dec_val, current_data.oct_str, sizeof(current_data.oct_str));
     dec_to_hex(dec_val, current_data.hex_str, sizeof(current_data.hex_str));
     data_ready = 1;
 
     pr_info("[%s] Chuyen doi: Dec=%ld, Bin=%s, Oct=%s, Hex=%s\n",
             DEVICE_NAME, dec_val,
             current_data.bin_str, current_data.oct_str, current_data.hex_str);
 }
 
 /* ===== File Operations ===== */
 
 static int dev_open(struct inode *inodep, struct file *filep)
 {
     pr_info("[%s] Device opened\n", DEVICE_NAME);
     return 0;
 }
 
 static int dev_release(struct inode *inodep, struct file *filep)
 {
     pr_info("[%s] Device closed\n", DEVICE_NAME);
     return 0;
 }
 
 /* Read: tra ve chuoi tat ca he so */
 static ssize_t dev_read(struct file *filep, char __user *buf,
                         size_t len, loff_t *offset)
 {
     char out[512];
     int out_len;
 
     if (!data_ready)
         return 0;
     if (*offset > 0)
         return 0;
 
     out_len = snprintf(out, sizeof(out),
         "He 10: %ld\nHe 2 : %s\nHe 8 : %s\nHe 16: %s\n",
         current_data.dec_val,
         current_data.bin_str,
         current_data.oct_str,
         current_data.hex_str);
 
     if (out_len > len)
         out_len = len;
 
     if (copy_to_user(buf, out, out_len))
         return -EFAULT;
 
     *offset += out_len;
     return out_len;
 }
 
 /* Write: nhan so he 10 tu userspace */
 static ssize_t dev_write(struct file *filep, const char __user *buf,
                          size_t len, loff_t *offset)
 {
     char kbuf[64];
     long val;
     int ret;
     int copy_len;
 
     copy_len = min((int)len, (int)(sizeof(kbuf) - 1));
     if (copy_from_user(kbuf, buf, copy_len))
         return -EFAULT;
     kbuf[copy_len] = '\0';
 
     /* Loai bo newline neu co */
     if (copy_len > 0 && kbuf[copy_len - 1] == '\n')
         kbuf[copy_len - 1] = '\0';
 
     ret = kstrtol(kbuf, 10, &val);
     if (ret) {
         pr_err("[%s] Gia tri khong hop le: \"%s\"\n", DEVICE_NAME, kbuf);
         return -EINVAL;
     }
 
     convert_all(val);
     return len;
 }
 
 /* IOCTL */
 static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
 {
     long dec_val;
 
     switch (cmd) {
     case IOCTL_SET_DEC:
         if (copy_from_user(&dec_val, (long __user *)arg, sizeof(long)))
             return -EFAULT;
         convert_all(dec_val);
         break;
 
     case IOCTL_GET_BIN:
     case IOCTL_GET_OCT:
     case IOCTL_GET_HEX:
     case IOCTL_GET_ALL:
         if (!data_ready)
             return -ENODATA;
         if (copy_to_user((struct base_data __user *)arg,
                          &current_data, sizeof(struct base_data)))
             return -EFAULT;
         break;
 
     default:
         return -ENOTTY;
     }
     return 0;
 }
 
 static const struct file_operations fops = {
     .owner          = THIS_MODULE,
     .open           = dev_open,
     .release        = dev_release,
     .read           = dev_read,
     .write          = dev_write,
     .unlocked_ioctl = dev_ioctl,
     .llseek         = default_llseek,
 };
 
 /* ===== Module Init / Exit ===== */
 static int __init bai1_init(void)
 {
     int ret;
 
     ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
     if (ret < 0) {
         pr_err("[%s] Loi alloc_chrdev_region\n", DEVICE_NAME);
         return ret;
     }
 
     cdev_init(&my_cdev, &fops);
     my_cdev.owner = THIS_MODULE;
     ret = cdev_add(&my_cdev, dev_num, 1);
     if (ret < 0) goto err_cdev;
 
     my_class = class_create(CLASS_NAME);
     if (IS_ERR(my_class)) { ret = PTR_ERR(my_class); goto err_class; }
 
     my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
     if (IS_ERR(my_device)) { ret = PTR_ERR(my_device); goto err_device; }
 
     pr_info("[%s] Module nap thanh cong (Major=%d, Minor=%d)\n",
             DEVICE_NAME, MAJOR(dev_num), MINOR(dev_num));
     return 0;
 
 err_device:
     class_destroy(my_class);
 err_class:
     cdev_del(&my_cdev);
 err_cdev:
     unregister_chrdev_region(dev_num, 1);
     return ret;
 }
 
 static void __exit bai1_exit(void)
 {
     device_destroy(my_class, dev_num);
     class_destroy(my_class);
     cdev_del(&my_cdev);
     unregister_chrdev_region(dev_num, 1);
     pr_info("[%s] Module da go\n", DEVICE_NAME);
 }
 
 module_init(bai1_init);
 module_exit(bai1_exit);
 