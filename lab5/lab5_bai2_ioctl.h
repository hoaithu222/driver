/*
 * lab5_bai2_ioctl.h - Dinh nghia IOCTL chung cho driver va userspace Bai 5.2
 */
 #ifndef LAB5_BAI2_IOCTL_H
 #define LAB5_BAI2_IOCTL_H
 
 #include <linux/ioctl.h>
 
 #define DEVICE_NAME2 "lab5.2_thu"
 #define DEVICE_PATH2 "/dev/lab5.2_thu"
 
 /* Magic number */
 #define LAB5_2_MAGIC 'T'
 
 /* Cau truc thoi gian tuyet doi (micro giay) */
 struct time_micro {
     long tv_sec;           /* Giay */
     long tv_usec;          /* Micro giay */
     int year, month, day;  /* Ngay thang nam */
     int hour, min, sec;    /* Gio phut giay */
 };
 
 /* Cau truc thoi gian tuyet doi (nano giay) */
 struct time_nano {
     long tv_sec;           /* Giay */
     long tv_nsec;          /* Nano giay */
     int year, month, day;
     int hour, min, sec;
 };
 
 /* Cau truc thoi gian tuong doi */
 struct time_relative {
     unsigned long uptime_sec;   /* Thoi gian he thong chay (giay) */
     unsigned long uptime_nsec;  /* Phan nano giay */
     unsigned long days;
     unsigned long hours;
     unsigned long minutes;
     unsigned long seconds;
 };
 
 /* Dinh nghia cac lenh IOCTL */
 #define IOCTL_GET_TIME_MICRO    _IOR(LAB5_2_MAGIC, 1, struct time_micro)
 #define IOCTL_GET_TIME_NANO     _IOR(LAB5_2_MAGIC, 2, struct time_nano)
 #define IOCTL_GET_TIME_RELATIVE _IOR(LAB5_2_MAGIC, 3, struct time_relative)
 
 #endif /* LAB5_BAI2_IOCTL_H */
 