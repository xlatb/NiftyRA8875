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
  
  tft.init();
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
  int barHeight = tft.getHeight() / 3;
  
  for (int i = 0; i <= 255; i++)
  {
    tft.fillRect((width / 256.0) * i, 0, (width / 256.0) * i + 1, barHeight, RGB332(i, 0, 0));
    tft.fillRect((width / 256.0) * i, barHeight, (width / 256.0) * i + 1, barHeight * 2, RGB332(0, i, 0));
    tft.fillRect((width / 256.0) * i, barHeight * 2, (width / 256.0) * i + 1, barHeight * 3, RGB332(0, 0, i));
    //tft.fillRect(0, 0, 479, 271, RGB332(0, i, 0));
    //tft.fillRect(0, 0, 479, 271, RGB332(0, 0, i));
  }
  
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

void loop()
{


}
