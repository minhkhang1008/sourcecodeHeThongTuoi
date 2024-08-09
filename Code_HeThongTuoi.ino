#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
//Gọi Các thư viện sử dụng trong code

char auth[] = "imgu6edEocKEFuzc0SlFWpnzbVEf9GEd";
char ssid[] = "Unlimited";
char pass[] = "Hoanganhkhang1@";
//Kết nối với dự án trên Blynk và kết nối Internet

#define SOIL_SENSOR_PIN A0
#define WATER_PUMP 0
#define DHT_PIN 4
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);
BlynkTimer timer;
//Các cổng kết nối máy bơm/cảm biến

bool pumping = false;
bool manualControl = false;
int moistureStartThreshold = 60; // Giá trị bắt đầu bơm mặc định
int moistureStopThreshold = 75;  // Giá trị dừng bơm mặc định
// Thiết lập giá trị mặc định

// Cập nhật hiển thị các thông số trên ứng dụng Blynk
void updateSettingsDisplay() {
  Blynk.virtualWrite(V0, "Tự thiết lập thông số riêng cho cây trồng");
  Blynk.virtualWrite(V2, moistureStartThreshold);  // Đồng bộ ngưỡng độ ẩm bắt đầu với ứng dụng
  Blynk.virtualWrite(V15, moistureStopThreshold);  // Đồng bộ ngưỡng độ ẩm dừng với ứng dụng
  Blynk.virtualWrite(V9, "Máy sẽ giữ độ ẩm đất");
  Blynk.virtualWrite(V10, String("Trong khoảng từ ") + moistureStartThreshold + "%" + " đến " + moistureStopThreshold + "%");
}

// Kiểm tra độ ẩm đất và các điều kiện môi trường
void checkSoilAndEnvironment() {
  int soilMoistureValue = analogRead(SOIL_SENSOR_PIN);
  int moisturePercent = map(soilMoistureValue, 1023, 0, 0, 100);
  Blynk.virtualWrite(V1, moisturePercent);
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  Blynk.virtualWrite(V5, (int)humidity); 
  Blynk.virtualWrite(V6, (int)temperature);   
  if (!manualControl) {
    // Điều khiển bơm tự động dựa trên độ ẩm đất
    if (moisturePercent < moistureStartThreshold && !pumping) {
      digitalWrite(WATER_PUMP, HIGH);  // Bật bơm
      pumping = true;
      Blynk.virtualWrite(V13, "Đang bơm");
      Blynk.virtualWrite(V14, "Tự động");
    } else if (moisturePercent >= moistureStopThreshold && pumping) {
      digitalWrite(WATER_PUMP, LOW);  // Tắt bơm
      pumping = false;
      Blynk.virtualWrite(V13, "Đang");
      Blynk.virtualWrite(V14, "Không Bơm");
    }
  } else {
    // Hiển thị điều khiển thủ công
    if (pumping) {
      Blynk.virtualWrite(V13, "Đang bơm");
      Blynk.virtualWrite(V14, "Thủ công");
    } else {
      Blynk.virtualWrite(V13, "Đang");
      Blynk.virtualWrite(V14, "Không Bơm");
    }
  }
}
// Đảo ngược giá trị nếu (Bắt đầu Bơm) > (Dừng Bơm)
void validateThresholds() {
  if (moistureStartThreshold > moistureStopThreshold) {
    int hold = moistureStartThreshold;
    moistureStartThreshold = moistureStopThreshold;
    moistureStopThreshold = hold;
  }
}
// Xử lý cập nhật từ ứng dụng (ngưỡng độ ẩm bắt đầu)
BLYNK_WRITE(V2) {
  moistureStartThreshold = param.asInt();
  validateThresholds();  // Đảm bảo ngưỡng bắt đầu nhỏ hơn hoặc bằng ngưỡng dừng
  Blynk.virtualWrite(V11, "Đang thiết lập");
  Blynk.virtualWrite(V12, "Thủ công");
  updateSettingsDisplay();
}

// Xử lý cập nhật từ ứng dụng (ngưỡng độ ẩm dừng)
BLYNK_WRITE(V15) {
  moistureStopThreshold = param.asInt();
  validateThresholds();  // Đảm bảo ngưỡng dừng lớn hơn hoặc bằng ngưỡng bắt đầu
  Blynk.virtualWrite(V11, "Đang thiết lập");
  Blynk.virtualWrite(V12, "Thủ công");
  updateSettingsDisplay();
}

// Xử lý điều khiển bơm thủ công
BLYNK_WRITE(V3) {
  if (!manualControl) return;  
  if (pumping) {
    digitalWrite(WATER_PUMP, LOW);  // Tắt bơm
    pumping = false;
    Blynk.virtualWrite(V13, "Đang");
    Blynk.virtualWrite(V14, "Không Bơm");
  } else {
    digitalWrite(WATER_PUMP, HIGH);  // Bật bơm
    pumping = true;
    Blynk.virtualWrite(V13, "Đang bơm");
    Blynk.virtualWrite(V14, "Thủ công");
  }
}

// Xử lý cập nhật từ nút cập nhật dữ liệu (kiểm tra độ ẩm và điều kiện môi trường)
BLYNK_WRITE(V4) {
  checkSoilAndEnvironment();
}
// Xử lý chuyển đổi chế độ giữa tự động và thủ công
BLYNK_WRITE(V7) {
  int switchState = param.asInt();
  manualControl = switchState == 0; // Đặt chế độ điều khiển thủ công dựa trên V7
  digitalWrite(WATER_PUMP, LOW);  // Tắt bơm
  pumping = false;
  Blynk.virtualWrite(V13, "Đang");
  Blynk.virtualWrite(V14, "Không Bơm");
  Blynk.virtualWrite(V3, 0);  // Đặt lại V3 thành tắt
  Blynk.setProperty(V3, "offLabel", manualControl ? "Tắt" : "Tự động");
  Blynk.setProperty(V3, "onLabel", manualControl ? "Bật" : "Tự động");
  Blynk.syncVirtual(V3); // Đồng bộ nhãn đã cập nhật đến ứng dụng
}

// Xử lý cập nhật từ chân ảo V8 (loại cây)
BLYNK_WRITE(V8) {
  int plantType = param.asInt();
  if (plantType == 1) {
    moistureStartThreshold = 70; 
    moistureStopThreshold = 80;  
    Blynk.virtualWrite(V2, moistureStartThreshold);  // Đồng bộ với ứng dụng
    Blynk.virtualWrite(V15, moistureStopThreshold);  // Đồng bộ với ứng dụng
    Blynk.virtualWrite(V10, String("Trong khoảng từ ") + moistureStartThreshold + "%" + " đến " + moistureStopThreshold + "%");
    Blynk.virtualWrite(V11, String("Cây đang chọn:"));
    Blynk.virtualWrite(V12, String("cây hoa hồng"));
    updateSettingsDisplay();
  } else if (plantType == 2) {
    moistureStartThreshold = 80;  
    moistureStopThreshold = 85;   
    Blynk.virtualWrite(V2, moistureStartThreshold);  // Đồng bộ với ứng dụng
    Blynk.virtualWrite(V15, moistureStopThreshold);  // Đồng bộ với ứng dụng
    Blynk.virtualWrite(V10, String("Trong khoảng từ ") + moistureStartThreshold + "%" + " đến " + moistureStopThreshold + "%");
    Blynk.virtualWrite(V11, String("Cây đang chọn:"));
    Blynk.virtualWrite(V12, String("hành tây"));
    updateSettingsDisplay();
  }
}

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass, "blynk-server.com", 8080); //Sử dụng server riêng blynk-server.con (cổng 8080) để được sử dụng Blynk không giới hạn
  pinMode(SOIL_SENSOR_PIN, INPUT);
  pinMode(WATER_PUMP, OUTPUT);
  digitalWrite(WATER_PUMP, LOW);  // Đảm bảo bơm tắt khi vừa kết nối với Wi-Fi và Server
  dht.begin();
  timer.setInterval(1000L, checkSoilAndEnvironment);
  Blynk.virtualWrite(V0, "Tự thiết lập thông số riêng cho cây trồng");
  updateSettingsDisplay();
}

void loop() {
  Blynk.run();
  timer.run();
} //Lặp lại quá trình chạy
// Lưu ý: Esp8266 và relay khi vừa cắm điện sẽ bật relay (có nghĩa là máy bơm sẽ được bật ngay khi cắm điện), hệ thống sẽ trở lại bình thường ngay khi có kết nối với server Blynk. Đây là vấn đề phần cứng và không thể giải quyết bằng code.