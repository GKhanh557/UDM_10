# UDM_10

## 👥 Thanh vien nhom

| STT | Ho Và Ten | MSSV | tai khoan github | Vai Tro |

| Nguyen Duong Gia Khanh | 056307008420 | @Gkhanh557 | Truong nhom
| Huynh Xuan Canh | 056206010544 | @canh0502 | Thanh Vien
| Nguyen Huu Quoc Huy | 056207008300 | @username_huy | Thanh Vien
| Nguyen Quoc Thanh | 060207007400 | @nguyenquocthanh599-png | Thanh Vien

## Kiến trúc hệ thống

- Mo hinh: Client – Server
- Giao thuc tang van chuyen: TCP
- Port mac dinh: `5000` (co the thay doi qua GUI, khong hard-code)
- Cau truc message: moi lan upload 1 file dung 1 ket noi TCP rieng. Client gui 1 dong
  header dang text `ULD1 <ten_file_percent_encoded> <kich_thuoc>\n`, theo sau la du lieu
  nhi phan tho cua file. Mo ta chi tiet: Dinh nghia giao thuc truyen file dung chung cho Server va Client.

Mô tả giao thức (mô tả này dùng để viết vào báo cáo, mục "Cấu trúc message"):

Mỗi lần upload 1 file sẽ dùng 1 kết nối TCP riêng (1 file = 1 socket).
Sau khi kết nối thành công, Client gửi 1 dòng HEADER dạng text, kết thúc bằng '\n':
ULD1 <fileName_percent_encoded> <fileSize>\n
Trong đó:
- "ULD1": magic string để Server nhận diện đúng giao thức (version 1)
- fileName_percent_encoded : tên file (không kèm đường dẫn), đã percent-encode (giống encode URL) để tránh lỗi khi tên file có khoảng trắng hoặc ký tự đặc biệt / tiếng Việt.
- fileSize: kích thước file tính theo byte (số nguyên)
Ngay sau dòng header, Client gửi liên tục đúng fileSize byte dữ liệu nhị phân của file. Server đọc đủ fileSize byte thì coi như nhận file xong.
Ví dụ header thực tế:
ULD1 bao%20cao.pdf 1048576\n
