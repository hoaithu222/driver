/*
 * Bai 5.1: Chuong trinh userspace - Menu chuyen doi he so
 *
 * Menu chuc nang:
 *   1. Open thiet bi
 *   2. Nhap so he 10 va ghi ra thiet bi so he 10, 2, 8, 16
 *   3. Doc so he 2
 *   4. Doc so he 8
 *   5. Doc so he 16
 *   6. Dong thiet bi va ket thuc ctr
 */
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <sys/ioctl.h>
 #include <errno.h>
 
 #include "lab5_bai1_ioctl.h"
 
 static int fd = -1;
 
 static void print_menu(void)
 {
     printf("\n");
     printf("========================================\n");
     printf("   Lab 5.1 - Chuyen doi he so\n");
     printf("   Driver: %s\n", DEVICE_NAME);
     printf("========================================\n");
     printf("  1. Open thiet bi\n");
     printf("  2. Nhap so he 10 va ghi ra thiet bi\n");
     printf("  3. Doc so he 2\n");
     printf("  4. Doc so he 8\n");
     printf("  5. Doc so he 16\n");
     printf("  6. Dong thiet bi va ket thuc\n");
     printf("========================================\n");
     printf("  Chon: ");
 }
 
 static void menu_open(void)
 {
     if (fd >= 0) {
         printf("[!] Thiet bi da duoc mo roi (fd = %d)\n", fd);
         return;
     }
 
     fd = open(DEVICE_PATH, O_RDWR);
     if (fd < 0) {
         perror("[!] Loi mo thiet bi");
         printf("    Dam bao da nap module: sudo insmod bai1_driver.ko\n");
         return;
     }
     printf("[+] Mo thiet bi thanh cong: %s (fd = %d)\n", DEVICE_PATH, fd);
 }
 
 static void menu_write_dec(void)
 {
     long val;
     int ret;
 
     if (fd < 0) {
         printf("[!] Chua mo thiet bi! Chon 1 truoc.\n");
         return;
     }
 
     printf("  Nhap so he 10: ");
     if (scanf("%ld", &val) != 1) {
         printf("[!] Gia tri khong hop le!\n");
         while (getchar() != '\n');
         return;
     }
 
     /* Gui so he 10 qua ioctl */
     ret = ioctl(fd, IOCTL_SET_DEC, &val);
     if (ret < 0) {
         perror("[!] Loi ioctl SET_DEC");
         return;
     }
 
     /* Doc tat ca ket qua */
     struct base_data data;
     ret = ioctl(fd, IOCTL_GET_ALL, &data);
     if (ret < 0) {
         perror("[!] Loi ioctl GET_ALL");
         return;
     }
 
     printf("\n  +--- Ket qua chuyen doi ---+\n");
     printf("  | He 10: %ld\n", data.dec_val);
     printf("  | He  2: %s\n", data.bin_str);
     printf("  | He  8: %s\n", data.oct_str);
     printf("  | He 16: %s\n", data.hex_str);
     printf("  +----------------------------+\n");
 }
 
 static void menu_read_bin(void)
 {
     struct base_data data;
     int ret;
 
     if (fd < 0) {
         printf("[!] Chua mo thiet bi! Chon 1 truoc.\n");
         return;
     }
 
     ret = ioctl(fd, IOCTL_GET_BIN, &data);
     if (ret < 0) {
         if (errno == ENODATA)
             printf("[!] Chua co du lieu! Nhap so he 10 truoc (chon 2).\n");
         else
             perror("[!] Loi doc so he 2");
         return;
     }
 
     printf("  So he 10: %ld\n", data.dec_val);
     printf("  So he  2: %s\n", data.bin_str);
 }
 
 static void menu_read_oct(void)
 {
     struct base_data data;
     int ret;
 
     if (fd < 0) {
         printf("[!] Chua mo thiet bi! Chon 1 truoc.\n");
         return;
     }
 
     ret = ioctl(fd, IOCTL_GET_OCT, &data);
     if (ret < 0) {
         if (errno == ENODATA)
             printf("[!] Chua co du lieu! Nhap so he 10 truoc (chon 2).\n");
         else
             perror("[!] Loi doc so he 8");
         return;
     }
 
     printf("  So he 10: %ld\n", data.dec_val);
     printf("  So he  8: %s\n", data.oct_str);
 }
 
 static void menu_read_hex(void)
 {
     struct base_data data;
     int ret;
 
     if (fd < 0) {
         printf("[!] Chua mo thiet bi! Chon 1 truoc.\n");
         return;
     }
 
     ret = ioctl(fd, IOCTL_GET_HEX, &data);
     if (ret < 0) {
         if (errno == ENODATA)
             printf("[!] Chua co du lieu! Nhap so he 10 truoc (chon 2).\n");
         else
             perror("[!] Loi doc so he 16");
         return;
     }
 
     printf("  So he 10: %ld\n", data.dec_val);
     printf("  So he 16: %s\n", data.hex_str);
 }
 
 static void menu_close(void)
 {
     if (fd >= 0) {
         close(fd);
         printf("[+] Da dong thiet bi.\n");
         fd = -1;
     }
     printf("[+] Ket thuc chuong trinh.\n");
 }
 
 int main(void)
 {
     int choice;
 
     printf("=== Chuong trinh test driver lab5.1_thu ===\n");
 
     while (1) {
         print_menu();
         if (scanf("%d", &choice) != 1) {
             printf("[!] Nhap khong hop le!\n");
             while (getchar() != '\n');
             continue;
         }
 
         switch (choice) {
         case 1:
             menu_open();
             break;
         case 2:
             menu_write_dec();
             break;
         case 3:
             menu_read_bin();
             break;
         case 4:
             menu_read_oct();
             break;
         case 5:
             menu_read_hex();
             break;
         case 6:
             menu_close();
             return 0;
         default:
             printf("[!] Lua chon khong hop le! Chon 1-6.\n");
             break;
         }
     }
 
     return 0;
 }
 