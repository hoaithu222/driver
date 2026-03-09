/*
 * Bai 5.2: Character Driver - Lay thoi gian trong nhan
 * Ten driver: lab5.2_thu
 * Cap phat device number dong (alloc_chrdev_region)
 *
 * Chuc nang:
 *   - Lay thoi gian tuyet doi chinh xac den micro giay
 *   - Lay thoi gian tuyet doi chinh xac den nano giay
 *   - Lay thoi gian tuong doi (uptime)
 */
 #include <linux/module.h>
 #include <linux/kernel.h>
 #include <linux/init.h>
 #include <linux/fs.h>
 #include <linux/cdev.h>
 #include <linux/device.h>
 #include <linux/uaccess.h>
 #include <linux/time.h>
 #include <linux/timekeeping.h>
 #include <linux/math64.h>
 
 #include "lab5_bai2_ioctl.h"
 
 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("Thu");
 MODULE_DESCRIPTION("Lab5 Bai2: Character driver lay thoi gian trong nhan");
 
 #define CLASS_NAME "lab5_2_class"
 
 static dev_t dev_num;
 static struct cdev my_cdev;
 static struct class *my_class;
 static struct device *my_device;
 
 /* ===== Ham tien ich: chuyen timestamp sang ngay/gio ===== */
 
 /* Tinh so ngay tu epoch den nam, tra ve ngay con lai */
 static void epoch_to_date(long total_sec, int *year, int *month, int *day,
                           int *hour, int *min, int *sec)
 {
     /* Su dung time64_to_tm */
     struct tm result;
 
     time64_to_tm((time64_t)total_sec, 0, &result);
 
     *year  = (int)(result.tm_year + 1900);
     *month = result.tm_mon + 1;
     *day   = result.tm_mday;
     *hour  = result.tm_hour;
     *min   = result.tm_min;
     *sec   = result.tm_sec;
 }
 
 /* ===== File Operations ===== */
 
 static int dev_open(struct inode *inodep, struct file *filep)
 {
     pr_info("[%s] Device opened\n", DEVICE_NAME2);
     return 0;
 }
 
 static int dev_release(struct inode *inodep, struct file *filep)
 {
     pr_info("[%s] Device closed\n", DEVICE_NAME2);
     return 0;
 }
 
 static ssize_t dev_read(struct file *filep, char __user *buf,
                         size_t len, loff_t *offset)
 {
     char out[256];
     int out_len;
     struct timespec64 ts;
 
     if (*offset > 0)
         return 0;
 
     ktime_get_real_ts64(&ts);
 
     out_len = snprintf(out, sizeof(out),
         "Thoi gian hien tai: %lld.%09ld giay tu Epoch\n",
         (long long)ts.tv_sec, ts.tv_nsec);
 
     if (out_len > len)
         out_len = len;
 
     if (copy_to_user(buf, out, out_len))
         return -EFAULT;
 
     *offset += out_len;
     return out_len;
 }
 
 /* IOCTL */
 static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
 {
     struct timespec64 ts;
 
     switch (cmd) {
     case IOCTL_GET_TIME_MICRO:
     {
         struct time_micro tm_data;
 
         ktime_get_real_ts64(&ts);
 
         tm_data.tv_sec = (long)ts.tv_sec;
         tm_data.tv_usec = ts.tv_nsec / 1000;  /* nano -> micro */
 
         epoch_to_date(ts.tv_sec, &tm_data.year, &tm_data.month,
                       &tm_data.day, &tm_data.hour, &tm_data.min, &tm_data.sec);
 
         pr_info("[%s] Thoi gian micro: %04d-%02d-%02d %02d:%02d:%02d.%06ld\n",
                 DEVICE_NAME2,
                 tm_data.year, tm_data.month, tm_data.day,
                 tm_data.hour, tm_data.min, tm_data.sec,
                 tm_data.tv_usec);
 
         if (copy_to_user((struct time_micro __user *)arg,
                          &tm_data, sizeof(tm_data)))
             return -EFAULT;
         break;
     }
 
     case IOCTL_GET_TIME_NANO:
     {
         struct time_nano tn_data;
 
         ktime_get_real_ts64(&ts);
 
         tn_data.tv_sec = (long)ts.tv_sec;
         tn_data.tv_nsec = ts.tv_nsec;
 
         epoch_to_date(ts.tv_sec, &tn_data.year, &tn_data.month,
                       &tn_data.day, &tn_data.hour, &tn_data.min, &tn_data.sec);
 
         pr_info("[%s] Thoi gian nano: %04d-%02d-%02d %02d:%02d:%02d.%09ld\n",
                 DEVICE_NAME2,
                 tn_data.year, tn_data.month, tn_data.day,
                 tn_data.hour, tn_data.min, tn_data.sec,
                 tn_data.tv_nsec);
 
         if (copy_to_user((struct time_nano __user *)arg,
                          &tn_data, sizeof(tn_data)))
             return -EFAULT;
         break;
     }
 
     case IOCTL_GET_TIME_RELATIVE:
     {
         struct time_relative tr_data;
         struct timespec64 uptime;
         unsigned long total_sec;
 
         ktime_get_boottime_ts64(&uptime);
 
         total_sec = (unsigned long)uptime.tv_sec;
         tr_data.uptime_sec = total_sec;
         tr_data.uptime_nsec = (unsigned long)uptime.tv_nsec;
 
         tr_data.days    = total_sec / 86400;
         tr_data.hours   = (total_sec % 86400) / 3600;
         tr_data.minutes = (total_sec % 3600) / 60;
         tr_data.seconds = total_sec % 60;
 
         pr_info("[%s] Uptime: %lu ngay %lu gio %lu phut %lu giay\n",
                 DEVICE_NAME2,
                 tr_data.days, tr_data.hours,
                 tr_data.minutes, tr_data.seconds);
 
         if (copy_to_user((struct time_relative __user *)arg,
                          &tr_data, sizeof(tr_data)))
             return -EFAULT;
         break;
     }
 
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
     .unlocked_ioctl = dev_ioctl,
 };
 
 /* ===== Module Init / Exit ===== */
 static int __init bai2_init(void)
 {
     int ret;
 
     ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME2);
     if (ret < 0) {
         pr_err("[%s] Loi alloc_chrdev_region\n", DEVICE_NAME2);
         return ret;
     }
 
     cdev_init(&my_cdev, &fops);
     my_cdev.owner = THIS_MODULE;
     ret = cdev_add(&my_cdev, dev_num, 1);
     if (ret < 0) goto err_cdev;
 
     my_class = class_create(CLASS_NAME);
     if (IS_ERR(my_class)) { ret = PTR_ERR(my_class); goto err_class; }
 
     my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME2);
     if (IS_ERR(my_device)) { ret = PTR_ERR(my_device); goto err_device; }
 
     pr_info("[%s] Module nap thanh cong (Major=%d, Minor=%d)\n",
             DEVICE_NAME2, MAJOR(dev_num), MINOR(dev_num));
     return 0;
 
 err_device:
     class_destroy(my_class);
 err_class:
     cdev_del(&my_cdev);
 err_cdev:
     unregister_chrdev_region(dev_num, 1);
     return ret;
 }
 
 static void __exit bai2_exit(void)
 {
     device_destroy(my_class, dev_num);
     class_destroy(my_class);
     cdev_del(&my_cdev);
     unregister_chrdev_region(dev_num, 1);
     pr_info("[%s] Module da go\n", DEVICE_NAME2);
 }
 
 module_init(bai2_init);
 module_exit(bai2_exit);
 