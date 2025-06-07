//================================================================================
// ส่วนของการ Include Libraries
//================================================================================
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <MQUnifiedsensor.h>
#include <base64.h>
#include "secrets.h" // <-- เพิ่มเข้ามาเพื่อดึงข้อมูลสำคัญจากไฟล์ secrets.h

//================================================================================
// ส่วนของการตั้งค่าคงที่ (Constants)
//================================================================================
// --- การตั้งค่าบอร์ดและเซ็นเซอร์ ---
const char* BOARD_TYPE = "ESP-32";
const char* SENSOR_TYPE = "MQ-2";
const float VOLTAGE_RESOLUTION = 3.3;
const int ADC_BIT_RESOLUTION = 12;
const int SENSOR_PIN = 34; // GPIO 34
const float RATIO_MQ2_CLEAN_AIR = 9.83;

// --- การตั้งค่าการทำงาน ---
const int SMOKE_THRESHOLD_PPM = 200;   // ระดับควัน (PPM) ที่จะให้โทรแจ้งเตือน
const unsigned long LOOP_DELAY_MS = 5000; // หน่วงเวลา 5 วินาทีในแต่ละรอบการทำงาน

//================================================================================
// ส่วนของการสร้าง Object และตัวแปร Global
//================================================================================
float smoke; // ตัวแปรสำหรับ Arduino Cloud

WiFiConnectionHandler ArduinoIoTPreferredConnection(WIFI_SSID, WIFI_PASSWORD);
MQUnifiedsensor MQ2(BOARD_TYPE, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, SENSOR_PIN, SENSOR_TYPE);

//================================================================================
// ฟังก์ชัน Setup: ทำงานครั้งเดียวเมื่อเปิดเครื่อง
//================================================================================
void setup() {
  Serial.begin(115200);
  delay(1500); // รอให้ Serial Monitor พร้อม

  setupMqSensor();
  setupArduinoCloud();

  Serial.println("=========================================");
  Serial.println("Setup Complete. System is now running.");
  Serial.println("=========================================");
}

//================================================================================
// ฟังก์ชัน Loop: ทำงานวนซ้ำไปเรื่อยๆ
//================================================================================
void loop() {
  ArduinoCloud.update(); // สิ่งสำคัญที่สุด! ต้องเรียกใช้เสมอเพื่อให้เชื่อมต่อกับ Cloud

  // 1. อ่านค่าจากเซ็นเซอร์เพียงครั้งเดียวต่อรอบ
  MQ2.update();
  float currentSmokePPM = MQ2.readSensor();
  smoke = currentSmokePPM; // อัปเดตค่าไปยังตัวแปรของ Cloud

  Serial.print("Smoke Level: ");
  Serial.print(currentSmokePPM);
  Serial.println(" PPM");

  // 2. ส่งข้อมูลไปยัง Google Apps Script
  sendToGoogleScript(currentSmokePPM);

  // 3. ตรวจสอบว่าระดับควันเกินค่าที่กำหนดหรือไม่
  if (currentSmokePPM > SMOKE_THRESHOLD_PPM) {
    Serial.println("SMOKE DETECTED! Threshold exceeded. Triggering call...");
    triggerTwilioCall();
  }

  // 4. หน่วงเวลาก่อนเริ่มรอบถัดไป
  delay(LOOP_DELAY_MS);
}

//================================================================================
// ฟังก์ชันย่อยต่างๆ (Helper Functions)
//================================================================================

/**
 * @brief ตั้งค่าเริ่มต้นและ Calibrate เซ็นเซอร์ MQ-2
 */
void setupMqSensor() {
  Serial.println("--- Initializing MQ-2 Sensor ---");
  // ตั้งค่า Regression-Method สำหรับการคำนวณค่า PPM ของควัน
  MQ2.setRegressionMethod(1); // 1 สำหรับ Linear, 0 สำหรับ Exponential
  MQ2.setA(605.18);
  MQ2.setB(-3.937);
  MQ2.init();

  // ทำการ Calibrate เซ็นเซอร์
  Serial.print("Calibrating sensor, please wait.");
  float calcR0 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ2.update();
    calcR0 += MQ2.calibrate(RATIO_MQ2_CLEAN_AIR);
    Serial.print(".");
  }
  MQ2.setR0(calcR0 / 10);
  Serial.println(" Done!");

  // ตรวจสอบความผิดพลาดในการเชื่อมต่อเซ็นเซอร์
  if (isinf(calcR0)) {
    Serial.println("Warning: Connection issue, R0 is infinite. Please check wiring.");
    while (1);
  }
  if (calcR0 == 0) {
    Serial.println("Warning: Connection issue, R0 is zero. Please check wiring.");
    while (1);
  }
  Serial.println("MQ-2 Sensor calibration complete.");
  // MQ2.serialDebug(true); // หากต้องการดูข้อมูล Debug ของเซ็นเซอร์ ให้เปิดใช้งานบรรทัดนี้
}

/**
 * @brief ตั้งค่าและเชื่อมต่อกับ Arduino IoT Cloud
 */
void setupArduinoCloud() {
  Serial.println("--- Connecting to Arduino IoT Cloud ---");
  initProperties(); // กำหนด Properties ของ Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
}

/**
 * @brief ส่งค่า PPM ไปยัง Google Apps Script
 * @param ppmValue ค่าควัน (PPM) ที่ต้องการส่ง
 */
void sendToGoogleScript(float ppmValue) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // สร้าง URL ให้ถูกต้อง โดยมีการส่งค่าเป็น parameter
    String url = String(APPS_SCRIPT_URL) + "?smoke=" + String(ppmValue);

    Serial.println("Sending data to Google Script: " + url);

    http.begin(url.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Google Script Response Code: " + String(httpCode));
      Serial.println("Google Script Response Payload: " + payload);
    } else {
      Serial.println("Google Script request failed.");
    }
    http.end();
  } else {
    Serial.println("Cannot send to Google Script, WiFi not connected.");
  }
}

/**
 * @brief โทรออกผ่าน Twilio API
 */
void triggerTwilioCall() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot make call, WiFi not connected.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure(); // ใช้สำหรับ Test เท่านั้น, สำหรับ Production ควรใช้ Certificate

  HTTPClient http;

  String path = "/2010-04-01/Accounts/" + String(TWILIO_ACCOUNT_SID) + "/Calls.json";
  String url = "https://api.twilio.com" + path;
  
  if (http.begin(client, url)) {
    String auth = base64::encode(String(TWILIO_ACCOUNT_SID) + ":" + String(TWILIO_AUTH_TOKEN));
    String postData = "To=" + urlEncode(String(TO_NUMBER)) + "&From=" + urlEncode(String(TWILIO_NUMBER)) + "&Url=" + urlEncode(String(TWIML_URL));

    http.addHeader("Authorization", "Basic " + auth);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    Serial.println("Making API call to Twilio...");
    int httpCode = http.POST(postData);

    if (httpCode > 0) {
      String response = http.getString();
      Serial.println("Twilio API Response Code: " + String(httpCode));
      Serial.println("Twilio API Response: " + response);
      if (httpCode == 201) {
        Serial.println("Call initiated successfully!");
      } else {
        Serial.println("Call failed.");
      }
    } else {
      Serial.printf("Twilio API request failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();

  } else {
    Serial.println("Could not connect to Twilio API.");
  }
}

/**
 * @brief เข้ารหัส String สำหรับใช้ใน URL
 * @param str String ที่ต้องการเข้ารหัส
 * @return String ที่เข้ารหัสแล้ว
 */
String urlEncode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }
  return encodedString;
}

// ฟังก์ชันนี้ถูกเรียกโดย ArduinoCloud.begin()
void initProperties() {
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.addProperty(smoke, READ, ON_CHANGE, NULL);
}

// ไม่จำเป็นต้องใช้ฟังก์ชัน ReadSmoke() แยกแล้ว เพราะเราอ่านค่าโดยตรงใน loop()
// void ReadSmoke() {
//  smoke = MQ2.readSensor();
// }
void ReadSmoke() {
  smoke = MQ2.readSensor();
}
