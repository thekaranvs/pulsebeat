#include "screen.h"
#include "operationPPG.h"

//==========================================================================================//
//                                1. Variable declaration                                   //
//==========================================================================================//

//================================= Pulse oximeter ===================================//

// All variables in your pulse oximeter //

const int loopFrequency = 50;                           // In Hz (sampling frequency = loopFrequency / 2)
const long loopPeriod = long(1000000 / loopFrequency);
int signalType = 0;                                     // Denotes which signal is turned on and measured (IR = 0, Red = 1)
unsigned long startTime, endTime;

int red_LED = DAC1;
int ir_LED = DAC0;
int sensor_input = A0;

double inputValue, filteredValue;

//========================= Heart Rate and SpO2 Calculation ============================//

// All variables required for peak detection and subsequent heart rate and SpO2 calculation //

long curr[2] = {0, 0};                    // 2 counter variables for each LED
const int windowSize = 3;                 // Window of readings (used to detect peaks/valleys)
Point bandpassFiltered[2][windowSize];    // 2D array to store temporal filtered values of each LED (bandpass)
double lowpassFiltered[2][windowSize];    // 2D array to store temporal filtered values of each LED (lowpass - no need for timestamp)

int heartrate = 0, hrCounter = 0;
const int hrBufferSize = 7;
int hrArray[hrBufferSize];

int SpO2 = 0, spo2Counter = 0;
const int spo2BufferSize = 7;
int spo2Array[spo2BufferSize];

//==========================================================================================//
//                            2. Main program (initialize environment)                      //
//==========================================================================================//
void setup() {
  Serial.begin(115200);
  lcd_begin();
  lcd_setTitle();
  analogReadResolution(12);

  initialiseFilters();
  initialiseACDCValues();
  initialiseHRValues();

  // Initialise readings of filtered values to 0
  Point emptyPoint = {0, 0};
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < windowSize; j++) {
      bandpassFiltered[i][j] = emptyPoint;
      lowpassFiltered[i][j] = 0;
    }
  }

  for(int i = 0; i < hrBufferSize; i++) {
    hrArray[i] = 0;
    spo2Array[i] = 0;
  }
}



//==========================================================================================//
//                                 3. Main program (looping)                                //
//==========================================================================================//
void loop() {

  if (ts.touched()) {
    lcd_changeOrientation();
  }
    
  // Set variable to record start timestamp for loop timestamp monitoring (i.e sampling period) //
  startTime = micros();
    
  // Sampling and operating the LEDs and photodiode //
  if (signalType == 0) {            // Turn on IR LED
      digitalWrite(red_LED, LOW);
      digitalWrite(ir_LED, HIGH);
  } else {                          // Turn on Red LED
      digitalWrite(red_LED, HIGH);
      digitalWrite(ir_LED, LOW);
  }
  
  // Delay by a constant amount of timestamp
  delayMicroseconds(500);
  inputValue = analogRead(sensor_input);

  if (inputValue >= 4000) {   // For finger out
    lcd_displayHR(-1);
    lcd_displaySpO2(-1);
    delay(500);
    return;
  }

  // BANDPASS SECTION
  // Apply filtering //
  filteredValue = fir_bandpass_filter(inputValue, signalType);
  
  // Store the filtered value in corresponding array (in bandpassFiltered)
  Point dataPoint = {filteredValue, micros()};
  bandpassFiltered[signalType][curr[signalType] % windowSize] = dataPoint;

  int midIndex = (curr[signalType] - (windowSize / 2)) % windowSize;  // Get middle reading in window
  int typePoint = analyzeWindow(bandpassFiltered[signalType], windowSize, midIndex);  // Tells if middle value is peak / valley / intermediate
  if (curr[signalType] < windowSize) typePoint = 0; // If we haven't collected enough values, set typePoint = 0

  // If wave is peak (and red), compute HR
  if (typePoint == 1 && signalType == 1) {
    heartrate = computeHR(signalType, bandpassFiltered[signalType][midIndex]);
    if (heartrate > 30 && heartrate < 150) {
      hrArray[hrCounter++ % hrBufferSize] = heartrate;
    }
  }

  if ((hrCounter % hrBufferSize) == 0 && hrCounter != 0 && signalType == 1) {
    double avgHR = 0;
    for (int i = 0; i < hrBufferSize; i++)
      avgHR += hrArray[i];
    avgHR /= hrBufferSize;
    lcd_displayHR(int(avgHR));
  }

   // Print filtered values to serial port for visualization //
  Serial.print(filteredValue);
  if (signalType == 1)
    Serial.println();
  else
    Serial.print(" ");

  if (signalType == 1 && filteredValue <= 1600 && filteredValue >= 100)
    lcd_displayPPG(filteredValue, curr[signalType]);


  // LOWPASS SECTION
  filteredValue = fir_lowpass_filter(inputValue, signalType);
  lowpassFiltered[signalType][curr[signalType] % windowSize] = filteredValue;
  typePoint = analyzeWindowRegular(lowpassFiltered[signalType], windowSize, midIndex);
  if (curr[signalType] < windowSize) typePoint = 0;
  updateACDCValues(filteredValue, signalType, typePoint);

  if (curr[signalType] % (loopFrequency) == 0) {
    double SpO2 = computeSpO2();
    if (SpO2 > 80) spo2Array[spo2Counter++ % spo2BufferSize] = SpO2;
  }

  if ((spo2Counter % spo2BufferSize) == 0 && spo2Counter != 0) {
    double avgSpo2 = 0;
    for (int i = 0; i < spo2BufferSize; i++) {
      avgSpo2 += spo2Array[i];
    }
    avgSpo2 /= spo2BufferSize;
    lcd_displaySpO2(int(avgSpo2));
  }

  curr[signalType] = curr[signalType] + 1; // Update counter
//  if (curr[signalType] >= 1500) curr[signalType] = 0;
  
  // Flip the signalType to other value (if 0 i.e. IR, make it 1 so that Red is turned on and measured next loop)
  signalType = (signalType == 0) ? 1 : 0;

  // Set vaiable to record end timestamp for loop timestamp monitoring (i.e sampling period) //
  endTime = micros();
  // Set delay equal to the difference between start and end timestamps to control loop period //
  delayMicroseconds(loopPeriod - (endTime - startTime));
}
