/* VARIABLES */
typedef struct Point {
  double value;
  unsigned long timestamp;
} Point;

// Filter order //
const int bandpassFilterOrder = 20;
const int lowpassFilterOrder = 50;

// Temporal storage array for the past readings //
double rawPastRed[bandpassFilterOrder + 1] = {};
double rawPastIR[bandpassFilterOrder + 1] = {};

double rawRed[lowpassFilterOrder + 1] = {};     // Another temporal array to store readings required
double rawIR[lowpassFilterOrder + 1] = {};      // for lowpass filter

// Filtering coefficient //
double bandpass_coefficient[bandpassFilterOrder + 1] = {-0.00491412182448558, -0.00584637190790136, -0.00322662937579576,  0.00282995063763284,  -0.00434254162286100, -0.0410007732172615,  -0.0806290104876500,  -0.0538074305569527,  0.0768561015244137, 0.246061026320796,  0.324653030774420,  0.246061026320796,  0.0768561015244137, -0.0538074305569527,  -0.0806290104876500,  -0.0410007732172615,  -0.00434254162286100, 0.00282995063763284,  -0.00322662937579576, -0.00584637190790136, -0.00491412182448558};
//double bandpass_coefficient[bandpassFilterOrder + 1] = {-0.00766677381809659,  -0.0250417229133841, -0.0333425652547867, 0.0725464221818444,  0.289815685294444, 0.409773471003693, 0.289815685294444, 0.0725464221818444,  -0.0333425652547867, -0.0250417229133841, -0.00766677381809659};
//double lowpass_coefficient[lowpassFilterOrder + 1] = {-0.00000000000000000050, -0.00125434957075823000, -0.00348982218829102000, -0.00681653329043544000, -0.00990524605271564000, -0.00988463340741338000, -0.00312430965817361000, 0.01328689097924160000, 0.03994323997258690000, 0.07413255381156990000, 0.11000513049742000000,  0.14000158648950700000,  0.15710549241746300000,  0.15710549241746300000,  0.14000158648950700000,  0.11000513049742000000,  0.07413255381156990000,  0.03994323997258690000,  0.01328689097924160000,  -0.00312430965817361000, -0.00988463340741338000, -0.00990524605271564000, -0.00681653329043544000, -0.00348982218829102000, -0.00125434957075823000, -0.00000000000000000050};
double lowpass_coefficient[lowpassFilterOrder + 1] = {-0.00000000000000000050,-0.00053565850763901200,-0.00110641937035495000,-0.00162568576474129000,-0.00188223042697520000,-0.00157413561655947000,-0.00043082678802142800,0.00161052488338632000,0.00424516784313387000,0.00674189629200811000,0.00804939564512551000,0.00708060969454923000,0.00312004615857226000,-0.00376206925373036000,-0.01243503445193870000,-0.02070195129419920000,-0.02562812044867000000,-0.02417154698719080000,-0.01398235439707840000,0.00583459978550164000,0.03421687689303090000,0.06807541337689010000,0.10272753278178800000,0.13276558510370600000,0.15316987059014100000,0.16039702851853000000,0.15316987059014100000,0.13276558510370600000,0.10272753278178800000,0.06807541337689010000,0.03421687689303090000,0.00583459978550164000,-0.01398235439707840000,-0.02417154698719080000,-0.02562812044867000000,-0.02070195129419920000,-0.01243503445193870000,-0.00376206925373036000,0.00312004615857226000,0.00708060969454923000,0.00804939564512551000,0.00674189629200811000,0.00424516784313387000,0.00161052488338632000,-0.00043082678802142800,-0.00157413561655947000,-0.00188223042697520000,-0.00162568576474129000,-0.00110641937035495000,-0.00053565850763901200,-0.00000000000000000050};
/* FUNCTIONS */

double getFilteredValue(double input, double filterCoeffs[], int filterOrder, double pastInput[])  {
    // Discard oldest reading in pastInput
    for (int i = filterOrder; i > 0; i--) {
        pastInput[i] = pastInput[i - 1];
    }
    pastInput[0] = input;

    double output = 0;
    for (int i = 0; i <= filterOrder; i++) {
        output += filterCoeffs[i] * pastInput[i];
    }
    return output;
}

double fir_bandpass_filter(double input, int signalType) {
  if (signalType == 0)
    return getFilteredValue(input, bandpass_coefficient, bandpassFilterOrder, rawPastIR);
  else
    return getFilteredValue(input, bandpass_coefficient, bandpassFilterOrder, rawPastRed);
}

double fir_lowpass_filter(double input, int signalType) {
  if (signalType == 0)
    return getFilteredValue(input, lowpass_coefficient, lowpassFilterOrder, rawIR);
  else
    return getFilteredValue(input, lowpass_coefficient, lowpassFilterOrder, rawRed);
}

/*  Returns whether middle point is peak, valley or intermediate
*   Return: 0 -> intermediary reading (not valuable), 1 -> peak in PPG, -1 -> trough in PPG
*/
int analyzeWindow(Point window[], int windowSize, int midIndex) {
  double midData = window[midIndex].value;

  int dy = 0;
  for (int i = 0; i < windowSize; i++) {
    double data = window[i].value;

    if (midData == data) continue;
    if (dy == 0) dy = (midData > data) ? 1 : -1;
    if (dy == 1 && midData < data)  return 0;
    if (dy == -1 && midData > data) return 0;
  }
  return dy;
}

int analyzeWindowRegular(double window[], int windowSize, int midIndex) {
  double midData = window[midIndex];

  int dy = 0;
  for (int i = 0; i < windowSize; i++) {
    double data = window[i];

    if (midData == data) continue;
    if (dy == 0) dy = (midData > data) ? 1 : -1;
    if (dy == 1 && midData < data)  return 0;
    if (dy == -1 && midData > data) return 0;
  }
  return dy;
}

void initialiseFilters() {
  for (int i = 0; i <= bandpassFilterOrder; i++) {
      rawPastRed[i] = 0;
      rawPastIR[i] = 0;
  }
  for (int i = 0; i <= lowpassFilterOrder; i++) {
      rawRed[i] = 0;
      rawIR[i] = 0;
  }
}
