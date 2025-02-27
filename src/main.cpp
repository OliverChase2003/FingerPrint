#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Fingerprint.h>
#include <Adafruit_SSD1306.h>

// macros
// ssd1306 defs
#define SSD1306_WIDTH   128   // ssd1306 oled pixel width
#define SSD1306_HEIGHT  64    // ssd1306 oled pixel height
// spi0 defs
#define CUSTOM_MOSI 23
#define CUSTOM_MISO 19
#define CUSTOM_SCK  18
#define CUSTOM_SS   5

// global var
// fingerprint
Adafruit_Fingerprint finger(&Serial1);
// sdcard
const int spi_chipSelect = 5;
// ssd1306
Adafruit_SSD1306 display(SSD1306_WIDTH, SSD1306_HEIGHT, &Wire, -1);
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
  Serial1.begin(57600);   // serial for AS608
  finger.begin(57600);
  debug_print("fingerprint init success!");
  // spi sdcard module init
  SPI.begin(CUSTOM_SCK, CUSTOM_MISO, CUSTOM_MOSI, CUSTOM_SS);
  if(SD.begin()){   // dont need to specify the spi bus, use spi0 as default
    debug_print("sdcard init success!");
  }else{
    panic("sdcard init failed!");
  }
  // ssd1306 screen init
  if(display.begin()){
    debug_print("ssd1306 init success!");
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