# โครงการตรวจจับควันด้วย ESP32 และ Twilio

โปรเจ็กต์นี้เป็นตัวอย่างการสร้างเครื่องตรวจจับควันด้วยบอร์ด **ESP32** และเซ็นเซอร์ **MQ-2** เมื่อระดับควันเกินค่าที่กำหนด ระบบจะโทรแจ้งเตือนอัตโนมัติผ่าน **Twilio Programmable Voice API** พร้อมทั้งเชื่อมต่อกับ **Arduino IoT Cloud** เพื่อดูข้อมูลแบบเรียลไทม์ และบันทึกข้อมูลลง **Google Sheets** ผ่าน Apps Script

## คุณสมบัติหลัก
- ตรวจจับความเข้มข้นของควันแบบเรียลไทม์ด้วยเซ็นเซอร์ MQ-2
- แสดงผลข้อมูลบน Dashboard ของ Arduino IoT Cloud
- โทรแจ้งเตือนอัตโนมัติผ่าน Twilio Voice API เมื่อระดับควันเกินเกณฑ์
- (เสริม) ส่งข้อมูลเพื่อบันทึกลงใน Google Sheets

## สิ่งที่ต้องเตรียม
### ฮาร์ดแวร์ (Hardware)
- บอร์ดพัฒนา ESP32
- เซ็นเซอร์ตรวจจับควันและแก๊ส MQ-2
- สายไฟเชื่อมต่อ (Jumper Wires)
- Breadboard

### ซอฟต์แวร์และบริการ (Software & Services)
- Arduino IDE พร้อมติดตั้ง ESP32 Board Manager
- Libraries ที่จำเป็น
  - ArduinoIoTCloud
  - Arduino_ConnectionHandler
  - MQUnifiedsensor
  - HTTPClient (มาพร้อมกับ ESP32 Core)
- บัญชี Arduino IoT Cloud
- บัญชี Twilio (แผนใช้งานฟรีก็เพียงพอ)
- บัญชี Google (หากต้องการบันทึกข้อมูลลง Google Sheets)

## ขั้นตอนการตั้งค่า
ดำเนินการตามขั้นตอนต่อไปนี้เพื่อเก็บข้อมูลที่จำเป็นก่อนคอมไพล์โค้ด

### 1. การตั้งค่า Twilio
#### 1.1 รวบรวมข้อมูลประจำตัว (Credentials)
เข้าสู่ระบบ Twilio และไปที่ Account Dashboard เพื่อเก็บข้อมูลดังต่อไปนี้
- **Account SID**
- **Auth Token**
- **Twilio Phone Number** (เบอร์โทรศัพท์จาก Twilio)

#### 1.2 สร้าง TwiML Bin สำหรับข้อความเสียง
1. ไปที่หน้า **TwiML Bins** ใน Twilio Console
2. สร้าง TwiML Bin ใหม่และตั้งชื่อให้สื่อความหมาย
3. ใส่โค้ดตัวอย่างต่อไปนี้แล้วบันทึก

```xml
<Response>
  <Say language="th-TH" voice="Polly.Salli">
    คำเตือน, ตรวจพบควันในระดับที่เป็นอันตราย กรุณาตรวจสอบพื้นที่ของคุณทันที
  </Say>
  <Hangup/>
</Response>
```

หลังบันทึก Twilio จะสร้าง URL สำหรับ TwiML Bin นี้ ให้นำ URL ไปใช้ในขั้นตอนถัดไป

#### 1.3 เตรียมเบอร์โทรศัพท์ผู้รับสาย
- ใช้รูปแบบสากล (E.164) เช่น `+66812345678`

### 2. การตั้งค่าไฟล์ `secrets.h`
เปิดไฟล์ `secrets.h` แล้วกรอกข้อมูลที่ได้จากขั้นตอนก่อนหน้าให้ครบถ้วน

### 3. การตั้งค่าอื่น ๆ
- **Arduino IoT Cloud**: สร้าง "Thing" และ "Device" เพื่อรับ `DEVICE_LOGIN_NAME` และ `DEVICE_KEY`
- **Google Apps Script**: หากต้องการบันทึกข้อมูล ให้สร้างโปรเจ็กต์ Apps Script แล้ว Deploy เป็น Web App เพื่อรับ `APPS_SCRIPT_URL`

## การใช้งาน
1. ตรวจสอบว่าได้กรอกข้อมูลใน `secrets.h` ครบถ้วนแล้ว
2. เชื่อมต่อบอร์ด ESP32 เข้ากับคอมพิวเตอร์
3. เลือก Board และ Port ที่ถูกต้องใน Arduino IDE
4. คอมไพล์และอัปโหลดโค้ดไปยังบอร์ด ESP32
5. เปิด Serial Monitor (Baud rate: 115200) เพื่อดูสถานะการทำงาน

เมื่อระบบทำงานและตรวจพบควันเกินค่า `SMOKE_THRESHOLD_PPM` จะได้รับการโทรแจ้งเตือนจาก Twilio ตามข้อความที่ตั้งค่าไว้ใน TwiML Bin
