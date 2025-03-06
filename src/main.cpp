#include <Arduino.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_Fingerprint.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>

// macros
// ssd1306 defs
#define SSD1306_WIDTH   128   // ssd1306 oled pixel width
#define SSD1306_HEIGHT  64    // ssd1306 oled pixel height
// spi-sdcard defs
#define SDCARD_SS   5
// keyboard
#define KEYPAD_ROW  4
#define KEYPAD_COL  4

// global var
// fingerprint
Adafruit_Fingerprint finger(&Serial2);
// sdcard
const int spi_chipSelect = 5;
// ssd1306
Adafruit_SSD1306 display(SSD1306_WIDTH, SSD1306_HEIGHT, &Wire, -1);
// keypad
//char keypad[KEYPAD_ROW][KEYPAD_COL] = {
//  {'7', '8', '9', ''},
//  {'4', '5', '6', ''},
//  {'1', '2', '3', ''},
//  {'', '0', '', ''},
//};

// program's vars
int num = 0;

// declarations
void debug_print(const char* fmt, ...);
void panic(const char* s);

// init func
void setup() {
  // put your setup code here, to run once:
  // debug uart port init
  Serial.begin(115200);   // serial for debug
  // fingerprint init
  Serial2.begin(57600);   // serial for AS608
  finger.begin(57600);
  if(!finger.verifyPassword()){
    panic("fingerprint init failed");
  }
  debug_print("fingerprint init success!");
  // spi sdcard module init
  if(SD.begin(SDCARD_SS)){   // dont need to specify the spi bus, use spi0 as default
    debug_print("sdcard init success!");
  }else{
    //panic("sdcard init failed!");
  }
  // ssd1306 screen init
  if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    delay(2000);
    debug_print("ssd1306 init success!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("init success");
    display.display();
  }else{
    panic("ssd1306 init failed!");
  }
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

void panic(const char* s){
  debug_print("panic: %s", s);
  while(1);
}