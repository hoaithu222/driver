/*
==== 
* Bài 1 : Character devicer - cấp phát device number động 
Tên driver : lab4.1_thu
* Tổng quan luồng hoạt động :
- insmod (nạp module)
 │
* lab1_init()
* Bước 1 : alloc_chrdev_region() ---> Kernel cấp major/minor tự động 
*  Bước 2 : cdev_init()  + cdev_add() -->  đăng kí character device vào kernel
*  Bước 3 : class_create  --> Tạo lớp trong /sys/class/
* Bước 4  : drive_create() --> Udev file /dev/lab4.1_thu tự động 
* Bước 5 : Khởi tạo dữ liệu mặc định

** Khi user space gọi open/read/write/close : 
open() ==> dev_open()
read() ==> dev_read() (copy dữ liệu từ kernel_buf ra user)
write() ==> dev_write() (copy dữ liệu từ user vào kernel_buf)
close() ==> dev_close()

rmmod(gỡ module)
   bai1_exit() --> dọn dẹp theo thứ tự ngược lại (LIFO)
====
*/ 

// ==== Nạp các thư viện cần thiết ========


#include <linux/module.h> // bắt buộc cho mọi kernel module
// Cung cấp các MODULE_LICENSE, module_init, module_exit, THIS_MODULE, EXPORT_SYMBOL....
#include <linux/kernel.h>// Cung cấp pr_info(), pr_err(), pr_warn......
// đay là "printf" trong không gian kernel
// output  xuất hiện trong dmesg hoặc /var/log/kern.log

#include <linux/init.h> // cung cấp macro __init và __exit 
/*
__init  --> Đanh dấu hàm chỉ dùng 1 lần khi khởi tạo,
kernel giải phóng bộ nhớ của hàm này sau khi gọi 
__exit ---> Đánh dáu hàm chỉ dùng khi gỡ module.
*/ 
#include <linux/fs.h> /*   File system header - quan trong nhất cho character driver.
Cung cấp : struct file_operations, struct inode, struct file, 
register_chrdev_region, alloc_chrdev_region, unregister_chrdev_region......
*/
#include <linux/cdev.h> /* linux/device.h 
Cung cấp các class_create(), class_destroy
device_create(), device_destroy()
--> Giúp udev tự động tạo file trong /dev/
*/
#include <linux/uaccess.h> /* Cung cấp 2 hàm cực kỳ quan trọng : 
copy_to_user(dst_user, src_kernel, n)
 --> copy n bytes từ kernel sang user spce
copy_from_user(dst_kernel,src_user, n)
--> Copy n bytes từ user sang kernel space
Không được dùng memcpy() trực tiếp giữa 2 không gian này vì địa chỉ virtual memory khác nhau hoàn toàn

*/
#include <linux/string.h>  /*
Cung cấp các hàm xử lý chuỗi an toàn  trong kernel
strscpy()  --> Copy độ dài chuỗi an toàn (không tràn buffer như strcpy)
strlen()  - Độ dài chuỗi
*/
// ============ Phần 2  Metadata - Thông tin module ============

MODULE_LICENSE("GPL"); 
/*
Bắt buộc phải có : 
- "GPL  = GNU general public license.
- Nếu không khai báo hoặc khai báo license không phải GPL
   - kernel sẽ bị đánh dấu "tainted" (bị nhiễm)
   - Một số symbol nội bộ của kernel sẽ khóa, không cho dùng 
   - lệnh dmesg sẽ hiển thị cảnh báo : "module tainted"


*/ 
MODULE_AUTHOR("Thu");
/* Tên tác giả – thông tin này có thể xem bằng: modinfo bai1_driver.ko */

MODULE_DESCRIPTION("Lab 4 Bai 1 Character Driver - Cap phat dong");
/* Mô tả ngắn về chức năng module */


// -===== Phần 3 : Hằng số cấu hình 

#define DEVICE_NAME "lab4.1_thu"
/*
Tên thiết bị - dùng nhiều nơi : 
- tên file trong /dev/lab4.1_thu (do device_create tạo)\
- Tên xuất hiện trong /proc/devices
- Tiền tố cho các log pr_info/pr_err
*/ 
#define CLASS_NAME "lab4_1_class"
/*
Tên lớp thiết bị - xuất hiện trong /sys/class/lab4_1_class/
* Lớp này giúp hệ thống udev biết loại thiết bị để tạo đúng file /dev
*/ 
#define BUF_SIZE 1024
/*
Kích thước bộ nhớ đệm (buffer) - Đóng vai trò như "bộ nhớ phần cứng ảo".
Trong driver thực tế, đây sẽ là register của hardware hoặc DMA buffer.
1024 bytes = 1KB, đủ cho mục đích học tập 
*/ 

static dev_t dev_num; 
/*
dev_t = kiểu dữ liệu lưu cặp (major, minor) dạng số nguyên 32-bit
Cấu trúc nội bộ : [20 bit major | 12 bit minor]
- Major (dev_num) --> lấy major number
- minor (dev_num) --> lấy minor number
"static" = Chỉ dùng trong file này không export ra ngoài

*/ 
static struct cdev my_cdev;
/*
Struct cdev = cấu trúc đại diện một character device trong kernel.
Bên trong chứa : con trỏ tới file_operations, kobject, owner......
Mỗi driver cần một cdev để kernel biết cách điều phối syscall
*/ 
static struct class *my_class;/*
Con trỏ tới device class _ Đại diện cho loại thiết bị. 
Ví dụ : /sys/class/tty, /sys/class/input, /sys/class/lab4_1_class
*/
static struct device *my_device; /*
Con trỏ tới device object - Đại diện cho node thiết bị cụ thể 
Sau khi device_create()  thành công ---> Kernel tạo /dev/lab4_thu

*/

static char kernel_buf[BUF_SIZE]; /*
Bộ đệm dữ liệu - đây là phần cứng ảo của driver này.
dữ liệu user ghi vào sẽ lưu ở đây, user đọc ra cũng từ đây 
Trong driver thực (UART, SPI) đây sẽ lưu địa chỉ registers.
*/

static int buf_len = 0; 
/*
Theo dõi dộ dài dữ liệu thực tế trong kernel_buf
cần thiết đẻ hàm read biết đọc đến đâu
Khởi tạo  = 0 (buffer rỗng khi driver mới nap)
*/ 

static int open_count = 0;
/*
Đếm số lần thiết bị mở(open()). 
Hữu ích để : 
debug : Khiểm tra có bao nhiêu process đang dùng driver
trong driver thực : có thể dùng để giới hạn 1 process dùng tại 1 thời điểm

*/ 
//  Phần 5 : Hàm file operations(đây là trái tim của driver - Xử lý các hàm systems call từ user-space)

/*
Hàm : dev_open() 
// Kích hoạt khi user-space gọi ("/dev/lab4.1_thu",..)
Tham số : 
 @inodep : con trỏ inode  - chứa thông tin về file trong filesystem 
 (major/minor number, quyền truy cập...) 
 @filep : con trỏ tới struct file - đai diện cho "Phiên mở file"  hiện tại (vị trí đọc/ghi, flags, private_data.....)
 Trả về : 0 = thành công, số âm  = lỗi (mã errno)
*/ 

static init dev_open(struct inode * inodep, struct file * filep){
   open_count++;
   /*
   pr_info()  = printk(KERN_INFO...)  xuất log mưc info
   xem log bằng dmesg | tail hoặc journalctl -k
   * Format : [tên_device] nội dung log
   */ 
   pr_info("[%s] Device opened (lan thu %d)\n", DEVICE_NAME, open_count);
   return 0; /* 0 = thành công. Nếu trả về âm, user-space nhận errno tương ứng */
}
/*
Hàm 2 :  dev_close() trong fops là .release
Kích hoạt khi : user-space gọi close(fd)
"release" trong kernel  = "đóng file descriptor cuối cùng"
Nếu 1 file được dup() nhiều lần, release chỉ gọi khi tất cả đều đóng
*/ 

static int dev_close(struct inode *inodep, struct file *filep){
   pr_info("[%s] Device closed\n", DEVICE_NAME);
   return 0;
}
/*
Phần 3 : Hàm dev_read(
Kích hoạt khi user-space gọi read(fd, buffer, count)
tham số : 
@filep : struct file của phiên mở hiện tại 
@buf : buffer của USER SPACE - nơi dữ liệu sẽ được copy vào
@len : số  bytes user muốn đọc 
@offset : con trỏ vị trí đọc (kernel tự quản, tự tăng)
*offset = 0 khi mới mở file
Trả về : 
> 0 : số bytes đọc thực tế 
= 0 : EOF (End of File)  - hết dữ liệu, báo cho user dùng đọc
*/ 

static ssize_t dev_read(struct file*filep, char __user *buf, size_t len, loff_t *offset){
   int bytes_to_read;
   /*
   Kiểm tra EOF: 
   *offset  = vị trí con trỏ đọc hiện tại byte thứ mấy 
   * buf_len =  tổng dữ liệu có trong buffer
   Nếu đã đọc hết (offset >= buf_len) → trả về 0 báo EOF
   Ví dụ: kernel_buf = "Hello" (5 bytes), buf_len = 5
   Lần đọc 1: *offset = 0 → đọc được
   Lần đọc 2: *offset = 5 → 5 >= 5 → trả về 0 (EOF)
   */ 
   if(*offset >= buf_len){
      return 0; /* EOF */
   }
   /*
   Tính số bytes có thể đọc thực tế : 
   min(a,b) = lấy giá trị nhỏ hơn - tránh đọc quá gới hạn 
   Ví dụ : 
   buf_len  = 10, *offset = 3 còn 7bytes chưa đọc 
   len (user muốn đọc) = 1024
   -> bytes_to_read = min(7, 1024) = 7
    (Không thể đọc 1024 bytes khi chỉ có 7 bytes)
    cast về (int)  để min hoạt động đúng (tránh so sánh loff_t và size_t)
   */ 
   bytes_to_read = min((int)(buf_len - *offset), (int)len);
   /*Copy dữ liệu Kernel space --> user space 
   Tại sao không dùng memcpy()? 
   user space và kernel space có không gian địa chỉ ảo khác nhau 
   địa chỉ "0x12345678" trong user space và kernel space sẽ khác nhau 
   trỏ tối vùng vật lý khác nhau
   copy_to_user() thực hiện : 
   1.Kiểm tra địa chỉ user(buf) có hợp lệ không 
   2. Kiểm tra user có quyền ghi vào vùng đó hay không 
   3. Thực hiện copy với page fault handling đúng cách 
   4. Trả về số bytes không copy được (0 = thành công hoàn toàn)
   Cú pháp: copy_to_user(dst_user, src_kernel, n_bytes) 
   
   */
   if(copy_to_user(buf, kernel_buf + *offset, bytes_to_read)){
      pr_err("[%s] Loi copy_to_user\n", DEVICE_NAME);
      return -EFAULT; /* -EFAULT = lỗi địa chỉ không hợp lệ (Bad Address)
      User-space nhận được errno = EFAULT (14) */
   }
   /* Cập nhật vị trí offset sau khi đọc thành công */
   *offset += bytes_to_read;
   pr_info("[%s] Da doc %d bytes, offset hien tai = %lld\n", DEVICE_NAME, bytes_to_read, *offset);
   return bytes_to_read; /* Trả về số bytes thực tế đã đọc */
}

/*
Hàm 4 : dev write()  
Kích hoạt khi: user-space gọi write(fd, data, count)
Tham số : 
filep : struct file của phiên bản mở hiện tại 
buf : buffer của USER SPACE - Chứa dữ liệu cần ghi 
    __user = annotation cho compiler và sparse tool, không ảnh hưởng đến runtime
   len: số bytes user muốn ghi
   offset : vị trí ghi (driver này bỏ qua offset, luôn ghi đè từ đầu)
   Trả về : 
   > 0 : số bytes đã ghi thực tế 
   < 0 : lỗi

*/ 

static ssize_t dev_write(struct file *filep, const char __user *buf, size_t len, loff_t *offset){
   int bytes_to_write;

   /*
   Giới hạn số bytes ghi : 
   Không ghi quá (BUF_SIZE - 1) để chừa 1 bytes cuối cùng cho ký tự '\0'
   ví dụ : BUF_SIZE  = 1024,user muốn ghi 2000 bytes 
   ---> bytes)_to_write = min(2000, 1023) = 1023
   (Chỉ ghi 1023 bytes cho  1024 bytes, byte 1024 dành cho \0)
   */ 
   bytes_to_write = min((int)len, (int)(BUF_SIZE - 1));
   /*
   Copy dữ liệu User space --> Kernel space 
   copy_from_user() tương tự copy_to_user() nhưng ngược chiều.
   Trả về số bytes không copy được (0 = thành công)
   Cú pháp : copy_from_user(dst_kernel, src_user, n_bytes)
   
   */


}