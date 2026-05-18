// User_Setup.h for Elecrow Crowpanel Basic 2.4 inch (CROWPANEL)
// Copy this to your TFT_eSPI/User_Setup.h or use it in your PlatformIO/Arduino config

#define ILI9341_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1
#define TFT_BL   27

#define TOUCH_CS 33

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

#define SPI_FREQUENCY  15999999
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  600000
