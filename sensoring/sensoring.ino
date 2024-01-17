// GrainTeller 1.0
// Made by Marcelo Gatica Ruedlinger
// Electrical Engineering Student at Universidad de Chile

#include <Wire.h>
#include <Adafruit_AS7341.h>
#include <LiquidCrystal_I2C.h>

#define MEASUREMENT_COUNT 10
#define THRESHOLD 0.3

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

    return readings[10] > 5000;
}
bool isFirstTime = true;
bool switchedToMixtureRatioMode = false;


// Vector for white light and no grain sample
uint16_t white[12] = {1962, 9994, 7576, 7116, 1, 1, 8665, 8019, 6401, 4631, 25091, 7311};

// Set vectors for each grain sample taken, which will the sample be compared to and classified as
// Sample 1: Corn
uint16_t corn[12] = {771, 3007, 2964, 3818, 0, 0, 6937, 7330, 5904, 3853, 13704, 2144};
// Sample 2: Soybean
uint16_t soy[12] = {606, 2817, 2837, 3194, 0, 0, 4675, 4693, 3600, 2437, 10176, 1526};
// Sample 3: Wheat
uint16_t wheat[12] = {733, 3514, 3236, 3509, 0, 0, 5153, 5163, 4176, 2860, 11916, 2058};
// Sample 4: Ground Soybean
uint16_t gro_soy[12] = {952, 4397, 4340, 4941, 0, 0, 7210, 7203, 5766, 3872, 14700, 2488};
// Sample 5: Ground Corn
uint16_t gro_corn[12] = {1097, 5515, 4634, 5772, 0, 0, 8601, 8004, 5999, 3697, 17279, 2070};

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
        lcd.print("  == Insert ==  ");
        lcd.setCursor(0, 1);
        lcd.print("  == Sample ==  ");
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

    double measuredValue = 0.0;
    double groundSoyValue = static_cast<double>(gro_soy[10]) / white[10];
    double groundCornValue = static_cast<double>(gro_corn[10]) / white[10];

    for (int i = 0; i < MEASUREMENT_COUNT; ++i) {
        if (!sensor.readAllChannels(readings)) {
            lcd.clear();
            lcd.print("Error reading");
            lcd.print("all channels!");
            return;
        }

        measuredValue += static_cast<double>(readings[10]) / white[10];

        delay(20); // Delay between measurements
    }

    measuredValue /= MEASUREMENT_COUNT;

    lcd.clear();

    double totalDistance = groundCornValue - groundSoyValue;
    
    if (totalDistance > 0) {
        double percentage = ((measuredValue - groundSoyValue) / totalDistance) * 100;

        lcd.setCursor(0, 0);
        lcd.print("Gro. Corn %:");
        lcd.print(percentage);
        lcd.print("%");

        lcd.setCursor(0, 1);
        lcd.print("Gro. Soy %:");
        lcd.print(100 - percentage);
        lcd.print("%");
    } else {
        lcd.print("Error calculating");
        lcd.setCursor(0, 1);
        lcd.print("mixture ratio");
    }

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
    delay(10000); // Delay before next scan
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
        lcd.print("  == Insert ==  ");
        lcd.setCursor(0, 1);
        lcd.print("  == Sample ==  ");
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
        normalizedGroSoy[i] = static_cast<double>(gro_soy[i]) / white[i];
        normalizedGroCorn[i] = static_cast<double>(gro_corn[i]) / white[i];
        normalizedCorn[i] = static_cast<double>(corn[i]) / white[i];
        normalizedSoy[i] = static_cast<double>(soy[i]) / white[i];
        normalizedWheat[i] = static_cast<double>(wheat[i]) / white[i];
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
            normalizedReadings[j] = static_cast<double>(readings[j]) / white[j];
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
        lcd.print("Sample: Unknown");
        lcd.setCursor(0, 1);
        lcd.print("     Retry");
    }

    delay(10000); // Delay before next scan

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

    // Perform actions based on the current mode
    switch (currentMode) {
        case GRAIN_SCANNING:
            grainScanningMode();
            break;
        case MIXTURE_RATIO_ESTIMATOR:
            mixtureRatioEstimatorMode();
            break;
    }

    delay(100); // Add a small delay to debounce the switch (adjust as needed)
}