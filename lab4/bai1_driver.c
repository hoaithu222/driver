/*
Bài 1 : Character Driver - Cấp phát Device Number động
- Tên Driver : lab4.1_thu
* tóm tắt chức năng ; 
- Sử dụng alloc_chrdev_region() để xin Kernel cấp phát tự động một major number.
- Sử dụng class_create() và device_create() để tự động tạo file thiết bị trong thu mục /dev
- Hỗ trợ các thao tác cơ bản từ user-space : open(mở),release(đóng), read(đọc), write(ghi)

*/ 

// khu vực này sẽ cấu hình import các thư viện
#include <linux/module.h> // Thư viện cốt lõi cho mọi Kernel module(hỗ trợ Module_License)
#include <linux/kernel.h> // Cung cấp các hàm nhân cơ bản như pr_info(), pr_err()
#include <linux/init.h> // cung cấp các macro __init và __exit 
#include <linux/fs.h> // thư viện hỗ trợ các cấu trúc file(như file_operations)
#include <linux/cdev.h> // Cung cấp cấu trúc và hàm quản lý Character Device(cdev)
#include <linux.device.h> // Hỗ trợ tạo lớp (class) và thiết bị (device node) tự động
#include <linux/uaccess.h> // cung cấp copy_to_user và copy_from_user (giao tiếp User/Kernel)
#include <linux/string.h> // Hỗ trợ xử lý chuỗi an toàn trong Kernel(strscpy, strlen)

// copy_to_user là hàm dùng để copy dữ liệu từ kernel space sang user space
// copy_from_user là hàm dùng để copy dữ liệu từ user space sang kernel space

// Khai báo thông tin metadata cho module 
MODULE_LICENSE("GPL"); // Phần này bắt buộc phải có để kernel có thể nhận diện module này là GPL | Khai báo giấy phép. Kernel yêu cầu GPL để dùng các symbol nội bộ; nếu thiếu sẽ bị cảnh báo 'tainted kernel'
MODULE_AUTHOR("Thu")
MODULE_DESCRIPTION("Lab 4 bào 1 Character Driver - Cấp phát động")


// Các hằng số định tên và kích thước

#define DEVICE_NAME "lab4.1_thu"// tên thiết bị sẽ hiển thị trong hệ thống
#define CLASS_NAME "bab4_1_class" // tên lớp thiết bị (sẽ xuất hiện trong syn/classs)
#define BUFSIZE 1024 // kích thước bộ nhớ đệms(đóng  vai trò như một bộ nhớ ảo của thiết bị)



// Các biến toàn cục quản lý trạng thái của driver 
static dev_t dev_num;// Biến lưu trữ cặp số hiệu thiết bị(major, minor)
static struct cdev my_cdev; // cấu trúc đại diện cho thiết bị ký tự trong Kernel
static struct class *my_class; // Con trỏ quản lý lớp thiết bị (Device class)
static struct device *my_device; // con trỏ quản lý node thiết bị (device file trong /dev)

static char kernel_buf[BUF_SIZE]; // bộ đệm ảo , lưu trữ dữ liệu thay cho phần cứng thật
static int buf_len =  0; // theo dõi độ dài chuỗi dữ liệu thực tế đang lưu trong buffer
static int open_count = 0; // biến đếm số lần thiết bị được người dùng/ứng dụng mở

//  == Các hàm file operatiosns (xủ lý yêu cầu từ User-space) ==s

// 1. Hàm đc kích hoạt khi user-space dùng lệnh open()

static int dev_open(struct inode *inodep, struct file *filep){
    open_count++;
    pr_info("[%s] Device opened(lần thứ %d)\n", DEVICE_NAME, open_count);
    return 0; // trả về 0 báo hiệu thành công
}
// 2. Hàm dược kích hoạt khi user-space dùng lệnh close()

static int dev_close(struct inode *inodep, struct file *filep){
    pr_info("[%s] device closed\n", DEVICE_NAME);
    return 0;// trả về 0 thông váo thành công
}

// 3. Hàm được kích hoạt khi user-space dùng lệnh read()

static  size_t dev_read(struct file *filep, char __user *buf, size_t len, loff_t *offset){
    int bytes_to_reads;
    // kiểm tra xem vị trí con trỏ đọc (offset) đã vượt quá dữ liệu hiện có hay chưa
    if(*offset >= buf_len){
        return 0; // trả về 0 nghĩa là EOF (end of file ) hết dữ liệu đọc
    }
    // tính toán số bytes có thể đọc (đảm bảo không dọc lố  ra ngoài phần dữ liệu hợp lệ)
    bytes_to_reads = min(int(buf_len - *offset),len(int));
    /*
    Quan trọng copy dữ liệu từ không gian kernel (kernel_buf) sang User(buf)
    Không được sử dụng phép gán bình thường vì vùng nhớ Kernel và User là hoàn toàn cách ly
    
    */ 
    if(copy_to_user(buf,kernel_buf + *offset,bytes_to_reads)){
        prr_err("[%s] Lỗi copy_to_user\n", DEVICE_NAME);
        return -EFAULT; // trả về lỗi bad Address nếu copy thất bại
    }
    *offset += bytes_to_reads; // trả về số bytes thực tế đọc được
    pr_info("[%s] Read %d bytes,offset = %lld\n",DEVICE_NAME,bytes_to_reads,*offset);
    return bytes_to_reads; // trả về số bytes thực tế đã đọc được

}


// 4. Hàm được kích hoạt khi user_space dùng lệnh write() để ghi dữ liệu xuống

static ssize_t dev_write(struct file *filep, const char __user *buf, size_t len, loff_t *offset){
    int bytes_to_writes;
    // đảm bảo số bytes muốn ghi không được làm tràn bộ nhớ đệm (chừa 1 byte cuối cho kí tự kết thúc chuỗi \0)
    bytes_to_writes = min((int)len,(int)(BUF_SIZE -1) )
    // Ghi dữ liệu User(buf) xuống kernel (kernel_buf) một cách an toàn
    if(copy_from_user(kernel_buf,buf,bytes_to_writes)){
        prr_err("[%s] Lỗi copy_from_user\n", DEVICE_NAME);
        return -EFAULT; // trả về lỗi bad Address nếu copy thất bại
    }
    // đảm bảo chuỗi kết thúc bằng kí tự null để thao tác xủ lý chỗi sau này không bị lõi
    kernel_buf[bytes_to_write] = '\0'
    buf_len = bytes_to_writes; // cập nhật  lại kích thước dữ liệu hiện hành trong buffer
    pr_info("[%s] Written %d bytes,offset = %lld\n",DEVICE_NAME,bytes_to_writes,*offset);
    return bytes_to_writes; // trả về số bytes thực tế đã ghi được

}


/*
Ánh xạ file operations (mapping)
Bảng cấu trúc này báo cho kernel biết hàm nào ở trên sẽ chịu trách nghiệm
Xử lý các systems cllas tương ứng  đến từ không gian người dùng

*/ 
static const struct file_operations fops = {
    .owner = THIS_MODULE,// đánh dấu chủ sở hữu ngăn chặn việc gỡ module (rmmod) khi đang có file được mở
    .open  = dev_open, // User gọi open() ==> chạy hàm dev_open()
    .release = dev_release, // user gọi close() ==> chạy hàm dev_release()
    .read = dev_read,// user gọi read() ==> Chạy hàm dev_read()
    .write = dev_write, // user gọi write() ==> Chạy hàm dev_write()
    .llseek = default_llseek, // hỗ trợ  lseek() mặc định để user tua (seek) qua lại trong file
}

//   ======== Hàm khởi  tạo module (chạy 1 lần khi gõ câu lệnh insmod) ====
static int __init lab1_int(void){

    int ret;
    pr_info("[%s] khởi tạo module...\n", DEVICE_NAME);

    /*Bước 1 : Cấp phát động một cặp Device number(major, minor)*/
    // hệ thống sẽ tự quét và cấp phát một major rảnh rỗi bắt đầu từ minor 0 số lượng 1
    ret  = alloc_chrdev_region(&dev_num,0,1,DEVICE_NAME);
    if(ret < 0){
        pr_err("[%s] lỗi allloc_)chrdev_region\n", DEVICE_NAME,ret);
        return ret;
    } 
    // in ra số major và minor được cấp phát để kiểm chứng
    pr_info("%s: major = %d, minor = %d\n", DEVICE_NAME, MAJOR(dev_num), MINOR(dev_num));

    // bước 2 : Khởi tạo và đang kí cấu trúc Character device (cdev)
    cdev_init(&my_cdev,  &fops); // liên kết thiêt bị với bảng file operations (fops) // các hàm đọc ghi

    // bơm thiết bị này vào trong hệ thống kernel 
    ret  = cdev_add(&my_cdev, dev_number,1)

    if(ret < 0){
        pr_err("[%s] lỗi cdev_add\n", DEVICE_NAME,ret);
        return ret;
    }
    
    /* BƯỚC 3: Tạo Device Class (Lớp thiết bị) */
    // Giúp hệ thống tạo thư mục quản lý trong /sys/class/
    my_class = class_create(CLASS_NAME);
    if (IS_ERR(my_class)) {
        pr_err("[%s] Loi class_create\n", DEVICE_NAME);
        ret = PTR_ERR(my_class);
        goto err_class;
    }
   /* BƯỚC 4: Tự động tạo Device Node (File thiết bị) */
    // Bước này nhờ dịch vụ udev của hệ điều hành tự động tạo file trong đường dẫn /dev/lab4.1_thu
    // Giúp người dùng không phải gõ lệnh "mknod" thủ công trên terminal
    my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(my_device)) {
        pr_err("[%s] Loi device_create\n", DEVICE_NAME);
        ret = PTR_ERR(my_device);
        goto err_device;
    }
    
    // Bước 5 : Khởi tạo dữ liệu mặc định cho thiết bị
    strscpy(kernel_buf,"Hello from lab4.1_thu driver\n",BUF_SIZE);
    buf_len = strlen(kernel_buf);
    pr_info("[%s] Module loaded successfully\n", DEVICE_NAME);
    return 0;// trả về 0 báo hiệu thành công

/* XỬ LÝ LỖI: Chỗ này áp dụng tư duy "rollback" */
// Nếu khởi tạo lỗi ở bước nào, nó sẽ nhảy đến đây và dọn dẹp những gì đã tạo ra trước đó
err_device:
    class_destroy(my_class); // xóa lớp thiết bị
err_class:
    cdev_del(&my_cdev); // xóa thiết bị khác từ cdev
err_cdev:
    unregister_chrdev_region(dev_num, 1); // trả lại major number để sử dụng cho các thiết bị khác
    return ret; // trả về lỗi để hệ thống có thể xử lý
}

// === Hàm gỡ bỏ module (chạy một lần duy nhất khi gõ lệnh rmmod)=====
static void __exit bai1_exit(void){

    /* * QUY TẮC DỌN DẸP: "Cái gì tạo ra sau cùng thì phải bị phá hủy đầu tiên" (LIFO)
     * Trật tự dọn dẹp ở đây phải ngược lại chính xác với trật tự lúc init()
     */
     device_destroy(my_class, dev_num);      // 1. Xóa node thiết bị (/dev/lab4.1_thu)
     class_destroy(my_class);                // 2. Xóa lớp quản lý thiết bị
     cdev_del(&my_cdev);                     // 3. Rút cdev ra khỏi hệ thống Kernel
     unregister_chrdev_region(dev_num, 1);   // 4. Trả lại giấy phép số Major/Minor cho Kernel
     
     pr_info("[%s] Module da duoc go\n", DEVICE_NAME);
 }  
 // đang kí 2 hàm init và exit 
 module_init(lab1_int);
 module_exit(bai1_exit);
