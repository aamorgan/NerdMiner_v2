#include "displayDriver.h"

#if defined(DEVKIT_AAM)

#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include "SPIFFS.h"
#include "media/Free_Fonts.h"
#include "OpenFontRender.h"
#include "media/images_320_170.h"
#include "media/images_bottom_320_70.h"
#include "media/myFonts.h"
#include "aamDisplayData.h"
#include "monitor.h"
#include "drivers/storage/storage.h"

// Global objects
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite background = TFT_eSprite(&tft);
OpenFontRender render;

extern TSettings Settings;

// Test function - can be called to verify display works from a given context
void testPostSplashDraw() {
  tft.fillScreen(TFT_PURPLE);
  tft.fillRect(50, 50, 100, 100, TFT_YELLOW);
  tft.setTextColor(TFT_WHITE, TFT_PURPLE);
  tft.setTextFont(4);
  tft.drawString("Post-Splash OK", 80, 120);
}

// Display dimensions (landscape 320x240)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define WIDTH 130
#define HEIGHT 170

// Screen definitions
#define SCREEN_MINING   0
#define SCREEN_CLOCK    1
#define SCREEN_GLOBAL   2
#define SCREEN_BTCPRICE 3
#define SCREEN_REMOTE   4
#define NUM_SCREENS     5

// State variables
int currentScreen = SCREEN_MINING;
unsigned long lastScreenChange = 0;
const unsigned long SCREEN_CYCLE_TIME = 10000; // 10 seconds per screen
bool hasChangedScreen = true;

// Touch calibration file
#define CALIBRATION_FILE "/TouchCalData1"
#define REPEAT_CAL false

// Forward declarations
void touch_calibrate();
void drawMiningScreen(unsigned long mElapsed);
void drawClockScreen(unsigned long mElapsed);
void drawGlobalScreen(unsigned long mElapsed);
void drawBTCPriceScreen(unsigned long mElapsed);
void drawRemoteScreen(unsigned long mElapsed);
void printPoolData();
bool createBackgroundSprite(int16_t wdt, int16_t hgt);

// External variables required by NerdMiner
extern DisplayDriver *currentDisplayDriver;

// Display data storage
static AAMDisplayData displayData;

// Update display data from real mining data
static void updateDisplayData(unsigned long mElapsed)
{
  // Get real data from monitor functions
  mining_data mData = getMiningData(mElapsed);
  clock_data cData = getClockData(mElapsed);
  coin_data coinData = getCoinData(mElapsed);
  pool_data poolData = getPoolData();

  // Pool footer data
  displayData.pool.workersCount = String(poolData.workersCount);
  displayData.pool.workersHash = poolData.workersHash;
  displayData.pool.bestDifficulty = poolData.bestDifficulty;

  // Mining screen data
  displayData.mining.totalMHashes = mData.totalMHashes;
  displayData.mining.templates = mData.templates;
  displayData.mining.bestDiff = mData.bestDiff;
  displayData.mining.completedShares = mData.completedShares;
  displayData.mining.timeMining = mData.timeMining;
  displayData.mining.valids = mData.valids;
  displayData.mining.temp = mData.temp;
  displayData.mining.currentTime = mData.currentTime;
  displayData.mining.currentHashRate = mData.currentHashRate;

  // Clock screen data
  displayData.clock.currentHashRate = cData.currentHashRate;
  displayData.clock.blockHeight = cData.blockHeight;
  displayData.clock.btcPrice = cData.btcPrice;
  displayData.clock.currentTime = cData.currentTime;

  // Remote screen data
  if (currentScreen == SCREEN_REMOTE) {
    remote_data rData = getRemoteMinerData();
    displayData.remote.board = rData.board;
    displayData.remote.hashRate = rData.hashRate;
    displayData.remote.shares = rData.shares;
    displayData.remote.bestDiff = rData.bestDiff;
    displayData.remote.valid = rData.valid;
    displayData.remote.rssi = rData.rssi;
    displayData.remote.connected = rData.connected;
    displayData.remote.currentTime = cData.currentTime;
    displayData.remote.timeMining = rData.timeMining;
  }

  // Global screen data
  displayData.global.btcPrice = coinData.btcPrice;
  displayData.global.currentTime = coinData.currentTime;
  displayData.global.halfHourFee = coinData.halfHourFee;
  displayData.global.networkDifficulty = coinData.networkDifficulty;
  displayData.global.globalHashRate = coinData.globalHashRate;
  displayData.global.remainingBlocks = coinData.remainingBlocks;
  displayData.global.blockHeight = coinData.blockHeight;
  displayData.global.progressPercent = coinData.progressPercent;

  // Price screen data
  displayData.price.currentHashRate = mData.currentHashRate;
  displayData.price.blockHeight = cData.blockHeight;
  displayData.price.currentTime = cData.currentTime;
  displayData.price.btcPrice = cData.btcPrice;
}

void aamDisplay_SetData(const AAMDisplayData &data)
{
  displayData = data;
  hasChangedScreen = true;
}

const AAMDisplayData &aamDisplay_GetData()
{
  return displayData;
}

// --- Helper Functions from Demo ---

void touch_calibrate() {
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  if (!SPIFFS.begin(true)) { // Use true to format if failed
    Serial.println("Formatting file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL) {
      SPIFFS.remove(CALIBRATION_FILE);
    } else {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    tft.setTouch(calData);
  } else {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.println("Touch corners as indicated");
    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

// Called at start of FreeRTOS monitor task to reinit TFT for task context
void aamDisplay_TaskInit() {
  Serial.println("[AAM] TaskInit - Reinitializing TFT for FreeRTOS task");
  
  // Small delay to ensure task is fully running
  delay(100);
  
  // Explicitly reinitialize SPI for this task context
  SPI.end();
  delay(50);
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  Serial.println("[AAM] SPI reinitialized");
  
  // Explicitly set backlight pin
  #ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);  // Force HIGH
  delay(50);
  Serial.printf("[AAM] Backlight pin %d set HIGH\n", TFT_BL);
  #endif
  
  // Full TFT reinit
  tft.init();
  delay(100);
  tft.setRotation(1);
  tft.setSwapBytes(true);
  Serial.println("[AAM] TFT reinitialized");
  
  // Reload font after reinit
  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers))) {
    Serial.println("[AAM] Font reload error");
  } else {
    Serial.println("[AAM] Font reloaded OK");
  }
  
  hasChangedScreen = true;
  lastScreenChange = millis();
  Serial.println("[AAM] TaskInit complete");
}

bool createBackgroundSprite(int16_t wdt, int16_t hgt) {
  if (background.created()) {
    background.deleteSprite();
  }
  background.createSprite(wdt, hgt);
  if (background.created()) {
    background.setColorDepth(16);
    background.setSwapBytes(true);
    render.setDrawer(background);
    render.setLineSpaceRatio(0.9);
    return true;
  }
  Serial.printf("[AAM] FAILED to create sprite %dx%d, heap=%u\n", wdt, hgt, ESP.getFreeHeap());
  return false;
}

void printPoolData() {
  // Print pool data at bottom of screen
  if (hasChangedScreen) {
    // Draw bottom pool screen
    tft.pushImage(0, 170, 320, 70, bottonPoolScreen);
    
    // Create sprite for pool data
    if (createBackgroundSprite(320, 50)) {
      background.pushImage(0, -20, 320, 70, bottonPoolScreen);
      
      render.setDrawer(background);
      render.setLineSpaceRatio(1);
      
      const PoolFooterData &footer = displayData.pool;

      // Workers count
      render.setFontSize(24);
      render.cdrawString(footer.workersCount.c_str(), 157, 16, TFT_BLACK);
      
      // Workers hash
      render.setFontSize(18);
      render.setAlignment(Align::BottomRight);
      render.cdrawString(footer.workersHash.c_str(), 265, 14, TFT_BLACK);
      
      // Best difficulty
      render.setAlignment(Align::BottomLeft);
      render.cdrawString(footer.bestDifficulty.c_str(), 54, 14, TFT_BLACK);
      
      background.pushSprite(0, 190);
      background.deleteSprite();
    }
  }
}

// --- Screen Drawing Functions ---

void drawMiningScreen(unsigned long mElapsed) {
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();

  if (!hasChangedScreen && currentMillis - lastUpdate < 1000) {
    return;
  }
  lastUpdate = currentMillis;
  
  // Update display data with real mining data
  updateDisplayData(mElapsed);
  
  // Draw background image if screen changed
  if (hasChangedScreen) {
    tft.fillScreen(TFT_BLACK);
    tft.pushImage(0, 0, initWidth, initHeight, MinerScreen);
  }
  
  printPoolData();
  hasChangedScreen = false;

  const MiningScreenData &data = displayData.mining;
  
  // Create sprite for right side of screen
  int wdtOffset = 190;
  if (createBackgroundSprite(WIDTH-5, HEIGHT-7)) {
    background.pushImage(-190, 0, MinerWidth, MinerHeight, MinerScreen);
    
    // Total hashes
    render.setFontSize(18);
    render.rdrawString(data.totalMHashes.c_str(), 268-wdtOffset, 138, TFT_BLACK);
    
    // Block templates
    render.setAlignment(Align::TopLeft);
    render.drawString(data.templates.c_str(), 189-wdtOffset, 20, 0xDEDB);
    
    // Best diff
    render.drawString(data.bestDiff.c_str(), 189-wdtOffset, 48, 0xDEDB);
    
    // Shares
    render.setFontSize(18);
    render.drawString(data.completedShares.c_str(), 189-wdtOffset, 76, 0xDEDB);
    
    // Mining time
    render.setFontSize(14);
    render.rdrawString(data.timeMining.c_str(), 315-wdtOffset, 104, 0xDEDB);
    
    // Valid blocks
    render.setFontSize(24);
    render.setAlignment(Align::TopCenter);
    render.drawString(data.valids.c_str(), 290-wdtOffset, 56, 0xDEDB);
    
    // Temperature
    render.setFontSize(10);
    render.rdrawString(data.temp.c_str(), 239-wdtOffset, 1, TFT_BLACK);
    render.setFontSize(4);
    render.rdrawString("C", 244-wdtOffset, 3, TFT_BLACK);
    
    // Current time
    render.setFontSize(10);
    render.rdrawString(data.currentTime.c_str(), 286-wdtOffset, 1, TFT_BLACK);
    
    background.pushSprite(190, 0);
    background.deleteSprite();
  }
  
  // Create sprite for hashrate (center-left)
  if (createBackgroundSprite(WIDTH-7, HEIGHT-100)) {
    background.pushImage(0, -90, MinerWidth, MinerHeight, MinerScreen);
    
    // Hashrate display
    render.setFontSize(30);
    render.setFontColor(TFT_BLACK);
    render.rdrawString(data.currentHashRate.c_str(), 118, 114-90, TFT_BLACK);
    
    background.pushSprite(0, 90);
    background.deleteSprite();
  }
}

void drawClockScreen(unsigned long mElapsed) {
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();
  
  if (!hasChangedScreen && currentMillis - lastUpdate < 1000) return;
  lastUpdate = currentMillis;
  
  // Update display data with real mining data
  updateDisplayData(mElapsed);
  
  if (hasChangedScreen) {
    tft.fillScreen(TFT_BLACK);
    tft.pushImage(0, 0, minerClockWidth, minerClockHeight, minerClockScreen);
  }
  
  printPoolData();
  hasChangedScreen = false;
  
  const ClockScreenData &data = displayData.clock;
  
  // Top section with hashrate and block height
  if (createBackgroundSprite(270, 36)) {
    background.pushImage(0, -130, minerClockWidth, minerClockHeight, minerClockScreen);
    
    // Hashrate
    render.setFontSize(25);
    render.setFontColor(TFT_BLACK);
    render.rdrawString(data.currentHashRate.c_str(), 95, 0, TFT_BLACK);
    
    // Block height
    render.setFontSize(18);
    render.rdrawString(data.blockHeight.c_str(), 254, 9, TFT_BLACK);
    
    background.pushSprite(0, 130);
    background.deleteSprite();
  }
  
  // Right section with BTC price and clock
  if (createBackgroundSprite(169, 105)) {
    background.pushImage(-130, -3, minerClockWidth, minerClockHeight, minerClockScreen);
    
    // BTC Price
    background.setFreeFont(FSSB9);
    background.setTextSize(1);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(TFT_BLACK);
    background.drawString(data.btcPrice.c_str(), 202-130, 0, GFXFF);
    
    // Clock
    background.setFreeFont(FF23);
    background.setTextSize(2);
    background.setTextColor(0xDEDB, TFT_BLACK);
    background.drawString(data.currentTime.c_str(), 0, 50, GFXFF);
    
    background.pushSprite(130, 3);
    background.deleteSprite();
  }
}

void drawRemoteScreen(unsigned long mElapsed) {
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();
  
  if (!hasChangedScreen && currentMillis - lastUpdate < 5000) return;
  lastUpdate = currentMillis;
  
  updateDisplayData(mElapsed);
  
  if (hasChangedScreen) {
    tft.fillScreen(TFT_BLACK);
    tft.pushImage(0, 0, initWidth, initHeight, MinerScreen);
  }
  
  printPoolData();
  hasChangedScreen = false;
  
  const RemoteScreenData &data = displayData.remote;
  
  if (!data.connected) {
      if (createBackgroundSprite(320, 170)) {
          background.pushImage(0, 0, initWidth, initHeight, MinerScreen);
          render.setDrawer(background);
          render.setFontSize(18);
          render.setAlignment(Align::TopCenter);
          render.drawString("Connecting...", 160, 60, TFT_RED);
          render.setFontSize(12);
          render.drawString(Settings.RemoteMinerURL.c_str(), 160, 90, TFT_BLACK);
          background.pushSprite(0, 0);
          background.deleteSprite();
      }
      return;
  }

  // Create sprite for right side of screen (Stats)
  int wdtOffset = 190;
  if (createBackgroundSprite(WIDTH-5, HEIGHT-7)) {
    background.pushImage(-190, 0, MinerWidth, MinerHeight, MinerScreen);
    
    // RSSI (replacing Total Hashes position)
    render.setFontSize(18);
    render.rdrawString(data.rssi.c_str(), 268-wdtOffset, 138, TFT_BLACK);
    
    // Label (replacing Templates position)
    render.setAlignment(Align::TopLeft);
    if (data.board.length() > 0) {
        render.drawString(data.board.c_str(), 189-wdtOffset, 20, 0xDEDB);
    } else {
        render.drawString("Remote", 189-wdtOffset, 20, 0xDEDB);
    }
    
    // Best diff
    render.drawString(data.bestDiff.c_str(), 189-wdtOffset, 48, 0xDEDB);
    
    // Shares
    render.setFontSize(18);
    render.drawString(data.shares.c_str(), 189-wdtOffset, 76, 0xDEDB);
    
    // Mining time
    render.setFontSize(14);
    render.rdrawString(data.timeMining.c_str(), 315-wdtOffset, 104, 0xDEDB);
    
    // Valid blocks
    render.setFontSize(24);
    render.setAlignment(Align::TopCenter);
    render.drawString(data.valid.c_str(), 290-wdtOffset, 56, 0xDEDB);
    
    // Current time
    render.setFontSize(10);
    render.rdrawString(data.currentTime.c_str(), 286-wdtOffset, 1, TFT_BLACK);
    
    background.pushSprite(190, 0);
    background.deleteSprite();
  }
  
  // Create sprite for hashrate (center-left)
  if (createBackgroundSprite(WIDTH-7, HEIGHT-100)) {
    background.pushImage(0, -90, MinerWidth, MinerHeight, MinerScreen);
    
    // Hashrate display
    render.setFontSize(30);
    render.setFontColor(TFT_BLACK);
    render.rdrawString(data.hashRate.c_str(), 118, 114-90, TFT_BLACK);
    
    background.pushSprite(0, 90);
    background.deleteSprite();
  }
}

void drawGlobalScreen(unsigned long mElapsed) {
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();
  
  if (!hasChangedScreen && currentMillis - lastUpdate < 1000) return;
  lastUpdate = currentMillis;
  
  // Update display data with real mining data
  updateDisplayData(mElapsed);
  
  if (hasChangedScreen) {
    tft.fillScreen(TFT_BLACK);
    tft.pushImage(0, 0, globalHashWidth, globalHashHeight, globalHashScreen);
  }
  
  printPoolData();
  hasChangedScreen = false;
  
  const GlobalScreenData &data = displayData.global;
  
  // Top right section with prices and time
  if (createBackgroundSprite(169, 105)) {
    background.pushImage(-160, -3, globalHashWidth, globalHashHeight, globalHashScreen);
    
    // BTC Price
    background.setFreeFont(FSSB9);
    background.setTextSize(1);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(TFT_BLACK);
    background.drawString(data.btcPrice.c_str(), 198-160, 0, GFXFF);
    
    // Current time
    background.drawString(data.currentTime.c_str(), 268-160, 0, GFXFF);
    
    // Fee
    background.setFreeFont(FSS9);
    background.setTextDatum(TR_DATUM);
    background.setTextColor(0x9C92);
    background.drawString(data.halfHourFee.c_str(), 302-160, 49, GFXFF);
    
    // Difficulty
    background.drawString(data.networkDifficulty.c_str(), 302-160, 85, GFXFF);
    
    background.pushSprite(160, 3);
    background.deleteSprite();
  }
  
  // Middle section with global hashrate
  if (createBackgroundSprite(280, 30)) {
    background.pushImage(0, -139, globalHashWidth, globalHashHeight, globalHashScreen);
    
    // Global hashrate
    render.setFontSize(17);
    render.rdrawString(data.globalHashRate.c_str(), 274, 145-139, TFT_BLACK);
    
    // Halving progress bar
    float progress = data.progressPercent / 100.0f;
    int x2 = 2 + (138 * progress);
    background.fillRect(2, 149-139, x2, 168, 0xDEDB);
    
    // Remaining blocks
    background.setTextFont(FONT2);
    background.setTextSize(1);
    background.setTextDatum(MC_DATUM);
    background.setTextColor(TFT_BLACK);
    background.drawString(data.remainingBlocks.c_str(), 72, 159-139, FONT2);
    
    background.pushSprite(0, 139);
    background.deleteSprite();
  }
  
  // Block height section
  if (createBackgroundSprite(140, 40)) {
    background.pushImage(-5, -100, globalHashWidth, globalHashHeight, globalHashScreen);
    
    render.setFontSize(28);
    render.rdrawString(data.blockHeight.c_str(), 140-5, 104-100, 0xDEDB);
    
    background.pushSprite(5, 100);
    background.deleteSprite();
  }
}

void drawBTCPriceScreen(unsigned long mElapsed) {
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();
  
  if (!hasChangedScreen && currentMillis - lastUpdate < 1000) return;
  lastUpdate = currentMillis;
  
  // Update display data with real mining data
  updateDisplayData(mElapsed);
  
  if (hasChangedScreen) {
    tft.fillScreen(TFT_BLACK);
    tft.pushImage(0, 0, priceScreenWidth, priceScreenHeight, priceScreen);
  }
  
  printPoolData();
  hasChangedScreen = false;
  
  const PriceScreenData &data = displayData.price;
  
  // Top section with hashrate and block height
  if (createBackgroundSprite(270, 36)) {
    background.pushImage(0, -130, priceScreenWidth, priceScreenHeight, priceScreen);
    
    // Hashrate
    render.setFontSize(25);
    render.setFontColor(TFT_BLACK);
    render.rdrawString(data.currentHashRate.c_str(), 95, 0, TFT_BLACK);
    
    // Block height
    render.setFontSize(18);
    render.rdrawString(data.blockHeight.c_str(), 254, 9, TFT_WHITE);
    
    background.pushSprite(0, 130);
    background.deleteSprite();
  }
  
  // Right section with time and BTC price
  if (createBackgroundSprite(180, 105)) {
    background.pushImage(-130, -3, priceScreenWidth, priceScreenHeight, priceScreen);
    
    // Time
    background.setFreeFont(FSSB9);
    background.setTextSize(1);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(TFT_BLACK);
    background.drawString(data.currentTime.c_str(), 202-130, 0, GFXFF);
    
    // BTC Price (large)
    background.setFreeFont(FF24);
    background.setTextSize(1);
    background.setTextColor(0xDEDB, TFT_BLACK);
    background.drawString(data.btcPrice.c_str(), 0, 50, GFXFF);
    
    background.pushSprite(130, 3);
    background.deleteSprite();
  }
}

// --- DisplayDriver Interface Implementation ---

void aamDisplay_Init(void) {
  Serial.println("[AAM] Display Init");
  
  // Initialize display
  tft.init();
  tft.setRotation(1); // Landscape mode
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  
  // Touch calibration
  touch_calibrate();
  
  // Load font
  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers))) {
    Serial.println("[AAM] Font load error");
  } else {
    Serial.println("[AAM] Font loaded OK");
  }
  
  // Initialize with empty data (will be populated with real data on first screen draw)
  lastScreenChange = millis();
  
  Serial.println("[AAM] Init complete");
}

void aamDisplay_AlternateScreenState(void) {
  // Not used in demo
}

void aamDisplay_AlternateRotation(void) {
  // Not used in demo
}

void aamDisplay_LoadingScreen(void) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextFont(4);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("NerdMiner v2", 160, 100);
  tft.setTextFont(2);
  tft.drawString("Starting...", 160, 130);
  delay(2000);
}

void aamDisplay_SetupScreen(void) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextFont(4);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Setup Mode", 160, 100);
  tft.setTextFont(2);
  tft.drawString("Connect to WiFi AP", 160, 130);
}

void aamDisplay_AnimateCurrentScreen(unsigned long frame) {
  // This is called every loop by NerdMiner (from FreeRTOS task)
  
  switch(currentScreen) {
    case SCREEN_MINING:
      drawMiningScreen(0);
      break;
    case SCREEN_CLOCK:
      drawClockScreen(0);
      break;
    case SCREEN_GLOBAL:
      drawGlobalScreen(0);
      break;
    case SCREEN_BTCPRICE:
      drawBTCPriceScreen(0);
      break;
    case SCREEN_REMOTE:
      drawRemoteScreen(0);
      break;
  }
}

void aamDisplay_DoLedStuff(unsigned long frame) {
  // This is called every loop by NerdMiner
  // We handle touch and auto-cycling here

  uint16_t t_x = 0, t_y = 0;
  bool pressed = tft.getTouch(&t_x, &t_y);
  unsigned long currentMillis = millis();
  
  // Manual screen change with touch
  if (pressed) {
    static unsigned long lastTouch = 0;
    if (currentMillis - lastTouch > 300) {
      lastTouch = currentMillis;
      if (t_x > 160) {
        // Right side - next screen
        currentScreen = (currentScreen + 1) % NUM_SCREENS;
        Serial.println("[AAM] Touch Next Screen");
      } else {
        // Left side - previous screen
        currentScreen = (currentScreen - 1 + NUM_SCREENS) % NUM_SCREENS;
        Serial.println("[AAM] Touch Prev Screen");
      }
      hasChangedScreen = true;
      lastScreenChange = currentMillis;
      currentDisplayDriver->current_cyclic_screen = currentScreen;
    }
  }
  
  // Auto-cycle screens
  if (currentMillis - lastScreenChange >= SCREEN_CYCLE_TIME) {
    currentScreen = (currentScreen + 1) % NUM_SCREENS;
    hasChangedScreen = true;
    lastScreenChange = currentMillis;
    currentDisplayDriver->current_cyclic_screen = currentScreen;
  }
}

// Array of cyclic screens (pointers to functions)
// We don't really use this array for dispatching in this demo implementation
// because we dispatch manually in AnimateCurrentScreen, but we need to provide it
// to satisfy the struct definition.
CyclicScreenFunction aamDisplayCyclicScreens[] = {
    drawMiningScreen,
    drawClockScreen,
    drawGlobalScreen,
    drawBTCPriceScreen,
    drawRemoteScreen
};

DisplayDriver aamDisplayDriver = {
    aamDisplay_Init,
    aamDisplay_AlternateScreenState,
    aamDisplay_AlternateRotation,
    aamDisplay_LoadingScreen,
    aamDisplay_SetupScreen,
    aamDisplayCyclicScreens,
    aamDisplay_AnimateCurrentScreen,
    aamDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(aamDisplayCyclicScreens),
    0,
    SCREEN_WIDTH,
    SCREEN_HEIGHT
};

#endif
