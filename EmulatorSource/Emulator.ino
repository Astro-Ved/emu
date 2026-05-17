#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

uint16_t calData[5] = { 275, 3620, 264, 3532, 1 };

enum State {
    LAUNCHER,
    ROM_SELECTION_ATARI,
    ROM_SELECTION_ARDUBOY,
    PLAYING_ATARI_PONG,
    PLAYING_ARDUBOY_SNAKE
};

State currentState = LAUNCHER;

bool t_up = false;
bool t_down = false;
bool t_left = false;
bool t_right = false;
bool t_a = false;
bool t_b = false;
bool t_menu = false;

int paddle1Y = 60;
int paddle2Y = 60;
int ballX = 160;
int ballY = 80;
int ballDirX = 2;
int ballDirY = 2;
int score1 = 0;
int score2 = 0;

int snakeX[100];
int snakeY[100];
int snakeLen = 3;
int snakeDir = 1;
int foodX = 0;
int foodY = 0;
bool gameOver = false;

void drawButton(int x, int y, int w, int h, const char* text, uint16_t color) {
    tft.fillRoundRect(x, y, w, h, 5, color);
    tft.drawRoundRect(x, y, w, h, 5, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(text, x + w/2, y + h/2, 2);
}

void drawLauncher() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("MULTI-EMULATOR", 160, 40, 4);

    drawButton(60, 100, 200, 40, "1. Atari Emulator", TFT_DARKGREY);
    drawButton(60, 160, 200, 40, "2. ArduBoy Emulator", TFT_DARKGREEN);
}

void drawRomSelection(const char* title, const char* rom1) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(title, 160, 30, 4);

    drawButton(40, 80, 240, 40, rom1, TFT_BLUE);
    drawButton(40, 180, 240, 40, "Back to Launcher", TFT_RED);
}

void drawControls() {
    tft.fillRect(0, 160, 320, 80, TFT_BLACK);
    drawButton(50, 160, 40, 35, "U", TFT_DARKGREY);
    drawButton(50, 200, 40, 35, "D", TFT_DARKGREY);
    drawButton(5, 180, 40, 35, "L", TFT_DARKGREY);
    drawButton(95, 180, 40, 35, "R", TFT_DARKGREY);

    drawButton(220, 180, 40, 40, "B", TFT_RED);
    drawButton(270, 180, 40, 40, "A", TFT_RED);

    drawButton(140, 200, 40, 30, "MENU", TFT_DARKGREY);
}

void readTouch() {
    uint16_t x, y;
    bool pressed = tft.getTouch(&x, &y);
    t_up = t_down = t_left = t_right = t_a = t_b = t_menu = false;

    if (pressed) {
        if (y > 160) {
            if (x > 50 && x < 90 && y < 195) t_up = true;
            if (x > 50 && x < 90 && y > 200) t_down = true;
            if (x > 5 && x < 45 && y > 180 && y < 215) t_left = true;
            if (x > 95 && x < 135 && y > 180 && y < 215) t_right = true;

            if (x > 220 && x < 260 && y > 180) t_b = true;
            if (x > 270 && x < 310 && y > 180) t_a = true;

            if (x > 140 && x < 180 && y > 200) t_menu = true;
        }
    }
}

void initPong() {
    paddle1Y = 60;
    paddle2Y = 60;
    ballX = 160;
    ballY = 80;
    score1 = 0;
    score2 = 0;
    tft.fillScreen(TFT_BLACK);
    drawControls();
}

void runPong() {
    readTouch();
    if (t_menu) {
        currentState = LAUNCHER;
        drawLauncher();
        delay(300);
        return;
    }

    tft.fillRect(0, 0, 320, 160, TFT_BLACK);

    if (t_up && paddle1Y > 0) paddle1Y -= 4;
    if (t_down && paddle1Y < 120) paddle1Y += 4;

    if (ballY < paddle2Y + 20) paddle2Y -= 3;
    if (ballY > paddle2Y + 20) paddle2Y += 3;

    ballX += ballDirX;
    ballY += ballDirY;

    if (ballY <= 0 || ballY >= 150) ballDirY = -ballDirY;

    if (ballX <= 20 && ballY >= paddle1Y && ballY <= paddle1Y + 40) ballDirX = -ballDirX;
    if (ballX >= 300 && ballY >= paddle2Y && ballY <= paddle2Y + 40) ballDirX = -ballDirX;

    if (ballX < 0) { score2++; ballX = 160; ballY = 80; }
    if (ballX > 320) { score1++; ballX = 160; ballY = 80; }

    tft.fillRect(10, paddle1Y, 10, 40, TFT_WHITE);
    tft.fillRect(300, paddle2Y, 10, 40, TFT_WHITE);
    tft.fillRect(ballX, ballY, 6, 6, TFT_WHITE);

    tft.setTextDatum(MC_DATUM);
    tft.drawNumber(score1, 100, 20, 2);
    tft.drawNumber(score2, 220, 20, 2);

    delay(20);
}

void initSnake() {
    snakeLen = 3;
    snakeDir = 1;
    for(int i=0; i<snakeLen; i++) {
        snakeX[i] = 10 - i;
        snakeY[i] = 10;
    }
    foodX = random(0, 32);
    foodY = random(0, 16);
    gameOver = false;
    tft.fillScreen(TFT_BLACK);

    tft.drawRect(31, 15, 258, 130, TFT_DARKGREY);

    drawControls();
}

unsigned long lastSnakeMove = 0;

void runSnake() {
    readTouch();
    if (t_menu) {
        currentState = LAUNCHER;
        drawLauncher();
        delay(300);
        return;
    }

    if (gameOver) {
        if (t_a || t_b) initSnake();
        return;
    }

    if (t_up && snakeDir != 2) snakeDir = 0;
    else if (t_right && snakeDir != 3) snakeDir = 1;
    else if (t_down && snakeDir != 0) snakeDir = 2;
    else if (t_left && snakeDir != 1) snakeDir = 3;

    if (millis() - lastSnakeMove > 100) {
        lastSnakeMove = millis();

        tft.fillRect(32, 16, 256, 128, TFT_BLACK);

        for(int i=snakeLen-1; i>0; i--) {
            snakeX[i] = snakeX[i-1];
            snakeY[i] = snakeY[i-1];
        }

        if (snakeDir == 0) snakeY[0]--;
        if (snakeDir == 1) snakeX[0]++;
        if (snakeDir == 2) snakeY[0]++;
        if (snakeDir == 3) snakeX[0]--;

        if (snakeX[0] == foodX && snakeY[0] == foodY) {
            snakeLen++;
            foodX = random(0, 32);
            foodY = random(0, 16);
        }

        if (snakeX[0] < 0 || snakeX[0] >= 32 || snakeY[0] < 0 || snakeY[0] >= 16) {
            gameOver = true;
        }
        for(int i=1; i<snakeLen; i++) {
            if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) gameOver = true;
        }

        tft.fillRect(32 + foodX*8, 16 + foodY*8, 8, 8, TFT_WHITE);

        for(int i=0; i<snakeLen; i++) {
            tft.fillRect(32 + snakeX[i]*8, 16 + snakeY[i]*8, 8, 8, TFT_WHITE);
        }

        if (gameOver) {
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.drawString("GAME OVER", 160, 80, 2);
        }
    }
}

void setup() {
    Serial.begin(115200);
    tft.init();
    tft.setRotation(1);
    tft.setTouch(calData);
    drawLauncher();
}

void loop() {
    uint16_t x, y;
    bool pressed = tft.getTouch(&x, &y);

    if (currentState == LAUNCHER) {
        if (pressed) {
            if (y > 100 && y < 140) {
                currentState = ROM_SELECTION_ATARI;
                drawRomSelection("Atari 2600 ROMs", "1. Pong");
            } else if (y > 160 && y < 200) {
                currentState = ROM_SELECTION_ARDUBOY;
                drawRomSelection("ArduBoy ROMs", "1. Snake");
            }
            delay(300);
        }
    } else if (currentState == ROM_SELECTION_ATARI) {
        if (pressed) {
            if (y > 80 && y < 120) {
                currentState = PLAYING_ATARI_PONG;
                initPong();
            } else if (y > 180 && y < 220) {
                currentState = LAUNCHER;
                drawLauncher();
            }
            delay(300);
        }
    } else if (currentState == ROM_SELECTION_ARDUBOY) {
        if (pressed) {
            if (y > 80 && y < 120) {
                currentState = PLAYING_ARDUBOY_SNAKE;
                initSnake();
            } else if (y > 180 && y < 220) {
                currentState = LAUNCHER;
                drawLauncher();
            }
            delay(300);
        }
    } else if (currentState == PLAYING_ATARI_PONG) {
        runPong();
    } else if (currentState == PLAYING_ARDUBOY_SNAKE) {
        runSnake();
    }
}
