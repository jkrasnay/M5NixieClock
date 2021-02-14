#include <M5Core2.h>
#include <WiFi.h>

#include "nixie.h"

// === Network Functions

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -5 * 3600; // Eastern time
const int   daylightOffset_sec = 3600;


// Expects you to have a .cpp file in this directory with the following
// set appropriately for your WiFi network
//
extern const char* wifi_ssid;
extern const char* wifi_password;


boolean connectWiFi() {
    int count = 0;
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(wifi_ssid, wifi_password);
    Serial.println("Connecting to WiFi..");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        count++;
        if (count > 20) {
            Serial.println("\nWiFi connection failed");
            return false;
        }
    }
    Serial.println("\nWiFi connected");
    return true;
}

void disconnectWiFi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi disconnected");
}

void configTimeFromNtp() {

    Serial.println("Configure time from NTP");

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    struct tm timeinfo2;
    if (!getLocalTime(&timeinfo2)) {
      Serial.println("Failed to obtain time");
      return;
    }

    RTC_TimeTypeDef TimeStruct;
    TimeStruct.Hours   = timeinfo2.tm_hour;
    TimeStruct.Minutes = timeinfo2.tm_min;
    TimeStruct.Seconds = timeinfo2.tm_sec;
    M5.Rtc.SetTime(&TimeStruct);

    RTC_DateTypeDef DateStruct;
    DateStruct.WeekDay = timeinfo2.tm_wday;
    DateStruct.Month =   timeinfo2.tm_mon + 1;
    DateStruct.Date =    timeinfo2.tm_mday;
    DateStruct.Year =    timeinfo2.tm_year + 1900;
    M5.Rtc.SetDate(&DateStruct);

    Serial.printf("Year: %d", timeinfo2.tm_year + 1900);
    Serial.println("Successfully configured time from NTP");
}


// === Display Functions

const uint16_t screen_w = 320;

const uint16_t large_w = 64;
const uint16_t small_w = 24;


const uint16_t time_sp = 20;
const uint16_t time_w = 4 * large_w + time_sp;
const uint16_t time_x = (screen_w - time_w) / 2;
const uint16_t time_y = 50;

const uint16_t date_sp = 10;
const uint16_t date_w = 8 * small_w + 2 * date_sp;
const uint16_t date_x = (screen_w - date_w) / 2;
const uint16_t date_y = 200;

int lastHour;
int lastMinute;
int lastDay;


void drawNumber(image images[], int i, uint16_t x, uint16_t y) {
    M5.Lcd.drawJpg(images[i].bytes, images[i].size, x, y);
}

void drawHour(int hour) {
    uint16_t x = time_x;
    uint16_t y = time_y;
    drawNumber(nixie_64, hour / 10, x, y);
    x += large_w;
    drawNumber(nixie_64, hour % 10, x, y);
}

void drawMinute(int minute) {
    uint16_t x = time_x + 2 * large_w + time_sp;
    uint16_t y = time_y;
    drawNumber(nixie_64, minute / 10, x, y);
    x += large_w;
    drawNumber(nixie_64, minute % 10, x, y);
}

void drawYear(int year) {
    uint16_t x = date_x + 3 * small_w;
    uint16_t y = date_y;
    for (int i = 0; i < 4; i++) {
        drawNumber(nixie_24, year % 10, x, y);
        year /= 10;
        x -= small_w;
    }
}

void drawMonth(int month) {
    uint16_t x = date_x + 4 * small_w + date_sp;
    uint16_t y = date_y;
    drawNumber(nixie_24, month / 10, x, y);
    x += small_w;
    drawNumber(nixie_24, month % 10, x, y);
}

void drawDay(int day) {
    uint16_t x = date_x + 6 * small_w + 2 * date_sp;
    uint16_t y = date_y;
    drawNumber(nixie_24, day / 10, x, y);
    x += small_w;
    drawNumber(nixie_24, day % 10, x, y);
}


// === Setup

void setup() {

    M5.begin();

    // for some reason this is always the port setting on my Mac in spite of my
    // stty efforts
    Serial.begin(9600);

    if (connectWiFi()) {
        configTimeFromNtp();
        disconnectWiFi();
    }

    RTC_TimeTypeDef time;
    M5.Rtc.GetTime(&time);
    drawHour(time.Hours);
    drawMinute(time.Minutes);

    lastMinute = time.Minutes;
    lastHour = time.Hours;

    RTC_DateTypeDef date;
    M5.Rtc.GetDate(&date);
    drawYear(date.Year);
    drawMonth(date.Month);
    drawDay(date.Date);

    lastDay = date.Date;

}


// === Loop

void loop() {

    RTC_TimeTypeDef time;
    M5.Rtc.GetTime(&time);

    if (time.Minutes != lastMinute) {
        drawMinute(time.Minutes);
        lastMinute = time.Minutes;
    }

    if (time.Hours != lastHour) {

        drawHour(time.Hours);
        lastHour = time.Hours;

        if (connectWiFi()) {
            configTimeFromNtp();
            disconnectWiFi();
        }
    }

    RTC_DateTypeDef date;
    M5.Rtc.GetDate(&date);
    if (date.Date != lastDay) {
        drawYear(date.Year);
        drawMonth(date.Month);
        drawDay(date.Date);
        lastDay = date.Date;
    }

    Serial.println("loop");

    delay(5000);

}
