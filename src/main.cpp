#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Fingerprint.h>

// global var
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);
int num = 0;

// declarations
void debug_print(const char* fmt, ...);

void setup() {
  // put your setup code here, to run once:
  // debug uart port init
  Serial.begin(115200);   // serial for debug
  // fingerprint init
  Serial1.begin(57600);   // serial for AS608
  finger.begin(57600);
}

void loop() {
  // put your main code here, to run repeatedly:
 
}

void debug_print(const char* fmt, ...){
  char buf[64];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.println(buf);
}
