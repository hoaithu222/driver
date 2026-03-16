/*
Bài 1 : Character Driver - Cấp phát device number động
- Tên driver : lab4.1_thu
* Tóm tắt chức năng : 
- Sử dụng alloc_chrdev_region() để xin kernel cấp phát tự động một major number.
- Sử dụng class_create() và device_create() để tự động tạo file thiết bị trong thư mục /dev
- Hỗ trợ các thao tác cơ bản từ user-space : open(mở), release(đóng), read(đọc), write(ghi)
*/ 


// khu vực này dùng để cấu hình import các thư viện 
#include <linux/module.h>  // thư viện cốt lõi cho mọi kernke module(hỗ trợ module_license)
#include <linux/kernel.h> // cung cấp các hàm nhân cơ bản như pr_info(), pr_err()
#include <linux/init.h> // cung cấp các macro __init và __exit 
#include <linux/fs.h> // thư viện hỗ trợ cấu trúc file (như file_operations)
#include <linux/cdev.h> // Cung cấp cấu trúc và hàm quản lý Character Device(cdev)
