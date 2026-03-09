/*
 * lab5_bai1_ioctl.h - Dinh nghia IOCTL chung cho driver va userspace Bai 5.1
 */
 #ifndef LAB5_BAI1_IOCTL_H
 #define LAB5_BAI1_IOCTL_H
 
 #include <linux/ioctl.h>
 
 #define DEVICE_NAME  "lab5.1_thu"
 #define DEVICE_PATH  "/dev/lab5.1_thu"
 
 /* Magic number cho IOCTL */
 #define LAB5_1_MAGIC 'L'
 
 /* Cau truc du lieu trao doi giua user va kernel */
 struct base_data {
     long dec_val;          /* Gia tri he 10 */
     char bin_str[128];     /* Chuoi he 2 */
     char oct_str[64];      /* Chuoi he 8 */
     char hex_str[64];      /* Chuoi he 16 */
 };
 
 /* Dinh nghia cac lenh IOCTL */
 #define IOCTL_SET_DEC    _IOW(LAB5_1_MAGIC, 1, long)            /* Ghi so he 10 */
 #define IOCTL_GET_BIN    _IOR(LAB5_1_MAGIC, 2, struct base_data) /* Doc he 2 */
 #define IOCTL_GET_OCT    _IOR(LAB5_1_MAGIC, 3, struct base_data) /* Doc he 8 */
 #define IOCTL_GET_HEX    _IOR(LAB5_1_MAGIC, 4, struct base_data) /* Doc he 16 */
 #define IOCTL_GET_ALL    _IOR(LAB5_1_MAGIC, 5, struct base_data) /* Doc tat ca */
 
 #endif /* LAB5_BAI1_IOCTL_H */
 