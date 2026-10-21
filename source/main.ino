#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_MCP23X17.h>
#include <LovyanGFX.hpp>

constexpr int I2C_SDA = 33;
constexpr int I2C_SCL = 32;

constexpr int SPI_SCK = 18;
constexpr int SPI_MISO = 19;
constexpr int SPI_MOSI = 23;

constexpr int SD_CS = 14;

constexpr int TFT_CS_PIN = 5;
constexpr int TFT_DC_PIN = 21;
constexpr int TFT_RST_PIN = -1;
constexpr int TOUCH_CS = 26;
constexpr int TOUCH_IRQ = 27;

constexpr uint8_t MCP_ADDRESS = 0x20;

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9486 panel;
  lgfx::Bus_SPI bus;

public:
  LGFX() {
    {
      auto cfg = bus.config();
      cfg.spi_host = VSPI_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 20000000;
      cfg.freq_read = 10000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = SPI_SCK;
      cfg.pin_mosi = SPI_MOSI;
      cfg.pin_miso = SPI_MISO;
      cfg.pin_dc = TFT_DC_PIN;
      bus.config(cfg);
      panel.setBus(&bus);
    }
    {
      auto cfg = panel.config();
      cfg.pin_cs = TFT_CS_PIN;
      cfg.pin_rst = TFT_RST_PIN;
      cfg.pin_busy = -1;
      cfg.panel_width = 320;
      cfg.panel_height = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.readable = false;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;
      panel.config(cfg);
    }
    setPanel(&panel);
  }
};

LGFX tft;

constexpr int NUM_BUTTONS = 12;

const char* BUTTON_NAMES[NUM_BUTTONS] = {
  "SW1 (BA0)",
  "SW3 (BA1)",
  "SW2 (BA2)",
  "SW4 (BA3)",
  "SW5 (BA4)",
  "SW6 (BA5)",
  "SW7 (BA6)",
  "SW8 (BA7)",
  "SW9 (BB0)",
  "SW10 (BB1)",
  "SW11 (BB2)",
  "SW12 (BB3)"
};

Adafruit_MCP23X17 mcp;

uint16_t lastState = 0;

void initButtons() {
  Wire.begin(I2C_SDA,I2C_SCL);

  if (!mcp.begin_I2C(MCP_ADDRESS,&Wire)) {
    Serial.println("MCP23017 not found! Check wiring / address.");
    while (true) {
      delay(1000);
    }
  }

  for (int i = 0; i < NUM_BUTTONS; i++) {
    mcp.pinMode(i,INPUT_PULLUP);
  }

  Serial.println("MCP23017 OK");
}

void testSD() {
  SPI.begin(SPI_SCK,SPI_MISO,SPI_MOSI,SD_CS);

  if (!SD.begin(SD_CS,SPI)) {
    Serial.println("SD card mount failed!");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card inserted");
    return;
  }

  Serial.printf("SD card OK, size: %llu MB\n",SD.cardSize() / (1024 * 1024));

  File file = SD.open("/test.txt",FILE_WRITE);
  if (file) {
    file.println("Hello from the handheld!");
    file.close();
    Serial.println("Wrote /test.txt");
  } else {
    Serial.println("Failed to write /test.txt");
  }

  Serial.println("Files on card:");
  File root = SD.open("/");
  File entry = root.openNextFile();
  while (entry) {
    Serial.printf("  %s (%u bytes)\n",entry.name(),(unsigned int)entry.size());
    entry = root.openNextFile();
  }
}

void drawTriangle() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  int w = tft.width();
  int h = tft.height();

  tft.fillTriangle(w / 2,h / 4,w / 4,3 * h / 4,3 * w / 4,3 * h / 4,TFT_GREEN);

  Serial.println("Triangle drawn");
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("Handheld test starting...");

  pinMode(SD_CS,OUTPUT);
  digitalWrite(SD_CS,HIGH);
  pinMode(TFT_CS_PIN,OUTPUT);
  digitalWrite(TFT_CS_PIN,HIGH);
  pinMode(TOUCH_CS,OUTPUT);
  digitalWrite(TOUCH_CS,HIGH);

  initButtons();
  testSD();
  drawTriangle();

  Serial.println("Press some buttons!");
}

void loop() {
  uint16_t raw = mcp.readGPIOAB();

  uint16_t pressed = (~raw) & 0x0FFF;

  if (pressed != lastState) {
    uint16_t changed = pressed ^ lastState;

    for (int i = 0; i < NUM_BUTTONS; i++) {
      if (changed & (1 << i)) {
        Serial.printf("%s %s\n",BUTTON_NAMES[i],(pressed & (1 << i)) ? "PRESSED" : "released");
      }
    }

    lastState = pressed;
  }

  delay(10);
}
