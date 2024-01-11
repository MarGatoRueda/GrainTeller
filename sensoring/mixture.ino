#include <Wire.h>
#include <Adafruit_AS7341.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2);  // Set the LCD address to 0x3F
Adafruit_AS7341 sensor;

#define MEASUREMENT_COUNT 10
#define THRESHOLD 0.15

double minDistance = 1000000.0;

double euclideanDistance(double *reading1, double *reading2, uint8_t size) {
    double sum = 0.0;

    for (uint8_t i = 0; i < size; i++) {
        if (i == 4 || i == 5) continue; // Skip indices 4, 5, and 12.
        double diff = reading1[i] - reading2[i];
        sum += diff * diff;
    }

    return sqrt(sum);
}

bool isScanTriggered() {
    uint16_t readings[12];
    sensor.readAllChannels(readings);

    return readings[10] > 5000;
}

// Vector for white light and no grain sample
uint16_t white[12] = {1781, 12097, 7831, 7244, 1, 1, 8948, 7583, 5667, 3356, 23032, 1186};

// Set vectors for each grain sample taken, which will be compared to and classified as
// Sample 4: Ground Soybean
uint16_t gro_soy[12] = {1106, 6061, 5501, 6390, 0, 0, 9581, 9189, 7199, 4387, 18791, 1188};
// Sample 5: Ground Wheat
uint16_t gro_wheat[12] = {1311, 7414, 5625, 7027, 0, 0, 10600, 9603, 7051, 4058, 21321, 1274};

double normalizedDistanceGroSoyToGroWheat;
double normalizedGroSoy[12];
double normalizedGroWheat[12];


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

    // Define normalized vectors for ground soy and ground wheat
    for (int i = 0; i < 12; ++i) {
        normalizedGroSoy[i] = static_cast<double>(gro_soy[i]) / white[i];
        normalizedGroWheat[i] = static_cast<double>(gro_wheat[i]) / white[i];
    }

    // Calculate and store the normalized distance between ground soy and ground wheat
    normalizedDistanceGroSoyToGroWheat = euclideanDistance(normalizedGroSoy, normalizedGroWheat, 12);
}

void loop() {
    while (!isScanTriggered()) {
        lcd.setCursor(0, 0);
        lcd.print("  == Insert ==  ");
        lcd.setCursor(0, 1);
        lcd.print("  == Sample ==  ");
        delay(500); // Adjust the delay as needed for scanning frequency
    }

    // Sample detected countdown
    for (int i = 3; i > 0; --i) {
        lcd.clear();
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
    double sumDistancesGroWheat = 0.0;

    double normalizedGroSoy[12];
    double normalizedGroWheat[12];

    for (int i = 0; i < 12; ++i) {
        normalizedGroSoy[i] = static_cast<double>(gro_soy[i]) / white[i];
        normalizedGroWheat[i] = static_cast<double>(gro_wheat[i]) / white[i];
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

        double distanceGroSoy = euclideanDistance(normalizedReadings, normalizedGroSoy, 12);
        double distanceGroWheat = euclideanDistance(normalizedReadings, normalizedGroWheat, 12);

        if (distanceGroSoy < minDistance) {
            minDistance = distanceGroSoy;
        }

        if (distanceGroWheat < minDistance) {
            minDistance = distanceGroWheat;
        }

        sumDistancesGroSoy += distanceGroSoy;
        sumDistancesGroWheat += distanceGroWheat;

        delay(20); // Adjust delay between measurements as needed
    }

    double averageDistanceGroSoy = sumDistancesGroSoy / MEASUREMENT_COUNT;
    double averageDistanceGroWheat = sumDistancesGroWheat / MEASUREMENT_COUNT;

    // Calculate percentage of ground soy and ground wheat in the mixture
    double percentageGroWheat = (averageDistanceGroSoy / normalizedDistanceGroSoyToGroWheat) * 100;
    double percentageGroSoy = 100 - percentageGroWheat;

    lcd.setCursor(0, 0);
    lcd.print("GroSoy %: ");
    lcd.print(percentageGroSoy, 2); // Print with 2 decimal places

    lcd.setCursor(0, 1);
    lcd.print("GroWheat %: ");
    lcd.print(percentageGroWheat, 2); // Print with 2 decimal places

    delay(10000); // Adjust delay before next scan
}
