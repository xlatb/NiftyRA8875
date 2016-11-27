#pragma GCC diagnostic warning "-Wall"
#include "RA8875.h"

const int intPin = 0;
const int csPin = 10;
const int resetPin = 9;

RA8875 tft = RA8875(csPin, intPin);

void setup()
{
  Serial.begin(9600);

//  while (!Serial)
//    ;

  delay(2000);

  Serial.println("Hello!");

  if (!tft.init(480, 272, 16))
  {
    Serial.println("TFT init failed.");
    return;
  }

  tft.clearMemory();
  tft.setBacklight(true);
  tft.setScrollWindow(0, 479, 0, 271);
  
  Serial.println("TFT init complete.");

//  tft.setDrawLayer(2);
//  tft.clearMemory();
//  tft.setDrawLayer(1);

  textTest();

  delay(1000);

  pixelTest();

  delay(1000);

  triangleTest();
  
  delay(1000);

  circleTest();

  delay(1000);

  gradientTest();

  delay(1000);

  smpteBarsTest();

  //tft.setLayerMode(RA8875_LAYER_OR);

  //tft.setDrawLayer(2);

//  tft.drawRect(50, 50, 100, 100, RGB332(0, 255, 0));
//  tft.fillRect(150, 50, 200, 100, RGB332(0, 255, 255));
//  tft.drawLine(250, 50, 300, 100, RGB332(255, 0, 255));
//  tft.drawRect(0, 256, 479, 271, RGB332(255, 0, 255));
}

void textTest()
{
  Serial.println("Text test.");

  tft.setCursor(0, 0);
  Serial.println("Set cursor position complete.");

  tft.setCursorVisibility(false, true);
  Serial.println("Set cursor visibility complete.");

  // Test each underlying "write" method
  tft.setTextSize(1);
  tft.write("Hello, ");
  tft.write((const uint8_t *) "world", 5);
  tft.write((uint8_t) '!');
  tft.write((uint8_t) '\n');

  // Test text sizes
  for (int s = 1; s <= 4; s++)
  {
    tft.setTextSize(s);
    tft.print("Text size ");
    tft.println(s);
  }
  
  Serial.println("Print text complete.");  
}

void smpteBarsTest()
{
  Serial.println("SMPTE colour bars test.");

  tft.clearMemory();

  int width = tft.getWidth();
  int height = tft.getHeight();

  int barWidth = width / 7;
  int boxWidth = width * (7.0 / 40.0);

  int barHeight = height * (2.0 / 3.0);  // Top two thirds
  int miniBarHeight = height * (1.0 / 3.0 / 4.0);  // One quarter of remaining third

  // Top and middle bars
  for (int i = 0; i < 7; i++)
  {
    // Main top bar
    int r = ((i % 4) < 2) ? (255 * 0.75) : 0;
    int g = (i < 4) ? (255 * 0.75) : 0;
    int b = (i % 2) ? 0 : (255 * 0.75);
    tft.fillRect(barWidth * i, 0, barWidth * (i + 1) - 1, barHeight, RGB565(r, g, b));

     // Mini middle bar
    if ((i % 2) == 0)
    {
      tft.fillRect((barWidth * 7) - (barWidth * i), barHeight + 1, (barWidth * 7) - (barWidth * (i + 1) - 1), barHeight + miniBarHeight, RGB565(r, g, b));
    }
  }

  // Bottom boxes
  tft.fillRect(0, barHeight + miniBarHeight + 1, boxWidth - 1, height, RGB565(0, 0, 16));  // Dark blue
  tft.fillRect(boxWidth, barHeight + miniBarHeight + 1, boxWidth * 2 - 1, height, RGB565(255, 255, 255));  // White
  tft.fillRect(boxWidth * 2, barHeight + miniBarHeight + 1, boxWidth * 3 - 1, height, RGB565(16, 0, 16));  // Dark purple
}

void gradientTest()
{
  Serial.println("Gradient test.");

  tft.clearMemory();

  int width = tft.getWidth();
  int barHeight = tft.getHeight() / 4;

  uint32_t starttime = millis();
  
  for (int i = 0; i <= 255; i++)
  {
    tft.fillRect((width / 256.0) * i, 0, (width / 256.0) * i + 1, barHeight, RGB565(i, 0, 0));
    tft.fillRect((width / 256.0) * i, barHeight, (width / 256.0) * i + 1, barHeight * 2, RGB565(0, i, 0));
    tft.fillRect((width / 256.0) * i, barHeight * 2, (width / 256.0) * i + 1, barHeight * 3, RGB565(0, 0, i));
    tft.fillRect((width / 256.0) * i, barHeight * 3, (width / 256.0) * i + 1, barHeight * 4, RGB565(i, i, i));
  }

  uint32_t elapsedtime = millis() - starttime;
  Serial.print("Gradient test took "); Serial.print(elapsedtime); Serial.println(" ms");
}

void pixelTest()
{
  Serial.println("Pixel test.");

  int width = tft.getWidth();
  int height = tft.getHeight();

  for (int i = 0; i < 20000; i++)
  {
    int x = random(0, width);
    int y = random(0, height);
    
    tft.drawPixel(x, y, 0x55);
  }
    
}

void triangleTest()
{
  Serial.println("Triangle test.");

  int width = tft.getWidth();
  int height = tft.getHeight();

  uint32_t starttime = millis();

  for (int i = 0; i < 2000; i++)
  {
    int x1 = random(0, width);
    int y1 = random(0, height);
    int x2 = random(0, width);
    int y2 = random(0, height);
    int x3 = random(0, width);
    int y3 = random(0, height);

    uint16_t color = RGB565(random(0, 255), random(0, 255), random(0, 255));

    tft.fillTriangle(x1, y1, x2, y2, x3, y3, color);
  }

  uint32_t elapsedtime = millis() - starttime;
  Serial.print("Triangle test took "); Serial.print(elapsedtime); Serial.println(" ms");
}

void circleTest()
{
  Serial.println("Circle test.");

  int width = tft.getWidth();
  int height = tft.getHeight();

  uint32_t starttime = millis();

  for (int i = 0; i < 200; i++)
  {
    int x = random(0, width);
    int y = random(0, height);
    int r = random(0, 255);

    uint16_t color = RGB565(random(0, 255), random(0, 255), random(0, 255));

    tft.fillCircle(x, y, r, color);
  }

  uint32_t elapsedtime = millis() - starttime;
  Serial.print("Circle test took "); Serial.print(elapsedtime); Serial.println(" ms");
}

void loop()
{


}
