#include <Arduino.h>
#include <TinyGPS++.h>
#include <sys/time.h>
#include <time.h>

// TinyGPS++ and Serial configuration assumed here
TinyGPSPlus gps;

// RTC memory variable to remember which timezone we detected last
RTC_DATA_ATTR int savedTimezoneOffsetHours = 0; 
RTC_DATA_ATTR bool timezoneKnown = false;

void setLocalTimezone(float longitude, int year, int month, int day) {
    char tzString[32];
    
    if (longitude > 0) {
        // Philippines: UTC+8. 
        // Note: POSIX TZ format is inverted for positive offsets! UTC+8 is written as -8.
        strcpy(tzString, "PHT-8");
        savedTimezoneOffsetHours = 8;
    } else {
        // Eastern Time Zone (US). Handles EDT/EST if date logic is added.
        // For simplicity, a basic static check or standard POSIX string can be used:
        // "EST5EDT,M3.2.0,M11.1.0" handles EST (UTC-5) and EDT (UTC-4) automatically via standard C lib!
        strcpy(tzString, "EST5EDT,M3.2.0,M11.1.0");
        savedTimezoneOffsetHours = -5; // Base offset
    }
    
    setenv("TZ", tzString, 1);
    tzset();
    timezoneKnown = true;
    Serial.printf("Timezone configured to: %s\n", tzString);
}

void syncSystemTimeWithGPS() {
    struct tm t;
    t.tm_year = gps.date.year() - 1900;
    t.tm_mon = gps.date.month() - 1;
    t.tm_mday = gps.date.day();
    t.tm_hour = gps.time.hour();
    t.tm_min = gps.time.minute();
    t.tm_sec = gps.time.second();
    t.tm_isdst = -1; // Let the library determine DST from the TZ string
    
    time_t t_of_day = mktime(&t);
    
    // Apply UTC to system time
    struct timeval tv = { .tv_sec = t_of_day, .tv_usec = 0 };
    settimeofday(&tv, NULL);
}

void printLocalTime() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "Local Time: %A, %B %d %Y %H:%M:%S");
}

void setup() {
    Serial.begin(115200);
    
    // Check if we already know the time from a previous run before deep sleep
    struct tm timeinfo;
    if (getLocalTime(&timeinfo) && timezoneKnown) {
        Serial.println("Woke up from deep sleep. RTC time is valid.");
        printLocalTime();
    } else {
        Serial.println("Cold boot or invalid time. Waiting for GPS fix...");
        // 1. Read from your GY-NEO6MV2 module here until gps.location.isValid() and gps.time.isValid() are true
        // 2. Once valid:
        // float lon = gps.location.lng();
        // setLocalTimezone(lon, gps.date.year(), gps.date.month(), gps.date.day());
        // syncSystemTimeWithGPS();
    }
    
    // Perform your tasks here...
    
    // Go back to deep sleep. The ESP32-S3 internal RTC will keep updating the timeval system clock.
    Serial.println("Entering deep sleep for 10 minutes...");
    esp_sleep_enable_timer_wakeup(10 * 60 * 1000000ULL);
    esp_deep_sleep_start();
}

void loop() {
  // put your main code here, to run repeatedly:

}
