#include "filter.h"

const int numValues = 10;
double acValues[2][numValues];        // Stores the AC amplitude of the 2 waves
double dcValues[2][numValues];        // Stores the DC values of the 2 waves
long acCounter[2] = {0, 0};           // Corresponding counter variables for AC DC of the 2 waves
long dcCounter[2] = {0, 0};

double avgAC[2] = {0, 0};             // Stores the averaged value of the AC and DC values collected
double avgDC[2] = {0, 0};             // avgAC[0] -> ir_AC | avgAC[1] -> red_AC | avgDC[0] -> ir_DC | avgDC[1] -> red_DC

void initialiseACDCValues() {
  for (int i = 0; i < numValues; i++) {
    acValues[0][i] = 0;
    acValues[1][i] = 0;
  }
}

void computeAverageACDC() {
  double temp = 0;
  int tempCounter = 0;
  for (int i = 0; i < acCounter[0] && i < numValues; i++) {
    temp += acValues[0][i]; tempCounter++;
  }
  if (tempCounter != 0) avgAC[0] = (temp / tempCounter);

  temp = 0; tempCounter = 0;
  for (int i = 0; i < acCounter[1] && i < numValues; i++) {
    temp += acValues[1][i]; tempCounter++;
  }
  if (tempCounter != 0) avgAC[1] = (temp / tempCounter);

  temp = 0; tempCounter = 0;
  for (int i = 0; i < dcCounter[0] && i < numValues; i++) {
    temp += dcValues[0][i]; tempCounter++;
  }
  if (tempCounter != 0) avgDC[0] = (temp / tempCounter);

  temp = 0; tempCounter = 0;
  for (int i = 0; i < dcCounter[1] && i < numValues; i++) {
    temp += dcValues[1][i]; tempCounter++;
  }
  if (tempCounter != 0) avgDC[1] = (temp / tempCounter);
}

void updateACDCValues(double input, int signalType, int typePoint) {
  if (typePoint == -1) {
    dcValues[signalType][dcCounter[signalType]++ % numValues] = input;
  }
  else if (typePoint == 1) {
    computeAverageACDC();
    if (dcCounter[signalType] == 0) return;
    acValues[signalType][acCounter[signalType]++ % numValues] = (input - avgDC[signalType]);
    return;
  }
}

double computeSpO2() {
  computeAverageACDC();
  if (avgAC[0] == 0 || avgAC[1] == 0 || avgDC[0] == 0 || avgDC[1] == 0) return 0;

  double R = avgAC[0] * avgDC[1] / avgDC[0] / avgAC[1];   // irAC * redDC / irDC / redAC
  double SpO2 = 0;
  
  if (R >= 1.15) {
    SpO2 = 99;
  } else if (R >= 0.85) {
    SpO2 = 99 - 5 * (1.15 - R) / 0.3 + 0.5;
  } else if (R > 0.3) {
    SpO2 = 95 - 45 * (0.85 - R) - 0.5;
  } else {
    SpO2 = 75 + R * 10;
  }

  return SpO2;
}

// HR CALCULATION AREA
Point peak[2][2];

double peakDiffs[10];
int peakDiffCounter = 0;
double peakDifferenceThreshold = 7.0;

void initialiseHRValues() {
  Point emptyPoint = {0, 0};
  for (int i = 0; i < 2; i++) {
    peak[i][0] = emptyPoint;
    peak[i][1] = emptyPoint;
  }
  for (int i = 0; i < 10; i++) {
    peakDiffs[i] = 0;
  }
}

void updateAveragePeakDifference(double peakDifference) {
  peakDiffs[peakDiffCounter++ % 10] = peakDifference;
  if (peakDiffCounter < 10) return;
  double temp = 0;
  for (int i = 0; i < 10; i++) {
    temp += peakDiffs[i];
  }
  peakDifferenceThreshold = 1.6 * (temp / 10);
}

int computeHR(int signalType, Point newPeak) {
  Point oldPeak = peak[signalType][0];
  peak[signalType][0] = peak[signalType][1]; // Shift peaks to left to make space for newPeak to go into index 1
  peak[signalType][1] = newPeak;

  if (oldPeak.timestamp == 0) return 0;  // if very first peak, return 0

  // oldPeak -> 2nd last | peak[signalType][0] -> last reading | newPeak -> current reading
  Point lastPeak = oldPeak;
  
//  if (abs(newPeak.value - peak[signalType][0].value) <= peakDifferenceThreshold && abs(newPeak.value - lastPeak.value) > abs(newPeak.value - peak[signalType][0].value))  // use the previous peak (for cases where diastolic peak isn't present)
//    lastPeak = peak[signalType][0];
//  if (abs(newPeak.value - lastPeak.value) > peakDifferenceThreshold) {
//    if (abs(newPeak.value - peak[signalType][0].value) < peakDifferenceThreshold)
//      lastPeak = peak[signalType][0];
//    else
//      return 0;
//  }
//  updateAveragePeakDifference(abs(newPeak.value - lastPeak.value));
  
  double hrPeriod = (newPeak.timestamp - lastPeak.timestamp) / 1000000.0;
  int hr = int(60 / hrPeriod);
  return hr;
}
