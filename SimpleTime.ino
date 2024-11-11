#define BLYNK_TEMPLATE_ID "TMPL35MjB44Fo"
#define BLYNK_TEMPLATE_NAME "Digital Alarm with Clock"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "time.h"
#include "esp_sntp.h"
#include "LiquidCrystal_I2C.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);


const char *ssid = "ganesh's S23 FE";
const char *password = "Ganesh@03";
const char *auth = "SSAiSsIqqlIxdPjuvHCREpEMGVMKFZ2S";  // Replace with your Blynk auth token

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 19800; // GMT offset for India (+5:30)
const int daylightOffset_sec = 0; // No daylight saving

const int buzzerPin = 27;           // GPIO pin connected to the buzzer
const int alarmOffButtonPin = 14;   // GPIO pin for alarm-off button

int alarmHour = 0;             // Set default alarm hour
int alarmMinute = 0;           // Set default alarm minute
bool alarmTriggered = false;   // Flag to track if alarm is triggered

// Debounce variables
int lastButtonState = HIGH;    // Initial state of button
int buttonState;               // Current state of button
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

BlynkTimer timer;

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);

  lcd.setCursor(0, 1);
  lcd.printf("%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  checkAlarm(timeinfo.tm_hour, timeinfo.tm_min);
}

void checkAlarm(int hour, int minute) {
  if (hour == alarmHour && minute == alarmMinute && !alarmTriggered) {
    Serial.println("Alarm Triggered!");
    alarmTriggered = true;
    digitalWrite(buzzerPin, HIGH); // Turn on the buzzer
  }

  int reading = digitalRead(alarmOffButtonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && alarmTriggered) {
      Serial.println("Alarm turned off");
      alarmTriggered = false;
      digitalWrite(buzzerPin, LOW); // Turn off the buzzer
    }
  }

  lastButtonState = reading;
}

// Callback function for Blynk to update alarm hour
BLYNK_WRITE(V0) {
  alarmHour = param.asInt();
  Serial.print("Alarm Hour set to: ");
  Serial.println(alarmHour);
}

// Callback function for Blynk to update alarm minute
BLYNK_WRITE(V1) {
  alarmMinute = param.asInt();
  Serial.print("Alarm Minute set to: ");
  Serial.println(alarmMinute);
}

// Callback function for Blynk button
BLYNK_WRITE(V2) {
  int buttonState = param.asInt();
  if (buttonState == 1 && alarmTriggered) {
    Serial.println("Alarm turned off via Blynk");
    alarmTriggered = false;
    digitalWrite(buzzerPin, LOW); // Turn off the buzzer
  }
}

// Callback function for NTP sync
void timeavailable(struct timeval *t) {
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  pinMode(buzzerPin, OUTPUT);
  pinMode(alarmOffButtonPin, INPUT_PULLUP);

  // Connect to Wi-Fi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");

  // Connect to Blynk
  Blynk.begin(auth, ssid, password);

  // Set notification call-back function
  sntp_set_time_sync_notification_cb(timeavailable);

  // Set time zone and configure NTP servers
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  // Timer to update time every second
  timer.setInterval(1000L, printLocalTime);
}

void loop() {
  Blynk.run();
  timer.run();
}