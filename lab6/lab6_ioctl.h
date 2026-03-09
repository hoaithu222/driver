/*
 * lab6_ioctl.h - Header IOCTL chung cho driver va userspace Lab 6
 * Driver mat ma: dich chuyen, hoan vi, thay the
 */
 #ifndef LAB6_IOCTL_H
 #define LAB6_IOCTL_H
 
 #include <linux/ioctl.h>
 
 #define DEVICE_NAME  "lab6_crypto_thu"
 #define DEVICE_PATH  "/dev/lab6_crypto_thu"
 #define CLASS_NAME   "lab6_crypto_class"
 
 #define LAB6_MAGIC   'C'
 #define MAX_STR      512
 #define MAX_KEY      128
 
 /* Loai ma hoa */
 #define CIPHER_SHIFT   1   /* Ma dich chuyen (Caesar) */
 #define CIPHER_SUBST   2   /* Ma thay the */
 #define CIPHER_PERM    3   /* Ma hoan vi */
 
 /* Cau truc du lieu trao doi */
 struct crypto_request {
     int cipher_type;            /* Loai ma: CIPHER_SHIFT / SUBST / PERM */
     int direction;              /* 0 = ma hoa, 1 = giai ma */
     int shift_key;              /* Khoa dich chuyen (cho CIPHER_SHIFT) */
     char subst_key[26];         /* Bang thay the 26 ky tu (cho CIPHER_SUBST) */
     int perm_key[MAX_KEY];      /* Mang chi so hoan vi (cho CIPHER_PERM) */
     int perm_key_len;           /* Do dai khoa hoan vi */
     char input[MAX_STR];        /* Xau dau vao */
     char output[MAX_STR];       /* Xau dau ra */
 };
 
 /* Lenh IOCTL */
 #define IOCTL_SET_PLAINTEXT  _IOW(LAB6_MAGIC, 1, char[MAX_STR])    /* Nhap xau ro */
 #define IOCTL_ENCRYPT        _IOWR(LAB6_MAGIC, 2, struct crypto_request) /* Ma hoa */
 #define IOCTL_DECRYPT        _IOWR(LAB6_MAGIC, 3, struct crypto_request) /* Giai ma */
 #define IOCTL_GET_CIPHER     _IOR(LAB6_MAGIC, 4, char[MAX_STR])    /* Doc xau ma */
 #define IOCTL_GET_PLAIN      _IOR(LAB6_MAGIC, 5, char[MAX_STR])    /* Doc xau ro */
 
 #endif /* LAB6_IOCTL_H */
 