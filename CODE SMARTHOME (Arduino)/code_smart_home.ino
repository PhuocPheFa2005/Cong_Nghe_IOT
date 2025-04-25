#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

// 1. Cấu hình WiFi và MQTT
const char* ssid = "Fablab 2.4G";
const char* password = "Fira@2024";
const char* mqtt_server = "192.168.69.131"; // Hoặc địa chỉ broker của bạn

WiFiClient espClient;
PubSubClient client(espClient);

// 2. Định nghĩa các chân GPIO
// 2.1. Sân trước
#define chan_PIR 25
#define chan_rung 32
#define chan_coi_truoc 18
#define chan_nut_che_do_coi_truoc 33

// 2.2. Phòng khách + bếp
#define chan_DHT 4
#define chan_lua_digital 15
#define chan_lua_analog 36
#define chan_MQ2_AO 34
#define chan_MQ2_DO 35
#define chan_coi_phongkhach 22
#define chan_nut_che_do_coi_phongkhach 23
#define chan_den_phongkhach 16
#define chan_nut_den_phongkhach 17
#define chan_den_bep 19
#define chan_nut_den_bep 13
#define chan_den_bao_gas 5

// 2.3. Phòng ngủ
#define chan_den_phongngu 27
#define chan_nut_den_phongngu 26

// 2.4. Phòng xe
#define chan_den_phongxe 12
#define chan_nut_den_phongxe 14

#define loai_DHT DHT11
DHT dht(chan_DHT, loai_DHT);

// 3. Biến trạng thái
// 3.1. Trạng thái đèn (0 = Tắt, 1 = Vừa, 2 = Cao)
int trang_thai_den_phongkhach = 0;
int trang_thai_den_bep = 0;
int trang_thai_den_phongngu = 0;
int trang_thai_den_phongxe = 0;

// 3.2. Trạng thái còi
int che_do_coi_truoc = 1;      // Mặc định là chế độ cảnh báo
int che_do_coi_phongkhach = 1; // Mặc định là chế độ cảnh báo

// 3.3. Biến thời gian
unsigned long lan_doc_cambien_cuoi = 0;
const long khoang_thoi_gian_doc_cambien = 2000;

// 3.4. Biến chống nhiễu cho nút nhấn
unsigned long lan_nhan_nut_phongkhach_cuoi = 0;
unsigned long lan_nhan_nut_bep_cuoi = 0;
unsigned long lan_nhan_nut_phongngu_cuoi = 0;
unsigned long lan_nhan_nut_phongxe_cuoi = 0;
unsigned long lan_nhan_nut_che_do_coi_truoc_cuoi = 0;
unsigned long lan_nhan_nut_che_do_coi_phongkhach_cuoi = 0;
const long thoi_gian_cho_chong_nhieu = 300;

bool trang_thai_nut_phongkhach_cuoi = HIGH;
bool trang_thai_nut_bep_cuoi = HIGH;
bool trang_thai_nut_phongngu_cuoi = HIGH;
bool trang_thai_nut_phongxe_cuoi = HIGH;
bool trang_thai_nut_che_do_coi_truoc_cuoi = HIGH;
bool trang_thai_nut_che_do_coi_phongkhach_cuoi = HIGH;

// 3.5. Trạng thái cảm biến
bool phat_hien_lua = false;
int gia_tri_lua_analog = 0;
int nguong_bao_dong_lua = 300;
bool phat_hien_gas = false;
bool phat_hien_chuyen_dong = false;
bool phat_hien_rung = false;

// 3.6. Cường độ ánh sáng
const int SANG_VUA = 150;
const int SANG_CAO = 255;

// 3.7. Biến cho đèn báo gas nhấp nháy
bool trang_thai_den_gas = false;
unsigned long lan_chuyen_den_gas_cuoi = 0;
const long thoi_gian_nhap_nhay_den_gas = 500;

// 3.8. Biến cho còi nhấp nháy
unsigned long lan_chuyen_coi_cuoi = 0;
const long thoi_gian_nhap_nhay_coi_nhanh = 200;
const long thoi_gian_nhap_nhay_coi_cham = 500;
bool trang_thai_coi_phongkhach = false;
bool trang_thai_coi_truoc = false;

// 3.9. Tần số còi
const int tan_so_coi_phongkhach = 4000;
const int tan_so_coi_truoc = 4000;

// 4. Hàm kết nối WiFi
void ket_noi_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Đang kết nối tới WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Đã kết nối WiFi");
  Serial.println("Địa chỉ IP: ");
  Serial.println(WiFi.localIP());
}

// 5. Hàm callback nhận tin nhắn MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Tin nhắn nhận được [");
  Serial.print(topic);
  Serial.print("] ");
  
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Xử lý tin nhắn theo topic
  if (String(topic) == "nha_thong_minh/den_phongkhach/set") {
    if (message == "0") trang_thai_den_phongkhach = 0;
    else if (message == "1") trang_thai_den_phongkhach = 1;
    else if (message == "2") trang_thai_den_phongkhach = 2;
    Serial.print("MQTT - Đèn phòng khách: ");
    switch (trang_thai_den_phongkhach) {
      case 0: Serial.println("TẮT"); break;
      case 1: Serial.println("Mức VỪA"); break;
      case 2: Serial.println("Mức CAO"); break;
    }
    // Cập nhật trạng thái đèn ngay lập tức
    dieu_khien_den();
    // Gửi lại trạng thái để đồng bộ
    client.publish("nha_thong_minh/den_phongkhach/state", String(trang_thai_den_phongkhach).c_str());
  }
  else if (String(topic) == "nha_thong_minh/den_bep/set") {
    if (message == "0") trang_thai_den_bep = 0;
    else if (message == "1") trang_thai_den_bep = 1;
    else if (message == "2") trang_thai_den_bep = 2;
    Serial.print("MQTT - Đèn bếp: ");
    switch (trang_thai_den_bep) {
      case 0: Serial.println("TẮT"); break;
      case 1: Serial.println("Mức VỪA"); break;
      case 2: Serial.println("Mức CAO"); break;
    }
    dieu_khien_den();
    client.publish("nha_thong_minh/den_bep/state", String(trang_thai_den_bep).c_str());
  }
  else if (String(topic) == "nha_thong_minh/den_phongngu/set") {
    if (message == "0") trang_thai_den_phongngu = 0;
    else if (message == "1") trang_thai_den_phongngu = 1;
    else if (message == "2") trang_thai_den_phongngu = 2;
    Serial.print("MQTT - Đèn phòng ngủ: ");
    switch (trang_thai_den_phongngu) {
      case 0: Serial.println("TẮT"); break;
      case 1: Serial.println("Mức VỪA"); break;
      case 2: Serial.println("Mức CAO"); break;
    }
    dieu_khien_den();
    client.publish("nha_thong_minh/den_phongngu/state", String(trang_thai_den_phongngu).c_str());
  }
  else if (String(topic) == "nha_thong_minh/den_phongxe/set") {
    if (message == "0") trang_thai_den_phongxe = 0;
    else if (message == "1") trang_thai_den_phongxe = 1;
    else if (message == "2") trang_thai_den_phongxe = 2;
    Serial.print("MQTT - Đèn phòng xe: ");
    switch (trang_thai_den_phongxe) {
      case 0: Serial.println("TẮT"); break;
      case 1: Serial.println("Mức VỪA"); break;
      case 2: Serial.println("Mức CAO"); break;
    }
    dieu_khien_den();
    client.publish("nha_thong_minh/den_phongxe/state", String(trang_thai_den_phongxe).c_str());
  }
  else if (String(topic) == "nha_thong_minh/che_do_coi_truoc/set") {
    if (message == "0") che_do_coi_truoc = 0;
    else if (message == "1") che_do_coi_truoc = 1;
    else if (message == "2") che_do_coi_truoc = 2;
    Serial.print("MQTT - Chế độ còi sân trước: ");
    switch (che_do_coi_truoc) {
      case 0: Serial.println("TẮT"); break;
      case 1: Serial.println("BẬT KHI CÓ CHUYỂN ĐỘNG/RUNG"); break;
      case 2: Serial.println("LUÔN BẬT"); break;
    }
    client.publish("nha_thong_minh/che_do_coi_truoc/state", String(che_do_coi_truoc).c_str());
  }
  else if (String(topic) == "nha_thong_minh/che_do_coi_phongkhach/set") {
    if (message == "0") che_do_coi_phongkhach = 0;
    else if (message == "1") che_do_coi_phongkhach = 1;
    else if (message == "2") che_do_coi_phongkhach = 2;
    Serial.print("MQTT - Chế độ còi phòng khách: ");
    switch (che_do_coi_phongkhach) {
      case 0: Serial.println("TẮT"); break;
      case 1: Serial.println("BẬT KHI CÓ NGUY HIỂM"); break;
      case 2: Serial.println("LUÔN BẬT"); break;
    }
    client.publish("nha_thong_minh/che_do_coi_phongkhach/state", String(che_do_coi_phongkhach).c_str());
  }
}

// 6. Hàm kết nối lại MQTT nếu mất kết nối
void reconnect() {
  while (!client.connected()) {
    Serial.print("Đang kết nối tới MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Đã kết nối");
      
      // Đăng ký các topic
      client.subscribe("nha_thong_minh/den_phongkhach/set");
      client.subscribe("nha_thong_minh/den_bep/set");
      client.subscribe("nha_thong_minh/den_phongngu/set");
      client.subscribe("nha_thong_minh/den_phongxe/set");
      client.subscribe("nha_thong_minh/che_do_coi_truoc/set");
      client.subscribe("nha_thong_minh/che_do_coi_phongkhach/set");
      
      // Gửi trạng thái ban đầu
      client.publish("nha_thong_minh/den_phongkhach/state", String(trang_thai_den_phongkhach).c_str());
      client.publish("nha_thong_minh/den_bep/state", String(trang_thai_den_bep).c_str());
      client.publish("nha_thong_minh/den_phongngu/state", String(trang_thai_den_phongngu).c_str());
      client.publish("nha_thong_minh/den_phongxe/state", String(trang_thai_den_phongxe).c_str());
      client.publish("nha_thong_minh/che_do_coi_truoc/state", String(che_do_coi_truoc).c_str());
      client.publish("nha_thong_minh/che_do_coi_phongkhach/state", String(che_do_coi_phongkhach).c_str());
    } else {
      Serial.print("Lỗi, rc=");
      Serial.print(client.state());
      Serial.println(" Thử lại sau 5 giây...");
      delay(5000);
    }
  }
}

// 7. Hàm setup
void setup() {
  Serial.begin(115200);

  // Khởi tạo chân GPIO
  // 7.1. Sân trước
  pinMode(chan_PIR, INPUT);
  pinMode(chan_rung, INPUT);
  pinMode(chan_coi_truoc, OUTPUT);
  pinMode(chan_nut_che_do_coi_truoc, INPUT_PULLUP);

  // 7.2. Phòng khách + bếp
  dht.begin();
  pinMode(chan_lua_digital, INPUT);
  pinMode(chan_lua_analog, INPUT);
  pinMode(chan_MQ2_AO, INPUT);
  pinMode(chan_MQ2_DO, INPUT);
  pinMode(chan_den_bao_gas, OUTPUT);
  pinMode(chan_coi_phongkhach, OUTPUT);
  pinMode(chan_nut_che_do_coi_phongkhach, INPUT_PULLUP);
  pinMode(chan_den_phongkhach, OUTPUT);
  pinMode(chan_nut_den_phongkhach, INPUT_PULLUP);
  pinMode(chan_den_bep, OUTPUT);
  pinMode(chan_nut_den_bep, INPUT_PULLUP);

  // 7.3. Phòng ngủ
  pinMode(chan_den_phongngu, OUTPUT);
  pinMode(chan_nut_den_phongngu, INPUT_PULLUP);

  // 7.4. Phòng xe
  pinMode(chan_den_phongxe, OUTPUT);
  pinMode(chan_nut_den_phongxe, INPUT_PULLUP);

  // Tắt tất cả đèn và còi khi khởi động
  analogWrite(chan_den_phongkhach, 0);
  analogWrite(chan_den_bep, 0);
  analogWrite(chan_den_phongngu, 0);
  analogWrite(chan_den_phongxe, 0);
  digitalWrite(chan_den_bao_gas, LOW);
  noTone(chan_coi_truoc);
  noTone(chan_coi_phongkhach);

  // Kết nối WiFi và MQTT
  ket_noi_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("=== HỆ THỐNG NHÀ THÔNG MINH ĐÃ KHỞI ĐỘNG ===");
  Serial.println("Chế độ còi sân trước: BẬT KHI CÓ CHUYỂN ĐỘNG/RUNG");
  Serial.println("Chế độ còi phòng khách/bếp: BẬT KHI CÓ NGUY HIỂM");
  Serial.println("--------------------------------------------");
}

// 8. Hàm loop chính
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long thoi_gian_hien_tai = millis();

  // Đọc cảm biến định kỳ
  if (thoi_gian_hien_tai - lan_doc_cambien_cuoi >= khoang_thoi_gian_doc_cambien) {
    doc_cambien(thoi_gian_hien_tai);
    lan_doc_cambien_cuoi = thoi_gian_hien_tai;
  }

  // Xử lý nút nhấn
  xu_ly_nut_nhan(thoi_gian_hien_tai);
  
  // Điều khiển đèn
  dieu_khien_den();
  
  // Điều khiển còi
  dieu_khien_coi(thoi_gian_hien_tai);
  
  // Điều khiển đèn báo gas
  dieu_khien_den_gas(thoi_gian_hien_tai);

  delay(50);
}

// 9. Hàm đọc cảm biến
void doc_cambien(unsigned long thoi_gian_hien_tai) {
  // Đọc nhiệt độ, độ ẩm
  float do_am = dht.readHumidity();
  float nhiet_do = dht.readTemperature();

  if (!isnan(do_am) && !isnan(nhiet_do)) {
    Serial.print("Thông số môi trường - Độ ẩm: ");
    Serial.print(do_am);
    Serial.print("% | Nhiệt độ: ");
    Serial.print(nhiet_do);
    Serial.println("°C");
    
    // Gửi dữ liệu lên MQTT
    client.publish("nha_thong_minh/cambien/do_am", String(do_am).c_str());
    client.publish("nha_thong_minh/cambien/nhiet_do", String(nhiet_do).c_str());
  }

  // Đọc cảm biến PIR (chuyển động)
  int gia_tri_PIR = digitalRead(chan_PIR);
  bool phat_hien_chuyen_dong_moi = (gia_tri_PIR == HIGH);
  
  if (phat_hien_chuyen_dong_moi && !phat_hien_chuyen_dong) {
    Serial.println("⚠️ CẢNH BÁO: Phát hiện chuyển động đáng ngờ ở sân trước!");
    client.publish("nha_thong_minh/cambien/chuyen_dong", "1");
  }
  phat_hien_chuyen_dong = phat_hien_chuyen_dong_moi;

  // Đọc cảm biến rung
  int gia_tri_rung = digitalRead(chan_rung);
  bool phat_hien_rung_moi = (gia_tri_rung == HIGH);
  
  if (phat_hien_rung_moi && !phat_hien_rung) {
    Serial.println("⚠️⚠️ KHẨN CẤP: Phát hiện rung động mạnh - Có thể động đất!");
    client.publish("nha_thong_minh/cambien/rung", "1");
  }
  phat_hien_rung = phat_hien_rung_moi;

  // Đọc cảm biến lửa
  int gia_tri_lua_digital = digitalRead(chan_lua_digital);
  gia_tri_lua_analog = analogRead(chan_lua_analog);
  bool phat_hien_lua_moi = (gia_tri_lua_digital == HIGH) || (gia_tri_lua_analog < nguong_bao_dong_lua);
  
  if (phat_hien_lua_moi && !phat_hien_lua) {
    Serial.print("🔥🔥🔥 NGUY HIỂM: Phát hiện LỬA phòng khách! Cường độ: ");
    Serial.print(gia_tri_lua_analog);
    Serial.println("Hãy sơ tán ngay!");
    client.publish("nha_thong_minh/cambien/lua", "1");
  }
  phat_hien_lua = phat_hien_lua_moi;

  // Hiển thị giá trị cảm biến lửa
  Serial.print("Thông số cảm biến lửa - Digital: ");
  Serial.print(gia_tri_lua_digital);
  Serial.print(" | Analog: ");
  Serial.println(gia_tri_lua_analog);
  client.publish("nha_thong_minh/cambien/lua_analog", String(gia_tri_lua_analog).c_str());

  // Đọc cảm biến khí gas
  int gia_tri_gas_analog = analogRead(chan_MQ2_AO);
  int gia_tri_gas_digital = digitalRead(chan_MQ2_DO);
  bool phat_hien_gas_moi = (gia_tri_gas_digital == LOW);
  
  if (phat_hien_gas_moi && !phat_hien_gas) {
    Serial.print("⚠️⚠️ NGUY HIỂM: Phát hiện RÒ RỈ KHÍ GAS! Mức độ: ");
    Serial.println(gia_tri_gas_analog);
    Serial.println("Hãy tắt nguồn gas và thông gió ngay!");
    client.publish("nha_thong_minh/cambien/gas", "1");
  }
  phat_hien_gas = phat_hien_gas_moi;
  client.publish("nha_thong_minh/cambien/gas_analog", String(gia_tri_gas_analog).c_str());
}

// 10. Hàm xử lý nút nhấn
void xu_ly_nut_nhan(unsigned long thoi_gian_hien_tai) {
  bool trang_thai_nut_phongkhach = digitalRead(chan_nut_den_phongkhach);
  bool trang_thai_nut_bep = digitalRead(chan_nut_den_bep);
  bool trang_thai_nut_phongngu = digitalRead(chan_nut_den_phongngu);
  bool trang_thai_nut_phongxe = digitalRead(chan_nut_den_phongxe);
  bool trang_thai_nut_che_do_coi_truoc = digitalRead(chan_nut_che_do_coi_truoc);
  bool trang_thai_nut_che_do_coi_phongkhach = digitalRead(chan_nut_che_do_coi_phongkhach);

  // Xử lý nút phòng khách (0-Tắt, 1-Vừa, 2-Cao)
  if (trang_thai_nut_phongkhach == LOW && trang_thai_nut_phongkhach_cuoi == HIGH && 
      (thoi_gian_hien_tai - lan_nhan_nut_phongkhach_cuoi) > thoi_gian_cho_chong_nhieu) {
    trang_thai_den_phongkhach = (trang_thai_den_phongkhach + 1) % 3;
    Serial.print("Điều khiển đèn - Phòng khách: ");
    switch (trang_thai_den_phongkhach) {
      case 0: Serial.println("TẮT"); break;
      case 1: Serial.println("Mức VỪA"); break;
      case 2: Serial.println("Mức CAO"); break;
    }
    // Cập nhật trạng thái đèn ngay lập tức
    dieu_khien_den();
    // Gửi trạng thái mới lên MQTT
    client.publish("nha_thong_minh/den_phongkhach/state", String(trang_thai_den_phongkhach).c_str());
    lan_nhan_nut_phongkhach_cuoi = thoi_gian_hien_tai;
  }
  trang_thai_nut_phongkhach_cuoi = trang_thai_nut_phongkhach;

  // Xử lý nút bếp (0-Tắt, 1-Vừa, 2-Cao)
  if (trang_thai_nut_bep == LOW && trang_thai_nut_bep_cuoi == HIGH && 
      (thoi_gian_hien_tai - lan_nhan_nut_bep_cuoi) > thoi_gian_cho_chong_nhieu) {
    trang_thai_den_bep = (trang_thai_den_bep + 1) % 3;
    Serial.print("Điều khiển đèn - Bếp: ");
    switch (trang_thai_den_bep) {
      case 0: Serial.println("TẮT"); break;
      case 1: Serial.println("Mức VỪA"); break;
      case 2: Serial.println("Mức CAO"); break;
    }
    dieu_khien_den();
    client.publish("nha_thong_minh/den_bep/state", String(trang_thai_den_bep).c_str());
    lan_nhan_nut_bep_cuoi = thoi_gian_hien_tai;
  }
  trang_thai_nut_bep_cuoi = trang_thai_nut_bep;

  // Xử lý nút phòng ngủ (0-Tắt, 1-Vừa, 2-Cao)
  if (trang_thai_nut_phongngu == LOW && trang_thai_nut_phongngu_cuoi == HIGH && 
      (thoi_gian_hien_tai - lan_nhan_nut_phongngu_cuoi) > thoi_gian_cho_chong_nhieu) {
    trang_thai_den_phongngu = (trang_thai_den_phongngu + 1) % 3;
    Serial.print("Điều khiển đèn - Phòng ngủ: ");
    switch (trang_thai_den_phongngu) {
      case 0: Serial.println("TẮT"); break;
      case 1: Serial.println("Mức VỪA"); break;
      case 2: Serial.println("Mức CAO"); break;
    }
    dieu_khien_den();
    client.publish("nha_thong_minh/den_phongngu/state", String(trang_thai_den_phongngu).c_str());
    lan_nhan_nut_phongngu_cuoi = thoi_gian_hien_tai;
  }
  trang_thai_nut_phongngu_cuoi = trang_thai_nut_phongngu;

  // Xử lý nút phòng xe (0-Tắt, 1-Vừa, 2-Cao)
  if (trang_thai_nut_phongxe == LOW && trang_thai_nut_phongxe_cuoi == HIGH && 
      (thoi_gian_hien_tai - lan_nhan_nut_phongxe_cuoi) > thoi_gian_cho_chong_nhieu) {
    trang_thai_den_phongxe = (trang_thai_den_phongxe + 1) % 3;
    Serial.print("Điều khiển đèn - Phòng xe: ");
    switch (trang_thai_den_phongxe) {
      case 0: Serial.println("TẮT"); break;
      case 1: Serial.println("Mức VỪA"); break;
      case 2: Serial.println("Mức CAO"); break;
    }
    dieu_khien_den();
    client.publish("nha_thong_minh/den_phongxe/state", String(trang_thai_den_phongxe).c_str());
    lan_nhan_nut_phongxe_cuoi = thoi_gian_hien_tai;
  }
  trang_thai_nut_phongxe_cuoi = trang_thai_nut_phongxe;

  // Xử lý nút chế độ còi sân trước
  if (trang_thai_nut_che_do_coi_truoc == LOW && trang_thai_nut_che_do_coi_truoc_cuoi == HIGH && 
      (thoi_gian_hien_tai - lan_nhan_nut_che_do_coi_truoc_cuoi) > thoi_gian_cho_chong_nhieu) {
    che_do_coi_truoc = (che_do_coi_truoc + 1) % 3;
    Serial.print("Cài đặt - Chế độ còi sân trước: ");
    switch (che_do_coi_truoc) {
      case 0: Serial.println("TẮT"); break;
      case 1: Serial.println("BẬT KHI CÓ CHUYỂN ĐỘNG/RUNG"); break;
      case 2: Serial.println("LUÔN BẬT"); break;
    }
    client.publish("nha_thong_minh/che_do_coi_truoc/state", String(che_do_coi_truoc).c_str());
    lan_nhan_nut_che_do_coi_truoc_cuoi = thoi_gian_hien_tai;
  }
  trang_thai_nut_che_do_coi_truoc_cuoi = trang_thai_nut_che_do_coi_truoc;

  // Xử lý nút chế độ còi phòng khách/bếp
  if (trang_thai_nut_che_do_coi_phongkhach == LOW && trang_thai_nut_che_do_coi_phongkhach_cuoi == HIGH && 
      (thoi_gian_hien_tai - lan_nhan_nut_che_do_coi_phongkhach_cuoi) > thoi_gian_cho_chong_nhieu) {
    che_do_coi_phongkhach = (che_do_coi_phongkhach + 1) % 3;
    Serial.print("Cài đặt - Chế độ còi phòng khách/bếp: ");
    switch (che_do_coi_phongkhach) {
      case 0: Serial.println("TẮT"); break;
      case 1: Serial.println("BẬT KHI CÓ NGUY HIỂM"); break;
      case 2: Serial.println("LUÔN BẬT"); break;
    }
    client.publish("nha_thong_minh/che_do_coi_phongkhach/state", String(che_do_coi_phongkhach).c_str());
    lan_nhan_nut_che_do_coi_phongkhach_cuoi = thoi_gian_hien_tai;
  }
  trang_thai_nut_che_do_coi_phongkhach_cuoi = trang_thai_nut_che_do_coi_phongkhach;
}

// 11. Hàm điều khiển đèn
void dieu_khien_den() {
  // Điều khiển đèn phòng khách
  switch (trang_thai_den_phongkhach) {
    case 0: analogWrite(chan_den_phongkhach, 0); break;
    case 1: analogWrite(chan_den_phongkhach, SANG_VUA); break;
    case 2: analogWrite(chan_den_phongkhach, SANG_CAO); break;
  }

  // Điều khiển đèn bếp
  switch (trang_thai_den_bep) {
    case 0: analogWrite(chan_den_bep, 0); break;
    case 1: analogWrite(chan_den_bep, SANG_VUA); break;
    case 2: analogWrite(chan_den_bep, SANG_CAO); break;
  }

  // Điều khiển đèn phòng ngủ
  switch (trang_thai_den_phongngu) {
    case 0: analogWrite(chan_den_phongngu, 0); break;
    case 1: analogWrite(chan_den_phongngu, SANG_VUA); break;
    case 2: analogWrite(chan_den_phongngu, SANG_CAO); break;
  }

  // Điều khiển đèn phòng xe
  switch (trang_thai_den_phongxe) {
    case 0: analogWrite(chan_den_phongxe, 0); break;
    case 1: analogWrite(chan_den_phongxe, SANG_VUA); break;
    case 2: analogWrite(chan_den_phongxe, SANG_CAO); break;
  }
}

// 12. Hàm điều khiển còi
void dieu_khien_coi(unsigned long thoi_gian_hien_tai) {
  // Điều khiển còi sân trước (PIR/rung)
  if (che_do_coi_truoc == 2) {
    // Chế độ luôn bật
    tone(chan_coi_truoc, tan_so_coi_truoc);
  } 
  else if (che_do_coi_truoc == 1 && (phat_hien_chuyen_dong || phat_hien_rung)) {
    // Chế độ cảnh báo - nhấp nháy khi có chuyển động/rung
    if (thoi_gian_hien_tai - lan_chuyen_coi_cuoi >= thoi_gian_nhap_nhay_coi_cham) {
      trang_thai_coi_truoc = !trang_thai_coi_truoc;
      if (trang_thai_coi_truoc) {
        tone(chan_coi_truoc, tan_so_coi_truoc);
      } else {
        noTone(chan_coi_truoc);
      }
      lan_chuyen_coi_cuoi = thoi_gian_hien_tai;
    }
  } 
  else {
    noTone(chan_coi_truoc);
  }

  // Điều khiển còi phòng khách/bếp (lửa/khí gas)
  if (che_do_coi_phongkhach == 2) {
    // Chế độ luôn bật
    tone(chan_coi_phongkhach, tan_so_coi_phongkhach);
  } 
  else if (che_do_coi_phongkhach == 1 && (phat_hien_lua || phat_hien_gas)) {
    // Chế độ cảnh báo - nhấp nháy nhanh khi có lửa/khí gas
    if (thoi_gian_hien_tai - lan_chuyen_coi_cuoi >= thoi_gian_nhap_nhay_coi_nhanh) {
      trang_thai_coi_phongkhach = !trang_thai_coi_phongkhach;
      if (trang_thai_coi_phongkhach) {
        tone(chan_coi_phongkhach, tan_so_coi_phongkhach);
      } else {
        noTone(chan_coi_phongkhach);
      }
      lan_chuyen_coi_cuoi = thoi_gian_hien_tai;
    }
  } 
  else {
    noTone(chan_coi_phongkhach);
  }
}

// 13. Hàm điều khiển đèn báo gas
void dieu_khien_den_gas(unsigned long thoi_gian_hien_tai) {
  // Đèn báo gas nhấp nháy khi phát hiện khí gas
  if (phat_hien_gas) {
    if (thoi_gian_hien_tai - lan_chuyen_den_gas_cuoi >= thoi_gian_nhap_nhay_den_gas) {
      trang_thai_den_gas = !trang_thai_den_gas;
      digitalWrite(chan_den_bao_gas, trang_thai_den_gas ? HIGH : LOW);
      lan_chuyen_den_gas_cuoi = thoi_gian_hien_tai;
    }
  } 
  else {
    digitalWrite(chan_den_bao_gas, LOW);
  }
}