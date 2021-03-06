#pragma GCC diagnostic warning "-Wall"
#include "NiftyRA8875.h"

void RA8875::writeCmd(uint8_t x)
{
  digitalWrite(m_csPin, LOW);
  SPI.transfer(RA8875_CMD_WRITE);
  SPI.transfer(x);
  digitalWrite(m_csPin, HIGH);
}

void RA8875::writeData(uint8_t x)
{
  digitalWrite(m_csPin, LOW);
  SPI.transfer(RA8875_DATA_WRITE);
  SPI.transfer(x);
  digitalWrite(m_csPin, HIGH);
}

uint8_t RA8875::readData(void)
{
  digitalWrite(m_csPin, LOW);
  SPI.transfer(RA8875_DATA_READ);
  uint8_t x = SPI.transfer(0);
  digitalWrite(m_csPin, HIGH);
  return x;
}

// Reads the special status register.
// This register uses a special cycle type instead of having an address like other registers.
uint8_t RA8875::readStatus(void)
{
  digitalWrite(m_csPin, LOW);
  SPI.transfer(RA8875_STATUS_READ);
  uint8_t x = SPI.transfer(0);
  digitalWrite(m_csPin, HIGH);
  return x;
}

void RA8875::writeReg(uint8_t reg, uint8_t x)
{
  writeCmd(reg);
  writeData(x);
}

uint8_t RA8875::readReg(uint8_t reg) 
{
  writeCmd(reg);
  return readData();
}

RA8875::RA8875(int csPin, int resetPin)
{
  m_csPin    = csPin;
  m_resetPin = resetPin;
  m_intPin   = -1;

  m_width  = 0;
  m_height = 0;
  m_depth  = 0;

  m_tracePrint = NULL;
}

// Pulse the reset pin low
void RA8875::hardReset(void)
{
  RA8875_TRACE("hardReset");

  delay(5);
  digitalWrite(m_resetPin, LOW);
  delay(5);
  digitalWrite(m_resetPin, HIGH);
  delay(5);
}

void RA8875::softReset(void)
{
  RA8875_TRACE("softReset");

  SPI.beginTransaction(m_spiSettings);

  delay(50);
  uint8_t pwrr = readReg(RA8875_REG_PWRR);
  RA8875_TRACE("  PWRR is: %02X", pwrr);
  delay(50);
  pwrr |= 0x01;
  RA8875_TRACE("  Soft reset 1");
  writeReg(RA8875_REG_PWRR, pwrr);
  delay(50);
  pwrr &= 0xFE;
  RA8875_TRACE("  Soft reset 2");
  writeReg(RA8875_REG_PWRR, pwrr);
  delay(50);

  SPI.endTransaction();

}

// Set up PLL
// 320x240: 20MHz crystal * (10 + 1) / (2 ^ 2) = 55MHz system clock (SYS_CLK)
// 480x272: 20MHz crystal * (10 + 1) / (2 ^ 2) = 55MHz system clock (SYS_CLK)
// 640x480: 20MHz crystal * (11 + 1) / (2 ^ 2) = 60MHz system clock (SYS_CLK)
// 800x480: 20MHz crystal * (11 + 1) / (2 ^ 2) = 60MHz system clock (SYS_CLK)
bool RA8875::initPLL(void)
{
  RA8875_TRACE("initPLL");

  int pllc1;
  int pllc2 = 0x02;  // Divide by (2 ^ 2) = 4

  if ((m_height == 240) || (m_height == 272))
    pllc1 = 0x0A;  // PLL input parameter
  else if (m_height == 480)
    pllc1 = 0x0B;  // PLL input parameter
  else
    return false;  // Don't know how to configure PLL for this size

  SPI.beginTransaction(m_spiSettings);

  writeReg(RA8875_REG_PLLC1, pllc1);

  delay(2);

  writeReg(RA8875_REG_PLLC2, pllc2);  // PLL output divider

  delay(2);

  SPI.endTransaction();

  return true;
}

// Set up display for current colour depth and resolution.
bool RA8875::initDisplay(void)
{
  RA8875_TRACE("initDisplay");

  uint8_t pcsr, hndftr, hndr, hstr, hpwr, vpwr;
  uint16_t vndr, vstr;

  if ((m_width == 480) && (m_height == 272))
  {
    pcsr   = 0x82;  // PDAT fetched on falling edge, PCLK is SYS_CLK / 4

    hndftr = 0x00;  // DE polarity high, 0 pixels of tuning
    hndr   = 0x01;  // (0x01 + 1) * 8 = 16px
    hstr   = 0x00;  // (0x00 + 1) * 8 = 8px
    hpwr   = 0x05;  // HSYNC active low, pulse width (0x05 + 1) * 8 = 48px

    vndr   = 0x02;  // Vertical non-display period. 0x02 = 3 lines
    vstr   = 0x07;  // Vertical start position
    vpwr   = 0x09;  // VSYNC pulse width active low, width (0x09 + 1) = 10 lines
  }
  else if ((m_width == 800) && (m_height == 480))
  {
    pcsr   = 0x81;  // PDAT fetched on falling edge, PCLK is SYS_CLK / 2

    hndftr = 0x00;  // DE polarity high, 0 pixels of tuning
    hndr   = 0x03;  // (0x03 + 1) * 8 = 32px
    hstr   = 0x03;  // (0x03 + 1) * 8 = 32px
    hpwr   = 0x0B;  // HSYNC active low, pulse width (0x0B + 1) * 8 = 96px

    vndr   = 0x20;  // Vertical non-display period. 0x20 = 33 lines
    vstr   = 0x16;  // Vertical start position
    vpwr   = 0x01;  // VSYNC pulse width active low, width (0x01 + 1) = 2 lines
  }
  else
    return false;

  SPI.beginTransaction(m_spiSettings);

  // Set colour depth
  writeReg(RA8875_REG_SYSR, (m_depth == 16) ? 0x08 : 0x00);
  writeReg(RA8875_REG_PCSR, pcsr);  // 0x80 = PDAT fetched at PCLK falling edge, 0x02 = PCLK period is 4 times system clock period

  delay(5);

  // --- Horizontal regs ---
  // Horizontal width: (HDWR + 1) * 8
  writeReg(RA8875_REG_HDWR, (m_width / 8) - 1);  // Horizontal width is (HDWR + 1) * 8 = 480px. Max width is 0x63 = 800px.
  // Horizontal non-display period. Total non-display period in pixels is (HNDR + 1) * 8 + (HNDFTR / 2 + 1) + 2
  writeReg(RA8875_REG_HNDFTR, hndftr);  // Horiz non-display fine tuning
  writeReg(RA8875_REG_HNDR, hndr);  // Horiz non-display period is (HNDR + 1) * 8
  // HSYNC
  writeReg(RA8875_REG_HSTR, hstr);  // HSYNC start position is (HSTR + 1) * 8
  writeReg(RA8875_REG_HPWR, hpwr);  // HSYNC pulse width

  // --- Vertical regs ---
  // Vertial height: ((VDHR1 << 8) | VDHR0) + 1. Max height 0x1DF = 480px.
  writeReg(RA8875_REG_VDHR0, (m_height - 1) & 0xFF);
  writeReg(RA8875_REG_VDHR1, (m_height - 1) >> 8);
  // Vertical non-display period. Total is ((VNDR1 << 8) | VNDR0) + 1
  writeReg(RA8875_REG_VNDR0, vndr & 0xFF);
  writeReg(RA8875_REG_VNDR1, vndr >> 8);
  // VSYNC. Start position in pixel lines is: ((VSTR1 << 8) | VSTR0) + 1
  writeReg(RA8875_REG_VSTR0, vstr & 0xFF);
  writeReg(RA8875_REG_VSTR1, vstr >> 8);
  writeReg(RA8875_REG_VPWR, vpwr);  // VSYNC pulse width active low, width (0x09 + 1) = 10 lines

  SPI.endTransaction();

  delay(5);

  return true;
}

bool RA8875::init(int width, int height, int depth)
{
  RA8875_TRACE("init() started");

  // Check resolution
  if (!((width == 480) && (height == 272)) &&
      !((width == 800) && (height == 480)))
    return false;

  // Check colour depth
  if ((depth != 8) && (depth != 16))
    return false;

  m_width  = width;
  m_height = height;
  m_depth  = depth;

  m_textColor = RGB565(255, 255, 255);

  // Set up CS pin
  pinMode(m_csPin, OUTPUT);
  digitalWrite(m_csPin, HIGH);

  // If we have an int pin, set it up
  if (m_intPin >= 0)
    pinMode(m_intPin, INPUT);

  // If we have a reset pin, do a hard reset
  if (m_resetPin >= 0)
  {
    pinMode(m_resetPin, OUTPUT);
    digitalWrite(m_resetPin, HIGH);

    hardReset();
  }

  SPI.begin();

  m_spiSettings = SPISettings(RA8875_SPI_SPEED, MSBFIRST, SPI_MODE3);

  // If no reset pin is hooked up, try software reset command
  if (m_resetPin < 0)
    softReset();

  if (!initPLL())
    return false;

  if (!initDisplay())
    return false;

  SPI.beginTransaction(m_spiSettings);

  // --- Enable layers ---
  writeReg(RA8875_REG_DPCR, 0x80);

  SPI.endTransaction();

  setActiveWindow(0, m_width - 1, 0, m_height - 1);

  selectInternalFont(RA8875_FONT_ENCODING_8859_1);

  // Turn display on
  SPI.beginTransaction(m_spiSettings);
  writeReg(RA8875_REG_PWRR, 0x80);  // Display on, normal mode, no reset
  SPI.endTransaction();

  RA8875_TRACE("init() completed");
  return true;
}

void RA8875::initExternalFontRom(int spiIf, enum RA8875_External_Font_Rom chip)
{
  SPI.beginTransaction(m_spiSettings);

  // TODO: Calculate the clock from the system clock. Could probably go faster.
  //  Need to rewrite initPLL() first.
  writeReg(RA8875_REG_SFCLR, 0x03);  // Use system clock / 4 for SPI

  uint8_t sroc = 0x08;  // 24-bit address mode, SPI mode 0, 1 byte dummy cycle, font mode, single mode
  sroc |= (spiIf & 0x01) << 7;
  writeReg(RA8875_REG_SROC, sroc);

  // Turn on font/DMA mode
  writeReg(RA8875_REG_SACS_MODE, 0x00);

  // Select font chip
  uint8_t sfrs = readReg(RA8875_REG_SFRS);
  sfrs = (sfrs & 0x1F) | ((chip & 0x07) << 5);
  writeReg(RA8875_REG_SFRS, sfrs);

  SPI.endTransaction();
}

void RA8875::setBacklight(bool enabled)
{
  SPI.beginTransaction(m_spiSettings);

  // Adafruit module uses GPIOX register to enable display
  writeCmd(RA8875_REG_GPIOX);
  writeData(enabled ? 1 : 0);

  // Backlight is tied to PWM1
  writeCmd(RA8875_REG_P1CR);
  writeData(enabled ? 0x8A : 0x0A);  // SYS_CLK / 1024
  writeCmd(RA8875_REG_P1DCR);
  writeData(0xFF);  // Duty cycle (brightness)
  
  SPI.endTransaction();
}

void RA8875::setActiveWindow(int xStart, int xEnd, int yStart, int yEnd)
{
  SPI.beginTransaction(m_spiSettings);

  // Active window X start
  writeCmd(RA8875_REG_HSAW0);
  writeData(xStart & 0xFF);
  writeCmd(RA8875_REG_HSAW1);
  writeData(xStart >> 8);

  // Active window X end
  writeCmd(RA8875_REG_HEAW0);
  writeData(xEnd & 0xFF);
  writeCmd(RA8875_REG_HEAW1);
  writeData(xEnd >> 8);

  // Active window Y start
  writeCmd(RA8875_REG_VSAW0);
  writeData(yStart & 0xFF);
  writeCmd(RA8875_REG_VSAW1);
  writeData(yStart >> 8);  

  // Active window Y end
  writeCmd(RA8875_REG_VEAW0);
  writeData(yEnd & 0xFF);
  writeCmd(RA8875_REG_VEAW1);
  writeData(yEnd >> 8);
  
  SPI.endTransaction();
}

// Clears the frame buffer memory.
// This seems to only affect the current layer. You can call setDrawLayer() first to select which layer will be cleared.
void RA8875::clearMemory(void)
{
  SPI.beginTransaction(m_spiSettings);

  writeReg(RA8875_REG_MCLR, 0x80);  // Start memory clear

  // Wait for completion
  uint32_t starttime = millis();
  uint8_t status;
  do
  {
    status = readReg(RA8875_REG_MCLR);
    RA8875_TRACE("MCLR: %02X", status);
  } while ((status & 0x80) && ((millis() - starttime) < 250));

  SPI.endTransaction();
}

void RA8875::setTextMode(void)
{
  waitBusy();

  // Restore text colour
  writeReg(RA8875_REG_FGCR0, m_textColor >> 11);            // R
  writeReg(RA8875_REG_FGCR1, (m_textColor & 0x07E0) >> 5);  // G
  writeReg(RA8875_REG_FGCR2, m_textColor & 0x1F);           // B

  uint8_t mwcr0 = readReg(RA8875_REG_MWCR0);
  writeReg(RA8875_REG_MWCR0, mwcr0 | 0x80);  // Enable text mode
}

void RA8875::setGraphicsMode(void)
{
  waitBusy();

  // Set graphics mode
  uint8_t mwcr0 = readReg(RA8875_REG_MWCR0);
  writeReg(RA8875_REG_MWCR0, mwcr0 & ~0x80);  // Enable graphics mode
}

void RA8875::setCursor(int x, int y)
{
  SPI.beginTransaction(m_spiSettings);

  // Cursor X position
  writeReg(RA8875_REG_FCURX0, x & 0xFF);
  writeReg(RA8875_REG_FCURX1, x >> 8);

  // Cursor Y position
  writeReg(RA8875_REG_FCURY0, y & 0xFF);
  writeReg(RA8875_REG_FCURY1, y >> 8);
  
  SPI.endTransaction();
}

int RA8875::getCursorX(void)
{
  return (readReg(RA8875_REG_FCURX1) << 8) | readReg(RA8875_REG_FCURX0);
}

int RA8875::getCursorY(void)
{
  return (readReg(RA8875_REG_FCURY1) << 8) | readReg(RA8875_REG_FCURY0);
}

void RA8875::setCursorVisibility(bool visible, bool blink)
{
  SPI.beginTransaction(m_spiSettings);

  writeCmd(RA8875_REG_MWCR0);
  uint8_t mwcr0 = readData();

  if (visible)
    mwcr0 |= 0x40;
  else
    mwcr0 &= ~0x40;

  if (blink)
    mwcr0 |= 0x20;
  else
    mwcr0 &= ~0x20;
  
  writeData(mwcr0);
  
  SPI.endTransaction();
}

void RA8875::selectInternalFont(enum RA8875_Font_Encoding enc)
{
  // Invalid encodings become Latin 1
  if (!(enc & 0x10) || (enc & 0xEC))
    enc = RA8875_FONT_ENCODING_8859_1;

  SPI.beginTransaction(m_spiSettings);

  // Select ROM font, internal ROM, charset
  uint8_t fncr0 = 0x00 | (enc & 0x03);
  writeReg(RA8875_REG_FNCR0, fncr0);

  // Datasheet says this register must be zero.
  // Is that true? Just clear the low two bits for now.
  uint8_t sfrs = readReg(RA8875_REG_SFRS);
  writeReg(RA8875_REG_SFRS, sfrs & 0xFC);

  SPI.endTransaction();
}

void RA8875::selectExternalFont(enum RA8875_External_Font_Family family, enum RA8875_Font_Size size, enum RA8875_Font_Encoding enc, RA8875_Font_Flags flags)
{
  // Invalid encodings become ASCII
  if (enc & 0xF8)
    enc = RA8875_FONT_ENCODING_ASCII;

  SPI.beginTransaction(m_spiSettings);

  // Select ROM font, external ROM,
  writeReg(RA8875_REG_FNCR0, 0x20);

  // Select font size
  writeReg(RA8875_REG_FWTSR, (size & 0x03) << 6);

  uint8_t sfrs = readReg(RA8875_REG_SFRS);
  sfrs = (sfrs & 0xE0) | (enc << 2) | (family & 0x03);
  writeReg(RA8875_REG_SFRS, sfrs);
  //Serial.print("sfrs: "); Serial.println(sfrs, HEX);

  SPI.endTransaction();
}

void RA8875::setTextSize(int xScale, int yScale)
{
  SPI.beginTransaction(m_spiSettings);

// This register does not seem to apply to the built-in ROM font
//  uint8_t fwtsr = readReg(RA8875_REG_FWTSR);
//
//  if (height == 16)
//    fwtsr = (fwtsr & 0x3F) | 0x00;
//  else if (height == 24)
//    fwtsr = (fwtsr & 0x3F) | 0x40;
//  else if (height == 32)
//    fwtsr = (fwtsr & 0x3F) | 0x80;
//
//  writeData(fwtsr);

  xScale = constrain(xScale, 1, 4);
  yScale = constrain(yScale, 1, 4);

  uint8_t fncr1 = readReg(RA8875_REG_FNCR1);

  fncr1 = (fncr1 & 0xF0) | ((xScale - 1) << 2) | (yScale - 1);

  writeData(fncr1);

  SPI.endTransaction();
}

int RA8875::getTextSizeX(void)
{
  // TODO: Cache?
  SPI.beginTransaction(m_spiSettings);

  uint8_t fncr1 = readReg(RA8875_REG_FNCR1);

  SPI.endTransaction();

  return ((fncr1 >> 2) & 0x03) + 1;
}

int RA8875::getTextSizeY(void)
{
  // TODO: Cache?
  SPI.beginTransaction(m_spiSettings);

  uint8_t fncr1 = readReg(RA8875_REG_FNCR1);

  SPI.endTransaction();

  return (fncr1 & 0x03) + 1;
}

// Write a single byte (called from class Print).
size_t RA8875::write(uint8_t c)
{
  if (c == '\r')
    return 1;  // Ignored
  else if (c == '\n')
    setCursor(0, getCursorY() + (RA8875_ROM_TEXT_HEIGHT * getTextSizeY()));
  else
  {
    SPI.beginTransaction(m_spiSettings);

    setTextMode();

    writeReg(RA8875_REG_MRWC, c);

    setGraphicsMode();

    SPI.endTransaction();
  }

  return 1;
}

// Write a string to the display (called from class Print).
size_t RA8875::write(const char *s)
{
  SPI.beginTransaction(m_spiSettings);

  setTextMode();

  writeCmd(RA8875_REG_MRWC);

  size_t count = 0;

  while (char c = *s++)
  {
    count++;

    if (c == '\r')
      ;  // Ignored
    else if (c == '\n')
      setCursor(0, getCursorY() + (RA8875_ROM_TEXT_HEIGHT * getTextSizeY()));
    else
    {
      waitBusy();
      writeData(c);
    }
  }

  setGraphicsMode();

  SPI.endTransaction();

  return count;
}

// Write a number of bytes to the display (called from class Print).
size_t RA8875::write(const uint8_t *bytes, size_t size)
{
  SPI.beginTransaction(m_spiSettings);

  setTextMode();

  writeCmd(RA8875_REG_MRWC);

  for (unsigned int i = 0; i < size; i++)
  {
    char c = bytes[i];

    if (c == '\r')
      ;  // Ignored
    else if (c == '\n')
      setCursor(0, getCursorY() + (RA8875_ROM_TEXT_HEIGHT * getTextSizeY()));
    else
    {
      waitBusy();
      writeData(c);
    }
  }

  setGraphicsMode();

  SPI.endTransaction();

  return size;
}

void RA8875::putChars(const char *buffer, size_t size)
{
  SPI.beginTransaction(m_spiSettings);

  setTextMode();

  // Write characters
  writeCmd(RA8875_REG_MRWC);
  for (unsigned int i = 0; i < size; i++)
  {
    waitBusy();
    writeData(buffer[i]);
  }

  setGraphicsMode();

  SPI.endTransaction();
}

void RA8875::putChars16(const uint16_t *buffer, unsigned int count)
{
  SPI.beginTransaction(m_spiSettings);

  setTextMode();

  // Write characters
  writeCmd(RA8875_REG_MRWC);
  for (unsigned int i = 0; i < count; i++)
  {
    waitBusy();
    writeData(buffer[i] >> 8);

    waitBusy();
    writeData(buffer[i] & 0xFF);
  }

  setGraphicsMode();

  SPI.endTransaction();
}

void RA8875::setScrollWindow(int xStart, int xEnd, int yStart, int yEnd)
{
  SPI.beginTransaction(m_spiSettings);

  // X start
  writeReg(RA8875_REG_HSSW0, xStart & 0xFF);
  writeReg(RA8875_REG_HSSW1, xStart >> 8);

  // X end
  writeReg(RA8875_REG_HESW0, xEnd & 0xFF);
  writeReg(RA8875_REG_HESW1, xEnd >> 8);
  
  // Y start
  writeReg(RA8875_REG_VSSW0, yStart & 0xFF);
  writeReg(RA8875_REG_VSSW1, yStart >> 8);

  // Y end
  writeReg(RA8875_REG_VESW0, yEnd & 0xFF);
  writeReg(RA8875_REG_VESW1, yEnd >> 8);
  
  SPI.endTransaction();
}

void RA8875::setScrollOffset(int x, int y)
{
  SPI.beginTransaction(m_spiSettings);

  // X offset
  writeReg(RA8875_REG_HOFS0, x & 0xFF);
  writeReg(RA8875_REG_HOFS1, x >> 8);

  // Y offset
  writeReg(RA8875_REG_VOFS0, y & 0xFF);
  writeReg(RA8875_REG_VOFS1, y >> 8);

  SPI.endTransaction();
}

void RA8875::setLayerMode(enum RA8875_Layer_Mode mode)
{
  SPI.beginTransaction(m_spiSettings);

  uint8_t ltpr0 = readReg(RA8875_REG_LTPR0);

  //Serial.print("mode: "); Serial.println(mode);
  ltpr0 = (ltpr0 & 0xF8) | mode;
  writeReg(RA8875_REG_LTPR0, ltpr0);
  //Serial.print("LTPR0: "); Serial.println(ltpr0, HEX);

  writeReg(RA8875_REG_LTPR1, 0x00);  // Enable display of both layers
  
  SPI.endTransaction();
}

// Sets drawing layer. Valid layers are 1 and 2.
void RA8875::setDrawLayer(int layer)
{
  SPI.beginTransaction(m_spiSettings);

  layer = constrain(layer, 1, 2);

  uint8_t mwcr1 = readReg(RA8875_REG_MWCR1);

  writeReg(RA8875_REG_MWCR1, (mwcr1 & 0xFE) | (layer - 1));
  
  SPI.endTransaction();
}

void RA8875::drawPixel(int x, int y, uint16_t color)
{
  SPI.beginTransaction(m_spiSettings);

  // Set memory write cursor
  writeReg(RA8875_REG_CURH0, x & 0xFF);
  writeReg(RA8875_REG_CURH1, x >> 8);
  writeReg(RA8875_REG_CURV0, y & 0xFF);
  writeReg(RA8875_REG_CURV1, y >> 8);  
  
  writeCmd(RA8875_REG_MRWC);

  if (m_depth == 8)
    writeData(color);
  else
  {
    writeData(color >> 8);
    writeData(color & 0xFF);
  }

//  digitalWrite(m_csPin, LOW);
//  SPI.transfer(RA8875_DATA_WRITE);
//  SPI.transfer(color);
//  digitalWrite(m_csPin, HIGH);
  
  SPI.endTransaction();
}

void RA8875::setDrawPosition(int x, int y)
{
  SPI.beginTransaction(m_spiSettings);
  
  writeReg(RA8875_REG_CURH0, x & 0xFF);
  writeReg(RA8875_REG_CURH1, x >> 8);
  writeReg(RA8875_REG_CURV0, y & 0xFF);
  writeReg(RA8875_REG_CURV1, y >> 8);  

  SPI.endTransaction();
}

void RA8875::pushPixel(uint16_t color)
{
  SPI.beginTransaction(m_spiSettings);

  writeCmd(RA8875_REG_MRWC);

  if (m_depth == 8)
    writeData(color);
  else
  {
    writeData(color >> 8);
    writeData(color & 0xFF);
  }

  SPI.endTransaction();
}

void RA8875::copyToScreen(int srcX, int srcY, int width, int height, int dstX, int dstY, bool transparent, uint8_t bgColor)
{
  SPI.beginTransaction(m_spiSettings);

  // Source in layer 2
  writeReg(RA8875_REG_HSBE0, srcX & 0xFF);
  writeReg(RA8875_REG_HSBE1, srcX >> 8);
  writeReg(RA8875_REG_VSBE0, srcY & 0xFF);
  writeReg(RA8875_REG_VSBE1, (srcY >> 8) | 0x80);

  // Destination in layer 1
  writeReg(RA8875_REG_HDBE0, dstX & 0xFF);
  writeReg(RA8875_REG_HDBE1, dstX >> 8);
  writeReg(RA8875_REG_VDBE0, dstY & 0xFF);
  writeReg(RA8875_REG_VDBE1, dstY >> 8);

  // Width
  writeReg(RA8875_REG_BEWR0, width & 0xFF);
  writeReg(RA8875_REG_BEWR1, width >> 8);

  // Height
  writeReg(RA8875_REG_BEHR0, height & 0xFF);
  writeReg(RA8875_REG_BEHR1, height >> 8);

  // Transparency colour
  if (transparent)
  {
    writeReg(RA8875_REG_FGCR0, bgColor >> 5);           // R
    writeReg(RA8875_REG_FGCR1, (bgColor & 0x1C) >> 2);  // G
    writeReg(RA8875_REG_FGCR2, bgColor & 0x3);          // B
  }

  // BTE operation
  if (transparent)
  {
    // NOTE: According to data sheet section 7-6, ROP only works with specific operations, and Transparent Move
    //  is not one of them. But in testing, it seems to apply the ROP anyway.
    writeReg(RA8875_REG_BECR1, 0xC5);  // Operation = Transparent Move BTE in Positive Direction, ROP = source
  }
  else
  {
    writeReg(RA8875_REG_BECR1, 0xC2);  // Operation = Move in positive direction with ROP, ROP = source
  }

  // Start operation
  writeReg(RA8875_REG_BECR0, 0x80);  // Start operation, source is block, destination is block

  // Wait for status register bit 6 to be clear
#if RA8875_PRINT_TIMING
  uint32_t startTime = micros();
  int iter = 0;
  while (readStatus() & 0x40)
    iter++;
  uint32_t endTime = micros();
  Serial.print("copyToScreen() BTE done in "); Serial.print(endTime - startTime); Serial.print(" us "); Serial.print(iter); Serial.println(" iter");
#else
  while (readStatus() & 0x40)
    ;
#endif

  SPI.endTransaction();
}

void RA8875::copyFromScreen(int srcX, int srcY, int width, int height, int dstX, int dstY)
{
  SPI.beginTransaction(m_spiSettings);

  // Source in layer 1
  writeReg(RA8875_REG_HSBE0, srcX & 0xFF);
  writeReg(RA8875_REG_HSBE1, srcX >> 8);
  writeReg(RA8875_REG_VSBE0, srcY & 0xFF);
  writeReg(RA8875_REG_VSBE1, srcY >> 8);

  // Destination in layer 2
  writeReg(RA8875_REG_HDBE0, dstX & 0xFF);
  writeReg(RA8875_REG_HDBE1, dstX >> 8);
  writeReg(RA8875_REG_VDBE0, dstY & 0xFF);
  writeReg(RA8875_REG_VDBE1, (dstY >> 8) | 0x80);

  // Width
  writeReg(RA8875_REG_BEWR0, width & 0xFF);
  writeReg(RA8875_REG_BEWR1, width >> 8);

  // Height
  writeReg(RA8875_REG_BEHR0, height & 0xFF);
  writeReg(RA8875_REG_BEHR1, height >> 8);

  writeReg(RA8875_REG_BECR1, 0xC2);  // Operation = Move in positive direction with ROP, ROP = source

  // Start operation
  writeReg(RA8875_REG_BECR0, 0x80);  // Start operation, source is block, destination is block

  // Wait for status register bit 6 to be clear
#if RA8875_PRINT_TIMING
  uint32_t startTime = micros();
  int iter = 0;
  while (readStatus() & 0x40)
    iter++;
  uint32_t endTime = micros();
  Serial.print("copyFromScreen() BTE done in "); Serial.print(endTime - startTime); Serial.print(" us "); Serial.print(iter); Serial.println(" iter");
#else
  while (readStatus() & 0x40)
    ;
#endif

  SPI.endTransaction();  
}

void RA8875::copy(int srcLayer, int srcX, int srcY, int width, int height, int dstLayer, int dstX, int dstY, bool transparent, uint8_t bgColor)
{
  // Don't bother attempting zero-area copies
  if ((width == 0) || (height == 0))
    return;
  
  SPI.beginTransaction(m_spiSettings);

  // Source
  writeReg(RA8875_REG_HSBE0, srcX & 0xFF);
  writeReg(RA8875_REG_HSBE1, srcX >> 8);
  writeReg(RA8875_REG_VSBE0, srcY & 0xFF);
  writeReg(RA8875_REG_VSBE1, (srcY >> 8) | ((srcLayer == 2) ? 0x80 : 0x00));

  // Destination
  writeReg(RA8875_REG_HDBE0, dstX & 0xFF);
  writeReg(RA8875_REG_HDBE1, dstX >> 8);
  writeReg(RA8875_REG_VDBE0, dstY & 0xFF);
  writeReg(RA8875_REG_VDBE1, (dstY >> 8) | ((dstLayer == 2) ? 0x80 : 0x00));

  // Width
  writeReg(RA8875_REG_BEWR0, width & 0xFF);
  writeReg(RA8875_REG_BEWR1, width >> 8);

  // Height
  writeReg(RA8875_REG_BEHR0, height & 0xFF);
  writeReg(RA8875_REG_BEHR1, height >> 8);

  // Transparency colour
  if (transparent)
  {
    writeReg(RA8875_REG_FGCR0, bgColor >> 5);           // R
    writeReg(RA8875_REG_FGCR1, (bgColor & 0x1C) >> 2);  // G
    writeReg(RA8875_REG_FGCR2, bgColor & 0x3);          // B
  }

  // BTE operation
  if (transparent)
  {
    // NOTE: According to data sheet section 7-6, ROP only works with specific operations, and Transparent Move
    //  is not one of them. But in testing, it seems to apply the ROP anyway.
    writeReg(RA8875_REG_BECR1, 0xC5);  // Operation = Transparent Move BTE in Positive Direction, ROP = source
  }
  else
  {
    writeReg(RA8875_REG_BECR1, 0xC2);  // Operation = Move in positive direction with ROP, ROP = source
  }

  // Start operation
  writeReg(RA8875_REG_BECR0, 0x80);  // Start operation, source is block, destination is block

  // Wait for status register bit 6 to be clear
#if RA8875_PRINT_TIMING
  uint32_t startTime = micros();
  int iter = 0;
  while (readStatus() & 0x40)
    iter++;
  uint32_t endTime = micros();
  Serial.print("copy() BTE done in "); Serial.print(endTime - startTime); Serial.print(" us "); Serial.print(iter); Serial.println(" iter");
#else
  while (readStatus() & 0x40)
    ;
#endif

  SPI.endTransaction();  
}

// Draws a 2-point shape (line, outline rect, filled rect)
void RA8875::drawTwoPointShape(int x1, int y1, int x2, int y2, uint16_t color, uint8_t cmd)
{
  SPI.beginTransaction(m_spiSettings);

  // Start point
  writeReg(RA8875_REG_DLHSR0, x1 & 0xFF);
  writeReg(RA8875_REG_DLHSR1, x1 >> 8);
  writeReg(RA8875_REG_DLVSR0, y1 & 0xFF);
  writeReg(RA8875_REG_DLVSR1, y1 >> 8);

  // Destination
  writeReg(RA8875_REG_DLHER0, x2 & 0xFF);
  writeReg(RA8875_REG_DLHER1, x2 >> 8);
  writeReg(RA8875_REG_DLVER0, y2 & 0xFF);
  writeReg(RA8875_REG_DLVER1, y2 >> 8);

  // Color
  writeReg(RA8875_REG_FGCR0, color >> 11);            // R
  writeReg(RA8875_REG_FGCR1, (color & 0x07E0) >> 5);  // G
  writeReg(RA8875_REG_FGCR2, color & 0x1F);           // B

  // Begin drawing
  writeReg(RA8875_REG_DCR, 0x80 | cmd);

  // Wait for completion
  while (readReg(RA8875_REG_DCR) & 0x80)
    ;

  SPI.endTransaction();  
}

// Draw 3-point shape (triangle or filled triangle)
void RA8875::drawThreePointShape(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color, uint8_t cmd)
{
  SPI.beginTransaction(m_spiSettings);

  // First point
  writeReg(RA8875_REG_DLHSR0, x1 & 0xFF);
  writeReg(RA8875_REG_DLHSR1, x1 >> 8);
  writeReg(RA8875_REG_DLVSR0, y1 & 0xFF);
  writeReg(RA8875_REG_DLVSR1, y1 >> 8);

  // Second point
  writeReg(RA8875_REG_DLHER0, x2 & 0xFF);
  writeReg(RA8875_REG_DLHER1, x2 >> 8);
  writeReg(RA8875_REG_DLVER0, y2 & 0xFF);
  writeReg(RA8875_REG_DLVER1, y2 >> 8);

  // Third point
  writeReg(RA8875_REG_DTPH0, x3 & 0xFF);
  writeReg(RA8875_REG_DTPH1, x3 >> 8);
  writeReg(RA8875_REG_DTPV0, y3 & 0xFF);
  writeReg(RA8875_REG_DTPV1, y3 >> 8);

  // Color
  writeReg(RA8875_REG_FGCR0, color >> 11);            // R
  writeReg(RA8875_REG_FGCR1, (color & 0x07E0) >> 5);  // G
  writeReg(RA8875_REG_FGCR2, color & 0x1F);           // B

  // Begin drawing
  writeReg(RA8875_REG_DCR, 0x80 | cmd);

  // Wait for completion
  while (readReg(RA8875_REG_DCR) & 0x80)
    ;

  SPI.endTransaction();
}

// Draw circle shape (circle or filled circle)
void RA8875::drawCircleShape(int x, int y, int radius, uint16_t color, uint8_t cmd)
{
  SPI.beginTransaction(m_spiSettings);

  // Centre point
  writeReg(RA8875_REG_DCHR0, x & 0xFF);
  writeReg(RA8875_REG_DCHR1, x >> 8);
  writeReg(RA8875_REG_DCVR0, y & 0xFF);
  writeReg(RA8875_REG_DCVR1, y >> 8);

  // Radius
  writeReg(RA8875_REG_DCRR, radius);

  // Color
  writeReg(RA8875_REG_FGCR0, color >> 11);            // R
  writeReg(RA8875_REG_FGCR1, (color & 0x07E0) >> 5);  // G
  writeReg(RA8875_REG_FGCR2, color & 0x1F);           // B

  // Begin drawing
  writeReg(RA8875_REG_DCR, 0x40 | cmd);

  // Wait for completion
  while (readReg(RA8875_REG_DCR) & 0x40)
    ;

  SPI.endTransaction();
}
