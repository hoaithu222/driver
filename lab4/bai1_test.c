/*
 * Bai 1: Chuong trinh userspace su dung driver lab4.1_thu
 *
 * Chuong trinh thuc hien:
 *   1. Mo device /dev/lab4.1_thu
 *   2. Doc du lieu tu driver
 *   3. Ghi du lieu vao driver
 *   4. Doc lai du lieu da ghi
 *   5. Dong device
 */
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <errno.h>
 
 #define DEVICE_PATH "/dev/lab4.1_thu"
 #define BUF_SIZE    1024
 
 int main(void)
 {
     int fd;
     char buf[BUF_SIZE];
     ssize_t ret;
 
     printf("=== Chuong trinh test driver lab4.1_thu ===\n\n");
 
     /* 1. Mo device */
     printf("[1] Mo device: %s\n", DEVICE_PATH);
     fd = open(DEVICE_PATH, O_RDWR);
     if (fd < 0) {
         perror("    Loi mo device");
         printf("    -> Dam bao da nap module: sudo insmod bai1_driver.ko\n");
         return EXIT_FAILURE;
     }
     printf("    -> Mo thanh cong (fd = %d)\n\n", fd);
 
     /* 2. Doc du lieu mac dinh tu driver */
     printf("[2] Doc du lieu tu driver:\n");
     memset(buf, 0, BUF_SIZE);
     ret = read(fd, buf, BUF_SIZE);
     if (ret < 0) {
         perror("    Loi doc");
     } else {
         printf("    Doc duoc %zd bytes: \"%s\"\n\n", ret, buf);
     }
 
     /* 3. Ghi du lieu vao driver */
     {
         const char *msg = "Xin chao tu userspace! - Thu";
         printf("[3] Ghi du lieu vao driver:\n");
         printf("    Du lieu ghi: \"%s\"\n", msg);
         ret = write(fd, msg, strlen(msg));
         if (ret < 0) {
             perror("    Loi ghi");
         } else {
             printf("    Ghi thanh cong %zd bytes\n\n", ret);
         }
     }
 
     /* 4. Doc lai du lieu vua ghi */
     printf("[4] Doc lai du lieu vua ghi:\n");
     /* Reset file offset */
     lseek(fd, 0, SEEK_SET);
     memset(buf, 0, BUF_SIZE);
     ret = read(fd, buf, BUF_SIZE);
     if (ret < 0) {
         perror("    Loi doc");
     } else {
         printf("    Doc duoc %zd bytes: \"%s\"\n\n", ret, buf);
     }
 
     /* 5. Dong device */
     printf("[5] Dong device\n");
     close(fd);
     printf("    -> Dong thanh cong\n\n");
 
     printf("=== Ket thuc test ===\n");
     return EXIT_SUCCESS;
 }
 