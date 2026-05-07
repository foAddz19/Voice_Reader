#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <DFRobotDFPlayerMini.h>

// =====================================================
// OLED SSD1309 128x64 I2C
// SDA = GPIO21
// SCL = GPIO22
// =====================================================

U8G2_SSD1309_128X64_NONAME2_F_HW_I2C display(
  U8G2_R0,
  U8X8_PIN_NONE
);

// ถ้าจอไม่ขึ้น ให้ลองเปลี่ยนเป็นตัวนี้แทน
// U8G2_SSD1309_128X64_NONAME0_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);


// =====================================================
// DFPlayer Mini
// =====================================================

HardwareSerial dfSerial(2);
DFRobotDFPlayerMini player;

#define DF_RX 16   // ESP32 รับข้อมูลจาก TX ของ DFPlayer
#define DF_TX 17   // ESP32 ส่งข้อมูลไป RX ของ DFPlayer ผ่าน R 1K


// =====================================================
// Buttons
// =====================================================

#define BTN_1 25
#define BTN_2 26
#define BTN_3 27
#define BTN_4 32
#define BTN_5 33


// =====================================================
// Setting
// =====================================================

const int volumeLevel = 25;   // ระดับเสียง 0 - 30
const unsigned long debounceDelay = 350;

unsigned long lastPressTime = 0;
bool isPlaying = false;


// =====================================================
// Category and Words
// =====================================================

// จำนวนไฟล์เสียงในแต่ละหมวด
int fileCount[5] = {
  5,  // 01 สัตว์
  5,  // 02 ผลไม้
  5,  // 03 สี
  5,  // 04 ตัวเลข
  5   // 05 ทักทาย
};

// ชื่อหมวดภาษาไทย
const char* categoryTH[5] = {
  "สัตว์",
  "ผลไม้",
  "สี",
  "ตัวเลข",
  "ทักทาย"
};

// ชื่อหมวดภาษาอังกฤษ
const char* categoryEN[5] = {
  "Animals",
  "Fruits",
  "Colors",
  "Numbers",
  "Greetings"
};

// คำศัพท์ภาษาไทย
const char* wordsTH[5][5] = {
  {"แมว", "สุนัข", "นก", "ปลา", "ช้าง"},
  {"แอปเปิล", "กล้วย", "ส้ม", "มะม่วง", "องุ่น"},
  {"แดง", "น้ำเงิน", "เขียว", "เหลือง", "ดำ"},
  {"หนึ่ง", "สอง", "สาม", "สี่", "ห้า"},
  {"สวัสดี", "ลาก่อน", "ขอบคุณ", "ขอโทษ", "อรุณสวัสดิ์"}
};

// คำศัพท์ภาษาอังกฤษ
const char* wordsEN[5][5] = {
  {"cat", "dog", "bird", "fish", "elephant"},
  {"apple", "banana", "orange", "mango", "grape"},
  {"red", "blue", "green", "yellow", "black"},
  {"one", "two", "three", "four", "five"},
  {"hello", "goodbye", "thank you", "sorry", "good morning"}
};


// =====================================================
// Display Functions
// =====================================================

void displayThaiFont() {
  display.setFont(u8g2_font_etl16thai_t);
  display.enableUTF8Print();
}

void showBootScreen() {
  display.clearBuffer();

  displayThaiFont();

  display.setCursor(0, 15);
  display.print("กล่องคำศัพท์");

  display.setCursor(0, 35);
  display.print("ระบบกำลังเริ่ม...");

  display.setCursor(0, 55);
  display.print("ESP32 + DFPlayer");

  display.sendBuffer();
}

void showReadyScreen() {
  display.clearBuffer();

  displayThaiFont();

  display.setCursor(0, 14);
  display.print("พร้อมใช้งาน");

  display.setCursor(0, 32);
  display.print("กดปุ่ม 1-5");

  display.setCursor(0, 50);
  display.print("สุ่มเสียงคำศัพท์");

  display.sendBuffer();
}

void showDFPlayerError() {
  display.clearBuffer();

  displayThaiFont();

  display.setCursor(0, 15);
  display.print("DFPlayer Error");

  display.setCursor(0, 35);
  display.print("เช็คสาย / SD Card");

  display.setCursor(0, 55);
  display.print("ไฟล์ 001.mp3");

  display.sendBuffer();
}

void showPlayingScreen(int folderNumber, int fileNumber) {
  int categoryIndex = folderNumber - 1;
  int wordIndex = fileNumber - 1;

  display.clearBuffer();

  displayThaiFont();

  // บรรทัด 1: หมวด
  display.setCursor(0, 13);
  display.print("หมวด: ");
  display.print(categoryTH[categoryIndex]);

  // บรรทัด 2: คำไทย
  display.setCursor(0, 31);
  display.print("คำ: ");
  display.print(wordsTH[categoryIndex][wordIndex]);

  // บรรทัด 3: คำอังกฤษ
  display.setFont(u8g2_font_7x14B_tf);
  display.setCursor(0, 49);
  display.print(wordsEN[categoryIndex][wordIndex]);

  // บรรทัด 4: สถานะ
  displayThaiFont();
  display.setCursor(0, 64);
  display.print("กำลังเล่น...");

  display.sendBuffer();
}

void showButtonCategoryScreen(int folderNumber) {
  int categoryIndex = folderNumber - 1;

  display.clearBuffer();

  displayThaiFont();

  display.setCursor(0, 15);
  display.print("เลือกหมวด");

  display.setCursor(0, 35);
  display.print(categoryTH[categoryIndex]);

  display.setFont(u8g2_font_7x14B_tf);
  display.setCursor(0, 55);
  display.print(categoryEN[categoryIndex]);

  display.sendBuffer();
}


// =====================================================
// Button Function
// =====================================================

bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW) {
    if (millis() - lastPressTime > debounceDelay) {
      lastPressTime = millis();

      // รอจนปล่อยปุ่ม ป้องกันกดค้างแล้วเล่นซ้ำ
      while (digitalRead(pin) == LOW) {
        delay(10);
      }

      return true;
    }
  }

  return false;
}

// =====================================================
// Play Sound
// =====================================================

void playRandomSound(int folderNumber) {
  int categoryIndex = folderNumber - 1;

  if (categoryIndex < 0 || categoryIndex > 4) {
    return;
  }

  int maxFile = fileCount[categoryIndex];
  int randomFile = random(1, maxFile + 1);

  Serial.println("================================");
  Serial.print("Folder: ");
  Serial.println(folderNumber);

  Serial.print("Category TH: ");
  Serial.println(categoryTH[categoryIndex]);

  Serial.print("Category EN: ");
  Serial.println(categoryEN[categoryIndex]);

  Serial.print("File: ");
  Serial.println(randomFile);

  Serial.print("Word TH: ");
  Serial.println(wordsTH[categoryIndex][randomFile - 1]);

  Serial.print("Word EN: ");
  Serial.println(wordsEN[categoryIndex][randomFile - 1]);
  Serial.println("================================");

  showPlayingScreen(folderNumber, randomFile);

  // เล่นไฟล์ในโฟลเดอร์ เช่น /01/003.mp3
  player.playFolder(folderNumber, randomFile);

  isPlaying = true;
}

// =====================================================
// Setup
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("Vocabulary Sound Box Starting...");

  // ปุ่มแบบ INPUT_PULLUP
  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);
  pinMode(BTN_3, INPUT_PULLUP);
  pinMode(BTN_4, INPUT_PULLUP);
  pinMode(BTN_5, INPUT_PULLUP);

  // สุ่มจากค่า noise analog
  randomSeed(analogRead(34));

  // เริ่มจอ I2C
  Wire.begin(21, 22);
  display.begin();

  // ถ้าจอไม่ขึ้น อาจลองเปิดบรรทัดนี้
  // display.setI2CAddress(0x3C * 2);

  display.enableUTF8Print();
  showBootScreen();

  delay(1000);

  // เริ่ม DFPlayer
  dfSerial.begin(9600, SERIAL_8N1, DF_RX, DF_TX);

  Serial.println("Starting DFPlayer Mini...");

  if (!player.begin(dfSerial)) {
    Serial.println("DFPlayer Mini not found!");
    Serial.println("Please check wiring, SD card, and power.");

    showDFPlayerError();

    while (true) {
      delay(1000);
    }
  }

  Serial.println("DFPlayer Mini Ready!");

  player.volume(volumeLevel);
  delay(300);

  showReadyScreen();

  Serial.println("System Ready!");
}


// =====================================================
// Loop
// =====================================================

void loop() {
  if (buttonPressed(BTN_1)) {
    playRandomSound(1);
  }

  if (buttonPressed(BTN_2)) {
    playRandomSound(2);
  }

  if (buttonPressed(BTN_3)) {
    playRandomSound(3);
  }

  if (buttonPressed(BTN_4)) {
    playRandomSound(4);
  }

  if (buttonPressed(BTN_5)) {
    playRandomSound(5);
  }
}
