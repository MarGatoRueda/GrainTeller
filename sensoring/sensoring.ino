// GrainTeller 1.0
// Made by Marcelo Gatica Ruedlinger
// Electrical Engineering Student at Universidad de Chile

#include <Wire.h>
#include <Adafruit_AS7341.h>
#include <LiquidCrystal_I2C.h>

#define MEASUREMENT_COUNT 10
#define THRESHOLD 0.35

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
uint16_t white[12] = {1344, 8936, 6312, 5692, 1, 1, 6931, 5938, 4346, 2633, 17128, 936};

// Set vectors for each grain sample taken, which will the sample be compared to and classified as
// Sample 1: Corn
uint16_t corn[12] = {610, 2729, 2707, 3435, 0, 0, 5989, 6077, 4880, 2936, 11756, 890};
// Sample 2: Soybean
uint16_t soy[12] = {472, 2334, 2363, 2706, 0, 0, 4067, 3976, 3210, 2030, 8629, 672};
// Sample 3: Wheat
uint16_t wheat[12] = {548, 2920, 2706, 2915, 0, 0, 4261, 4202, 3351, 2114, 9582, 702};
// Sample 4: Ground Soybean
uint16_t gro_soy[12] = {569, 4468, 3699, 4606, 0, 0, 6892, 6324, 4673, 2667, 12885, 862};
// Sample 5: Ground Corn
uint16_t gro_corn[12] = {838, 4468, 3699, 4606, 0, 0, 6892, 6324, 4673, 2667, 12885, 862};

// Mixture Samples for Mixture Ratio Estimator Mode:
// Sample 1: 20% Ground Corn, 80% Ground Soybean
uint16_t gro_soy_20_gro_corn_80[12] = {580, 2909, 2815, 3312, 0, 0, 4999, 4895, 3867, 2341, 10113, 744};
// Sample 2: 50% Ground Corn, 50% Ground Soybean
uint16_t gro_soy_50_gro_corn_50[12] = {692, 3584, 3219, 3828, 0, 0, 5694, 5452, 4188, 2517, 11429, 804};
// Sample 3: 70% Ground Corn, 30% Ground Soybean
uint16_t gro_soy_70_gro_corn_30[12] = {714, 3608, 3273, 3929, 0, 0, 5990, 5762, 4373, 2670, 12009, 858};
// Sample 4: 90% Ground Corn, 10% Ground Soybean
uint16_t gro_soy_90_gro_corn_10[12] = {753, 3869, 3363, 4193, 0, 0, 6345, 6056, 4541, 2671, 12667, 888};
// Sample 5: 100% Ground Corn
uint16_t gro_corn_100[12] = {838, 4468, 3699, 4606, 0, 0, 6892, 6324, 4673, 2667, 12885, 862};
// Sample 6: 100% Ground Soybean
uint16_t gro_soy_100[12] = {569, 4468, 3699, 4606, 0, 0, 6892, 6324, 4673, 2667, 12885, 862};


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

    double sumDistancesGroSoy20GroCorn80 = 0.0;
    double sumDistancesGroSoy50GroCorn50 = 0.0;
    double sumDistancesGroSoy70GroCorn30 = 0.0;
    double sumDistancesGroSoy90GroCorn10 = 0.0;
    double sumDistancesGroCorn100 = 0.0;
    double sumDistancesGroSoy100 = 0.0;

    double normalizedGroSoy20GroCorn80[12];
    double normalizedGroSoy50GroCorn50[12];
    double normalizedGroSoy70GroCorn30[12];
    double normalizedGroSoy90GroCorn10[12];
    double normalizedGroCorn100[12];
    double normalizedGroSoy100[12];

    for (int i = 0; i < 12; ++i) {
        normalizedGroSoy20GroCorn80[i] = static_cast<double>(gro_soy_20_gro_corn_80[i]) / white[i];
        normalizedGroSoy50GroCorn50[i] = static_cast<double>(gro_soy_50_gro_corn_50[i]) / white[i];
        normalizedGroSoy70GroCorn30[i] = static_cast<double>(gro_soy_70_gro_corn_30[i]) / white[i];
        normalizedGroSoy90GroCorn10[i] = static_cast<double>(gro_soy_90_gro_corn_10[i]) / white[i];
        normalizedGroCorn100[i] = static_cast<double>(gro_corn_100[i]) / white[i];
        normalizedGroSoy100[i] = static_cast<double>(gro_soy_100[i]) / white[i];
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

        double distanceGroSoy20GroCorn80 = euclideanDistance(normalizedReadings, normalizedGroSoy20GroCorn80, 12);
        double distanceGroSoy50GroCorn50 = euclideanDistance(normalizedReadings, normalizedGroSoy50GroCorn50, 12);
        double distanceGroSoy70GroCorn30 = euclideanDistance(normalizedReadings, normalizedGroSoy70GroCorn30, 12);
        double distanceGroSoy90GroCorn10 = euclideanDistance(normalizedReadings, normalizedGroSoy90GroCorn10, 12);
        double distanceGroCorn100 = euclideanDistance(normalizedReadings, normalizedGroCorn100, 12);
        double distanceGroSoy100 = euclideanDistance(normalizedReadings, normalizedGroSoy100, 12);

        sumDistancesGroSoy20GroCorn80 += distanceGroSoy20GroCorn80;
        sumDistancesGroSoy50GroCorn50 += distanceGroSoy50GroCorn50;
        sumDistancesGroSoy70GroCorn30 += distanceGroSoy70GroCorn30;
        sumDistancesGroSoy90GroCorn10 += distanceGroSoy90GroCorn10;
        sumDistancesGroCorn100 += distanceGroCorn100;
        sumDistancesGroSoy100 += distanceGroSoy100;

        delay(20); // Delay between measurements
    }

    double averageDistanceGroSoy20GroCorn80 = sumDistancesGroSoy20GroCorn80 / MEASUREMENT_COUNT;
    double averageDistanceGroSoy50GroCorn50 = sumDistancesGroSoy50GroCorn50 / MEASUREMENT_COUNT;
    double averageDistanceGroSoy70GroCorn30 = sumDistancesGroSoy70GroCorn30 / MEASUREMENT_COUNT;
    double averageDistanceGroSoy90GroCorn10 = sumDistancesGroSoy90GroCorn10 / MEASUREMENT_COUNT;
    double averageDistanceGroCorn100 = sumDistancesGroCorn100 / MEASUREMENT_COUNT;
    double averageDistanceGroSoy100 = sumDistancesGroSoy100 / MEASUREMENT_COUNT;

    lcd.clear();

    double distances[6] = {
        averageDistanceGroSoy20GroCorn80,
        averageDistanceGroSoy50GroCorn50,
        averageDistanceGroSoy70GroCorn30,
        averageDistanceGroSoy90GroCorn10,
        averageDistanceGroCorn100,
        averageDistanceGroSoy100
    };

    double minDist = distances[0];
    int index = 0;

    for (int i = 1; i < 6; ++i) {
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
                lcd.print("20% Ground Corn");
                break;
            case 1:
                lcd.print("50% Ground Corn");
                break;
            case 2:
                lcd.print("70% Ground Corn");
                break;
            case 3:
                lcd.print("90% Ground Corn");
                break;
            case 4:
                lcd.print("100% Ground Corn");
                break;
            case 5:
                lcd.print("100% Ground Soy");
                break;
        }
    } else {
        lcd.print("Sample: Unknown");
        lcd.setCursor(0, 1);
        lcd.print("     Retry");
    }
    

    // Scan again after the sensor detects a drop in the clear channel
    while (true) {
        if (!isScanTriggered()) {
            break;
        }
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

     // Scan again after the sensor detects a drop in the clear channel
    while (true) {
        if (!isScanTriggered()) {
            break;
        }
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