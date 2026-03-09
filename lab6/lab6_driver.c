/*
 * Lab 6: Character Driver mat ma
 * Ten driver: lab6_crypto_thu
 * Cap phat device number dong
 *
 * Thuat toan:
 *   1. Ma dich chuyen (Caesar/Shift Cipher)
 *   2. Ma thay the (Substitution Cipher)
 *   3. Ma hoan vi (Permutation/Transposition Cipher)
 */
 #include <linux/module.h>
 #include <linux/kernel.h>
 #include <linux/init.h>
 #include <linux/fs.h>
 #include <linux/cdev.h>
 #include <linux/device.h>
 #include <linux/uaccess.h>
 #include <linux/string.h>
 #include <linux/slab.h>
 
 #include "lab6_ioctl.h"
 
 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("Thu");
 MODULE_DESCRIPTION("Lab6: Character driver mat ma - dich chuyen, thay the, hoan vi");
 
 static dev_t dev_num;
 static struct cdev my_cdev;
 static struct class *my_class;
 static struct device *my_device;
 
 /* Buffer luu xau ro va xau ma */
 static char plaintext[MAX_STR];
 static char ciphertext[MAX_STR];
 static int plain_len = 0;
 static int cipher_len = 0;
 
 /* ============================================================
  * 1. MA DICH CHUYEN (Caesar / Shift Cipher)
  * ============================================================
  * Dich chuyen tung ky tu theo khoa k:
  *   Ma hoa:  C = (P + k) mod 26
  *   Giai ma: P = (C - k + 26) mod 26
  */
 static void shift_encrypt(const char *input, char *output, int key)
 {
     int i;
     int k = ((key % 26) + 26) % 26; /* Dam bao k duong */
 
     for (i = 0; input[i] != '\0'; i++) {
         char c = input[i];
         if (c >= 'A' && c <= 'Z')
             output[i] = 'A' + (c - 'A' + k) % 26;
         else if (c >= 'a' && c <= 'z')
             output[i] = 'a' + (c - 'a' + k) % 26;
         else
             output[i] = c; /* Giu nguyen ky tu khac */
     }
     output[i] = '\0';
 }
 
 static void shift_decrypt(const char *input, char *output, int key)
 {
     int i;
     int k = ((key % 26) + 26) % 26;
 
     for (i = 0; input[i] != '\0'; i++) {
         char c = input[i];
         if (c >= 'A' && c <= 'Z')
             output[i] = 'A' + (c - 'A' - k + 26) % 26;
         else if (c >= 'a' && c <= 'z')
             output[i] = 'a' + (c - 'a' - k + 26) % 26;
         else
             output[i] = c;
     }
     output[i] = '\0';
 }
 
 /* ============================================================
  * 2. MA THAY THE (Substitution Cipher)
  * ============================================================
  * Khoa: bang 26 ky tu thay the tuong ung A-Z
  * VD: khoa = "QWERTYUIOPASDFGHJKLZXCVBNM"
  *     A->Q, B->W, C->E, ...
  */
 static void subst_encrypt(const char *input, char *output, const char *key)
 {
     int i;
 
     for (i = 0; input[i] != '\0'; i++) {
         char c = input[i];
         if (c >= 'A' && c <= 'Z')
             output[i] = key[c - 'A'];
         else if (c >= 'a' && c <= 'z')
             output[i] = key[c - 'a'] - 'A' + 'a';
         else
             output[i] = c;
     }
     output[i] = '\0';
 }
 
 static void subst_decrypt(const char *input, char *output, const char *key)
 {
     int i, j;
     /* Tao bang nghich dao */
     char inv_key[26];
 
     for (j = 0; j < 26; j++) {
         char upper = (key[j] >= 'a' && key[j] <= 'z') ?
                      key[j] - 'a' + 'A' : key[j];
         if (upper >= 'A' && upper <= 'Z')
             inv_key[upper - 'A'] = 'A' + j;
     }
 
     for (i = 0; input[i] != '\0'; i++) {
         char c = input[i];
         if (c >= 'A' && c <= 'Z')
             output[i] = inv_key[c - 'A'];
         else if (c >= 'a' && c <= 'z')
             output[i] = inv_key[c - 'a'] - 'A' + 'a';
         else
             output[i] = c;
     }
     output[i] = '\0';
 }
 
 /* ============================================================
  * 3. MA HOAN VI (Permutation / Transposition Cipher)
  * ============================================================
  * Khoa: mang chi so hoan vi (1-indexed)
  * VD: khoa = {3,1,4,2,5} do dai 5
  * Chia xau thanh cac khoi co do dai = do dai khoa,
  * hoan vi vi tri cac ky tu trong tung khoi.
  */
 static void perm_encrypt(const char *input, char *output,
                          int *key, int key_len)
 {
     int len = strlen(input);
     int i, j;
     int idx = 0;
     int padded_len = ((len + key_len - 1) / key_len) * key_len;
 
     for (i = 0; i < padded_len; i += key_len) {
         for (j = 0; j < key_len; j++) {
             int src = i + key[j] - 1; /* key la 1-indexed */
             if (src < len)
                 output[idx++] = input[src];
             else
                 output[idx++] = '_'; /* Padding */
         }
     }
     output[idx] = '\0';
 }
 
 static void perm_decrypt(const char *input, char *output,
                          int *key, int key_len)
 {
     int len = strlen(input);
     int i, j;
     int idx;
 
     memset(output, 0, MAX_STR);
 
     /* Nghich dao qua trinh hoan vi */
     for (i = 0; i < len; i += key_len) {
         idx = 0;
         for (j = 0; j < key_len && (i + j) < len; j++) {
             int dest = i + key[j] - 1;
             if (dest < MAX_STR - 1)
                 output[dest] = input[i + j];
             idx++;
         }
     }
 
     /* Bo padding cuoi */
     idx = strlen(output);
     while (idx > 0 && output[idx - 1] == '_')
         idx--;
     output[idx] = '\0';
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
 
 /* Read: doc xau ma hien tai */
 static ssize_t dev_read(struct file *filep, char __user *buf,
                         size_t len, loff_t *offset)
 {
     char *out;
     int out_len;
     ssize_t ret;
 
     if (*offset > 0)
         return 0;
 
     out = kmalloc(MAX_STR * 2 + 128, GFP_KERNEL);
     if (!out)
         return -ENOMEM;
 
     out_len = snprintf(out, MAX_STR * 2 + 128,
         "Xau ro : %s\nXau ma : %s\n",
         plain_len > 0 ? plaintext : "(trong)",
         cipher_len > 0 ? ciphertext : "(trong)");
 
     if (out_len > len)
         out_len = len;
 
     if (copy_to_user(buf, out, out_len)) {
         kfree(out);
         return -EFAULT;
     }
 
     ret = out_len;
     *offset += out_len;
     kfree(out);
     return ret;
 }
 
 /* Write: nhap xau ro */
 static ssize_t dev_write(struct file *filep, const char __user *buf,
                          size_t len, loff_t *offset)
 {
     int copy_len = min((int)len, (int)(MAX_STR - 1));
 
     if (copy_from_user(plaintext, buf, copy_len))
         return -EFAULT;
 
     /* Loai bo newline */
     if (copy_len > 0 && plaintext[copy_len - 1] == '\n')
         copy_len--;
     plaintext[copy_len] = '\0';
     plain_len = copy_len;
 
     pr_info("[%s] Nhap xau ro: \"%s\" (%d ky tu)\n",
             DEVICE_NAME, plaintext, plain_len);
     return len;
 }
 
 /* IOCTL: xu ly ma hoa / giai ma */
 static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
 {
     struct crypto_request *req;
     long ret = 0;
 
     switch (cmd) {
     case IOCTL_SET_PLAINTEXT:
         if (copy_from_user(plaintext, (char __user *)arg, MAX_STR))
             return -EFAULT;
         plaintext[MAX_STR - 1] = '\0';
         plain_len = strlen(plaintext);
         pr_info("[%s] Set plaintext: \"%s\"\n", DEVICE_NAME, plaintext);
         break;
 
     case IOCTL_ENCRYPT:
         req = kmalloc(sizeof(*req), GFP_KERNEL);
         if (!req)
             return -ENOMEM;
 
         if (copy_from_user(req, (struct crypto_request __user *)arg, sizeof(*req))) {
             kfree(req);
             return -EFAULT;
         }
 
         req->input[MAX_STR - 1] = '\0';
 
         switch (req->cipher_type) {
         case CIPHER_SHIFT:
             shift_encrypt(req->input, req->output, req->shift_key);
             pr_info("[%s] Ma hoa dich chuyen (k=%d): \"%s\" -> \"%s\"\n",
                     DEVICE_NAME, req->shift_key, req->input, req->output);
             break;
         case CIPHER_SUBST:
             subst_encrypt(req->input, req->output, req->subst_key);
             pr_info("[%s] Ma hoa thay the: \"%s\" -> \"%s\"\n",
                     DEVICE_NAME, req->input, req->output);
             break;
         case CIPHER_PERM:
             if (req->perm_key_len <= 0 || req->perm_key_len > MAX_KEY) {
                 kfree(req);
                 return -EINVAL;
             }
             perm_encrypt(req->input, req->output, req->perm_key, req->perm_key_len);
             pr_info("[%s] Ma hoa hoan vi (len=%d): \"%s\" -> \"%s\"\n",
                     DEVICE_NAME, req->perm_key_len, req->input, req->output);
             break;
         default:
             kfree(req);
             return -EINVAL;
         }
 
         /* Luu ket qua */
         strscpy(plaintext, req->input, MAX_STR);
         plain_len = strlen(plaintext);
         strscpy(ciphertext, req->output, MAX_STR);
         cipher_len = strlen(ciphertext);
 
         if (copy_to_user((struct crypto_request __user *)arg, req, sizeof(*req))) {
             kfree(req);
             return -EFAULT;
         }
         kfree(req);
         break;
 
     case IOCTL_DECRYPT:
         req = kmalloc(sizeof(*req), GFP_KERNEL);
         if (!req)
             return -ENOMEM;
 
         if (copy_from_user(req, (struct crypto_request __user *)arg, sizeof(*req))) {
             kfree(req);
             return -EFAULT;
         }
 
         req->input[MAX_STR - 1] = '\0';
 
         switch (req->cipher_type) {
         case CIPHER_SHIFT:
             shift_decrypt(req->input, req->output, req->shift_key);
             pr_info("[%s] Giai ma dich chuyen (k=%d): \"%s\" -> \"%s\"\n",
                     DEVICE_NAME, req->shift_key, req->input, req->output);
             break;
         case CIPHER_SUBST:
             subst_decrypt(req->input, req->output, req->subst_key);
             pr_info("[%s] Giai ma thay the: \"%s\" -> \"%s\"\n",
                     DEVICE_NAME, req->input, req->output);
             break;
         case CIPHER_PERM:
             if (req->perm_key_len <= 0 || req->perm_key_len > MAX_KEY) {
                 kfree(req);
                 return -EINVAL;
             }
             perm_decrypt(req->input, req->output, req->perm_key, req->perm_key_len);
             pr_info("[%s] Giai ma hoan vi (len=%d): \"%s\" -> \"%s\"\n",
                     DEVICE_NAME, req->perm_key_len, req->input, req->output);
             break;
         default:
             kfree(req);
             return -EINVAL;
         }
 
         /* Luu ket qua */
         strscpy(plaintext, req->output, MAX_STR);
         plain_len = strlen(plaintext);
 
         if (copy_to_user((struct crypto_request __user *)arg, req, sizeof(*req))) {
             kfree(req);
             return -EFAULT;
         }
         kfree(req);
         break;
 
     case IOCTL_GET_CIPHER:
         if (cipher_len == 0)
             return -ENODATA;
         if (copy_to_user((char __user *)arg, ciphertext, MAX_STR))
             return -EFAULT;
         break;
 
     case IOCTL_GET_PLAIN:
         if (plain_len == 0)
             return -ENODATA;
         if (copy_to_user((char __user *)arg, plaintext, MAX_STR))
             return -EFAULT;
         break;
 
     default:
         return -ENOTTY;
     }
     return ret;
 }
 
 static const struct file_operations fops = {
     .owner          = THIS_MODULE,
     .open           = dev_open,
     .release        = dev_release,
     .read           = dev_read,
     .write          = dev_write,
     .unlocked_ioctl = dev_ioctl,
 };
 
 /* ===== Module Init / Exit ===== */
 static int __init lab6_init(void)
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
 
     pr_info("[%s] Module nap thanh cong (Major=%d)\n",
             DEVICE_NAME, MAJOR(dev_num));
     pr_info("[%s] Thuat toan: Dich chuyen, Thay the, Hoan vi\n", DEVICE_NAME);
     return 0;
 
 err_device:
     class_destroy(my_class);
 err_class:
     cdev_del(&my_cdev);
 err_cdev:
     unregister_chrdev_region(dev_num, 1);
     return ret;
 }
 
 static void __exit lab6_exit(void)
 {
     device_destroy(my_class, dev_num);
     class_destroy(my_class);
     cdev_del(&my_cdev);
     unregister_chrdev_region(dev_num, 1);
     pr_info("[%s] Module da go\n", DEVICE_NAME);
 }
 
 module_init(lab6_init);
 module_exit(lab6_exit);
 