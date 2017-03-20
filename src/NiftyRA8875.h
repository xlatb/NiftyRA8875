#pragma GCC diagnostic warning "-Wall"

#ifndef RA8875_H
#define RA8875_H

#include <Arduino.h>
#include <SPI.h>

#define RA8875_PRINT_TIMING 0
#define RA8875_ALLOW_TRACE 1

#if RA8875_ALLOW_TRACE
# define RA8875_TRACE(fmt...) do { if (m_tracePrint) { char tracebuf[128]; snprintf(tracebuf, 128, fmt); m_tracePrint->println(tracebuf); } } while(false)
#else
# define RA8875_TRACE(fmt...)
#endif

//#define RGBPACK(r, g, b) ((((r) & 0x07) << 5) | (((g) & 0x07) << 2) | ((b) & 0x03))
#define RGB332(r, g, b) (((r) & 0xE0) | (((g) & 0xE0) >> 3) | (((b) & 0xE0) >> 6))
#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

// TODO: Try 1MHz. What speed is the RA8875 capable of?
// Datasheet says:
// --- snip ---
// The maximum clock rate of 4-Wire SPI write SCL is system clock / 3 (i.e. SPI clock high duty
//  must large than 1.5 system clock) and the maximum clock rate of 4-Wire SPI read SCL is system
//  clock / 6.
// --- snip ---
// System clock (SYS_CLK) is set up by initPLL(). By default it is the same as the external
//  crystal (presumed to be 20MHz), but can go as high as 60MHz.
// The default of 20MHz / 6 is approximately 3MHz, so we wouldn't want to use a faster speed than
//  that until the initPLL() step is completed. After initPLL() is done, we could conceivably try
//  speeds as high as 9 or 10MHz.
#define RA8875_SPI_SPEED 1000000

enum RA8875_Mode
{
  RA8875_MODE_TEXT,
  RA8875_MODE_GRAPHICS
};

enum RA8875_Layer_Mode
{
  RA8875_LAYER_1           = 0x00,
  RA8875_LAYER_2           = 0x01,
  RA8875_LAYER_LIGHTEN     = 0x02,
  RA8875_LAYER_TRANSPARENT = 0x03,
  RA8875_LAYER_OR          = 0x04,
  RA8875_LAYER_AND         = 0x05,
  RA8875_LAYER_FLOAT       = 0x06
};

enum RA8875_Font_Size
{
  RA8875_FONT_SIZE_16 = 0x00,
  RA8875_FONT_SIZE_24 = 0x01,
  RA8875_FONT_SIZE_32 = 0x02
};

enum RA8875_Font_Encoding
{
  // External font ROM encodings
  RA8875_FONT_ENCODING_GB2312   = 0x00,  // GB2312 (Simplified Chinese)
  RA8875_FONT_ENCODING_GB18030  = 0x01,  // GB12345/GB18030 (Chinese)
  RA8875_FONT_ENCODING_BIG5     = 0x02,  // Big5 (Traditional Chinese)

  RA8875_FONT_ENCODING_UNICODE  = 0x03,  // Unicode

  RA8875_FONT_ENCODING_ASCII    = 0x04,  // ASCII

  RA8875_FONT_ENCODING_UNIJAPAN = 0x05,  // Uni-Japanese
  RA8875_FONT_ENCODING_JIS0208  = 0x06,  // JIS X 0208 (Shift JIS?)

  RA8875_FONT_ENCODING_LGCA     = 0x07,  // Latin/Greek/Cyrillic/Arabic

  // Internal font ROM encodings
  RA8875_FONT_ENCODING_8859_1   = 0x10,  // ISO 8859-1 (Latin 1)
  RA8875_FONT_ENCODING_8859_2   = 0x11,  // ISO 8859-2 (Latin 2: Eastern European)
  RA8875_FONT_ENCODING_8859_3   = 0x12,  // ISO 8859-3 (Latin 3: South European)
  RA8875_FONT_ENCODING_8859_4   = 0x13   // ISO 8859-4 (Latin 4: Northern European)
};

enum RA8875_External_Font_Rom
{
  RA8875_FONT_ROM_GT21L16T1W = 0,
  RA8875_FONT_ROM_GT30L16U2W = 1,
  RA8875_FONT_ROM_GT30L24T3Y = 2,
  RA8875_FONT_ROM_GT30L24M1Z = 3,
  RA8875_FONT_ROM_GT30L32S4W = 4
};

enum RA8875_External_Font_Family
{
  RA8875_FONT_FAMILY_FIXED      = 0,
  RA8875_FONT_FAMILY_ARIAL      = 1,
  RA8875_FONT_FAMILY_TIMES      = 2,
  RA8875_FONT_FAMILY_FIXED_BOLD = 3
};

typedef uint8_t RA8875_Font_Flags;

// Dimensions of the built-in ROM font
#define RA8875_ROM_TEXT_WIDTH  8
#define RA8875_ROM_TEXT_HEIGHT 16

// With SPI, the RA8875 expects an initial byte where the top two bits are meaningful. Bit 7
// is RS, bit 6 is RW. See data sheet section 6-1-2-2.
// RS: 0 for data, 1 for command
// RW: 0 for write, 1 for read
#define RA8875_DATA_WRITE  0x00
#define RA8875_DATA_READ   0x40
#define RA8875_CMD_WRITE   0x80
#define RA8875_STATUS_READ 0xC0

// Data sheet 5-2: System & configuration registers
#define RA8875_REG_PWRR   0x01  // Power and display control register
#define RA8875_REG_MRWC   0x02  // Memory read/write command
#define RA8875_REG_PCSR   0x04  // Pixel clock setting register
#define RA8875_REG_SROC   0x05  // Serial Flash/ROM configuration register
#define RA8875_REG_SFCLR  0x06  // Serial Flash/ROM CLK setting register
#define RA8875_REG_SYSR   0x10  // System configuration register
#define RA8875_REG_HDWR   0x14  // Horizontal display width register
#define RA8875_REG_HNDFTR 0x15  // Horiz non-display period fine tuning option register
#define RA8875_REG_HNDR   0x16  // Horiz non-display period register
#define RA8875_REG_HSTR   0x17  // HSYNC start position register
#define RA8875_REG_HPWR   0x18  // HSYNC pulse width register
#define RA8875_REG_VDHR0  0x19  // Vertical display height register 0
#define RA8875_REG_VDHR1  0x1A  // Vertical display height register 1
#define RA8875_REG_VNDR0  0x1B  // Vertical non-display period register 0
#define RA8875_REG_VNDR1  0x1C  // Vertical non-display period register 1
#define RA8875_REG_VSTR0  0x1D  // VSYNC start position register 0
#define RA8875_REG_VSTR1  0x1E  // VSYNC start position register 1
#define RA8875_REG_VPWR   0x1F  // VSYNC pulse width register

// Data sheet 5-3: LCD display control registers
#define RA8875_REG_DPCR   0x20  // Display configuration register
#define RA8875_REG_FNCR0  0x21  // Font control register 0
#define RA8875_REG_FNCR1  0x22  // Font control register 1
#define RA8875_REG_CGSR   0x23  // CGRam select register
#define RA8875_REG_HOFS0  0x24  // Horizontal scroll offset register 0
#define RA8875_REG_HOFS1  0x25  // Horizontal scroll offset register 1
#define RA8875_REG_VOFS0  0x26  // Vertical scroll offset register 0
#define RA8875_REG_VOFS1  0x27  // Vertical scroll offset register 1
#define RA8875_REG_FCURX0 0x2A  // Font write cursor x register 0 (F_CURXL)
#define RA8875_REG_FCURX1 0x2B  // Font write cursor x register 1 (F_CURXH)
#define RA8875_REG_FCURY0 0x2C  // Font write cursor y register 0 (F_CURYL)
#define RA8875_REG_FCURY1 0x2D  // Font write cursor y register 1 (F_CURYH)
#define RA8875_REG_FWTSR  0x2E  // Font write type setting register
#define RA8875_REG_SFRS   0x2F  // Serial Font ROM Setting

// Data sheet 5-4: Active window & scroll window setting registers
#define RA8875_REG_HSAW0  0x30  // Horizontal start point 0 of active window
#define RA8875_REG_HSAW1  0x31  // Horizontal start point 1 of active window
#define RA8875_REG_VSAW0  0x32  // Vertical start point 0 of active window
#define RA8875_REG_VSAW1  0x33  // Vertical start point 1 of active window
#define RA8875_REG_HEAW0  0x34  // Horizontal end point 0 of active window
#define RA8875_REG_HEAW1  0x35  // Horizontal end point 1 of active window
#define RA8875_REG_VEAW0  0x36  // Vertical end point 0 of active window
#define RA8875_REG_VEAW1  0x37  // Vertical end point 1 of active window
#define RA8875_REG_HSSW0  0x38  // Horizontal start point 0 of scroll window
#define RA8875_REG_HSSW1  0x39  // Horizontal start point 1 of scroll window
#define RA8875_REG_VSSW0  0x3A  // Vertical start point 0 of scroll window
#define RA8875_REG_VSSW1  0x3B  // Vertical start point 1 of scroll window
#define RA8875_REG_HESW0  0x3C  // Horizontal end point 0 of scroll window
#define RA8875_REG_HESW1  0x3D  // Horizontal end point 1 of scroll window
#define RA8875_REG_VESW0  0x3E  // Vertical end point 0 of scroll window
#define RA8875_REG_VESW1  0x3F  // Vertical end point 1 of scroll window

// Data sheet 5-5: Cursor setting registers
#define RA8875_REG_MWCR0  0x40  // Memory write control register 0
#define RA8875_REG_MWCR1  0x41  // Memory write control register 1
#define RA8875_REG_CURH0  0x46  // Memory write cursor horizontal position 0
#define RA8875_REG_CURH1  0x47  // Memory write cursor horizontal position 1
#define RA8875_REG_CURV0  0x48  // Memory write cursor vertical position 0
#define RA8875_REG_CURV1  0x49  // Memory write cursor vertical position 1

// Data sheet 5-6: Block Transfer Engine (BTE) registers
#define RA8875_REG_BECR0  0x50  // BTE function control register 0
#define RA8875_REG_BECR1  0x51  // BTE function control register 1
#define RA8875_REG_LTPR0  0x52  // Layer transparency register 0
#define RA8875_REG_LTPR1  0x53  // Layer transparency register 1
#define RA8875_REG_HSBE0  0x54  // Horizontal source point 0 of BTE
#define RA8875_REG_HSBE1  0x55  // Horizontal source point 1 of BTE
#define RA8875_REG_VSBE0  0x56  // Vertical source point 0 of BTE
#define RA8875_REG_VSBE1  0x57  // Vertical source point 1 of BTE
#define RA8875_REG_HDBE0  0x58  // Horizontal destination point 0 of BTE
#define RA8875_REG_HDBE1  0x59  // Horizontal destination point 1 of BTE
#define RA8875_REG_VDBE0  0x5A  // Vertical destination point 0 of BTE
#define RA8875_REG_VDBE1  0x5B  // Vertical destination point 1 of BTE
#define RA8875_REG_BEWR0  0x5C  // BTE width register 0
#define RA8875_REG_BEWR1  0x5D  // BTE width register 1
#define RA8875_REG_BEHR0  0x5E  // BTE height register 0
#define RA8875_REG_BEHR1  0x5F  // BTE height register 1
#define RA8875_REG_BGCR0  0x60  // Background colour register 0 (red)
#define RA8875_REG_BGCR1  0x61  // Background colour register 1 (green)
#define RA8875_REG_BGCR2  0x62  // Background colour register 2 (blue)
#define RA8875_REG_FGCR0  0x63  // Foreground colour register 0 (red)
#define RA8875_REG_FGCR1  0x64  // Foreground colour register 1 (green)
#define RA8875_REG_FGCR2  0x65  // Foreground colour register 2 (blue)
#define RA8875_REG_PTNO   0x66  // Patterno set number for BTE
#define RA8875_REG_BGTR0  0x67  // Background colour register for transparency 0 (red)
#define RA8875_REG_BGTR1  0x68  // Background colour register for transparency 1 (green)
#define RA8875_REG_BGTR2  0x69  // Background colour register for transparency2 (blue)

// Data sheet 5-9: PLL setting registers
#define RA8875_REG_PLLC1  0x88  // PLL control register 1
#define RA8875_REG_PLLC2  0x89  // PLL control register 2

// Data sheet 5-10: PWM control registers
#define RA8875_REG_P1CR   0x8A  // PWM1 control register
#define RA8875_REG_P1DCR  0x8B  // PWM1 duty cycle register
#define RA8875_REG_MCLR   0x8E  // Memory clear control register

// Data sheet 5-11: Drawing control registers
#define RA8875_REG_DCR    0x90  // Draw Control Register
#define RA8875_REG_DLHSR0 0x91  // Draw Line Horizontal Start Register 0
#define RA8875_REG_DLHSR1 0x92  // Draw Line Horizontal Start Register 1
#define RA8875_REG_DLVSR0 0x93  // Draw Line Vertical Start Register 0
#define RA8875_REG_DLVSR1 0x94  // Draw Line Vertical Start Register 1
#define RA8875_REG_DLHER0 0x95  // Draw Line Horizontal End Register 0
#define RA8875_REG_DLHER1 0x96  // Draw Line Horizontal End Register 1
#define RA8875_REG_DLVER0 0x97  // Draw Line Vertical End Register 0
#define RA8875_REG_DLVER1 0x98  // Draw Line Vertical End Register 1
#define RA8875_REG_DCHR0  0x99  // Draw Circle Horizontal Register 0
#define RA8875_REG_DCHR1  0x9A  // Draw Circle Horizontal Register 1
#define RA8875_REG_DCVR0  0x9B  // Draw Circle Vertical Register 0
#define RA8875_REG_DCVR1  0x9C  // Draw Circle Vertical Register 1
#define RA8875_REG_DCRR   0x9D  // Draw Cricle Radius Register
#define RA8875_REG_DTPH0  0xA9  // Draw Triangle Point Horizontal Register 0
#define RA8875_REG_DTPH1  0xAA  // Draw Triangle Point Horizontal Register 1
#define RA8875_REG_DTPV0  0xAB  // Draw Triangle Point Vertical Register 0
#define RA8875_REG_DTPV1  0xAC  // Draw Triangle Point Vertical Register 1

// Data sheet 5-13: Key & IO control registers
#define RA8875_REG_GPIOX  0xC7  // Extra general purpose IO register

// Data sheet 5-15: Serial flash control registers
#define RA8875_REG_SACS_MODE 0xE0  // Serial Flash/ROM Access Mode

// Data sheet 5-16: Interrupt control registers
#define RA8875_REG_INTC1  0xF0  // Interrupt control register 1
#define RA8875_REG_INTC2  0xF1  // Interrupt control register 2

class RA8875 : public Print
{
private:
  int m_csPin;
  int m_intPin;
  int m_resetPin;

  int m_width;
  int m_height;
  int m_depth;

  uint16_t m_textColor;

  SPISettings m_spiSettings;

  Print *m_tracePrint;

  void hardReset(void);
  void softReset(void);

  void writeCmd(uint8_t x);
  void writeData(uint8_t x);
  uint8_t readData(void);
  uint8_t readStatus(void);

  void writeReg(uint8_t reg, uint8_t x);
  uint8_t readReg(uint8_t reg);

  inline void waitBusy(void) { while (readStatus() & 0xC0); };

  void setTextMode(void);
  void setGraphicsMode(void);

  bool initPLL(void);
  bool initDisplay(void);
public:
  RA8875(int csPin, int resetPin = -1);

  // Init
  bool init(int width, int height, int depth);
  void initExternalFontRom(int spiIf, enum RA8875_External_Font_Rom chip);

  void clearMemory();
  void setBacklight(bool enabled);
  
  void setActiveWindow(int xStart, int xEnd, int yStart, int yEnd);

  // Dimensions
  int getWidth() { return m_width; };
  int getHeight() { return m_height; };

  // Text cursor
  void setCursor(int x, int y);
  int getCursorX(void);
  int getCursorY(void);
  void setCursorVisibility(bool visible, bool blink);

  // Text font
  void selectInternalFont(enum RA8875_Font_Encoding enc = RA8875_FONT_ENCODING_8859_1);
  void selectExternalFont(enum RA8875_External_Font_Family family, enum RA8875_Font_Size size, enum RA8875_Font_Encoding enc, RA8875_Font_Flags flags = 0);

  // Text size
  void setTextSize(int xScale, int yScale);
  void setTextSize(int scale) { setTextSize(scale, scale); };
  int getTextSizeX(void);
  int getTextSizeY(void);

  // Text colour
  void setTextColor(uint16_t color) { m_textColor = color; };
  void setTextColor(uint8_t r, uint8_t g, uint8_t b) { m_textColor = RGB565(r, g, b); };
  
  // Text drawing
  virtual size_t write(uint8_t);
  virtual size_t write(const char *str);
  virtual size_t write(const uint8_t *buffer, size_t size);
  void putChar(char c) { putChars(&c, 1); };
  void putChars(const char *buffer, size_t size);
  void putChar16(uint16_t c) { putChars16(&c, 1); };
  void putChars16(const uint16_t *buffer, unsigned int count);

  // Scrolling
  void setScrollWindow(int xStart, int xEnd, int yStart, int yEnd);
  void setScrollOffset(int x, int y);

  // Layers
  void setLayerMode(enum RA8875_Layer_Mode mode);
  void setDrawLayer(int layer);

  // Drawing
  void drawPixel(int x, int y, uint16_t color);
  void setDrawPosition(int x, int y);
  void pushPixel(uint16_t color);

  // Block transfer
  void copyToScreen(int srcX, int srcY, int width, int height, int dstX, int dstY) { copyToScreen(srcX, srcY, width, height, dstX, dstY, false, 0); };
  void copyToScreen(int srcX, int srcY, int width, int height, int dstX, int dstY, bool transparent, uint8_t bgColor);
  void copyFromScreen(int srcX, int srcY, int width, int height, int dstX, int dstY);
  void copy(int srcLayer, int srcX, int srcY, int width, int height, int dstLayer, int dstX, int dstY) { copy(srcLayer, srcX, srcY, width, height, dstLayer, dstX, dstY, false, 0); };
  void copy(int srcLayer, int srcX, int srcY, int width, int height, int dstLayer, int dstX, int dstY, bool transparent, uint8_t bgColor);

  // Low-level shapes
  void drawTwoPointShape(int x1, int y1, int x2, int y2, uint16_t color, uint8_t cmd);
  void drawThreePointShape(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color, uint8_t cmd);
  void drawCircleShape(int x, int y, int radius, uint16_t color, uint8_t cmd);

  // Shapes
  void drawRect(int x1, int y1, int x2, int y2, uint16_t color) { drawTwoPointShape(x1, y1, x2, y2, color, 0x10); };
  void fillRect(int x1, int y1, int x2, int y2, uint16_t color) { drawTwoPointShape(x1, y1, x2, y2, color, 0x30); };
  void drawLine(int x1, int y1, int x2, int y2, uint16_t color) { drawTwoPointShape(x1, y1, x2, y2, color, 0x00); };
  void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color) { drawThreePointShape(x1, y1, x2, y2, x3, y3, color, 0x01); };
  void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color) { drawThreePointShape(x1, y1, x2, y2, x3, y3, color, 0x21); };
  void drawCircle(int x, int y, int radius, uint16_t color) { drawCircleShape(x, y, radius, color, 0x00); };
  void fillCircle(int x, int y, int radius, uint16_t color) { drawCircleShape(x, y, radius, color, 0x20); };

  // Debug trace
  void setTrace(Print *p) { m_tracePrint = p; };
};

#endif
