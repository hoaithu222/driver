/*
 * Bai 5.2: Chuong trinh userspace - Menu lay thoi gian
 *
 * Menu chuc nang:
 *   1. Lay thoi gian tuyet doi, chinh xac den micro giay
 *   2. Lay thoi gian tuyet doi, chinh xac den nano giay
 *   3. Lay thoi gian tuong doi
 *   4. Ket thuc ctr
 */
 #include <stdio.h>
 #include <stdlib.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <sys/ioctl.h>
 #include <errno.h>
 
 #include "lab5_bai2_ioctl.h"
 
 static int fd = -1;
 
 static void print_menu(void)
 {
     printf("\n");
     printf("========================================\n");
     printf("   Lab 5.2 - Lay thoi gian trong nhan\n");
     printf("   Driver: %s\n", DEVICE_NAME2);
     printf("========================================\n");
     printf("  1. Lay thoi gian tuyet doi (micro giay)\n");
     printf("  2. Lay thoi gian tuyet doi (nano giay)\n");
     printf("  3. Lay thoi gian tuong doi (uptime)\n");
     printf("  4. Ket thuc ctr\n");
     printf("========================================\n");
     printf("  Chon: ");
 }
 
 static int ensure_open(void)
 {
     if (fd >= 0)
         return 0;
 
     fd = open(DEVICE_PATH2, O_RDWR);
     if (fd < 0) {
         perror("[!] Loi mo thiet bi");
         printf("    Dam bao da nap module: sudo insmod bai2_driver.ko\n");
         return -1;
     }
     printf("[+] Mo thiet bi: %s (fd = %d)\n", DEVICE_PATH2, fd);
     return 0;
 }
 
 static void menu_time_micro(void)
 {
     struct time_micro data;
     int ret;
 
     if (ensure_open() < 0) return;
 
     ret = ioctl(fd, IOCTL_GET_TIME_MICRO, &data);
     if (ret < 0) {
         perror("[!] Loi ioctl GET_TIME_MICRO");
         return;
     }
 
     printf("\n  +--- Thoi gian tuyet doi (micro giay) ---+\n");
     printf("  | Ngay: %04d-%02d-%02d\n", data.year, data.month, data.day);
     printf("  | Gio : %02d:%02d:%02d.%06ld\n",
            data.hour, data.min, data.sec, data.tv_usec);
     printf("  | Epoch: %ld.%06ld giay\n", data.tv_sec, data.tv_usec);
     printf("  +------------------------------------------+\n");
 }
 
 static void menu_time_nano(void)
 {
     struct time_nano data;
     int ret;
 
     if (ensure_open() < 0) return;
 
     ret = ioctl(fd, IOCTL_GET_TIME_NANO, &data);
     if (ret < 0) {
         perror("[!] Loi ioctl GET_TIME_NANO");
         return;
     }
 
     printf("\n  +--- Thoi gian tuyet doi (nano giay) ---+\n");
     printf("  | Ngay: %04d-%02d-%02d\n", data.year, data.month, data.day);
     printf("  | Gio : %02d:%02d:%02d.%09ld\n",
            data.hour, data.min, data.sec, data.tv_nsec);
     printf("  | Epoch: %ld.%09ld giay\n", data.tv_sec, data.tv_nsec);
     printf("  +-----------------------------------------+\n");
 }
 
 static void menu_time_relative(void)
 {
     struct time_relative data;
     int ret;
 
     if (ensure_open() < 0) return;
 
     ret = ioctl(fd, IOCTL_GET_TIME_RELATIVE, &data);
     if (ret < 0) {
         perror("[!] Loi ioctl GET_TIME_RELATIVE");
         return;
     }
 
     printf("\n  +--- Thoi gian tuong doi (uptime) ---+\n");
     printf("  | He thong da chay:\n");
     printf("  |   %lu ngay, %lu gio, %lu phut, %lu giay\n",
            data.days, data.hours, data.minutes, data.seconds);
     printf("  | Tong: %lu.%09lu giay\n", data.uptime_sec, data.uptime_nsec);
     printf("  +--------------------------------------+\n");
 }
 
 int main(void)
 {
     int choice;
 
     printf("=== Chuong trinh test driver lab5.2_thu ===\n");
 
     while (1) {
         print_menu();
         if (scanf("%d", &choice) != 1) {
             printf("[!] Nhap khong hop le!\n");
             while (getchar() != '\n');
             continue;
         }
 
         switch (choice) {
         case 1:
             menu_time_micro();
             break;
         case 2:
             menu_time_nano();
             break;
         case 3:
             menu_time_relative();
             break;
         case 4:
             if (fd >= 0) {
                 close(fd);
                 printf("[+] Da dong thiet bi.\n");
             }
             printf("[+] Ket thuc chuong trinh.\n");
             return 0;
         default:
             printf("[!] Lua chon khong hop le! Chon 1-4.\n");
             break;
         }
     }
 
     return 0;
 }
 