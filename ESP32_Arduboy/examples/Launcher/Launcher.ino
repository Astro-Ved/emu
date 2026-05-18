#include <Arduboy2.h>
#include <SD.h>
#include <SPI.h>
#include <Update.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"

Arduboy2 arduboy;

#define MAX_FILES 50
String fileNames[MAX_FILES];
int fileCount = 0;
int selectedFile = 0;

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(15);

  // VSPI pins for SD Card
  SPI.begin(18, 19, 23, 5);
  if (!SD.begin(5, SPI)) {
    arduboy.clear();
    arduboy.setCursor(0, 0);
    arduboy.print("SD Card Mount Failed");
    arduboy.display();
    while(1);
  }

  File root = SD.open("/");
  if (!root) {
    arduboy.clear();
    arduboy.setCursor(0, 0);
    arduboy.print("Failed to open root");
    arduboy.display();
    while(1);
  }

  File file = root.openNextFile();
  while (file && fileCount < MAX_FILES) {
    if (!file.isDirectory()) {
      String name = file.name();
      if (name.endsWith(".bin") || name.endsWith(".BIN")) {
        fileNames[fileCount] = name;
        fileCount++;
      }
    }
    file = root.openNextFile();
  }
}

void flashGame(String filename) {
  File file = SD.open("/" + filename);
  if (!file) {
    arduboy.clear();
    arduboy.setCursor(0, 0);
    arduboy.print("Failed to open file");
    arduboy.display();
    delay(2000);
    return;
  }

  arduboy.clear();
  arduboy.setCursor(0, 0);
  arduboy.print("Flashing...");
  arduboy.setCursor(0, 10);
  arduboy.print(filename);
  arduboy.display();

  size_t fileSize = file.size();
  if (Update.begin(fileSize, U_FLASH)) {
    size_t written = Update.writeStream(file);
    if (written == fileSize) {
      if (Update.end()) {
        arduboy.clear();
        arduboy.setCursor(0, 0);
        arduboy.print("Flash success!");
        arduboy.display();
        delay(1000);
        ESP.restart();
      }
    }
  }

  arduboy.clear();
  arduboy.setCursor(0, 0);
  arduboy.print("Flash failed!");
  arduboy.display();
  delay(2000);
}

void loop() {
  if (!arduboy.nextFrame()) return;
  arduboy.pollButtons();

  if (arduboy.justPressed(DOWN_BUTTON)) {
    selectedFile++;
    if (selectedFile >= fileCount) selectedFile = 0;
  }
  if (arduboy.justPressed(UP_BUTTON)) {
    selectedFile--;
    if (selectedFile < 0) selectedFile = fileCount - 1;
  }

  if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
    if (fileCount > 0) {
      flashGame(fileNames[selectedFile]);
    }
  }

  arduboy.clear();

  arduboy.setCursor(0, 0);
  arduboy.print("Arduboy SD Launcher");

  if (fileCount == 0) {
    arduboy.setCursor(0, 20);
    arduboy.print("No .bin files found");
  } else {
    int startIdx = max(0, selectedFile - 2);
    int endIdx = min(fileCount, startIdx + 5);

    for (int i = startIdx; i < endIdx; i++) {
      int y = 20 + (i - startIdx) * 10;
      if (i == selectedFile) {
        arduboy.setCursor(0, y);
        arduboy.print(">");
      }
      arduboy.setCursor(10, y);
      arduboy.print(fileNames[i]);
    }
  }

  arduboy.display();
}
