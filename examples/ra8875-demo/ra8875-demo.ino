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

  tft.setTextSize(1);
  tft.println("Text size 1");

  tft.setTextSize(2);
  tft.println("Text size 2");

  tft.setTextSize(3);
  tft.println("Text size 3");

  tft.setTextSize(4);
  tft.println("Text size 4");
  
  Serial.println("Print text complete.");  
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
