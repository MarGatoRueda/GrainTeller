#include <Wire.h>
#include <Adafruit_AS7341.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2);  // Set the LCD address to 0x3F for a 16 chars and 2 line display
Adafruit_AS7341 sensor;

void setup() {
  Wire.begin();
  lcd.init();                      // Initialize the LCD
  lcd.backlight();                 // Turn on the backlight
  // Turn on the sensor's LEDs
  sensor.setLEDCurrent(100);
  sensor.enableLED(true);

  if (!sensor.begin()) {
    lcd.setCursor(0, 0);
    lcd.print("AS7341 not found");
    while (1);
  }
    // Wait for communication with the host computer serial monitor
  while (!Serial) {
    delay(1);
  }
  
  if (!sensor.begin()){
    Serial.println("Could not find AS7341");
    while (1) { delay(10); }
  }

  sensor.setATIME(100);
  sensor.setASTEP(999);
  sensor.setGain(AS7341_GAIN_256X);
}

void loop() {
  // Read all channels at the same time and store in as7341 object
  if (!sensor.readAllChannels()){
    lcd.print("Error reading");
    lcd.print("all channels!");
    return;
  }
  sensor.setLEDCurrent(30);    // Set LED's intensity
  sensor.enableLED(true);       // Turn on the LED's

// Get values of different channels
  float channel415 = sensor.getChannel(AS7341_CHANNEL_415nm_F1);
  float channel445 = sensor.getChannel(AS7341_CHANNEL_445nm_F2);
  float channel480 = sensor.getChannel(AS7341_CHANNEL_480nm_F3);
  float channel515 = sensor.getChannel(AS7341_CHANNEL_515nm_F4);
  float channel555 = sensor.getChannel(AS7341_CHANNEL_555nm_F5);
  float channel590 = sensor.getChannel(AS7341_CHANNEL_590nm_F6);
  float channel630 = sensor.getChannel(AS7341_CHANNEL_630nm_F7);
  float channel680 = sensor.getChannel(AS7341_CHANNEL_680nm_F8);

  // Find the channel with the highest value
  float maxValue = max(max(max(max(max(max(channel415, channel445), channel480), channel515), channel555), channel590), channel630);

  // Display the color corresponding to the channel with the highest value on the LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  if (maxValue == channel415) {
    lcd.print("Color: 415nm");
  } else if (maxValue == channel445) {
    lcd.print("Color: 445nm");
  } else if (maxValue == channel480) {
    lcd.print("Color: 480nm");
  } else if (maxValue == channel515) {
    lcd.print("Color: 515nm");
  } else if (maxValue == channel555) {
    lcd.print("Color: 555nm");
  } else if (maxValue == channel590) {
    lcd.print("Color: 590nm");
  } else if (maxValue == channel630) {
    lcd.print("Color: 630nm");
  }

  delay(1000); // Adjust delay as needed
}