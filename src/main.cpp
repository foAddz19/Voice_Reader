#include <Arduino.h>
#include <esp_system.h>
#include <DFRobotDFPlayerMini.h>

HardwareSerial dfSerial(2);
DFRobotDFPlayerMini df;

#define DF_RX 27
#define DF_TX 26

const int CATEGORY_COUNT = 5;
const int MAX_WORDS_PER_CATEGORY = 150;

const int swPins[CATEGORY_COUNT] = {4, 5, 18, 19, 15};

const unsigned long debounceDelay = 250;
unsigned long lastButtonPress[CATEGORY_COUNT] = {0, 0, 0, 0, 0};
int fileCount[CATEGORY_COUNT] = {0, 0, 0, 0, 0};

const char* categoryNames[CATEGORY_COUNT] = {
  "Animals",
  "Fruits",
  "Colors",
  "Numbers",
  "Greetings"
};

const char* wordNames[CATEGORY_COUNT][MAX_WORDS_PER_CATEGORY] = {
  // Folder 01: Animals
  {
    "cat",       // 001.mp3
    "dog",       // 002.mp3
    "bird",      // 003.mp3
    "fish",      // 004.mp3
    "elephant",  // 005.mp3
    // Add 006.mp3 - 150.mp3 here.
  },

  // Folder 02: Fruits
  {
    "apple",     // 001.mp3
    "banana",   // 002.mp3
    "orange",   // 003.mp3
    "mango",    // 004.mp3
    "grape",    // 005.mp3
    // Add 006.mp3 - 150.mp3 here.
  },

  // Folder 03: Colors
  {
    "red",       // 001.mp3
    "blue",      // 002.mp3
    "green",     // 003.mp3
    "yellow",    // 004.mp3
    "black",     // 005.mp3
    // Add 006.mp3 - 150.mp3 here.
  },

  // Folder 04: Numbers
  {
    "one",       // 001.mp3
    "two",       // 002.mp3
    "three",     // 003.mp3
    "four",      // 004.mp3
    "five",      // 005.mp3
    // Add 006.mp3 - 150.mp3 here.
  },

  // Folder 05: Greetings
  {
    "hello",        // 001.mp3
    "goodbye",      // 002.mp3
    "thank you",    // 003.mp3
    "sorry",        // 004.mp3
    "good morning", // 005.mp3
    // Add 006.mp3 - 150.mp3 here.
  }
};

const char* getWordName(int categoryIndex, int fileNumber) {
  const char* wordName = wordNames[categoryIndex][fileNumber - 1];

  if (wordName == nullptr || wordName[0] == '\0') {
    return "not named in wordNames";
  }

  return wordName;
}

void readFolderFileCounts() {
  Serial.println("Reading MP3 file counts from SD card...");

  for (int i = 0; i < CATEGORY_COUNT; i++) {
    int folderNumber = i + 1;
    int count = df.readFileCountsInFolder(folderNumber);

    if (count < 0) {
      count = 0;
    }
    if (count > MAX_WORDS_PER_CATEGORY) {
      count = MAX_WORDS_PER_CATEGORY;
    }

    fileCount[i] = count;

    Serial.print("Folder /");
    if (folderNumber < 10) {
      Serial.print("0");
    }
    Serial.print(folderNumber);
    Serial.print(" has ");
    Serial.print(fileCount[i]);
    Serial.println(" file(s)");

    delay(250);
  }
}

void playCategory(int categoryIndex) {
  if (categoryIndex < 0 || categoryIndex >= CATEGORY_COUNT) {
    return;
  }

  int maxFile = fileCount[categoryIndex];
  if (maxFile > MAX_WORDS_PER_CATEGORY) {
    maxFile = MAX_WORDS_PER_CATEGORY;
  }
  if (maxFile < 1) {
    Serial.print("No MP3 files found in folder ");
    Serial.println(categoryIndex + 1);
    return;
  }

  int folderNumber = categoryIndex + 1;
  int fileNumber = random(1, maxFile + 1);

  Serial.println("================================");
  Serial.print("SW: ");
  Serial.print(categoryIndex + 1);
  Serial.print(" GPIO");
  Serial.println(swPins[categoryIndex]);

  Serial.print("Category: ");
  Serial.println(categoryNames[categoryIndex]);

  Serial.print("Folder: ");
  Serial.print(folderNumber);
  Serial.print("  File: ");
  Serial.println(fileNumber);

  Serial.print("Path: /");
  if (folderNumber < 10) {
    Serial.print("0");
  }
  Serial.print(folderNumber);
  Serial.print("/");
  if (fileNumber < 100) {
    Serial.print("0");
  }
  if (fileNumber < 10) {
    Serial.print("0");
  }
  Serial.print(fileNumber);
  Serial.println(".mp3");

  Serial.print("Show: ");
  Serial.println(getWordName(categoryIndex, fileNumber));
  Serial.println("================================");

  df.playFolder(folderNumber, fileNumber);
}

void setup() {
  Serial.begin(115200);
  delay(900);

  for (int i = 0; i < CATEGORY_COUNT; i++) {
    pinMode(swPins[i], INPUT_PULLUP);
  }

  Serial.println("ESP32 DFPlayer Test GPIO26/27");

  dfSerial.begin(9600, SERIAL_8N1, DF_RX, DF_TX);
  delay(1500);

  // false helps with some MP3-TF clone modules.
  if (!df.begin(dfSerial, false)) {
    Serial.println("DFPlayer begin failed, but try play anyway");
  } else {
    Serial.println("DFPlayer online");
  }

  df.volume(25);
  delay(500);

  randomSeed(esp_random());
  readFolderFileCounts();

  Serial.println("Ready: SW1-SW5 play folders 01-05");
}

void loop() {
  for (int i = 0; i < CATEGORY_COUNT; i++) {
    if (digitalRead(swPins[i]) == LOW && millis() - lastButtonPress[i] > debounceDelay) {
      lastButtonPress[i] = millis();
      playCategory(i);

      while (digitalRead(swPins[i]) == LOW) {
        delay(10);
      }
    }
  }

  if (Serial.available()) {
    char c = Serial.read();

    if (c == '1') {
      playCategory(0);
    } else if (c == '2') {
      playCategory(1);
    } else if (c == '3') {
      playCategory(2);
    } else if (c == '4') {
      playCategory(3);
    } else if (c == '5') {
      playCategory(4);
    } else if (c == '0') {
      Serial.println("Stop");
      df.stop();
    }
  }
}
