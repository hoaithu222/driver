/*
 * Lab 6: Chuong trinh userspace test driver mat ma
 * Menu 7 chuc nang:
 *   1. Nhap vao 1 xau
 *   2. Ma hoa dich chuyen
 *   3. Ma hoa thay the
 *   4. Ma hoa hoan vi
 *   5. Giai ma dich chuyen
 *   6. Giai ma thay the
 *   7. Giai ma hoan vi
 */
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <sys/ioctl.h>
 #include <ctype.h>
 
 #include "lab6_ioctl.h"
 
 static char current_string[MAX_STR] = "";
 static char last_cipher[MAX_STR] = "";
 
 void print_menu(void)
 {
     printf("\n");
     printf("======================================\n");
     printf("  LAB 6 - MAT MA (Crypto Driver)\n");
     printf("  Khoa CNTT - HVKTMM\n");
     printf("  Sinh vien: Thu\n");
     printf("======================================\n");
     printf("  1. Nhap vao 1 xau\n");
     printf("  2. Ma hoa dich chuyen\n");
     printf("  3. Ma hoa thay the\n");
     printf("  4. Ma hoa hoan vi\n");
     printf("  5. Giai ma dich chuyen\n");
     printf("  6. Giai ma thay the\n");
     printf("  7. Giai ma hoan vi\n");
     printf("  0. Thoat\n");
     printf("======================================\n");
     printf("  Xau hien tai: \"%s\"\n", current_string);
     if (strlen(last_cipher) > 0)
         printf("  Xau ma cuoi : \"%s\"\n", last_cipher);
     printf("======================================\n");
     printf("Lua chon: ");
 }
 
 /* Nhap xau an toan */
 void read_str(const char *prompt, char *buf, int maxlen)
 {
     printf("%s", prompt);
     if (fgets(buf, maxlen, stdin) == NULL) {
         buf[0] = '\0';
         return;
     }
     int len = strlen(buf);
     if (len > 0 && buf[len - 1] == '\n')
         buf[len - 1] = '\0';
 }
 
 /* Kiem tra khoa thay the hop le (26 ky tu A-Z, khong lap) */
 int validate_subst_key(const char *key)
 {
     int i, used[26] = {0};
 
     if (strlen(key) != 26) {
         printf("[LOI] Khoa thay the phai co dung 26 ky tu!\n");
         return 0;
     }
     for (i = 0; i < 26; i++) {
         char c = toupper(key[i]);
         if (c < 'A' || c > 'Z') {
             printf("[LOI] Khoa chi chua ky tu A-Z!\n");
             return 0;
         }
         if (used[c - 'A']) {
             printf("[LOI] Ky tu '%c' bi lap trong khoa!\n", c);
             return 0;
         }
         used[c - 'A'] = 1;
     }
     return 1;
 }
 
 /* ===== Chuc nang 1: Nhap xau ===== */
 void func_input_string(int fd __attribute__((unused)))
 {
     char buf[MAX_STR];
 
     read_str("Nhap xau: ", buf, MAX_STR);
     strncpy(current_string, buf, MAX_STR - 1);
     current_string[MAX_STR - 1] = '\0';
     printf("Da luu xau: \"%s\"\n", current_string);
 }
 
 /* ===== Chuc nang 2: Ma hoa dich chuyen ===== */
 void func_encrypt_shift(int fd)
 {
     struct crypto_request req;
     int shift;
 
     if (strlen(current_string) == 0) {
         printf("[LOI] Chua nhap xau! Chon 1 de nhap truoc.\n");
         return;
     }
 
     printf("Nhap khoa dich chuyen (so nguyen): ");
     scanf("%d", &shift);
     getchar(); /* Bo newline */
 
     memset(&req, 0, sizeof(req));
     req.cipher_type = CIPHER_SHIFT;
     req.direction = 0; /* encrypt */
     req.shift_key = shift;
     strncpy(req.input, current_string, MAX_STR - 1);
 
     if (ioctl(fd, IOCTL_ENCRYPT, &req) < 0) {
         perror("[LOI] IOCTL_ENCRYPT shift");
         return;
     }
 
     strncpy(last_cipher, req.output, MAX_STR - 1);
     printf("\n  Xau goc     : \"%s\"\n", req.input);
     printf("  Khoa        : %d\n", shift);
     printf("  Xau ma hoa  : \"%s\"\n", req.output);
 }
 
 /* ===== Chuc nang 3: Ma hoa thay the ===== */
 void func_encrypt_subst(int fd)
 {
     struct crypto_request req;
     char key_buf[128];
     int i;
 
     if (strlen(current_string) == 0) {
         printf("[LOI] Chua nhap xau! Chon 1 de nhap truoc.\n");
         return;
     }
 
     printf("Nhap khoa thay the (26 ky tu A-Z, VD: QWERTYUIOPASDFGHJKLZXCVBNM): ");
     read_str("", key_buf, sizeof(key_buf));
 
     if (!validate_subst_key(key_buf))
         return;
 
     memset(&req, 0, sizeof(req));
     req.cipher_type = CIPHER_SUBST;
     req.direction = 0;
     strncpy(req.input, current_string, MAX_STR - 1);
     for (i = 0; i < 26; i++)
         req.subst_key[i] = toupper(key_buf[i]);
 
     if (ioctl(fd, IOCTL_ENCRYPT, &req) < 0) {
         perror("[LOI] IOCTL_ENCRYPT subst");
         return;
     }
 
     strncpy(last_cipher, req.output, MAX_STR - 1);
     printf("\n  Xau goc     : \"%s\"\n", req.input);
     printf("  Khoa        : %.26s\n", req.subst_key);
     printf("  Xau ma hoa  : \"%s\"\n", req.output);
 }
 
 /* ===== Chuc nang 4: Ma hoa hoan vi ===== */
 void func_encrypt_perm(int fd)
 {
     struct crypto_request req;
     int n, i;
 
     if (strlen(current_string) == 0) {
         printf("[LOI] Chua nhap xau! Chon 1 de nhap truoc.\n");
         return;
     }
 
     printf("Nhap do dai khoa hoan vi: ");
     scanf("%d", &n);
     getchar();
 
     if (n <= 0 || n > MAX_KEY) {
         printf("[LOI] Do dai khoa khong hop le (1-%d)!\n", MAX_KEY);
         return;
     }
 
     memset(&req, 0, sizeof(req));
     req.cipher_type = CIPHER_PERM;
     req.direction = 0;
     req.perm_key_len = n;
     strncpy(req.input, current_string, MAX_STR - 1);
 
     printf("Nhap %d chi so hoan vi (1-indexed, cach nhau boi dau cach):\n", n);
     printf("VD voi n=5: 3 1 4 2 5\n");
     for (i = 0; i < n; i++) {
         scanf("%d", &req.perm_key[i]);
         if (req.perm_key[i] < 1 || req.perm_key[i] > n) {
             printf("[LOI] Chi so %d khong hop le (phai tu 1 den %d)!\n",
                    req.perm_key[i], n);
             getchar();
             return;
         }
     }
     getchar();
 
     if (ioctl(fd, IOCTL_ENCRYPT, &req) < 0) {
         perror("[LOI] IOCTL_ENCRYPT perm");
         return;
     }
 
     strncpy(last_cipher, req.output, MAX_STR - 1);
     printf("\n  Xau goc     : \"%s\"\n", req.input);
     printf("  Khoa        : [");
     for (i = 0; i < n; i++)
         printf("%d%s", req.perm_key[i], i < n - 1 ? " " : "");
     printf("]\n");
     printf("  Xau ma hoa  : \"%s\"\n", req.output);
 }
 
 /* ===== Chuc nang 5: Giai ma dich chuyen ===== */
 void func_decrypt_shift(int fd)
 {
     struct crypto_request req;
     char input_buf[MAX_STR];
     int shift;
 
     printf("Nhap xau can giai ma (Enter de dung xau ma cuoi): ");
     read_str("", input_buf, MAX_STR);
 
     if (strlen(input_buf) == 0) {
         if (strlen(last_cipher) == 0) {
             printf("[LOI] Chua co xau ma nao!\n");
             return;
         }
         strncpy(input_buf, last_cipher, MAX_STR - 1);
         printf("  Dung xau ma cuoi: \"%s\"\n", input_buf);
     }
 
     printf("Nhap khoa dich chuyen (so nguyen): ");
     scanf("%d", &shift);
     getchar();
 
     memset(&req, 0, sizeof(req));
     req.cipher_type = CIPHER_SHIFT;
     req.direction = 1; /* decrypt */
     req.shift_key = shift;
     strncpy(req.input, input_buf, MAX_STR - 1);
 
     if (ioctl(fd, IOCTL_DECRYPT, &req) < 0) {
         perror("[LOI] IOCTL_DECRYPT shift");
         return;
     }
 
     strncpy(current_string, req.output, MAX_STR - 1);
     printf("\n  Xau ma      : \"%s\"\n", req.input);
     printf("  Khoa        : %d\n", shift);
     printf("  Xau giai ma : \"%s\"\n", req.output);
 }
 
 /* ===== Chuc nang 6: Giai ma thay the ===== */
 void func_decrypt_subst(int fd)
 {
     struct crypto_request req;
     char input_buf[MAX_STR];
     char key_buf[128];
     int i;
 
     printf("Nhap xau can giai ma (Enter de dung xau ma cuoi): ");
     read_str("", input_buf, MAX_STR);
 
     if (strlen(input_buf) == 0) {
         if (strlen(last_cipher) == 0) {
             printf("[LOI] Chua co xau ma nao!\n");
             return;
         }
         strncpy(input_buf, last_cipher, MAX_STR - 1);
         printf("  Dung xau ma cuoi: \"%s\"\n", input_buf);
     }
 
     printf("Nhap khoa thay the (26 ky tu A-Z): ");
     read_str("", key_buf, sizeof(key_buf));
 
     if (!validate_subst_key(key_buf))
         return;
 
     memset(&req, 0, sizeof(req));
     req.cipher_type = CIPHER_SUBST;
     req.direction = 1;
     strncpy(req.input, input_buf, MAX_STR - 1);
     for (i = 0; i < 26; i++)
         req.subst_key[i] = toupper(key_buf[i]);
 
     if (ioctl(fd, IOCTL_DECRYPT, &req) < 0) {
         perror("[LOI] IOCTL_DECRYPT subst");
         return;
     }
 
     strncpy(current_string, req.output, MAX_STR - 1);
     printf("\n  Xau ma      : \"%s\"\n", req.input);
     printf("  Khoa        : %.26s\n", req.subst_key);
     printf("  Xau giai ma : \"%s\"\n", req.output);
 }
 
 /* ===== Chuc nang 7: Giai ma hoan vi ===== */
 void func_decrypt_perm(int fd)
 {
     struct crypto_request req;
     char input_buf[MAX_STR];
     int n, i;
 
     printf("Nhap xau can giai ma (Enter de dung xau ma cuoi): ");
     read_str("", input_buf, MAX_STR);
 
     if (strlen(input_buf) == 0) {
         if (strlen(last_cipher) == 0) {
             printf("[LOI] Chua co xau ma nao!\n");
             return;
         }
         strncpy(input_buf, last_cipher, MAX_STR - 1);
         printf("  Dung xau ma cuoi: \"%s\"\n", input_buf);
     }
 
     printf("Nhap do dai khoa hoan vi: ");
     scanf("%d", &n);
     getchar();
 
     if (n <= 0 || n > MAX_KEY) {
         printf("[LOI] Do dai khoa khong hop le!\n");
         return;
     }
 
     memset(&req, 0, sizeof(req));
     req.cipher_type = CIPHER_PERM;
     req.direction = 1;
     req.perm_key_len = n;
     strncpy(req.input, input_buf, MAX_STR - 1);
 
     printf("Nhap %d chi so hoan vi (1-indexed):\n", n);
     for (i = 0; i < n; i++) {
         scanf("%d", &req.perm_key[i]);
         if (req.perm_key[i] < 1 || req.perm_key[i] > n) {
             printf("[LOI] Chi so khong hop le!\n");
             getchar();
             return;
         }
     }
     getchar();
 
     if (ioctl(fd, IOCTL_DECRYPT, &req) < 0) {
         perror("[LOI] IOCTL_DECRYPT perm");
         return;
     }
 
     strncpy(current_string, req.output, MAX_STR - 1);
     printf("\n  Xau ma      : \"%s\"\n", req.input);
     printf("  Khoa        : [");
     for (i = 0; i < n; i++)
         printf("%d%s", req.perm_key[i], i < n - 1 ? " " : "");
     printf("]\n");
     printf("  Xau giai ma : \"%s\"\n", req.output);
 }
 
 int main(void)
 {
     int fd;
     int choice;
 
     fd = open(DEVICE_PATH, O_RDWR);
     if (fd < 0) {
         perror("[LOI] Khong mo duoc device " DEVICE_PATH);
         printf("Hay chay: sudo insmod lab6_driver.ko\n");
         return EXIT_FAILURE;
     }
 
     printf("Mo device %s thanh cong.\n", DEVICE_PATH);
 
     while (1) {
         print_menu();
         if (scanf("%d", &choice) != 1) {
             /* Clear input buffer */
             while (getchar() != '\n');
             printf("[LOI] Nhap so tu 0-7!\n");
             continue;
         }
         getchar(); /* Bo newline */
 
         switch (choice) {
         case 0:
             printf("Thoat chuong trinh.\n");
             close(fd);
             return EXIT_SUCCESS;
         case 1:
             func_input_string(fd);
             break;
         case 2:
             func_encrypt_shift(fd);
             break;
         case 3:
             func_encrypt_subst(fd);
             break;
         case 4:
             func_encrypt_perm(fd);
             break;
         case 5:
             func_decrypt_shift(fd);
             break;
         case 6:
             func_decrypt_subst(fd);
             break;
         case 7:
             func_decrypt_perm(fd);
             break;
         default:
             printf("[LOI] Lua chon khong hop le!\n");
             break;
         }
     }
 
     close(fd);
     return EXIT_SUCCESS;
 }
 