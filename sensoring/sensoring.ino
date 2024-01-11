// GrainTeller 1.0
// Made by Marcelo Gatica Ruedlinger
// Electrical Engineering Student at Universidad de Chile

#include <Wire.h>
#include <Adafruit_AS7341.h>
#include <LiquidCrystal_I2C.h>

#define MEASUREMENT_COUNT 10
#define THRESHOLD 0.15

LiquidCrystal_I2C lcd(0x3F, 16, 2);  // Set the LCD address to 0x3F
Adafruit_AS7341 sensor;

double euclideanDistance(double *reading1, double *reading2, uint8_t size) {
    double sum = 0.0;

    for (uint8_t i = 0; i < size; i++) {
        if (i == 4 || i == 5) continue; // Skip indices 4, 5, since those are duplicates
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

double normalizedDistanceGroSoyToGroWheat;


// Vector for white light and no grain sample
uint16_t white[12] = {1781, 12097, 7831, 7244, 1, 1, 8948, 7583, 5667, 3356, 23032, 1186};

// Set vectors for each grain sample taken, which will the sample be compared to and classified as
// Sample 1: Corn
uint16_t corn[12] = {787, 3775, 3307, 4535, 0, 0, 8159, 8154, 6458, 3824, 15420, 1042};
// Sample 2: Soybean
uint16_t soy[12] = {709, 3750, 3190, 3730, 0, 0, 5522, 5273, 4180, 2673, 12523, 856};
// Sample 3: Wheat
uint16_t wheat[12] = {733, 4109, 3470, 3869, 0, 0, 5852, 5675, 4566, 2900, 13113, 896};
// Sample 4: Ground Soybean
uint16_t gro_soy[12] = {1106, 6061, 5501, 6390, 0, 0, 9581, 9189, 7199, 4387, 18791, 1188};
// Sample 5: Ground Wheat
uint16_t gro_wheat[12] = {1311, 7414, 5625, 7027, 0, 0, 10600, 9603, 7051, 4058, 21321, 1274};

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

}

void loop() {
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
    double sumDistancesGroWheat = 0.0;

    double normalizedGroSoy[12];
    double normalizedGroWheat[12];
    double normalizedCorn[12];
    double normalizedSoy[12];
    double normalizedWheat[12];

    for (int i = 0; i < 12; ++i) {
        normalizedGroSoy[i] = static_cast<double>(gro_soy[i]) / white[i];
        normalizedGroWheat[i] = static_cast<double>(gro_wheat[i]) / white[i];
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
        double distanceGroWheat = euclideanDistance(normalizedReadings, normalizedGroWheat, 12);

        sumDistancesCorn += distanceCorn;
        sumDistancesSoy += distanceSoy;
        sumDistancesWheat += distanceWheat;
        sumDistancesGroSoy += distanceGroSoy;
        sumDistancesGroWheat += distanceGroWheat;

        delay(20); // Delay between measurements
    }

    double averageDistanceCorn = sumDistancesCorn / MEASUREMENT_COUNT;
    double averageDistanceSoy = sumDistancesSoy / MEASUREMENT_COUNT;
    double averageDistanceWheat = sumDistancesWheat / MEASUREMENT_COUNT;
    double averageDistanceGroSoy = sumDistancesGroSoy / MEASUREMENT_COUNT;
    double averageDistanceGroWheat = sumDistancesGroWheat / MEASUREMENT_COUNT;

    lcd.clear();

    double distances[5] = {
        averageDistanceCorn,
        averageDistanceSoy,
        averageDistanceWheat,
        averageDistanceGroSoy,
        averageDistanceGroWheat
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
                lcd.print("     Wheat");
                break;
            case 3:
                lcd.print("  Ground Soy");
                break;
            case 4:
                lcd.print("  Ground Wheat");
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