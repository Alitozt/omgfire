#pragma once

//================================================================================
// ข้อมูลการเชื่อมต่อ WiFi
//================================================================================
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

//================================================================================
// ข้อมูลสำหรับ Arduino IoT Cloud
// ดูค่าเหล่านี้ได้จากหน้า Setup Device ของคุณใน Arduino IoT Cloud
//================================================================================
#define DEVICE_LOGIN_NAME "YOUR_ARDUINO_CLOUD_DEVICE_ID"
#define DEVICE_KEY "YOUR_ARDUINO_CLOUD_SECRET_KEY"

//================================================================================
// ข้อมูลสำหรับ Twilio API (สำหรับโทรออก)
//================================================================================
#define TWILIO_ACCOUNT_SID "YOUR_TWILIO_SID"
#define TWILIO_AUTH_TOKEN "YOUR_TWILIO_AUTH_TOKEN"
#define TWILIO_NUMBER "YOUR_TWILIO_NUMBER"
#define TO_NUMBER "NUMBER_THAT_YOU_NEED_TO_CALL"
#define TWIML_URL "YOUR_TWIML_URL"

//================================================================================
// ข้อมูลสำหรับ Google Apps Script
//================================================================================
#define APPS_SCRIPT_URL "YOUR_APPS_SCRIPT_URL"
