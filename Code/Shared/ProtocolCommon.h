#pragma once
#include <QtGlobal>

// Giao thức truyền file: mỗi file dùng 1 socket riêng.
// Client gửi 1 dòng header trước, dạng: "ULD1|tenfile|kichthuoc\n"
// rồi gửi tiếp đúng "kichthuoc" byte dữ liệu file ngay sau đó.
// Server đọc header xong thì biết cần đọc thêm bao nhiêu byte nữa.

namespace Protocol {
const QByteArray MAGIC = "ULD1";
const quint16 DEFAULT_PORT = 5000;
const qint64 CHUNK_SIZE = 65536; // 64KB, đọc/ghi từng phần cho có progress
}
