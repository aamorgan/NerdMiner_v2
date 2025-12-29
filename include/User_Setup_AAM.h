#define USER_SETUP_INFO "User_Setup"

#define ILI9341_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Pin definitions - PLEASE EDIT THESE TO MATCH YOUR HARDWARE
// Updated for standard ESP32-2432S028R (Cheap Yellow Display)
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define TFT_CS   4
#define TFT_DC   16
#define TFT_RST  17
#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

#define TOUCH_CS 2 // Chip select pin for touch controller
#define TOUCH_CLK 18
#define TOUCH_MISO 19
#define TOUCH_MOSI 23
#define TOUCH_IRQ 15

/* Original VSPI pins (commented out)
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define TFT_CS   4
#define TFT_DC   16
#define TFT_RST  17
#define TFT_BL   21
#define TOUCH_CS 2
*/

#define TFT_INVERSION_ON

#define SPI_FREQUENCY        27000000
#define SPI_READ_FREQUENCY   20000000
#define SPI_TOUCH_FREQUENCY  2500000

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT
