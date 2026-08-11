# UDM_10

## 👥 Thanh vien nhom

| STT | Ho Và Ten | MSSV | Tai Khoan github | Vai Tro |

| 1 | Nguyen Duong Gia Khanh | 056307008420 | @Gkhanh557 | Truong nhom
| 2 | Huynh Xuan Canh | 056206010544 | @canh0502 | Thanh Vien
| 3 | Nguyen Huu Quoc Huy | 056207008300 | @username_huy | Thanh Vien
| 4 | Nguyen Quoc Thanh | 060207007400 | @nguyenquocthanh599-png | Thanh Vien

## Kiến trúc hệ thống

- Mo hinh: Client – Server
- Giao thuc tang van chuyen: TCP
- Port mac dinh: `5000` (co the thay doi qua GUI, khong hard-code)
- Cau truc message: moi lan upload 1 file dung 1 ket noi TCP rieng. Client gui 1 dong
  header dang text `ULD1 <ten_file_percent_encoded> <kich_thuoc>\n`, theo sau la du lieu
  nhi phan tho cua file. Mo ta chi tiet: Dinh nghia giao thuc truyen file dung chung cho Server va Client.

Mo ta giao thuc (mo ta nay dung de viet vao bao cao, muc "Cau truc message"):
Moi lan upload 1 file se dung 1 ket noi TCP rieng (1 file = 1 socket).
Sau khi ket noi thanh cong, Client gui 1 dong HEADER dang text, ket thuc bang '\n':
ULD1 <fileName_percent_encoded> <fileSize>\n
Trong do:
- "ULD1": magic string de Server nhan dien dung giao thuc (version 1)
- fileName_percent_encoded : ten file (khong kem duong dan), da percent-encode (giong encode URL) de tranh loi khi ten file co khoang trang hoac ky tu dac biet / tieng Viet.
- fileSize: kich thuoc file tinh theo byte (so nguyen)
Ngay sau dong header, Client gui lien tuc dung fileSize byte du lieu nhi phan cua file. Server doc du fileSize byte thi coi nhu nhan file xong.
Vi du header thuc te:
ULD1 bao%20cao.pdf 1048576\n
