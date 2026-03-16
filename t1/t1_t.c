/*
 * ============================================================
 *  CHƯƠNG TRÌNH USER-SPACE: TEST DRIVER BÀI 1 VÀ BÀI 2
 *  File: bai1_userspace.c (dùng cho lab4.1_thu)
 *  File: bai2_userspace.c (dùng cho lab4.2_thu – chỉ đổi DEVICE_PATH)
 * ============================================================
 *
 * TỔNG QUAN LUỒNG THỰC THI:
 * ─────────────────────────────────────────────────────────────
 *  main()
 *   │
 *   ├─ [1] open("/dev/lab4.1_thu", O_RDWR)
 *   │       → Kernel gọi dev_open() trong driver
 *   │       → Trả về file descriptor (fd = số nguyên dương)
 *   │
 *   ├─ [2] read(fd, buf, BUF_SIZE)
 *   │       → Kernel gọi dev_read() trong driver
 *   │       → Driver dùng copy_to_user() để copy kernel_buf → buf
 *   │       → Đọc dữ liệu mặc định "Hello from lab4.1_thu driver"
 *   │
 *   ├─ [3] write(fd, msg, strlen(msg))
 *   │       → Kernel gọi dev_write() trong driver
 *   │       → Driver dùng copy_from_user() để copy msg → kernel_buf
 *   │
 *   ├─ [4] lseek(fd, 0, SEEK_SET)  ← Reset offset về đầu file
 *   │       read(fd, buf, BUF_SIZE)  ← Đọc lại dữ liệu vừa ghi
 *   │
 *   └─ [5] close(fd)
 *           → Kernel gọi dev_close() trong driver
 * ─────────────────────────────────────────────────────────────
 *
 * CÁCH BIÊN DỊCH VÀ CHẠY:
 *   gcc -o test_bai1 bai1_userspace.c
 *   ./test_bai1
 *   (Đảm bảo đã insmod driver trước)
 */

/* ============================================================
 *  PHẦN 1: INCLUDE THƯ VIỆN USER-SPACE (POSIX/C Standard)
 * ============================================================
 *
 * ⚠ QUAN TRỌNG: Đây là chương trình USER-SPACE, KHÔNG dùng
 *   linux/module.h hay bất kỳ header kernel nào.
 *   Chỉ dùng thư viện C chuẩn và POSIX.
 */

 #include <stdio.h>      /* printf(), perror()
 * perror(str): in thông báo lỗi dựa trên errno toàn cục
 * VD: perror("open") → "open: No such file or directory" */

#include <stdlib.h>     /* EXIT_SUCCESS (= 0), EXIT_FAILURE (= 1)
 * Dùng thay vì magic number để code rõ ràng hơn */

#include <string.h>     /* strlen(), memset()
 * memset(buf, 0, size): điền 0 vào toàn bộ buffer
 * → Tránh đọc phải "rác" trong bộ nhớ chưa khởi tạo */

#include <fcntl.h>      /* open(), O_RDWR, O_RDONLY, O_WRONLY
 * open() là "cổng vào" cho mọi thao tác với file/device */

#include <unistd.h>     /* read(), write(), close(), lseek()
 * Đây là POSIX syscall wrapper – gọi trực tiếp kernel */

#include <errno.h>      /* errno – biến toàn cục chứa mã lỗi của syscall cuối
 * Sau khi syscall thất bại, errno chứa lý do (ENOENT, EACCES...)
 * perror() tự đọc errno để in thông báo */


/* ============================================================
*  PHẦN 2: HẰNG SỐ CẤU HÌNH
* ============================================================ */

/*
* ĐƯỜNG DẪN FILE THIẾT BỊ:
* Đây là file được tạo bởi device_create() trong driver.
* Kernel driver → class_create + device_create → udev → tạo file /dev/
*
* Để dùng cho bài 2: đổi thành "/dev/lab4.2_thu"
*/
#define DEVICE_PATH "/dev/lab4.1_thu"

#define BUF_SIZE 1024  /* Kích thước buffer đọc/ghi – phải ≤ BUF_SIZE trong driver */


/* ============================================================
*  PHẦN 3: HÀM MAIN
* ============================================================ */
int main(void)
{
int fd;          /* File descriptor – số nguyên kernel trả về sau open()
* fd = 0: stdin, fd = 1: stdout, fd = 2: stderr
* fd ≥ 3: file/device mở bởi chương trình */

char buf[BUF_SIZE]; /* Buffer tạm để đọc dữ liệu từ driver */
ssize_t ret;        /* ssize_t = signed size_t → có thể âm (khi lỗi) */

printf("=== Chuong trinh test driver lab4.1_thu ===\n\n");

/* ─────────────────────────────────────────────────────────
* BƯỚC 1: MỞ FILE THIẾT BỊ
* ─────────────────────────────────────────────────────────
*
* open(path, flags) → syscall mở file/device
*
* O_RDWR = O_RDONLY | O_WRONLY = cho phép đọc VÀ ghi
* Các flag khác: O_RDONLY (chỉ đọc), O_WRONLY (chỉ ghi),
*                O_CREAT (tạo mới nếu không tồn tại), O_TRUNC...
*
* Khi gọi open():
*   1. Kernel kiểm tra /dev/lab4.1_thu có tồn tại không
*   2. Tra cứu major:minor của file đó
*   3. Tìm driver đã đăng ký major:minor đó
*   4. Gọi hàm .open trong fops của driver (= dev_open())
*   5. Nếu thành công → cấp phát fd và trả về
*
* Trả về:
*   ≥ 0: file descriptor hợp lệ (= thành công)
*   -1:  lỗi, errno chứa mã lỗi cụ thể
*     - ENOENT (2): file không tồn tại → driver chưa được nạp
*     - EACCES (13): không đủ quyền → cần sudo hoặc thêm user vào group
*     - EBUSY (16): thiết bị đang bận (nếu driver giới hạn 1 user)
*/
printf("[1] Mo device: %s\n", DEVICE_PATH);
fd = open(DEVICE_PATH, O_RDWR);

if (fd < 0) {
/*
* perror("Loi mo device"):
* In ra: "Loi mo device: <thông báo lỗi tương ứng errno>"
* VD: "Loi mo device: No such file or directory"
*/
perror("    Loi mo device");
printf("    -> Kiem tra: sudo insmod bai1_driver.ko\n");
printf("    -> Kiem tra: ls /dev/lab4.1_thu\n");
return EXIT_FAILURE; /* = return 1; báo shell chương trình lỗi */
}
printf("    -> Mo thanh cong (fd = %d)\n\n", fd);
/*
* fd thường bắt đầu từ 3 vì 0,1,2 đã được dùng (stdin,stdout,stderr)
* Ví dụ: fd = 3 hoặc 4 tùy chương trình
*/

/* ─────────────────────────────────────────────────────────
* BƯỚC 2: ĐỌC DỮ LIỆU MẶC ĐỊNH TỪ DRIVER
* ─────────────────────────────────────────────────────────
*
* read(fd, buf, count) → syscall đọc dữ liệu
*
* Kernel nhận read() → gọi dev_read() trong driver
* driver thực hiện copy_to_user(buf, kernel_buf, bytes)
* Dữ liệu đọc được là "Hello from lab4.1_thu driver\n"
* (khởi tạo trong driver bằng strscpy ở bước 5 của init)
*
* Trả về:
*   > 0: số bytes đã đọc thực tế
*   = 0: EOF (hết dữ liệu)
*   -1:  lỗi
*/
printf("[2] Doc du lieu mac dinh tu driver:\n");

memset(buf, 0, BUF_SIZE);
/*
* memset() điền 0 vào toàn bộ buf trước khi đọc.
* Lý do: tránh printf("%s") in ra "rác" nếu read() không thêm '\0'
* Thực tế driver đã thêm '\0', nhưng đây là good practice.
*/

ret = read(fd, buf, BUF_SIZE);
if (ret < 0) {
perror("    Loi doc");
} else {
printf("    Doc duoc %zd bytes: \"%s\"\n\n", ret, buf);
/* %zd = format specifier cho ssize_t/size_t */
}

/* ─────────────────────────────────────────────────────────
* BƯỚC 3: GHI DỮ LIỆU VÀO DRIVER
* ─────────────────────────────────────────────────────────
*
* write(fd, buf, count) → syscall ghi dữ liệu
*
* Kernel nhận write() → gọi dev_write() trong driver
* driver thực hiện copy_from_user(kernel_buf, buf, bytes)
* Dữ liệu ghi vào sẽ thay thế nội dung trong kernel_buf
*
* ⚠ KHÔNG ghi strlen+1 (tức là không ghi cả ký tự '\0')
*   Vì driver tự thêm '\0' sau khi copy_from_user
*   Nếu ghi cả '\0' thì buf_len = strlen+1 (sai lệch nhỏ)
*/
{
const char *msg = "Xin chao tu userspace! - Thu";
printf("[3] Ghi du lieu vao driver:\n");
printf("    Du lieu ghi: \"%s\"\n", msg);

ret = write(fd, msg, strlen(msg));
/* strlen(msg) = số ký tự KHÔNG kể '\0' */

if (ret < 0) {
perror("    Loi ghi");
} else {
printf("    Ghi thanh cong %zd bytes\n\n", ret);
}
}

/* ─────────────────────────────────────────────────────────
* BƯỚC 4: ĐỌC LẠI DỮ LIỆU VỪA GHI
* ─────────────────────────────────────────────────────────
*
* SAU KHI GHI, con trỏ offset vẫn đang ở vị trí cuối.
* Nếu gọi read() ngay lập tức → offset ≥ buf_len → trả về 0 (EOF)
*
* GIẢI PHÁP: dùng lseek() để reset offset về đầu file
*
* lseek(fd, offset, whence)
*   @fd:     file descriptor
*   @offset: số byte dịch chuyển (0 = không dịch)
*   @whence: gốc tính:
*     SEEK_SET (0): tính từ đầu file    → lseek(fd, 0, SEEK_SET) = về đầu
*     SEEK_CUR (1): tính từ vị trí hiện tại
*     SEEK_END (2): tính từ cuối file   → lseek(fd, 0, SEEK_END) = về cuối
*
* Kernel gọi .llseek trong fops = default_llseek() (từ driver)
* default_llseek() cập nhật filep->f_pos trong struct file
*/
printf("[4] Doc lai du lieu vua ghi:\n");

lseek(fd, 0, SEEK_SET); /* Reset offset về đầu */
memset(buf, 0, BUF_SIZE);

ret = read(fd, buf, BUF_SIZE);
if (ret < 0) {
perror("    Loi doc");
} else {
printf("    Doc duoc %zd bytes: \"%s\"\n\n", ret, buf);
/* Kết quả mong đợi: "Xin chao tu userspace! - Thu" */
}

/* ─────────────────────────────────────────────────────────
* BƯỚC 5: ĐÓNG FILE THIẾT BỊ
* ─────────────────────────────────────────────────────────
*
* close(fd) → syscall đóng file descriptor
*
* Kernel nhận close() → gọi dev_close() / .release trong fops
* File descriptor fd được giải phóng, không dùng được nữa.
*
* ⚠ LUÔN close() trước khi thoát.
* Nếu không: kernel tự đóng khi process kết thúc,
* nhưng driver không được thông báo đúng lúc → có thể gây lỗi.
*/
printf("[5] Dong device\n");
close(fd);
printf("    -> Dong thanh cong\n\n");

printf("=== Ket thuc test ===\n");
return EXIT_SUCCESS; /* = return 0; báo shell chương trình thành công */
}

/*
* ============================================================
*  HƯỚNG DẪN ĐỔI SANG BÀI 2
* ============================================================
*
* Để tạo bai2_userspace.c, chỉ cần thay đổi:
*   #define DEVICE_PATH "/dev/lab4.2_thu"
*
* Tất cả logic open/read/write/close/lseek hoàn toàn giống nhau
* vì driver bài 1 và bài 2 có cùng file_operations.
*
* Sự khác biệt (dynamic vs static) nằm hoàn toàn bên trong kernel,
* user-space không cần biết driver dùng cách cấp phát nào.
* ============================================================
*/