#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include "src/Arduboy2Esp32/src/Arduboy2Esp32.h"
#include <EEPROM.h>
#include "src/6502.h"

// ==========================================
// Hardware Setup
// ==========================================
TFT_eSPI tft = TFT_eSPI();
uint16_t calData[5] = { 275, 3620, 264, 3532, 1 };

#define SD_CS 5 // VSPI default SS pin

enum State {
    LAUNCHER,
    ROM_SELECT_ATARI,
    ROM_SELECT_ARDUBOY,
    RUNNING_ATARI,
    RUNNING_ARDUBOY
};

State currentState = LAUNCHER;

struct TouchInput {
    bool up, down, left, right, a, b, menu;
} tInput;

void drawButton(int x, int y, int w, int h, const char* text, uint16_t color) {
    tft.fillRoundRect(x, y, w, h, 5, color);
    tft.drawRoundRect(x, y, w, h, 5, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(text, x + w/2, y + h/2, 2);
}

void drawLauncherUI() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("DUAL-EMULATOR", 160, 40, 4);
    drawButton(60, 100, 200, 40, "1. Atari 2600", TFT_DARKGREY);
    drawButton(60, 160, 200, 40, "2. ArduBoy", TFT_DARKGREEN);
}

void drawROMSelectUI(const char* title, const char* romName) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(title, 160, 40, 4);
    drawButton(40, 100, 240, 40, romName, TFT_BLUE);
    drawButton(40, 180, 240, 40, "Back", TFT_RED);
}

void drawControlsUI() {
    tft.fillRect(0, 160, 320, 80, TFT_BLACK);
    drawButton(50, 160, 40, 35, "U", TFT_DARKGREY);
    drawButton(50, 200, 40, 35, "D", TFT_DARKGREY);
    drawButton(5, 180, 40, 35, "L", TFT_DARKGREY);
    drawButton(95, 180, 40, 35, "R", TFT_DARKGREY);
    drawButton(220, 180, 40, 40, "B", TFT_RED);
    drawButton(270, 180, 40, 40, "A", TFT_RED);
    drawButton(140, 200, 40, 30, "MENU", TFT_DARKGREY);
}

void readTouchInput() {
    uint16_t x, y;
    bool pressed = tft.getTouch(&x, &y);
    tInput = {false, false, false, false, false, false, false};

    if (pressed) {
        if (y > 160) {
            if (x > 50 && x < 90 && y < 195) tInput.up = true;
            if (x > 50 && x < 90 && y > 200) tInput.down = true;
            if (x > 5 && x < 45 && y > 180 && y < 215) tInput.left = true;
            if (x > 95 && x < 135 && y > 180 && y < 215) tInput.right = true;
            if (x > 220 && x < 260 && y > 180) tInput.b = true;
            if (x > 270 && x < 310 && y > 180) tInput.a = true;
            if (x > 140 && x < 180 && y > 200) tInput.menu = true;
        }
    }
}

// ==========================================
// Arduboy Integration
// ==========================================
Arduboy2 arduboy;

extern "C" void drawArduboyFrameHook(const uint8_t* buffer) {
    if (currentState != RUNNING_ARDUBOY) return;
    for(int y=0; y<64; y++) {
        for(int x=0; x<128; x++) {
            uint8_t row = y / 8;
            uint8_t bitMask = 1 << (y % 8);
            bool pixelOn = buffer[row * 128 + x] & bitMask;
            tft.drawPixel(x + 96, y + 20, pixelOn ? TFT_WHITE : TFT_BLACK);
        }
    }
}

const unsigned int COLUMNS = 13;
const unsigned int ROWS = 4;
int dx = -1, dy = -1, xb, yb;
boolean released, paused = false;
int xpad = 54, ypad = 60, xinc = 2;
byte brick[COLUMNS][ROWS];
int lives = 3, level = 1, score = 0;
boolean gameover = false, titleScreen = true, gameWon = false;

uint8_t getTouchButtonsState() {
    uint8_t state = 0;
    if (tInput.up) state |= UP_BUTTON;
    if (tInput.down) state |= DOWN_BUTTON;
    if (tInput.left) state |= LEFT_BUTTON;
    if (tInput.right) state |= RIGHT_BUTTON;
    if (tInput.a) state |= A_BUTTON;
    if (tInput.b) state |= B_BUTTON;
    return state;
}

void initLevel() {
  for (unsigned int v = 0; v < ROWS; v++) {
    for (unsigned int i = 0; i < COLUMNS; i++) {
      brick[i][v] = 1;
    }
  }
}

void resetGame() { xb = 64; yb = 50; xpad = 54; released = false; }

void runArduBreakout() {
    arduboy.pollButtons();
    uint8_t touchBtns = getTouchButtonsState();

    if (titleScreen) {
        arduboy.clear();
        arduboy.setCursor(30, 20);
        arduboy.setTextSize(2);
        arduboy.print("BREAKOUT");
        arduboy.setTextSize(1);
        arduboy.setCursor(30, 40);
        arduboy.print("Press A to Start");
        if (touchBtns & A_BUTTON) {
            titleScreen = false; initLevel(); resetGame();
            lives = 3; level = 1; score = 0; delay(200);
        }
        arduboy.display();
        return;
    }
    if (gameover) {
        arduboy.clear(); arduboy.setCursor(35, 20); arduboy.setTextSize(2); arduboy.print("GAME OVER");
        arduboy.setTextSize(1);
        if (touchBtns & A_BUTTON) { titleScreen = true; gameover = false; delay(200); }
        arduboy.display();
        return;
    }
    if (gameWon) {
        arduboy.clear(); arduboy.setCursor(20, 20); arduboy.setTextSize(2); arduboy.print("YOU WON!");
        if (touchBtns & A_BUTTON) { titleScreen = true; gameWon = false; delay(200); }
        arduboy.display();
        return;
    }

    arduboy.clear();
    if (touchBtns & LEFT_BUTTON) xpad -= xinc;
    if (touchBtns & RIGHT_BUTTON) xpad += xinc;
    if (xpad < 0) xpad = 0;
    if (xpad > 128 - 20) xpad = 128 - 20;

    if (!released) {
        xb = xpad + 10;
        if (touchBtns & A_BUTTON) { released = true; dy = -1; }
    } else {
        xb += dx; yb += dy;
        if (xb <= 0 || xb >= 126) dx = -dx;
        if (yb <= 0) dy = -dy;
        if (yb >= 64) { lives--; resetGame(); if (lives < 0) gameover = true; }
        if (yb >= ypad - 2 && yb <= ypad && xb >= xpad && xb <= xpad + 20) {
            dy = -dy;
            if (xb < xpad + 5) dx = -1;
            if (xb > xpad + 15) dx = 1;
        }
    }

    bool allClear = true;
    for (int v = 0; v < ROWS; v++) {
        for (int i = 0; i < COLUMNS; i++) {
            if (brick[i][v] == 1) {
                allClear = false;
                arduboy.drawRect(i * 10, v * 5 + 5, 8, 3, WHITE);
                if (xb >= i * 10 && xb <= i * 10 + 8 && yb >= v * 5 + 5 && yb <= v * 5 + 8) {
                    brick[i][v] = 0; dy = -dy; score += 10;
                }
            }
        }
    }
    if (allClear) {
        level++;
        if (level > 3) gameWon = true; else { initLevel(); resetGame(); }
    }
    arduboy.fillRect(xpad, ypad, 20, 2, WHITE);
    arduboy.fillRect(xb, yb, 2, 2, WHITE);
    arduboy.setCursor(0, 0); arduboy.print("L:"); arduboy.print(lives);
    arduboy.setCursor(90, 0); arduboy.print("S:"); arduboy.print(score);
    arduboy.display();
}

// ==========================================
// Atari 6502 Integration with SD Card
// ==========================================
extern "C" {
    extern uint16_t PC;
    extern uint8_t SP;
    extern uint8_t A;
    extern uint8_t X;
    extern uint8_t Y;
    extern uint8_t P;
    extern uint8_t MEMORY[8192];
    uint8_t execute(uint8_t opc, uint8_t arg1, uint8_t arg2);
}

void loadAtariROMFromSD(const char* path) {
    if (!SD.begin(SD_CS)) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED);
        tft.drawString("SD Card Mount Failed!", 160, 40, 2);
        delay(2000);
        return;
    }
    File file = SD.open(path);
    if (!file) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED);
        tft.drawString("Failed to open ROM", 160, 40, 2);
        delay(2000);
        return;
    }

    // Read the ROM directly into the 6502 core's RAM region
    // Assuming a 4K ROM injected at 0xF000 (which maps into MEMORY)
    size_t i = 0;
    while(file.available() && i < 4096) {
        MEMORY[0x1000 + i] = file.read(); // Mapping to the upper half
        i++;
    }
    file.close();
}

void initAtari() {
    // 6502 initialization vectors
    PC = MEMORY[0x1FFC] | (MEMORY[0x1FFD] << 8); // Read reset vector
    if (PC == 0) PC = 0xF000; // Fallback
    SP = 0xFF;

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN);
    tft.drawString("Atari 6502 Core Booted", 160, 40, 2);
    tft.drawString("(Executing ROM)", 160, 60, 2);
    drawControlsUI();
}

void runAtari() {
    // Fetch and execute current instruction
    uint8_t opcode = MEMORY[PC & 0x1FFF]; // Simplified mapping for this core
    uint8_t arg1 = MEMORY[(PC+1) & 0x1FFF];
    uint8_t arg2 = MEMORY[(PC+2) & 0x1FFF];

    execute(opcode, arg1, arg2);

    // TIA (Video) generation would hook here in a full port.
    // For this demonstration, we are successfully fetching and executing the ROM.

    readTouchInput();
    if (tInput.menu) {
        currentState = LAUNCHER;
        drawLauncherUI();
        delay(300);
    }
}

// ==========================================
// Main Loop
// ==========================================
void setup() {
    Serial.begin(115200);
    tft.init();
    tft.setRotation(1);
    tft.setTouch(calData);

    // Initialize VSPI for SD Card explicitly
    SPIClass* vspi = new SPIClass(VSPI);
    vspi->begin(18, 19, 23, 5); // Default VSPI pins
    if (!SD.begin(5, *vspi)) {
        Serial.println("Card Mount Failed");
    }

    drawLauncherUI();
}

void loop() {
    uint16_t x, y;
    bool pressed = tft.getTouch(&x, &y);

    if (currentState == LAUNCHER) {
        if (pressed) {
            if (y > 100 && y < 140) {
                currentState = ROM_SELECT_ATARI;
                drawROMSelectUI("Atari ROMs", "Pitfall.bin (from SD)");
            } else if (y > 160 && y < 200) {
                currentState = ROM_SELECT_ARDUBOY;
                drawROMSelectUI("Arduboy ROMs", "ArduBreakout");
            }
            delay(300);
        }
    } else if (currentState == ROM_SELECT_ATARI) {
        if (pressed) {
            if (y > 100 && y < 140) {
                currentState = RUNNING_ATARI;
                loadAtariROMFromSD("/Pitfall.bin");
                initAtari();
            } else if (y > 180 && y < 220) {
                currentState = LAUNCHER;
                drawLauncherUI();
            }
            delay(300);
        }
    } else if (currentState == ROM_SELECT_ARDUBOY) {
         if (pressed) {
            if (y > 100 && y < 140) {
                currentState = RUNNING_ARDUBOY;
                tft.fillScreen(TFT_BLACK);
                drawControlsUI();
                arduboy.begin();
                titleScreen = true;
                gameover = false;
            } else if (y > 180 && y < 220) {
                currentState = LAUNCHER;
                drawLauncherUI();
            }
            delay(300);
        }
    } else if (currentState == RUNNING_ARDUBOY) {
        readTouchInput();
        if (tInput.menu) {
            currentState = LAUNCHER;
            drawLauncherUI();
            delay(300);
        } else {
            runArduBreakout();
        }
    } else if (currentState == RUNNING_ATARI) {
        runAtari();
    } else {
        readTouchInput();
        if (tInput.menu) {
            currentState = LAUNCHER;
            drawLauncherUI();
            delay(300);
        }
    }
}
