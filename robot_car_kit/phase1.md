# Giai đoạn 1: Tương tác Cơ bản và Theo dõi Khoảng cách (06/07 - 10/07)

Mục tiêu chính của tuần đầu tiên là thiết lập giao tiếp phản hồi nhanh giữa các tương tác vật lý bên ngoài và hệ thống động cơ của xe robot. Robot phải nhận diện chính xác khoảng cách của bàn tay và thực hiện các chuyển động phản hồi tương ứng.

- **Hiệu chuẩn theo dõi tay**: Thiết lập các cảm biến trên xe (như cảm biến hồng ngoại hoặc cảm biến khoảng cách siêu âm) để đo lường cự ly.
- **Cơ chế phản hồi động**: Lập trình logic sao cho xe robot giữ một khoảng cách cố định; tự động lùi lại khi tay tiến tới gần và đi về phía trước khi tay rút ra xa.
- **Giới hạn an toàn**: Cấu hình giới hạn tăng tốc tối đa và tối thiểu để ngăn ngừa việc va chạm ngoài ý muốn.

1. Rủi ro nhiễu tín hiệu (Sensor Noise) khiến xe bị giật cục
Cảm biến siêu âm hoặc hồng ngoại rất dễ bị dội sóng ảo hoặc nhiễu ánh sáng môi trường. Chỉ cần 1 khung hình (1 sample) đọc về bị lỗi (ví dụ: đang ở 20cm đột ngột đọc ra 0cm hoặc 150cm trong 1 mili-giây), xe của bạn sẽ bị khựng, dừng đột ngột hoặc giật mạnh ra phía sau rồi mới chạy tiếp.

Giải pháp: Cần thêm bộ lọc nhiễu số (như bộ lọc thông thấp EMA - Exponential Moving Average) trực tiếp trước khi xử lý khoảng cách.

2. Hiện tượng đổi hướng đột ngột gây hỏng động cơ và sụt nguồn
Nếu tay người dùng di chuyển nhanh từ vùng < lowerLimit (lùi) sang > upperLimit (tiến), động cơ sẽ bị ép đảo chiều ngay lập tức. Dòng điện ngược từ động cơ sẽ tăng vọt (Inrush Current), rất dễ làm sụt áp nguồn nuôi (khiến Arduino bị tự động reset) hoặc làm vỡ bánh răng cơ khí của xe.

Giải pháp: Thiết lập thuật toán tăng tốc/giảm tốc từ từ (Ramping) hoặc ép xe dừng hẳn trong vài mili-giây trước khi đổi hướng di chuyển.

3. Hàm getDistance() có nguy cơ gây nghẽn (Blocking)
Nếu hàm getDistance() bên trong code của bạn đang sử dụng lệnh pulseIn() của Arduino mà không cấu hình thời gian chờ tối đa (timeout), nó sẽ bị treo khoảng 1 giây mỗi khi không nhận được sóng siêu âm phản hồi. Điều này làm cho hệ thống phản ứng cực kỳ chậm trễ.