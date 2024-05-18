#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <Wire.h>
#include <SPI.h>

// Pin definition for LCD communication//
#define cs 13       //chip select pin; any digital pin
#define dc 11       // pin to tell the shield the upcoming signal is data/command; any digital pin
#define rst 12      // pin for shield rest; any digital pin

#define tcs 22
#define t_irq 23

int sadFaceDisplaying = 0;
int smileyFaceDisplaying = 0;

int screenOrientation = 0;

// Define coordinates for displaying data
int start_hr[2][2] = {{30, 40} , {100, 5}};
int start_spo2[2][2] = {{150, 40}, {100, 50}};
int start_face[2][2] = {{110, 80}, {20, 200}};
int ppg_start[2] = {140, 120};

const int smiley_face[][2] = {
  {0,2},{0,3}, {0,4}, {0,5}, {1,2}, {1,3}, {1,4}, {1,5}, {2,2}, {2,3}, {2,4},{2,5}, {3,2}, {3,3}, {3,4}, {3,5},  
  {0,10},{0,11}, {0,12}, {0,13}, {1,10}, {1,11}, {1,12}, {1,13}, {2,10}, {2,11}, {2,12},{2,13}, {3,10}, {3,11}, {3,12}, {3,13},
  {9,0}, {9,1},{9,2},{9,3},{9,4}, {10,0}, {10,1}, {10,2}, {10,3}, {10,4}, {11,0}, {11,1}, {11,2}, {11,3}, {11,4},
  {9,11},{9,12}, {9,13}, {9,14}, {9,15}, {10,11}, {10,12}, {10, 13}, {10, 14}, {10,15}, {1,11}, {11,12}, {11,13}, {11,14}, {11,15},
  {12,3}, {12,4}, {12,5}, {12,6}, {12,7}, {12,8}, {12,9}, {12,10}, {12,11}, {12,12},
  {13,3}, {13,4}, {13,5}, {13,6}, {13,7}, {13,8}, {13,9}, {13,10}, {13,11}, {13,12},
  {14,3}, {14,4}, {14,5}, {14,6}, {14,7}, {14,8}, {14,9}, {14,10}, {14,11}, {14,12},
  {15,5},{15,6},{15,7},{15,8},{15,9}, {15,10}
};

const int sad_face[][2] = {
  {0,2},{0,3}, {0,4}, {0,5}, {1,2}, {1,3}, {1,4}, {1,5}, {2,2}, {2,3}, {2,4},{2,5}, {3,2}, {3,3}, {3,4}, {3,5},  
  {0,10},{0,11}, {0,12}, {0,13}, {1,10}, {1,11}, {1,12}, {1,13}, {2,10}, {2,11}, {2,12},{2,13}, {3,10}, {3,11}, {3,12}, {3,13},
  {12,0}, {12,1},{12,2},{12,3},{12,4}, {13,0}, {13,1}, {13,2}, {13,3}, {13,4}, {14,0}, {14,1}, {14,2}, {14,3}, {14,4},
  {12,11},{12,12}, {12,13}, {12,14}, {12,15}, {13,11}, {13,12}, {13, 13}, {13, 14}, {13,15}, {14,11}, {14,12}, {14,13}, {14,14}, {14,15},
  {9,3}, {9,4}, {9,5}, {9,6}, {9,7}, {9,8}, {9,9}, {9,10}, {9,11}, {9,12},
  {10,3}, {10,4}, {10,5}, {10,6}, {10,7}, {10,8}, {10,9}, {10,10}, {10,11}, {10,12},
  {11,3}, {11,4}, {11,5}, {11,6}, {11,7}, {11,8}, {11,9}, {11,10}, {11,11}, {11,12},
  {8,5},{8,6},{8,7},{8,8},{8,9}, {8,10}
};

Adafruit_ILI9341 TFT_LCD = Adafruit_ILI9341(cs, dc);
XPT2046_Touchscreen ts(tcs, t_irq);

void lcd_Smile(int start_x, int start_y, uint16_t color);
void lcd_Sad(int start_x, int start_y, uint16_t color);
void lcd_end();
void lcd_setTitle();

void lcd_changeOrientation() {
  screenOrientation = (screenOrientation == 0) ? 1 : 0;
  TFT_LCD.setRotation(screenOrientation);
  sadFaceDisplaying = 0;
  smileyFaceDisplaying = 0;
  lcd_end();
  lcd_setTitle();
}

void lcd_begin() {
    TFT_LCD.begin();
    TFT_LCD.setRotation(0);
    TFT_LCD.fillScreen(ILI9341_BLACK);
    ts.begin();
    ts.setRotation(0);
    TFT_LCD.setTextColor(ILI9341_WHITE);
    TFT_LCD.setTextSize(2);
}

void lcd_setTitle(){
  TFT_LCD.setTextSize(2);
  TFT_LCD.setTextColor(ILI9341_WHITE);
  if (screenOrientation == 0) {
    TFT_LCD.setCursor(0, 0);
    TFT_LCD.println("   HR");
    TFT_LCD.setCursor(120, 0);
    TFT_LCD.println(" SpO2%");
  }
  else if (screenOrientation == 1) {
    TFT_LCD.setCursor(5, 5);
    TFT_LCD.println(" HR\n\n");
    TFT_LCD.println(" SpO2%");
  }
}

void lcd_displayHR(int HR){
    TFT_LCD.fillRect(start_hr[screenOrientation][0], start_hr[screenOrientation][1], 40, 20, ILI9341_BLACK);
    TFT_LCD.setCursor(start_hr[screenOrientation][0], start_hr[screenOrientation][1]);
    TFT_LCD.setTextSize(2);
    if (HR < 0) { TFT_LCD.println("--"); return; }
    if (HR>120) {
        TFT_LCD.setTextColor(ILI9341_RED);
        if (sadFaceDisplaying == 0) {
          sadFaceDisplaying == 1;
          smileyFaceDisplaying = 0;
          lcd_Sad(start_face[screenOrientation][0], start_face[screenOrientation][1], ILI9341_RED);
        }
    }
    else if (HR<60) {
        TFT_LCD.setTextColor(ILI9341_YELLOW);
        if (sadFaceDisplaying == 0) {
          sadFaceDisplaying == 1;
          smileyFaceDisplaying = 0;
          lcd_Sad(start_face[screenOrientation][0], start_face[screenOrientation][1], ILI9341_ORANGE);
        }
    } else {
      TFT_LCD.setTextColor(ILI9341_GREEN);
      if (smileyFaceDisplaying == 0) {
        smileyFaceDisplaying = 1;
        sadFaceDisplaying = 0;
        lcd_Smile(start_face[screenOrientation][0], start_face[screenOrientation][1], ILI9341_GREEN);
      }
    }
    TFT_LCD.println(HR);
}

void lcd_displaySpO2(int SpO2){
    TFT_LCD.fillRect(start_spo2[screenOrientation][0], start_spo2[screenOrientation][1], 40, 20, ILI9341_BLACK);
    TFT_LCD.setCursor(start_spo2[screenOrientation][0], start_spo2[screenOrientation][1]);
    TFT_LCD.setTextSize(2);
    if (SpO2 < 0) {
      TFT_LCD.println("--");
      return;
    }
    TFT_LCD.setTextColor(ILI9341_GREEN);
    if (SpO2 < 95)
          TFT_LCD.setTextColor(ILI9341_ORANGE);
    else if (SpO2 < 90)
          TFT_LCD.setTextColor(ILI9341_RED);
    if (SpO2 <= 70)
      TFT_LCD.println("--");
    else
      TFT_LCD.println(SpO2);
}

void lcd_displayPPG(double value, int counter) {
  int yMax = 1600, yMin = 100;
  double valueNormalized = (value - yMin) / (yMax - yMin);
  int yScaled = int(valueNormalized * (TFT_LCD.height() - ppg_start[screenOrientation]));    // multiply by max height of waveform on display
  TFT_LCD.fillRect(counter % TFT_LCD.width(), ppg_start[screenOrientation], 10, (TFT_LCD.height() - ppg_start[screenOrientation]), ILI9341_BLACK);
  TFT_LCD.drawPixel(counter % TFT_LCD.width(), TFT_LCD.height() - yScaled, ILI9341_WHITE);
}


void lcd_Smile(int start_x, int start_y,uint16_t color){
    int numPixels = sizeof(smiley_face) / sizeof(smiley_face[0]);
    TFT_LCD.fillRect(start_y, start_x, 16, 16,ILI9341_BLACK);
    for (int i = 0; i < numPixels; i++) {
      int y = start_x + smiley_face[i][0];
      int x = start_y + smiley_face[i][1];
      TFT_LCD.drawPixel(x, y,color);
    }
}

void lcd_Sad(int start_x, int start_y,uint16_t color){
    TFT_LCD.fillRect(start_y, start_x, 16, 16,ILI9341_BLACK);
    int numPixels = sizeof(sad_face) / sizeof(sad_face[0]);
    for (int i = 0; i < numPixels; i++) {
      int y = start_x + sad_face[i][0];
      int x = start_y + sad_face[i][1];
      TFT_LCD.drawPixel(x, y,color);
  }
}

void lcd_end(){
  TFT_LCD.fillRect(0, 0, TFT_LCD.width(), TFT_LCD.height(), ILI9341_BLACK);
}
