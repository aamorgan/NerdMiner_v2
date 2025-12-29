#pragma once

/***********************config*************************/
#define LCD_USB_QSPI_DREVER 1

#ifndef SPI_FREQUENCY
#define SPI_FREQUENCY 75000000
#endif

#ifndef TFT_SPI_MODE
#define TFT_SPI_MODE SPI_MODE0
#endif

#ifndef TFT_SPI_HOST
#define TFT_SPI_HOST SPI2_HOST
#endif

#define EXAMPLE_LCD_H_RES 536
#define EXAMPLE_LCD_V_RES 240
#define LVGL_LCD_BUF_SIZE (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES)

/***********************config*************************/

#ifndef TFT_WIDTH
#define TFT_WIDTH 240
#endif

#ifndef TFT_HEIGHT
#define TFT_HEIGHT 536
#endif

#ifndef SEND_BUF_SIZE
#define SEND_BUF_SIZE (0x4000) //(LCD_WIDTH * LCD_HEIGHT + 8) / 10
#endif

#ifndef TFT_TE
#define TFT_TE 9
#endif

#ifndef TFT_SDO
#define TFT_SDO 8
#endif

#ifndef TFT_DC
#define TFT_DC 7
#endif

#ifndef TFT_RES
#define TFT_RES 17
#endif

#ifndef TFT_CS
#define TFT_CS 6
#endif

#ifndef TFT_MOSI
#define TFT_MOSI 18
#endif

#ifndef TFT_SCK
#define TFT_SCK 47
#endif

#define TFT_QSPI_CS 6
#define TFT_QSPI_SCK 47
#define TFT_QSPI_D0 18
#define TFT_QSPI_D1 7
#define TFT_QSPI_D2 48
#define TFT_QSPI_D3 5
#define TFT_QSPI_RST 17

#define PIN_LED 38
#define PIN_BAT_VOLT 4

#define PIN_BUTTON_1 0
#define PIN_BUTTON_2 21