#include <Wire.h>
#include <Adafruit_AS7341.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2);  // Set the LCD address to 0x3F
Adafruit_AS7341 sensor;

double euclideanDistance(double *reading1, double *reading2, uint8_t size) {
    double sum = 0.0;

    for (uint8_t i = 0; i < size; i++) {
        if (i == 4 || i == 5 || i == 12) continue; // Skip indices 4, 5, and 12.
        double diff = reading1[i] - reading2[i];
        sum += diff * diff;
    }

    return sqrt(sum);
}

bool isScanTriggered() {
    uint16_t readings[12];
    sensor.readAllChannels(readings);

    return readings[10] < 40000;
}
// Vector for white light and no grain sample
uint16_t white[12] = {2962, 19474, 12672, 14186, 1, 1, 18389, 17509, 15542, 10570, 49024, 2990};

// Set vectors for each grain sample taken, which will the sample be compared to and classified as
// Sample 1: Corn
uint16_t corn[12] = {1189, 4407, 4275, 6414, 0, 0, 12080, 13469, 12684, 8721, 28080, 2260};
// Sample 2: Soybean
uint16_t soy[12] = {919, 4376, 4052, 5093, 0, 0, 7843, 8124, 7749, 5566, 20211, 1688};
// Sample 3: Wheat
uint16_t wheat[12] = {1029, 5082, 4332, 5386, 0, 0, 8308, 8875, 8653, 6217, 22494, 1832};

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

  sensor.setLEDCurrent(40);    // Set LED's intensity
  sensor.enableLED(true);       // Turn on the LED's
}

void loop() {
    while (!isScanTriggered()) {
        lcd.setCursor(0, 0);
        lcd.print("  == Insert ==  ");
        lcd.setCursor(0, 1);
        lcd.print("  == Sample ==  ");
        delay(500); // Adjust the delay as needed for scanning frequency
    }
  
    // Read all channels at the same time and store in as7341 object
    uint16_t readings[12];

    if (!sensor.readAllChannels(readings)){
        lcd.print("Error reading");
        lcd.print("all channels!");
        return;
    }

    double normalizedReadings[12];
    for (int i = 0; i < 12; ++i) {
        normalizedReadings[i] = static_cast<double>(readings[i]) / white[i];
    }

    double normalizedCorn[12];
    double normalizedSoy[12];
    double normalizedWheat[12];
    for (int i = 0; i < 12; ++i) {
        normalizedCorn[i] = static_cast<double>(corn[i]) / white[i];
        normalizedSoy[i] = static_cast<double>(soy[i]) / white[i];
        normalizedWheat[i] = static_cast<double>(wheat[i]) / white[i];
    }

    double distanceCorn = euclideanDistance(normalizedReadings, normalizedCorn, 12);   // Calculate Euclidean distance to corn
    double distanceSoy = euclideanDistance(normalizedReadings, normalizedSoy, 12);     // Calculate Euclidean distance to soy
    double distanceWheat = euclideanDistance(normalizedReadings, normalizedWheat, 12); // Calculate Euclidean distance to wheat
    
    // Determine the closest match based on normalized distances
    if (distanceCorn < distanceSoy && distanceCorn < distanceWheat && distanceCorn < 0.1) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sample: Corn");
        lcd.setCursor(0, 1);
        lcd.print("Distance: ");
        lcd.print(distanceCorn);
    } else if (distanceSoy < distanceWheat && distanceSoy < 0.1) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sample: Soybean");
        lcd.setCursor(0, 1);
        lcd.print("Distance: ");
        lcd.print(distanceSoy);
    } else if (distanceWheat < 0.1) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sample: Wheat");
        lcd.setCursor(0, 1);
        lcd.print("Distance: ");
        lcd.print(distanceWheat);
    } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Unknown sample");
        lcd.setCursor(0, 1);
        lcd.print("Please try again");
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

  /* 
  // we skip the first set of duplicate clear/NIR readings
  Serial.print("ADC4/Clear-");
  Serial.println(readings[4]);
  Serial.print("ADC5/NIR-");
  Serial.println(readings[5]);
  */
  
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