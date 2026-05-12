#include <Arduino.h>
#include <esp_system.h>
#include <DFRobotDFPlayerMini.h>

HardwareSerial dfSerial(2);
DFRobotDFPlayerMini df;

#define DF_RX 27
#define DF_TX 26

const int swPins[5] = {4, 5, 18, 19, 15};
const int fileCount[5] = {5, 5, 5, 5, 5};
const unsigned long debounceDelay = 250;
unsigned long lastButtonPress[5] = {0, 0, 0, 0, 0};

const char* categoryNames[5] = {
  "Animals",
  "Fruits",
  "Colors",
  "Numbers",
  "Greetings"
};

const char* wordNames[5][5] = {
  {"cat", "dog", "bird", "fish", "elephant"},
  {"apple", "banana", "orange", "mango", "grape"},
  {"red", "blue", "green", "yellow", "black"},
  {"one", "two", "three", "four", "five"},
  {"hello", "goodbye", "thank you", "sorry", "good morning"}
};

void playCategory(int categoryIndex) {
  int folderNumber = categoryIndex + 1;
  int fileNumber = random(1, fileCount[categoryIndex] + 1);

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
  Serial.println(wordNames[categoryIndex][fileNumber - 1]);
  Serial.println("================================");

  df.playFolder(folderNumber, fileNumber);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  for (int i = 0; i < 5; i++) {
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

  Serial.println("Ready: SW1-SW5 play folders 01-05");
}

void loop() {
  for (int i = 0; i < 5; i++) {
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
