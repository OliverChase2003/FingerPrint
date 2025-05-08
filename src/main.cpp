// arduino
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <Adafruit_Fingerprint.h>
#include <U8g2lib.h>
// std
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>

// macros
// debug
#define DEBUG 1
// wifi
#define WIFI_ID "103"
#define WIFI_PWD "HelloWorld\\0"
const char* ntpServer = "cn.pool.ntp.org";
const long utcOffsetInSeconds = 28800;
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
// ssd1306
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /*reset=*/ U8X8_PIN_NONE);
// bitmap
char bmap[256];

// declarations
// functions for debug
void debug_print(const char *fmt, ...);   // tested
int Serial2Int();
void panic(const char *s);    // tested
void ssd1306_print(const char *fmt, ...);   // tested
// wifi
void printLocalTime();
int time2str(char *time_str);
String wifi_addr_trans(int ip);
// bitmaps
void create_bitmap(const char *file);             // tested
int read_bitmap(const char *file, char *bitmap);  // tested
int write_bitmap(const char *file, char *bitmap); // tested
void print_bitmap(char *bitmap);
// log
void check_in(Adafruit_Fingerprint &finger, const char *bitmap_file);
// fingerprints
int finger_enroll(Adafruit_Fingerprint &finger, const char *bitmap_file);   //tested
int finger_delete(Adafruit_Fingerprint &finger, const char *bitmap_file, int number);   // tested
int finger_search(Adafruit_Fingerprint &finger, const char *bitmap_file);   // tested

void check_in_print(int ID) {
  char time_str[64];
  struct tm timeinfo;
  if(getLocalTime(&timeinfo))
    strftime(time_str, sizeof(time_str),"%Y-%m-%d %H:%M:%S", &timeinfo);
  else {
    strcpy(time_str, "fetch time failed");
    debug_print("check_in_print: fetch time failed");
  }
  File f = SD.open(CHECKINFILE, FILE_APPEND);
  f.print(time_str);
  f.print(" -> ");
  f.println(ID);
  f.close();
}

/**
 * @brief init function
 *
 */
void setup()
{
  // debug uart port init
  Serial.begin(115200); // serial for debug
  while (!Serial)
    yield();

  // ssd1306 screen init
  if (u8g2.begin())
    debug_print("ssd1306 init successed");
  else
    panic("ssd1306 init failed!");

  // wifi init
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(WIFI_ID, WIFI_PWD);
  while(WiFi.status() != WL_CONNECTED) delay(500);
  debug_print("STA Mode IP: %s", wifi_addr_trans(WiFi.localIP()));
  configTime(utcOffsetInSeconds, 0, ntpServer);

  // fingerprint init
  Serial2.begin(57600, SERIAL_8N1, 16, 17); // serial for AS608
  if (finger.verifyPassword())
    debug_print("fingerprint init success!");
  else
    panic("fingerprint init failed");

  // spi sdcard module init
  if (SD.begin()) {
    debug_print("sdcard init success!");
    ssd1306_print("sdcard found");
  }
  else{
    ssd1306_print("where's your sdcard");
    panic("sdcard init failed!");
  }

  // create checkin file and bitmap
  if (!SD.exists(CHECKINFILE)) {
    ssd1306_print("no check-in file, creating...");
    delay(200);
    File file = SD.open(CHECKINFILE, FILE_WRITE);
    file.println("start check-in");
    file.close();
  }
  if (!SD.exists(BITMAPFILE)) {
    ssd1306_print("no bitmap file, creating...");
    delay(200);
    create_bitmap(BITMAPFILE);
    read_bitmap(BITMAPFILE, bmap);
    debug_print("bitmap: %s", bmap);
    finger.emptyDatabase();
  }

  debug_print("All hardware init success");
  ssd1306_print("All hardware init success");
  delay(200);
#ifdef DEBUG
  read_bitmap(BITMAPFILE, bmap);
  debug_print("bitmap ->");
  debug_print("%s", bmap);
#endif
  //test1();
  //test2();
  //test3();
  //printLocalTime();
}

void loop()
{
  debug_print("please select mode");
  ssd1306_print("select mode");
  int delNum = -1;
  int mode = Serial2Int();
  switch(mode) {
    case(1):    // enroll mode
      debug_print("enroll_mode!!!");
      ssd1306_print("enroll_mode!!!");
      delay(2000);
      finger_enroll(finger, BITMAPFILE);
      break;
    case(2):    // check-in mode
      debug_print("check_in mode!!!");
      ssd1306_print("check_in mode!!!");
      delay(2000);
      check_in(finger, BITMAPFILE);
      break;
    case(3): {   // delete mode
      debug_print("delete_mode!!!");
      ssd1306_print("delete_mode!!!");
      delay(2000);
      debug_print("input number to del");
      ssd1306_print("input number to del");
      int delNum = Serial2Int();
      finger_delete(finger, BITMAPFILE, delNum);
      break;
    }
    default:
      debug_print("invalid mode input");
      ssd1306_print("invalid mode input");
      break;
  }
}

/**
 * @brief enroll a finger for fingerprint module
 *
 * @param Adafruit_Fingerprint finger: specify which finger module choosed
 * @return int: success for 0, fail for -1
 */
int finger_enroll(Adafruit_Fingerprint &finger, const char *bitmap_file) {
  int id;
  // set bitmap
  read_bitmap(bitmap_file, bmap);
  debug_print("read bitmap: %s", bmap);
  
  for(int i = 0; i < 127; i++){
    if(bmap[i] == '0') {
      id = i+1;
      break;
    }
  }

  int p = -1;
  debug_print("Waiting for finger to enroll as #%d", id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      debug_print("");
      debug_print("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      debug_print("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      debug_print("Imaging error");
      break;
    default:
      debug_print("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      debug_print("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      debug_print("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      debug_print("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      debug_print("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      debug_print("Could not find fingerprint features");
      return p;
    default:
      debug_print("Unknown error");
      return p;
  }

  p = finger.fingerFastSearch();
  if(p == FINGERPRINT_OK && (bmap[finger.fingerID] == '1')) {
    debug_print("finger_enroll: finger has been enrolled");
    return -1;
  }

  debug_print("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  debug_print("ID %d",id);
  p = -1;
  debug_print("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      debug_print("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      debug_print("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      debug_print("Imaging error");
      break;
    default:
      debug_print("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      debug_print("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      debug_print("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      debug_print("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      debug_print("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      debug_print("Could not find fingerprint features");
      return p;
    default:
      debug_print("Unknown error");
      return p;
  }

  // OK converted!
  debug_print("Creating model for #%d",id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    debug_print("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    debug_print("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    debug_print("Fingerprints did not match");
    return p;
  } else {
    debug_print("Unknown error");
    return p;
  }

  debug_print("ID: %d", id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    debug_print("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    debug_print("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    debug_print("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    debug_print("Error writing to flash");
    return p;
  } else {
    debug_print("Unknown error");
    return p;
  }

  bmap[id - 1] = '1';
  debug_print("%s", bmap);
  write_bitmap(bitmap_file, bmap);

  ssd1306_print("your ID: %d", id);
  delay(5000);
  return true;
}

/**
 * @brief 
 * 
 * @param finger: finger module class 
 * @param bitmap_file: the location of the bitmap file 
 * @param number: the number to delete 
 * @return int 
 */
int finger_delete(Adafruit_Fingerprint &finger, const char *bitmap_file, int id)
{
  int check_id = finger_search(finger, bitmap_file);
  if(check_id != id) {
    debug_print("finger_delete: finger does not match");
    ssd1306_print("finger does not match");
    return -1;
  }
  finger.deleteModel(id);
  read_bitmap(bitmap_file, bmap);
  bmap[id-1] = '0';
  write_bitmap(bitmap_file, bmap);
  // print info
  debug_print("finger_delete: finger deleted");
  ssd1306_print("finger deleted");
#ifdef DEBUG
  debug_print("bitmap after deleted:");
  debug_print("%s", bmap);
#endif
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
  // if finger can be searched, return number
  // else return -1
  int p = -1;
  debug_print("finger_search: waiting for finger");
  ssd1306_print("waiting for finger");
  while((p = finger.getImage()) != FINGERPRINT_OK) {
    // will add something later
    switch (p) {
      case FINGERPRINT_OK:
        debug_print("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        debug_print("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        debug_print("Imaging error");
        break;
      default:
        debug_print("Unknown error");
        break;
    }
  }
  debug_print("");  // enter

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    Serial.println("finger_search: image2Tz failed");
    return -1;
  }

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    debug_print("finger_search: fingerFastSearch failed");
    return -1;
  }
  
  int ret = finger.fingerID;
  read_bitmap(bitmap_file, bmap);
  if(bmap[ret-1] != '1') {
    debug_print("finger_search: bitmap not set");
    return -1;
  }
  return ret;
}

/**
 * @brief check the finger, get the real time and write check-in log
 * 
 * @param number
 * @param file 
 */
void check_in(Adafruit_Fingerprint &finger, const char *bitmap_file) {
  read_bitmap(bitmap_file, bmap);
  while(1) {
    int ID = -1;
    if((ID = finger_search(finger, bitmap_file)) < 0) {
      debug_print("check-in: finger not right");
      delay(2000);
      continue;
    }
    char time_str[16];
    check_in_print(ID);
    debug_print("ID: %d, check-in success", ID);
    ssd1306_print("ID: %d, check-in success", ID);
    delay(2000);
  }
}

/**
 * @brief Create a bitmap
 * 
 * @param file 
 */
void create_bitmap(const char *file) {
  File f = SD.open(file, FILE_WRITE);
  for(int i = 0; i < 128; i++) {
    f.write('0');
  }
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
  size_t byte_read = f.readBytes(bitmap, 128);
  bmap[128]= '\0';
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
  for(int i = 0; i < 128; i++) {
    f.write(bitmap[i]);
  }
  f.close();
  return 0;
}

/**
 * @brief print local time
 * 
 */
void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    debug_print("fetch time failed");
    return;
  }
  Serial.println(&timeinfo, "time: %Y-%m-%d %H:%M:%S");
}

/**
 * @brief get time info to string format
 * 
 * @param time_str 
 * @return int 
 */
int time2str(char *time_str) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    debug_print("fetch time failed");
    return -1;
  }
  sprintf(time_str, "%Y-%m-%d %H:%M:%S", &timeinfo);
  return 0;
}

/**
 * @brief get string ip addr
 * 
 * @param ip 
 * @return String 
 */
String wifi_addr_trans(int ip) {
  int seg;
  String ret;
  for(int i = 0; i < 4; i++) {
    seg = ip % 256;
    ret += String(seg);
    ip /= 4;
  }
  return ret;
}

/**
 * @brief display string on ssd1306
 *
 * @param int x
 * @param int y
 * @param char* fmt
 * @param ...
 */
void ssd1306_print(const char* fmt, ...) {
  char buf[52];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  // set screen
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_9x15_tr);
  //u8g2.setCursor(0, 14);  // 设置光标位置(x,y)
  // set buf
  int sz = strlen(buf);
  int tmp = sz;
  int n = 0;
  while(tmp > 0) {
    char line[15];
    int len = (tmp < 14)? tmp : 14;
    strncpy(line, buf + (sz - tmp), len);
    line[len] = '\0';
    u8g2.setCursor(0, (n + 1) * 14);
    u8g2.println(line);
    tmp -= 14;
    n++;
  }
  u8g2.sendBuffer();
}

/**
 * @brief get string from serial, turn it to int, and return that int
 * 
 * @return int 
 */
int Serial2Int() {
  String inputStr;
  char inputChar;
  while(inputChar != '\n') {
    while(!Serial.available()) delay(100);
    inputChar = Serial.read();
    if(inputChar != '\n') 
      inputStr += inputChar;
  }
  return inputStr.toInt();
}

/**
 * @brief print a string to default Serial like "printf()" to debug program
 * 
 * @param char* fmt 
 * @param ... 
 */
void debug_print(const char *fmt, ...)
{
  char buf[128];
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