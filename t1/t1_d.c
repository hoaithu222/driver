/*
 * ============================================================
 *  BÀI 1: CHARACTER DRIVER - CẤP PHÁT DEVICE NUMBER ĐỘNG
 *  Tên driver: lab4.1_thu
 * ============================================================
 *
 * TỔNG QUAN LUỒNG HOẠT ĐỘNG:
 * ─────────────────────────────────────────────────────────────
 *  insmod (nạp module)
 *      │
 *      ▼
 *  lab1_init()
 *      ├─ [Bước 1] alloc_chrdev_region()   → Kernel cấp Major/Minor tự động
 *      ├─ [Bước 2] cdev_init() + cdev_add() → Đăng ký Character Device vào Kernel
 *      ├─ [Bước 3] class_create()           → Tạo lớp trong /sys/class/
 *      ├─ [Bước 4] device_create()          → Udev tạo file /dev/lab4.1_thu tự động
 *      └─ [Bước 5] Khởi tạo dữ liệu mặc định
 *
 *  Khi user-space gọi open/read/write/close:
 *      open()   → dev_open()
 *      read()   → dev_read()   (copy dữ liệu từ kernel_buf ra user)
 *      write()  → dev_write()  (copy dữ liệu từ user vào kernel_buf)
 *      close()  → dev_close()
 *
 *  rmmod (gỡ module)
 *      └─ bai1_exit() → dọn dẹp theo thứ tự ngược lại (LIFO)
 * ─────────────────────────────────────────────────────────────
 *


/* ============================================================
 *  PHẦN 1: INCLUDE – NẠP CÁC THƯ VIỆN CẦN THIẾT
 * ============================================================ */

 #include <linux/module.h>   /* Bắt buộc cho mọi Kernel module.
 Cung cấp: MODULE_LICENSE, module_init, module_exit,
 THIS_MODULE, EXPORT_SYMBOL... */

#include <linux/kernel.h>   /* Cung cấp pr_info(), pr_err(), pr_warn()...
 Đây là "printf" trong không gian kernel.
 Output xuất hiện trong dmesg hoặc /var/log/kern.log */

#include <linux/init.h>     /* Cung cấp macro __init và __exit.
 __init  → đánh dấu hàm chỉ dùng 1 lần khi khởi tạo,
           kernel giải phóng bộ nhớ của hàm này sau khi gọi.
 __exit → đánh dấu hàm chỉ dùng khi gỡ module. */

#include <linux/fs.h>       /* File System header – quan trọng nhất cho Character Driver.
 Cung cấp: struct file_operations, struct inode, struct file,
 register_chrdev_region, alloc_chrdev_region,
 unregister_chrdev_region... */

#include <linux/cdev.h>     /* Cung cấp struct cdev và các hàm:
 cdev_init() – khởi tạo cdev
 cdev_add()  – đăng ký vào kernel
 cdev_del()  – gỡ đăng ký khỏi kernel */

#include <linux/device.h>   /* ⚠ GHI CHÚ: code gốc viết sai "linux.device.h" (dùng dấu chấm)
 Phải là linux/device.h (dùng dấu gạch chéo /).
 Cung cấp: class_create(), class_destroy(),
 device_create(), device_destroy()
 → Giúp udev tự động tạo file trong /dev/ */

#include <linux/uaccess.h>  /* Cung cấp 2 hàm cực kỳ quan trọng:
 copy_to_user(dst_user, src_kernel, n)
     → Copy n bytes từ kernel sang user space
 copy_from_user(dst_kernel, src_user, n)
     → Copy n bytes từ user sang kernel space
 ⚠ KHÔNG được dùng memcpy() trực tiếp giữa 2 không gian này
 vì địa chỉ virtual memory khác nhau hoàn toàn. */

#include <linux/string.h>   /* Cung cấp các hàm xử lý chuỗi an toàn trong kernel:
 strscpy() – copy chuỗi an toàn (không tràn buffer như strcpy)
 strlen()  – đo độ dài chuỗi */


/* ============================================================
*  PHẦN 2: MODULE METADATA – THÔNG TIN MODULE
* ============================================================ */

MODULE_LICENSE("GPL");
/*
* BẮT BUỘC PHẢI CÓ.
* "GPL" = GNU General Public License.
* Nếu không khai báo hoặc khai báo license không phải GPL:
*   - Kernel sẽ bị đánh dấu "tainted" (bị nhiễm)
*   - Một số symbol nội bộ của kernel sẽ bị khóa, không cho dùng
*   - Lệnh dmesg sẽ hiện cảnh báo: "module tainted"
*/

MODULE_AUTHOR("Thu");
/* Tên tác giả – thông tin này có thể xem bằng: modinfo bai1_driver.ko */

MODULE_DESCRIPTION("Lab 4 Bai 1 Character Driver - Cap phat dong");
/* Mô tả ngắn về chức năng module */


/* ============================================================
*  PHẦN 3: HẰNG SỐ CẤU HÌNH
* ============================================================ */

#define DEVICE_NAME "lab4.1_thu"
/*
* Tên thiết bị – dùng ở nhiều nơi:
*   - Tên file trong /dev/lab4.1_thu (do device_create tạo)
*   - Tên xuất hiện trong /proc/devices
*   - Tiền tố cho các log pr_info/pr_err
*/

#define CLASS_NAME "lab4_1_class"
/*
* Tên lớp thiết bị – xuất hiện trong /sys/class/lab4_1_class/
* Lớp này giúp hệ thống udev biết loại thiết bị để tạo đúng file /dev
* ⚠ code gốc viết "bab4_1_class" – lỗi đánh máy (b thay vì l)
*/

#define BUF_SIZE 1024
/*
* Kích thước bộ nhớ đệm (buffer) – đóng vai trò như "bộ nhớ phần cứng ảo".
* Trong driver thực tế, đây sẽ là register của hardware hoặc DMA buffer.
* 1024 bytes = 1 KB, đủ cho mục đích học tập.
* ⚠ code gốc dùng cả BUFSIZE và BUF_SIZE không nhất quán → lỗi compile
*/


/* ============================================================
*  PHẦN 4: BIẾN TOÀN CỤC – TRẠNG THÁI DRIVER
* ============================================================ */

static dev_t dev_num;
/*
* dev_t = kiểu dữ liệu lưu cặp (major, minor) dạng số nguyên 32-bit.
* Cấu trúc nội bộ: [20 bit major | 12 bit minor]
*   - MAJOR(dev_num) → lấy major number
*   - MINOR(dev_num) → lấy minor number
* "static" = chỉ dùng trong file này, không export ra ngoài.
*/

static struct cdev my_cdev;
/*
* struct cdev = cấu trúc đại diện một Character Device trong kernel.
* Bên trong chứa: con trỏ tới file_operations, kobject, owner...
* Mỗi driver cần một cdev để kernel biết cách điều phối syscall.
*/

static struct class *my_class;
/*
* Con trỏ tới Device Class – đại diện cho "loại" thiết bị.
* Ví dụ: /sys/class/tty, /sys/class/input, /sys/class/lab4_1_class
* Dùng để tổ chức thiết bị và cho udev biết cần tạo file /dev không.
*/

static struct device *my_device;
/*
* Con trỏ tới Device object – đại diện cho node thiết bị cụ thể.
* Sau khi device_create() thành công → kernel tạo /dev/lab4.1_thu
*/

static char kernel_buf[BUF_SIZE];
/*
* Bộ đệm dữ liệu – đây là "phần cứng ảo" của driver này.
* Dữ liệu user ghi vào sẽ lưu ở đây, user đọc ra cũng từ đây.
* Trong driver thực (UART, SPI...) đây sẽ là địa chỉ register.
*/

static int buf_len = 0;
/*
* Theo dõi độ dài dữ liệu thực tế trong kernel_buf.
* Cần thiết để hàm read biết đọc đến đâu.
* Khởi tạo = 0 (buffer rỗng khi driver mới nạp).
*/

static int open_count = 0;
/*
* Đếm số lần thiết bị được mở (open()).
* Hữu ích để:
*   - Debug: kiểm tra có bao nhiêu process đang dùng driver
*   - Trong driver thực: có thể dùng để giới hạn chỉ 1 process dùng tại 1 thời điểm
*/


/* ============================================================
*  PHẦN 5: HÀM FILE OPERATIONS
*  Đây là trái tim của driver – xử lý các system call từ user-space
* ============================================================ */

/*
* ──────────────────────────────────────────────────────────
*  HÀM 1: dev_open()
*  Kích hoạt khi: user-space gọi open("/dev/lab4.1_thu", ...)
*
*  Tham số:
*    @inodep: con trỏ tới inode – chứa thông tin về file trong filesystem
*             (major/minor number, quyền truy cập...)
*    @filep:  con trỏ tới struct file – đại diện cho "phiên mở file" hiện tại
*             (vị trí đọc/ghi, flags, private_data...)
*
*  Trả về: 0 = thành công, số âm = lỗi (mã lỗi errno)
* ──────────────────────────────────────────────────────────
*/
static int dev_open(struct inode *inodep, struct file *filep)
{
open_count++;
/* pr_info() = printk(KERN_INFO ...) – xuất log mức INFO
* Xem log bằng: dmesg | tail hoặc journalctl -k
* Format: [tên_device] nội dung log */
pr_info("[%s] Device opened (lan thu %d)\n", DEVICE_NAME, open_count);
return 0; /* 0 = thành công. Nếu trả về âm, user-space nhận errno tương ứng */
}


/*
* ──────────────────────────────────────────────────────────
*  HÀM 2: dev_close()  (tên trong fops là .release)
*  Kích hoạt khi: user-space gọi close(fd)
*
*  "release" trong kernel = "đóng file descriptor cuối cùng".
*  Nếu 1 file được dup() nhiều lần, release chỉ gọi khi tất cả đều đóng.
*
*  ⚠ code gốc: fops dùng .release = dev_release nhưng hàm tên dev_close
*              → lỗi compile "undeclared identifier"
*              → Phải thống nhất tên
* ──────────────────────────────────────────────────────────
*/
static int dev_close(struct inode *inodep, struct file *filep)
{
pr_info("[%s] Device closed\n", DEVICE_NAME);
return 0;
}


/*
* ──────────────────────────────────────────────────────────
*  HÀM 3: dev_read()
*  Kích hoạt khi: user-space gọi read(fd, buffer, count)
*
*  Tham số:
*    @filep:  struct file của phiên mở hiện tại
*    @buf:    buffer của USER SPACE – nơi dữ liệu sẽ được copy vào
*    @len:    số bytes user muốn đọc
*    @offset: con trỏ vị trí đọc (kernel tự quản lý, tự tăng)
*             *offset = 0 khi mới mở file
*
*  Trả về:
*    > 0: số bytes đã đọc thực tế
*    = 0: EOF (End Of File) – hết dữ liệu, báo cho user dừng đọc
*    < 0: lỗi (mã errno âm)
*
*  ⚠ KIỂU TRẢ VỀ PHẢI LÀ ssize_t (signed), KHÔNG PHẢI size_t (unsigned)
*     vì cần trả về số âm khi có lỗi
* ──────────────────────────────────────────────────────────
*/
static ssize_t dev_read(struct file *filep, char __user *buf, size_t len, loff_t *offset)
{
int bytes_to_read;

/*
* KIỂM TRA EOF:
* *offset = vị trí con trỏ đọc hiện tại (byte thứ mấy)
* buf_len = tổng dữ liệu có trong buffer
* Nếu đã đọc hết (offset >= buf_len) → trả về 0 báo EOF
*
* Ví dụ: kernel_buf = "Hello" (5 bytes), buf_len = 5
*   Lần đọc 1: *offset = 0 → đọc được
*   Lần đọc 2: *offset = 5 → 5 >= 5 → trả về 0 (EOF)
*/
if (*offset >= buf_len)
return 0; /* EOF */

/*
* TÍNH SỐ BYTES CÓ THỂ ĐỌC THỰC TẾ:
* min(a, b) = lấy giá trị nhỏ hơn – tránh đọc quá giới hạn
*
* Ví dụ:
*   buf_len = 10, *offset = 3 → còn 7 bytes chưa đọc
*   len (user muốn đọc) = 1024
*   → bytes_to_read = min(7, 1024) = 7
*   (Không thể đọc 1024 bytes khi chỉ có 7 bytes)
*
* Cast về (int) để min() hoạt động đúng (tránh so sánh loff_t và size_t)
*/
bytes_to_read = min((int)(buf_len - *offset), (int)len);

/*
* COPY DỮ LIỆU: KERNEL SPACE → USER SPACE
*
* ⚠ TẠI SAO KHÔNG DÙNG memcpy()?
* User space và kernel space có không gian địa chỉ ảo KHÁC NHAU.
* Địa chỉ "0x12345678" trong user space và kernel space
* trỏ tới vùng vật lý KHÁC NHAU.
*
* copy_to_user() thực hiện:
*   1. Kiểm tra địa chỉ user (buf) có hợp lệ không
*   2. Kiểm tra user có quyền ghi vào vùng đó không
*   3. Thực hiện copy với page fault handling đúng cách
*   4. Trả về số bytes KHÔNG copy được (0 = thành công hoàn toàn)
*
* Cú pháp: copy_to_user(dst_user, src_kernel, n_bytes)
*/
if (copy_to_user(buf, kernel_buf + *offset, bytes_to_read)) {
pr_err("[%s] Loi copy_to_user\n", DEVICE_NAME);
return -EFAULT; /* -EFAULT = lỗi địa chỉ không hợp lệ (Bad Address)
* User-space nhận được errno = EFAULT (14) */
}

/* Cập nhật vị trí offset sau khi đọc thành công */
*offset += bytes_to_read;

pr_info("[%s] Da doc %d bytes, offset hien tai = %lld\n",
DEVICE_NAME, bytes_to_read, *offset);

return bytes_to_read; /* Trả về số bytes thực tế đã đọc */
}


/*
* ──────────────────────────────────────────────────────────
*  HÀM 4: dev_write()
*  Kích hoạt khi: user-space gọi write(fd, data, count)
*
*  Tham số:
*    @filep:  struct file của phiên mở hiện tại
*    @buf:    buffer của USER SPACE – chứa dữ liệu cần ghi
*             __user = annotation cho compiler và sparse tool, không ảnh hưởng runtime
*    @len:    số bytes user muốn ghi
*    @offset: vị trí ghi (driver này bỏ qua offset, luôn ghi đè từ đầu)
*
*  Trả về:
*    > 0: số bytes đã ghi thực tế
*    < 0: lỗi
* ──────────────────────────────────────────────────────────
*/
static ssize_t dev_write(struct file *filep, const char __user *buf, size_t len, loff_t *offset)
{
int bytes_to_write;

/*
* GIỚI HẠN SỐ BYTES GHI:
* Không ghi quá (BUF_SIZE - 1) để chừa 1 byte cuối cho ký tự '\0'
*
* Ví dụ: BUF_SIZE = 1024, user muốn ghi 2000 bytes
*   → bytes_to_write = min(2000, 1023) = 1023
*   (Chỉ ghi 1023 bytes, byte 1024 dành cho '\0')
*/
bytes_to_write = min((int)len, (int)(BUF_SIZE - 1));

/*
* COPY DỮ LIỆU: USER SPACE → KERNEL SPACE
*
* copy_from_user() tương tự copy_to_user() nhưng ngược chiều.
* Trả về số bytes KHÔNG copy được (0 = thành công).
*
* Cú pháp: copy_from_user(dst_kernel, src_user, n_bytes)
*
* Driver này luôn ghi đè từ đầu buffer (kernel_buf + 0)
* → Dữ liệu cũ bị xóa hoàn toàn
*/
if (copy_from_user(kernel_buf, buf, bytes_to_write)) {
pr_err("[%s] Loi copy_from_user\n", DEVICE_NAME);
return -EFAULT;
}

/*
* THÊM KÝ TỰ KẾT THÚC CHUỖI (NULL TERMINATOR):
* Đảm bảo kernel_buf luôn là chuỗi C hợp lệ (kết thúc bằng '\0')
* → Cho phép dùng strlen(), pr_info("%s") an toàn sau này
*
* ⚠ code gốc: kernel_buf[bytes_to_write] = '\0' thiếu dấu chấm phẩy
*             và dùng sai tên biến bytes_to_writes
*/
kernel_buf[bytes_to_write] = '\0';

/* Cập nhật độ dài dữ liệu thực trong buffer */
buf_len = bytes_to_write;

pr_info("[%s] Da ghi %d bytes\n", DEVICE_NAME, bytes_to_write);

return bytes_to_write;
}


/* ============================================================
*  PHẦN 6: BẢNG FILE OPERATIONS (fops)
*  Ánh xạ system call → hàm xử lý trong driver
* ============================================================
*
*  Khi user-space gọi open()/read()/write()/close()...
*  Kernel tra bảng fops này để biết gọi hàm nào trong driver.
*
*  struct file_operations có ~30 con trỏ hàm, driver không cần implement tất cả.
*  Các trường không khai báo = NULL → kernel dùng hành vi mặc định hoặc trả lỗi.
*/
static const struct file_operations fops = {
.owner   = THIS_MODULE,
/*
* THIS_MODULE = con trỏ tới struct module của driver này.
* Tác dụng: ngăn kernel gỡ module (rmmod) khi đang có file descriptor mở.
* → Tránh kernel panic do dùng con trỏ hàm của module đã bị unload.
* Nếu bỏ .owner: kernel không tự tăng reference count → có thể crash.
*/

.open    = dev_open,
/* user: open("/dev/lab4.1_thu", O_RDWR) → kernel gọi dev_open() */

.release = dev_close,
/*
* "release" trong struct file_operations ≠ "close" của user.
* release() gọi khi file descriptor cuối cùng trỏ tới file được đóng.
* ⚠ code gốc dùng .release = dev_release nhưng hàm tên là dev_close
*   → phải thống nhất: hoặc đổi tên hàm thành dev_release,
*     hoặc để .release = dev_close (đã sửa ở đây)
*/

.read    = dev_read,
/* user: read(fd, buf, count) → kernel gọi dev_read() */

.write   = dev_write,
/* user: write(fd, buf, count) → kernel gọi dev_write() */

.llseek  = default_llseek,
/*
* Hỗ trợ system call lseek() để di chuyển vị trí đọc/ghi.
* default_llseek() là hàm có sẵn trong kernel.
* → User-space có thể dùng lseek(fd, 0, SEEK_SET) để đọc lại từ đầu.
* (Chương trình test dùng lseek() trước khi đọc lại sau khi ghi)
*/
};


/* ============================================================
*  PHẦN 7: HÀM KHỞI TẠO MODULE – lab1_init()
*  Chạy MỘT LẦN DUY NHẤT khi: sudo insmod bai1_driver.ko
* ============================================================ */
static int __init lab1_init(void)
{
int ret; /* Biến lưu mã lỗi trả về từ các hàm kernel */

pr_info("[%s] Bat dau khoi tao module...\n", DEVICE_NAME);

/* ─────────────────────────────────────────────────────────
* BƯỚC 1: XIN CẤP PHÁT DEVICE NUMBER ĐỘNG
* ─────────────────────────────────────────────────────────
*
* alloc_chrdev_region(dev, baseminor, count, name)
*   @dev:       con trỏ → kernel sẽ ghi cặp (major, minor) vào đây
*   @baseminor: minor number bắt đầu (thường là 0)
*   @count:     số lượng minor number cần (1 thiết bị = 1)
*   @name:      tên xuất hiện trong /proc/devices
*
* Kernel tự quét danh sách major number đang rảnh và cấp phát.
* → Khác với register_chrdev_region() (cấp tĩnh, phải chỉ định trước)
*
* Sau khi gọi thành công:
*   - /proc/devices sẽ có dòng: "<major> lab4.1_thu"
*   - MAJOR(dev_num) và MINOR(dev_num) trả về số được cấp
*/
ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
if (ret < 0) {
pr_err("[%s] Loi alloc_chrdev_region: %d\n", DEVICE_NAME, ret);
return ret; /* Thoát sớm – không có gì cần dọn dẹp ở bước này */
}
pr_info("[%s] Cap phat thanh cong: major=%d, minor=%d\n",
DEVICE_NAME, MAJOR(dev_num), MINOR(dev_num));

/* ─────────────────────────────────────────────────────────
* BƯỚC 2: KHỞI TẠO VÀ ĐĂNG KÝ CHARACTER DEVICE (cdev)
* ─────────────────────────────────────────────────────────
*
* cdev_init(&cdev, &fops):
*   Liên kết struct cdev với bảng fops.
*   Sau bước này, cdev biết cần gọi hàm nào khi có syscall.
*
* cdev_add(&cdev, dev_num, count):
*   Đăng ký cdev vào kernel với device number đã cấp phát.
*   Từ thời điểm này, kernel có thể nhận và điều phối syscall.
*
* ⚠ code gốc: cdev_add(&my_cdev, dev_number, 1)
*   → Sai tên biến: dev_number không tồn tại, phải là dev_num
*/
cdev_init(&my_cdev, &fops);
ret = cdev_add(&my_cdev, dev_num, 1); /* ← sửa dev_number → dev_num */
if (ret < 0) {
pr_err("[%s] Loi cdev_add: %d\n", DEVICE_NAME, ret);
goto err_cdev; /* Nhảy đến cleanup: giải phóng device number */
}

/* ─────────────────────────────────────────────────────────
* BƯỚC 3: TẠO DEVICE CLASS
* ─────────────────────────────────────────────────────────
*
* class_create(name):
*   Tạo thư mục /sys/class/lab4_1_class/
*   Hệ thống udev theo dõi /sys/class/ để biết thiết bị nào cần file /dev
*
* IS_ERR(ptr): kiểm tra con trỏ có phải mã lỗi không
* PTR_ERR(ptr): lấy mã lỗi từ con trỏ lỗi
*   (Kernel API dùng kỹ thuật "error pointer": mã lỗi được encode vào
*    địa chỉ cuối không gian địa chỉ kernel, VD: -12 → 0xFFFFF4)
*/
my_class = class_create(CLASS_NAME);
if (IS_ERR(my_class)) {
pr_err("[%s] Loi class_create\n", DEVICE_NAME);
ret = PTR_ERR(my_class);
goto err_class; /* Cleanup: xóa cdev + giải phóng device number */
}

/* ─────────────────────────────────────────────────────────
* BƯỚC 4: TẠO DEVICE NODE (File /dev/lab4.1_thu)
* ─────────────────────────────────────────────────────────
*
* device_create(class, parent, devt, drvdata, fmt, ...)
*   @class:   lớp thiết bị vừa tạo
*   @parent:  thiết bị cha (NULL = không có)
*   @devt:    device number (major:minor)
*   @drvdata: dữ liệu riêng của driver (NULL = không dùng)
*   @fmt:     tên file sẽ tạo trong /dev/
*
* Sau khi gọi thành công:
*   - /dev/lab4.1_thu được tạo tự động bởi udev
*   - /sys/class/lab4_1_class/lab4.1_thu/ xuất hiện
*   - User-space có thể dùng open("/dev/lab4.1_thu", ...)
*
* Nếu KHÔNG có bước này, phải tạo thủ công:
*   sudo mknod /dev/lab4.1_thu c <major> <minor>
*/
my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
if (IS_ERR(my_device)) {
pr_err("[%s] Loi device_create\n", DEVICE_NAME);
ret = PTR_ERR(my_device);
goto err_device; /* Cleanup: xóa class + cdev + device number */
}

/* ─────────────────────────────────────────────────────────
* BƯỚC 5: KHỞI TẠO DỮ LIỆU MẶC ĐỊNH
* ─────────────────────────────────────────────────────────
*
* strscpy(dst, src, size): copy chuỗi an toàn trong kernel
*   Không dùng strcpy() (có thể tràn buffer)
*   Không dùng memcpy() cho chuỗi (không tự thêm '\0')
*/
strscpy(kernel_buf, "Hello from lab4.1_thu driver\n", BUF_SIZE);
buf_len = strlen(kernel_buf);

pr_info("[%s] Module da duoc nap thanh cong!\n", DEVICE_NAME);
pr_info("[%s] Du lieu mac dinh: \"%s\"\n", DEVICE_NAME, kernel_buf);
return 0; /* 0 = khởi tạo thành công */

/* ─────────────────────────────────────────────────────────
* XỬ LÝ LỖI: ROLLBACK PATTERN (DỌN DẸP KHI LỖI)
* ─────────────────────────────────────────────────────────
*
* Kỹ thuật "goto cleanup" là CHUẨN trong kernel code.
* Nguyên tắc: cái gì tạo ra sau cùng thì phải hủy trước.
*
* Ví dụ nếu device_create() thất bại:
*   err_device:
*     → class_destroy()          (hủy class đã tạo)
*   err_class:
*     → cdev_del()               (gỡ cdev đã đăng ký)
*   err_cdev:
*     → unregister_chrdev_region() (trả lại device number)
*/
err_device:
class_destroy(my_class);      /* Hủy device class đã tạo */
err_class:
cdev_del(&my_cdev);           /* Gỡ cdev khỏi kernel */
err_cdev:
unregister_chrdev_region(dev_num, 1); /* Trả lại device number */
return ret;
}


/* ============================================================
*  PHẦN 8: HÀM GỠ BỎ MODULE – bai1_exit()
*  Chạy MỘT LẦN DUY NHẤT khi: sudo rmmod bai1_driver
* ============================================================
*
*  QUY TẮC DỌN DẸP: LIFO (Last In, First Out)
*  → Thứ tự ngược lại với init().
*  Tại sao? Đảm bảo không có dangling pointer.
*  Ví dụ: không thể xóa class trước khi xóa device (device dùng class).
*
*  Thứ tự init:   alloc → cdev_add → class_create → device_create
*  Thứ tự exit:   device_destroy → class_destroy → cdev_del → unregister
*/
static void __exit bai1_exit(void)
{
/* 1. Xóa device node /dev/lab4.1_thu
*    udev tự xóa file trong /dev/ */
device_destroy(my_class, dev_num);

/* 2. Xóa device class /sys/class/lab4_1_class/ */
class_destroy(my_class);

/* 3. Gỡ cdev khỏi kernel – kernel không còn điều phối syscall cho driver này */
cdev_del(&my_cdev);

/* 4. Trả lại device number cho kernel
*    Major number này có thể được cấp cho driver khác sau đó */
unregister_chrdev_region(dev_num, 1);

pr_info("[%s] Module da duoc go bo\n", DEVICE_NAME);
}


/* ============================================================
*  PHẦN 9: ĐĂNG KÝ HÀM INIT VÀ EXIT
* ============================================================
*
*  module_init(func): báo kernel gọi func() khi insmod
*  module_exit(func): báo kernel gọi func() khi rmmod
*
*  Macro này thực chất tạo ra các section đặc biệt trong ELF binary
*  (.init.text và .exit.text) mà kernel loader biết cách xử lý.
*
*  ⚠ Đảm bảo tên hàm phải khớp với tên đã định nghĩa ở trên:
*     lab1_init() và bai1_exit()
*/
module_init(lab1_init);
module_exit(bai1_exit);

/*
* ============================================================
*  HƯỚNG DẪN BIÊN DỊCH VÀ SỬ DỤNG
* ============================================================
*
*  1. Tạo Makefile:
*     obj-m += bai1_driver.o
*     all:
*         make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
*     clean:
*         make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
*
*  2. Biên dịch:
*     $ make
*
*  3. Nạp module:
*     $ sudo insmod bai1_driver.ko
*
*  4. Kiểm tra log:
*     $ dmesg | tail -20
*
*  5. Kiểm tra device number:
*     $ cat /proc/devices | grep lab4.1_thu
*
*  6. Kiểm tra file /dev:
*     $ ls -la /dev/lab4.1_thu
*
*  7. Chạy chương trình test:
*     $ gcc -o test_bai1 bai1_userspace.c
*     $ ./test_bai1
*
*  8. Gỡ module:
*     $ sudo rmmod bai1_driver
*     $ dmesg | tail -5
* ============================================================
*/