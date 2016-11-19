#pragma GCC diagnostic warning "-Wall"
#include "RA8875.h"

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

RA8875::RA8875(int csPin, int intPin, int resetPin)
{
  m_csPin    = csPin;
  m_intPin   = intPin;
  m_resetPin = resetPin;

  m_width  = 0;
  m_height = 0;
  m_depth  = 0;
}

// Pulse the reset pin low
void RA8875::hardReset(void)
{
  delay(5);
  digitalWrite(m_resetPin, LOW);
  delay(5);
  digitalWrite(m_resetPin, HIGH);
  delay(5);
}

void RA8875::softReset(void)
{
  // If no reset pin is hooked up, try software reset command
  SPI.beginTransaction(m_spiSettings);

  delay(50);
  Serial.println("Soft reset 0");
  uint8_t pwrr = readReg(RA8875_REG_PWRR);
  Serial.print("PWRR is: "); Serial.println(pwrr, HEX);
  delay(50);
  pwrr |= 0x01;
  Serial.println("Soft reset 1");
  writeReg(RA8875_REG_PWRR, pwrr);
  delay(50);
  pwrr &= 0xFE;
  Serial.println("Soft reset 2");
  writeReg(RA8875_REG_PWRR, pwrr);
  delay(50);

  SPI.endTransaction();

}

// Set up PLL for 480x272
// 20MHz system clock * (10 + 1) / (2 ^ 2) = 55MHz
void RA8875::initPLL(void)
{
  SPI.beginTransaction(m_spiSettings);

  writeCmd(RA8875_REG_PLLC1);
  writeData(0x0A);  // PLL input parameter
  
  delay(2);

  writeCmd(RA8875_REG_PLLC2);
  writeData(0x02);  // Divide by (2 ^ 2) = 4

  delay(2);

  SPI.endTransaction();
}

bool RA8875::init(int width, int height, int depth)
{
  // Check resolution
  if ((width != 480) || (height != 272))
    return false;

  // Check colour depth
  if ((depth != 8) && (depth != 16))
    return false;

  m_width  = width;
  m_height = height;
  m_depth  = depth;

  m_textColor = RGB565(255, 255, 255);

  pinMode(m_csPin, OUTPUT);
  pinMode(m_intPin, INPUT);

  digitalWrite(m_csPin, HIGH);

  if (m_resetPin < 0)
  {
    pinMode(m_resetPin, OUTPUT);
    digitalWrite(m_resetPin, HIGH);

    hardReset();
  }

  SPI.begin();
  
  m_spiSettings = SPISettings(RA8875_SPI_SPEED, MSBFIRST, SPI_MODE3);

  if (m_resetPin <= 0)
    softReset();

  // TODO: Erase
  Serial.println(m_csPin);
  Serial.println(m_resetPin);
  
  // TODO: SPI.usingInterrupt(interruptNumber)?

  initPLL();

  SPI.beginTransaction(m_spiSettings);

  // Set colour depth
  writeReg(RA8875_REG_SYSR, (m_depth == 16) ? 0x08 : 0x00);

  writeReg(RA8875_REG_PCSR, 0x82);  // 0x80 = PDAT fetched at PCLK falling edge, 0x02 = PCLK period is 4 times system clock period

  delay(5);

  // --- Horizontal regs ---
  // Horizontal width: (HDWR + 1) * 8
  writeReg(RA8875_REG_HDWR, 0x3B);  // Horizontal width is (0x3B + 1) * 8 = 480px. Max width is 0x63 = 800px.
  // Horizontal non-display period. Total non-display period in pixels is (HNDR + 1) * 8 + (HNDFTR / 2 + 1) + 2
  writeReg(RA8875_REG_HNDFTR, 0x00);  // DE polarity high, 0 pixels of tuning
  writeReg(RA8875_REG_HNDR, 0x01);  // Horiz non-display period is (0x01 + 1) * 8 = 16px
  // HSYNC
  writeReg(RA8875_REG_HSTR, 0x00);  // HSYNC start position is (0x00 + 1) * 8 = 8px
  writeReg(RA8875_REG_HPWR, 0x05);  // HSYNC active low, pulse width (0x05 + 1) * 8 = 48px

  // --- Vertical regs ---
  // Vertial height: ((VDHR1 << 8) | VDHR0) + 1. Max height 0x1DF = 480px.
  writeCmd(RA8875_REG_VDHR0);
  writeData(0x0F);
  writeCmd(RA8875_REG_VDHR1);
  writeData(0x01); // Vertical height ((0x01 << 8) | 0x0F) + 1 = 272px
  // Vertical non-display period. Total is ((VNDR1 << 8) | VNDR0) + 1
  writeCmd(RA8875_REG_VNDR0);  
  writeData(0x02);
  writeCmd(RA8875_REG_VNDR1);
  writeData(0x00);  // Vert non-display period is ((0x00 << 8) | 0x02) + 1 = 3 lines
  // VSYNC. Start position in pixel lines is: ((VSTR1 << 8) | VSTR0) + 1
  writeCmd(RA8875_REG_VSTR0);
  writeData(0x07);
  writeCmd(RA8875_REG_VSTR1);
  writeData(0x00);  // VSYNC start is ((0x00 << 8) | 0x07) + 1 = 8 lines
  writeCmd(RA8875_REG_VPWR);
  writeData(0x09);  // VSYNC pulse width active low, width (0x09 + 1) = 10 lines

  // --- Enable layers ---
  writeReg(RA8875_REG_DPCR, 0x80);

  SPI.endTransaction();

  setActiveWindow(0, m_width - 1, 0, m_height - 1);

  // Turn display on
  SPI.beginTransaction(m_spiSettings);
  writeReg(RA8875_REG_PWRR, 0x80);  // Display on, normal mode, no reset
  SPI.endTransaction();

  return true;
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

// Clear's the frame buffer memory.
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
    Serial.print("MCLR: ");
    Serial.println(status);
  } while ((status & 0x80) && ((millis() - starttime) < 250));
  
  SPI.endTransaction();
}

// TODO: Split this apart into two functions, and have them both assume SPI transaction has already been started by the caller.
void RA8875::setMode(RA8875_Mode mode)
{
  SPI.beginTransaction(m_spiSettings);

  uint8_t mwcr0 = readReg(RA8875_REG_MWCR0);

  if ((mode == RA8875_MODE_TEXT) && !(mwcr0 & 0x80))
  {
    writeData(mwcr0 | 0x80);  // Enable text mode

    // Restore text colour
    writeReg(RA8875_REG_FGCR0, m_textColor >> 11);            // R
    writeReg(RA8875_REG_FGCR1, (m_textColor & 0x07E0) >> 5);  // G
    writeReg(RA8875_REG_FGCR2, m_textColor & 0x1F);           // B

    writeReg(RA8875_REG_FNCR0, 0x00);  // ROM font, internal ROM, charset ISO8859-1
//    writeCmd(RA8875_REG_FNCR0);
//    //uint8_t fncr0 = readData();
//    //writeData(fncr0 & 0x6B);  // ROM font, internal ROM
//    writeData(0x00);
  }
  else if ((mode == RA8875_MODE_GRAPHICS) && (mwcr0 & 0x80))
  {
    writeData(mwcr0 & ~0x80);  // Enable graphics mode
  }
  
  SPI.endTransaction();
}

//// TODO: Replaced by setMode, remove?
//void RA8875::setTextMode()
//{
//  SPI.beginTransaction(m_spiSettings);
//
//  writeCmd(RA8875_REG_MWCR0);
//  uint8_t mwcr0 = readData();
//  writeData(mwcr0 | 0x80);  // Enable text mode
//
//  writeCmd(RA8875_REG_FNCR0);
//  //uint8_t fncr0 = readData();
//  //writeData(fncr0 & 0x6B);  // ROM font, internal ROM
//  writeData(0x00);  // ROM font, internal ROM, charset ISO8859-1
//  
//  SPI.endTransaction();
//}

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

  Serial.print("WMCR0: ");
  Serial.println(mwcr0);

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

void RA8875::print(const char *s)
{
  setMode(RA8875_MODE_TEXT);
  
  SPI.beginTransaction(m_spiSettings);

  writeCmd(RA8875_REG_MRWC);
  
  while (char c = *s++)
  {
    writeData(c);
    delay(5);
  }

  SPI.endTransaction();

  setMode(RA8875_MODE_GRAPHICS);
}

void RA8875::println(const char *s)
{
  print(s);

  setCursor(0, getCursorY() + (RA8875_ROM_TEXT_HEIGHT * getTextSizeY()));
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

  Serial.print("mode: "); Serial.println(mode);
  ltpr0 = (ltpr0 & 0xF8) | mode;
  writeReg(RA8875_REG_LTPR0, ltpr0);
  Serial.print("LTPR0: "); Serial.println(ltpr0, HEX);

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
