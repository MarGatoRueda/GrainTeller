// GrainTeller 1.0
// Made by Marcelo Gatica Ruedlinger
// Electrical Engineering Student at Universidad de Chile

#include <Wire.h>
#include <Adafruit_AS7341.h>
#include <LiquidCrystal_I2C.h>

#define MEASUREMENT_COUNT 10
#define THRESHOLD 0.1

LiquidCrystal_I2C lcd(0x3F, 16, 2);  // Set the LCD address to 0x3F
Adafruit_AS7341 sensor;

// Function template for both uint16_t and double arrays
template <typename T>
double euclideanDistance(T *reading1, T *reading2, uint8_t size) {
    double sum = 0.0;

    for (uint8_t i = 0; i < size; i++) {
        if (i == 4 || i == 5) continue; // Skip indices 4, 5, since those are duplicates
        double diff = static_cast<double>(reading1[i]) - static_cast<double>(reading2[i]);
        sum += diff * diff;
    }

    return sqrt(sum);
}

// Specialization for uint16_t arrays
template <>
double euclideanDistance(uint16_t *reading1, uint16_t *reading2, uint8_t size) {
    double sum = 0.0;

    for (uint8_t i = 0; i < size; i++) {
        if (i == 4 || i == 5) continue; // Skip indices 4, 5, since those are duplicates
        double diff = static_cast<double>(reading1[i]) - static_cast<double>(reading2[i]);
        sum += diff * diff;
    }

    return sqrt(sum);
}


bool isScanTriggered() {
    uint16_t readings[12];
    sensor.readAllChannels(readings);

    return readings[10] < 11200;
}
bool isFirstTime = true;
bool switchedToMixtureRatioMode = false;

// Vector for white light when samples were taken.
uint16_t whiteDatabase[12] = {897, 5964, 4261, 4014, 1, 1, 4959, 4241, 3073, 1815, 11935, 670};
uint16_t whiteReading[12];

// Set vectors for each grain sample taken, which will the sample be compared to and classified as
// Sample 1: Corn
uint16_t corn[12] = {488, 2273, 2091, 2617, 0, 0, 4392, 4472, 3600, 2161, 8751, 674};
// Sample 2: Soybean
uint16_t soy[12] = {333, 1597, 1640, 1897, 0, 0, 2806, 2774, 2239, 1412, 6339, 498};
// Sample 3: Wheat
uint16_t wheat[12] = {379, 1942, 1843, 2018, 0, 0, 3015, 3003, 2444, 1544, 6991, 550};
// Sample 4: Ground Soybean
uint16_t gro_soy[12] = {458, 2318, 2371, 2675, 0, 0, 3950, 3901, 3090, 1922, 8512, 612};
// Sample 5: Ground Corn
uint16_t gro_corn[12] = {649, 3428, 2975, 3608, 0, 0, 5411, 5039, 3736, 2199, 11145, 738};

// Mixture Samples for Mixture Ratio Estimator Mode:
// Sample 1: 20% Ground Corn, 80% Ground Soybean
uint16_t gro_soy_80_gro_corn_20[12] = {470, 2395, 2402, 2725, 0, 0, 4044, 3991, 3134, 1936, 8704, 636};
// Sample 2: 50% Ground Corn, 50% Ground Soybean
uint16_t gro_soy_50_gro_corn_50[12] = {519, 2706, 2515, 2941, 0, 0, 4417, 4257, 3274, 1993, 9001, 642};
// Sample 3: 70% Ground Corn, 30% Ground Soybean
uint16_t gro_soy_30_gro_corn_70[12] = {556, 2839, 2663, 3143, 0, 0, 4740, 4553, 3481, 2115, 9888, 700};
// Sample 4: 90% Ground Corn, 10% Ground Soybean
uint16_t gro_soy_10_gro_corn_90[12] = {612, 3220, 2887, 3460, 0, 0, 5256, 4944, 3706, 2218, 10658, 728};
// Sample 5: 100% Ground Soybean
uint16_t gro_soy_100[12] = {458, 2318, 2371, 2675, 0, 0, 3950, 3901, 3090, 1922, 8512, 612};
// Sample 6: 100% Ground Corn
uint16_t gro_corn_100[12] = {649, 3428, 2975, 3608, 0, 0, 5411, 5039, 3736, 2199, 11145, 738};



void setup() {
    Wire.begin();
    lcd.init();                      // Initialize the LCD
    lcd.backlight();                 // Turn on the backlight
    Serial.begin(115200);

    while (!Serial) {
        delay(1);
    }

    if (!sensor.begin()) {
        lcd.setCursor(0, 0);
        lcd.print("AS7341 not found");
        while (1);
    }

    sensor.setATIME(100);
    sensor.setASTEP(999);
    sensor.setGain(AS7341_GAIN_256X);

    sensor.setLEDCurrent(60);    // Set LED's intensity
    sensor.enableLED(true);       // Turn on the LED's

    lcd.setCursor(0, 0);
    lcd.print("GrainTeller 1.0");
    lcd.setCursor(0, 1);
    lcd.print("By: Marcelo G.R.");
    delay(3000);
    lcd.clear();

    pinMode(A0, INPUT);
    pinMode(A1, INPUT);

    // Obtain the white light vector
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrating,");
    lcd.setCursor(0, 1);
    lcd.print("Please wait.");

    uint16_t whiteReadings[12];
    double sumWhiteReadings[12] = {0.0};

    for (int i = 0; i < MEASUREMENT_COUNT; ++i) {
        if (!sensor.readAllChannels(whiteReadings)) {
            lcd.clear();
            lcd.print("Error reading");
            lcd.print("all channels!");
            return;
        }

        for (int j = 0; j < 12; ++j) {
            sumWhiteReadings[j] += whiteReadings[j];
        }

        delay(50); // Delay between measurements
    }

    for (int i = 0; i < 12; ++i) {
        whiteReading[i] = sumWhiteReadings[i] / MEASUREMENT_COUNT;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibration");
    lcd.setCursor(0, 1);
    lcd.print("complete!");
    delay(2000);
}



enum Mode {
    GRAIN_SCANNING,
    MIXTURE_RATIO_ESTIMATOR
};

Mode currentMode = GRAIN_SCANNING;

void mixtureRatioEstimatorMode() {

    if (switchedToMixtureRatioMode || isFirstTime) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Switched to");
        lcd.setCursor(0, 1);
        lcd.print("Mixture % Mode");
        delay(2000);
        isFirstTime = false;
        switchedToMixtureRatioMode = false;
    }

    while (!isScanTriggered()) {
        lcd.setCursor(0, 0);
        lcd.print("Please insert   ");
        lcd.setCursor(0, 1);
        lcd.print("Mixture sample  ");
        delay(500);
    }
    lcd.clear();
    // Sample detected countdown
    for (int i = 3; i > 0; --i) {
        lcd.setCursor(0, 0);
        lcd.print("Sample detected,");
        lcd.setCursor(0, 1);
        lcd.print("scanning in ");
        lcd.print(i);
        delay(1000);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scanning...");

    uint16_t readings[12];

    double sumDistancesGroSoy = 0.0;
    double sumDistancesGroCorn = 0.0;
    double normalizedGroSoy[12];
    double normalizedGroCorn[12];

    // Obtain the normalized vectors for the pure samples
    for (int i = 0; i < 12; ++i) {
        normalizedGroSoy[i] = static_cast<double>(gro_soy_100[i]) / whiteDatabase[i];
        normalizedGroCorn[i] = static_cast<double>(gro_corn_100[i]) / whiteDatabase[i];
    }

    for (int i = 0; i < MEASUREMENT_COUNT; ++i) {
        if (!sensor.readAllChannels(readings)) {
            lcd.clear();
            lcd.print("Error reading");
            lcd.print("all channels!");
            return;
        }

        double normalizedReadings[12];


        for (int j = 0; j < 12; ++j) {
            normalizedReadings[j] = static_cast<double>(readings[j]) / whiteReading[j];
        }
        double distanceGroSoy = euclideanDistance(normalizedReadings, normalizedGroSoy, 12);
        double distanceGroCorn = euclideanDistance(normalizedReadings, normalizedGroCorn, 12);

        sumDistancesGroSoy += distanceGroSoy;
        sumDistancesGroCorn += distanceGroCorn;

        delay(50); // Delay between measurements
    }

    double averageDistanceGroSoy = sumDistancesGroSoy / MEASUREMENT_COUNT;
    double averageDistanceGroCorn = sumDistancesGroCorn / MEASUREMENT_COUNT;

    // Compute the distance between the two pure samples at the 6th and 7th elements
    double totalDistance = euclideanDistance(normalizedGroSoy, normalizedGroCorn, 12);

    // Grade the distance of the scanned sample from 0 to 100
    double grade = 100 - ((averageDistanceGroSoy / totalDistance) * 100);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gro. Soy %");
    lcd.print(grade, 0);
    lcd.setCursor(0, 1);
    lcd.print("Gro. Corn %");
    lcd.print(100 - grade, 0);
    
    Serial.print("ADC0/F1 415nm : ");
    Serial.println(readings[0]);
    Serial.print("ADC1/F2 445nm : ");
    Serial.println(readings[1]);
    Serial.print("ADC2/F3 480nm : ");
    Serial.println(readings[2]);
    Serial.print("ADC3/F4 515nm : ");
    Serial.println(readings[3]);
    Serial.print("ADC0/F5 555nm : ");
    Serial.println(readings[6]);
    Serial.print("ADC1/F6 590nm : ");
    Serial.println(readings[7]);
    Serial.print("ADC2/F7 630nm : ");
    Serial.println(readings[8]);
    Serial.print("ADC3/F8 680nm : ");
    Serial.println(readings[9]);
    Serial.print("ADC4/Clear    : ");
    Serial.println(readings[10]);
    Serial.print("ADC5/NIR      : ");
    Serial.println(readings[11]);

    Serial.println();

    // Scan again after the sensor detects a spike in the clear channel
    while (true) {
        if (!isScanTriggered()) {
            break;
        }
    }
}

void grainScanningMode() {

    if (isFirstTime) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Switched to");
        lcd.setCursor(0, 1);
        lcd.print("Grain Scanner");
        delay(3000);
        isFirstTime = false;
        switchedToMixtureRatioMode = false;
    }

    while (!isScanTriggered()) {
        lcd.setCursor(0, 0);
        lcd.print("Please insert   ");
        lcd.setCursor(0, 1);
        lcd.print("Grain sample    ");
        delay(500);
    }
    lcd.clear();
    // Sample detected countdown
    for (int i = 3; i > 0; --i) {
        lcd.setCursor(0, 0);
        lcd.print("Sample detected,");
        lcd.setCursor(0, 1);
        lcd.print("scanning in ");
        lcd.print(i);
        delay(1000);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scanning...");

    uint16_t readings[12];

    double sumDistancesCorn = 0.0;
    double sumDistancesSoy = 0.0;
    double sumDistancesWheat = 0.0;
    double sumDistancesGroSoy = 0.0;
    double sumDistancesGroCorn = 0.0;

    double normalizedGroSoy[12];
    double normalizedGroCorn[12];
    double normalizedCorn[12];
    double normalizedSoy[12];
    double normalizedWheat[12];

    for (int i = 0; i < 12; ++i) {
        normalizedGroSoy[i] = static_cast<double>(gro_soy[i]) / whiteDatabase[i];
        normalizedGroCorn[i] = static_cast<double>(gro_corn[i]) / whiteDatabase[i];
        normalizedCorn[i] = static_cast<double>(corn[i]) / whiteDatabase[i];
        normalizedSoy[i] = static_cast<double>(soy[i]) / whiteDatabase[i];
        normalizedWheat[i] = static_cast<double>(wheat[i]) / whiteDatabase[i];
    }

    for (int i = 0; i < MEASUREMENT_COUNT; ++i) {
        if (!sensor.readAllChannels(readings)) {
            lcd.clear();
            lcd.print("Error reading");
            lcd.print("all channels!");
            return;
        }

        double normalizedReadings[12];
        for (int j = 0; j < 12; ++j) {
            normalizedReadings[j] = static_cast<double>(readings[j]) / whiteReading[j];
        }

        double distanceCorn = euclideanDistance(normalizedReadings, normalizedCorn, 12);
        double distanceSoy = euclideanDistance(normalizedReadings, normalizedSoy, 12);
        double distanceWheat = euclideanDistance(normalizedReadings, normalizedWheat, 12);
        double distanceGroSoy = euclideanDistance(normalizedReadings, normalizedGroSoy, 12);
        double distanceGroCorn = euclideanDistance(normalizedReadings, normalizedGroCorn, 12);

        sumDistancesCorn += distanceCorn;
        sumDistancesSoy += distanceSoy;
        sumDistancesWheat += distanceWheat;
        sumDistancesGroSoy += distanceGroSoy;
        sumDistancesGroCorn += distanceGroCorn;

        delay(20); // Delay between measurements
    }

    double averageDistanceCorn = sumDistancesCorn / MEASUREMENT_COUNT;
    double averageDistanceSoy = sumDistancesSoy / MEASUREMENT_COUNT;
    double averageDistanceWheat = sumDistancesWheat / MEASUREMENT_COUNT;
    double averageDistanceGroSoy = sumDistancesGroSoy / MEASUREMENT_COUNT;
    double averageDistanceGroCorn = sumDistancesGroCorn / MEASUREMENT_COUNT;

    lcd.clear();

    double distances[5] = {
        averageDistanceCorn,
        averageDistanceSoy,
        averageDistanceWheat,
        averageDistanceGroSoy,
        averageDistanceGroCorn
    };

    double minDist = distances[0];
    int index = 0;

    for (int i = 1; i < 5; ++i) {
        if (distances[i] < minDist) {
            minDist = distances[i];
            index = i;
        }
    }

    if (minDist < THRESHOLD) {
        lcd.setCursor(0, 0);
        lcd.print("     Sample: ");
        lcd.setCursor(0, 1);
        switch (index) {
            case 0:
                lcd.print("      Corn");
                break;
            case 1:
                lcd.print("     Soybean");
                break;
            case 2:
                lcd.print("      Wheat");
                break;
            case 3:
                lcd.print("   Ground Soy");
                break;
            case 4:
                lcd.print("   Ground Corn");
                break;
        }
    } else {
        lcd.print("Sample Unknown");
        lcd.setCursor(0, 1);
        lcd.print("Please retry");
    }

    Serial.print("Min distance: ");
    Serial.println(minDist, 3);
    Serial.print("ADC0/F1 415nm : ");
    Serial.println(readings[0]);
    Serial.print("ADC1/F2 445nm : ");
    Serial.println(readings[1]);
    Serial.print("ADC2/F3 480nm : ");
    Serial.println(readings[2]);
    Serial.print("ADC3/F4 515nm : ");
    Serial.println(readings[3]);
    Serial.print("ADC0/F5 555nm : ");
    Serial.println(readings[6]);
    Serial.print("ADC1/F6 590nm : ");
    Serial.println(readings[7]);
    Serial.print("ADC2/F7 630nm : ");
    Serial.println(readings[8]);
    Serial.print("ADC3/F8 680nm : ");
    Serial.println(readings[9]);
    Serial.print("ADC4/Clear    : ");
    Serial.println(readings[10]);
    Serial.print("ADC5/NIR      : ");
    Serial.println(readings[11]);

    Serial.println();

     // Scan again after the sensor detects a spike in the clear channel
    while (true) {
        if (!isScanTriggered()) {
            break;
        }
    }
}

void loop() {

    // Check the state of the switch connected to A0 or A1
    if (analogRead(A0) > analogRead(A1)) {
        if (currentMode != MIXTURE_RATIO_ESTIMATOR) {
            currentMode = MIXTURE_RATIO_ESTIMATOR;
            switchedToMixtureRatioMode = false;
            isFirstTime = true;
        }
    } else if (analogRead(A1) > analogRead(A0)) {
        if (currentMode != GRAIN_SCANNING) {
            currentMode = GRAIN_SCANNING;
            switchedToMixtureRatioMode = true;
            isFirstTime = true;
        }
    }

    switch (currentMode) {
        case GRAIN_SCANNING:
            grainScanningMode();
            break;
        case MIXTURE_RATIO_ESTIMATOR:
            mixtureRatioEstimatorMode();
            break;
    }
}