#include <Arduino.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_Fingerprint.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <string.h>
#include <stdlib.h>
#include <WiFi.h>

// macros
// wifi
#define WIFI_ID "103"
#define WIFI_PWD "HelloWorld\\0"
// ssd1306 defs
#define SSD1306_WIDTH 128 // ssd1306 oled pixel width
#define SSD1306_HEIGHT 64 // ssd1306 oled pixel height
// filenames
#define CHECKINFILE "/check-in.txt"
#define BITMAPFILE "/bitmap.txt"
// keyboard
#define KEYPAD_ROW 4
#define KEYPAD_COL 4

// global var
// fingerprint
Adafruit_Fingerprint finger(&Serial2);
// sdcard
// ssd1306
Adafruit_SSD1306 display(SSD1306_WIDTH, SSD1306_HEIGHT, &Wire, -1);
// keypad
// char keypad[KEYPAD_ROW][KEYPAD_COL] = {
//  {'7', '8', '9', ''},
//  {'4', '5', '6', ''},
//  {'1', '2', '3', ''},
//  {'', '0', '', ''},
//};

// program's vars
int num = 0;

// declarations
// functions for debug
void debug_print(const char *fmt, ...);
void panic(const char *s);
void ssd1306_print(Adafruit_SSD1306 display, int y, int x, const char *fmt, ...);
// bitmaps
void create_bitmap(const char *file);
int read_bitmap(const char *file, char *bitmap);
int write_bitmap(const char *file, char *bitmap);
// log
void write_log();
// fingerprints
int finger_enroll(Adafruit_Fingerprint &finger, const char *bitmap_file);
int finger_delete(Adafruit_Fingerprint &finger, const char *bitmap_file, int number);
int finger_search(Adafruit_Fingerprint &finger, const char *bitmap_file);

/**
 * @brief init function
 *
 */
void setup()
{
  // debug uart port init
  Serial.begin(115200); // serial for debug
  while (!Serial)
  { // wait debug serial
    yield();
  }

  // ssd1306 screen init
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    debug_print("ssd1306 init success!");
  }
  else
  {
    panic("ssd1306 init failed!");
  }

  // wifi init
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(WIFI_ID, WIFI_PWD);
  while(WiFi.status() != WL_CONNECTED) delay(500);
  debug_print("STA Mode IP: %s", WiFi.localIP());

  // fingerprint init
  Serial2.begin(57600, SERIAL_8N1, 16, 17); // serial for AS608
  if (finger.verifyPassword())
  {
    debug_print("fingerprint init success!");
  }
  else
  {
    panic("fingerprint init failed");
  }

  // spi sdcard module init
  if (SD.begin())
  { // dont need to specify the spi bus, use spi0 as default
    debug_print("sdcard init success!");
  }
  else
  {
    panic("sdcard init failed!");
  }

  // create checkin file and bitmap
  if (!SD.exists(CHECKINFILE))
  {
    File file = SD.open(CHECKINFILE, FILE_WRITE);
    file.close();
  }
  if (!SD.exists(BITMAPFILE))
  {
    create_bitmap(BITMAPFILE);
    finger.emptyDatabase();
  }

  ssd1306_print(display, 0, 0, "All hardware init success");
}

void loop()
{
  while(1);
}

/**
 * @brief enroll a finger for fingerprint module
 *
 * @param Adafruit_Fingerprint finger: specify which finger module choosed
 * @return int: success for 0, fail for -1
 */
int enroll(Adafruit_Fingerprint &finger, const char *bitmap_file) {
  char bitmap[128];  // a bitmap for fingerprint

  for (int i = 1; i < 3; i++)
  {
    int p = -1;
    while (p != FINGERPRINT_OK)
    {
      p = finger.getImage();
      switch (p)
      {
      case FINGERPRINT_OK:
        debug_print("getImage success");
        break;
      case FINGERPRINT_NOFINGER:
        debug_print(".");
        break;
      default:
        debug_print("fail to get first image");
        return -1;
      }
    }
    p = -1; // reset p
    // get the feature of the image 1
    if ((p = finger.image2Tz(i)) != FINGERPRINT_OK)
    {
      debug_print("first image2Tz fail");
      return -1;
    }
    if (i == 1)
    {
      ssd1306_print(display, 0, 0, "Remove finger and place again");
      delay(2000);
      p = 0;
      while (p != FINGERPRINT_NOFINGER)
      {
        p = finger.getImage();
      }
    }
  }

  // read bitmap and find which number is available and set that bit to true
  read_bitmap(bitmap_file, bitmap);
  int i;
  for(int i = 0; i <= 128; i++) {
    if(bitmap[i] == '0') {
      bitmap[i] = '1';
      break;
    }
    write_bitmap(bitmap_file, bitmap);
    if(i == 128) {
      debug_print("fingerprint: no slot for new fingerprint");
      return -1;
    }
  }

  // store the fingerprint
  ssd1306_print(display, 0, 0, "Creating model for #%d", i);
  int p;
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }
  
  p = finger.storeModel(i);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return 0;
}

/**
 * @brief 
 * 
 * @param finger 
 * @param bitmap_file 
 * @param number 
 * @return int 
 */
int finger_delete(Adafruit_Fingerprint &finger, const char *bitmap_file, int number)
{
  File f = SD.open(bitmap_file, FILE_WRITE);
  char bitmap[129];
  read_bitmap(bitmap_file, bitmap); 
  if(bitmap[number] == '0') {
    debug_print("finger_delete: finger is not registered");
    return -1;
  } else {
    // check if the finger mapped
    debug_print("please put your finger on ther sensor");
    ssd1306_print(display, 0, 0, "put your finger on sensor");
    int p = finger.getImage();
    if(number == finger_search(finger, bitmap_file)) {
      finger.deleteModel(number);
    }
  }
  f.close();
  return 0;
}

/**
 * @brief 
 * 
 * @param finger 
 * @param bitmap_file 
 * @param number 
 * @return int 
 */
int finger_search(Adafruit_Fingerprint &finger, const char *bitmap_file) {
  File f = SD.open(bitmap_file, FILE_READ);

  // if finger can be searched, return number
  int p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    if (p == FINGERPRINT_NOFINGER) {
      Serial.println("未检测到手指！");
    } else {
      Serial.println("图像获取失败，错误代码: 0x" + String(p, HEX));
    }
    return 128;
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    Serial.println("特征生成失败！");
    return 128;
  }

  f.close(); 
  if(finger.confidence > 50) return finger.fingerID;
  // else return 0;
  return 128;
}

/**
 * @brief get the real time and write check-in log
 * 
 * @param number
 * @param file 
 */
void write_log(int number, const char *file) {
  File f = SD.open(file, FILE_WRITE);
  String message = "Time:" + String(millis()) + ", Number" + String(number); 
  f.println(message);
  f.close();
}

/**
 * @brief Create a bitmap
 * 
 * @param file 
 */
void create_bitmap(const char *file) {
  File f = SD.open(file, FILE_WRITE);
  char empty_bitmap[129];
  for(int i = 0; i < 128; i++){
    empty_bitmap[i] = '0';
  }
  empty_bitmap[128] = '\0';
  f.println(empty_bitmap);
  f.close();
}

/**
 * @brief read bitmap from file to array
 * 
 * @param char* file 
 * @param char* bitmap 
 * @return int 
 */
int read_bitmap(const char *file, char *bitmap) {
  File f = SD.open(file, FILE_READ);
  size_t byte_read = f.readBytes(bitmap, sizeof(bitmap) - 1);
  bitmap[sizeof(bitmap)] = '\0';
  f.close();
  return 0;
}

/**
 * @brief write bitmap form array to file
 * 
 * @param char* file 
 * @param char* bitmap 
 * @return int 
 */
int write_bitmap(const char *file, char *bitmap) {
  File f = SD.open(file, FILE_WRITE);
  f.println(bitmap);
  f.close();
  return 0;
}

/**
 * @brief display string on ssd1306
 *
 * @param Adafruit_Fingerprint display
 * @param int y
 * @param int x
 * @param char* fmt
 * @param ...
 */
void ssd1306_print(Adafruit_SSD1306 display, int y, int x, const char *fmt, ...)
{
  char buf[64];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  display.clearDisplay();
  display.setCursor(y, x);
  display.setTextSize(2);
  display.println(buf);
}

/**
 * @brief print a string to default Serial like "printf()" to debug program
 * 
 * @param char* fmt 
 * @param ... 
 */
void debug_print(const char *fmt, ...)
{
  char buf[64];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.println(buf);
}

/**
 * @brief print the error message to serial 0 and stop the program
 *        by a while loop
 * 
 * @param s 
 */
void panic(const char *s)
{
  debug_print("panic: %s", s);
  while (1)
    ;
}