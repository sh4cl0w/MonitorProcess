
# ⚙️ MonitorProcess Driver

---

## 🧩 Giới thiệu

**MonitorProcess** là một Windows Kernel Driver có chức năng **giám sát và phân tích các tiến trình thực thi lệnh trên hệ thống**, đặc biệt là **cmd.exe** và **powershell.exe**.  
Khi phát hiện các tiến trình này được khởi tạo, driver sẽ **tạm dừng tiến trình trong 5 giây**, **ghi nhận câu lệnh được thực thi**, sau đó **cho phép tiến trình tiếp tục hoạt động bình thường**.

---

## 🧱 Chức năng chính

- 🔍 **Theo dõi tiến trình tạo mới** thông qua `PsSetCreateProcessNotifyRoutineEx`.
- 🧠 **Nhận diện tiến trình đặc biệt** (`cmd.exe`, `powershell.exe`) theo tên image.
- ⏸️ **Tạm dừng tiến trình trong 5 giây** bằng cách:
  - Gửi tín hiệu `SuspendProcess(PID)` hoặc sử dụng `ZwSuspendProcess`.
- 📜 **Ghi lại câu lệnh thực thi** (Command Line) bằng cách:
  - Truy cập `PEB` → `RTL_USER_PROCESS_PARAMETERS.CommandLine`.
- ▶️ **Tiếp tục tiến trình sau khi trích xuất xong dữ liệu** (`ZwResumeProcess`).
- 🪵 **Ghi log ra file hoặc output debug** để kiểm tra hành vi.

---

## 🧰 Môi trường phát triển

| Thành phần | Phiên bản khuyến nghị |
|-------------|------------------------|
| 💻 **Hệ điều hành** | Windows 10 / 11 (x64) |
| ⚙️ **Visual Studio** | 2019 hoặc 2022 |
| 🧩 **WDK (Windows Driver Kit)** | 10.0.22621 trở lên |
| 🧱 **Ngôn ngữ** | C / C++ (Kernel Mode) |

---

## ⚙️ Cách hoạt động

1. **Driver đăng ký callback**
   - Sử dụng `PsSetCreateProcessNotifyRoutineEx` để nhận thông tin khi tiến trình được tạo.
2. **Kiểm tra tên tiến trình**
   - Nếu `ImageFileName` là `cmd.exe` hoặc `powershell.exe`, driver sẽ tiếp tục xử lý.
3. **Tạm dừng tiến trình**
   - Gọi `ZwSuspendProcess` để dừng tạm thời trong 5 giây.
4. **Đọc command line**
   - Truy cập `PEB->ProcessParameters->CommandLine` để lấy lệnh mà tiến trình chuẩn bị thực thi.
5. **Ghi log**
   - Lưu câu lệnh ra `DbgPrint` .
6. **Tiếp tục tiến trình**
   - Sau 5 giây, driver gọi `ZwResumeProcess` để cho phép tiến trình tiếp tục hoạt động.
## 🚀 Cách cài và chạy

### 1️⃣ Biên dịch driver
- Mở project bằng Visual Studio + WDK
- Chọn cấu hình: `Release x64`
- Build → tạo file `MonitorProcess.sys`

### 2️⃣ Cài driver
Chạy dưới quyền Administrator:
```cmd
sc create MonitorProcess type= kernel binPath= "C:\Path\To\MonitorProcess.sys"
sc start MonitorProcess

