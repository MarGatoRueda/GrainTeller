// GrainTeller 1.0
// Hecho por Marcelo Gatica Ruedlinger en el MWL
// Estudiante de Ingeniería Civil Eléctrica en la Universidad de Chile.

#include <Wire.h>
#include <Adafruit_AS7341.h>
#include <LiquidCrystal_I2C.h>

#define MEASUREMENT_COUNT 10    // Número de mediciones que se promedian para obtener un resultado más preciso
#define THRESHOLD 0.22        // Umbral de distancia para clasificar una muestra como desconocida, ajustar según sea necesario

LiquidCrystal_I2C lcd(0x3F, 16, 2);
Adafruit_AS7341 sensor;

// Funciones auxiliares para el funcionamiento del sensor.
template <typename T>
double euclideanDistance(T *reading1, T *reading2, uint8_t size) {
    double sum = 0.0;

    for (uint8_t i = 0; i < size; i++) {
        if (i == 4 || i == 5) continue;
        double diff = static_cast<double>(reading1[i]) - static_cast<double>(reading2[i]);
        sum += diff * diff;
    }

    return sqrt(sum);
}


template <>
double euclideanDistance(uint16_t *reading1, uint16_t *reading2, uint8_t size) {
    double sum = 0.0;

    for (uint8_t i = 0; i < size; i++) {
        if (i == 4 || i == 5) continue;
        double diff = static_cast<double>(reading1[i]) - static_cast<double>(reading2[i]);
        sum += diff * diff;
    }

    return sqrt(sum);
}


bool isScanTriggered() {
    uint16_t readings[12];
    sensor.readAllChannels(readings);

    return readings[10] < 10960;
}
bool isFirstTime = true;
bool switchedToMixtureRatioMode = false;

// Vector de luz blanca en el momento de toma de muestras para la base de datos.
uint16_t whiteDatabase[12] = {897, 5964, 4261, 4014, 1, 1, 4959, 4241, 3073, 1815, 11935, 670};

// Vector de luz blanca en el momento de calibración del sensor.
uint16_t whiteReading[12];

// Vectores de muestras puras para el modo de clasificación de granos:
// Muestra 1: Maíz Argentino
uint16_t corn[12] = {488, 2273, 2091, 2617, 0, 0, 4392, 4472, 3600, 2161, 8751, 674};
// Muestra 2: Soja Uruguaya
uint16_t soy[12] = {333, 1597, 1640, 1897, 0, 0, 2806, 2774, 2239, 1412, 6339, 498};
// Sample 3: Trigo
uint16_t wheat[12] = {379, 1942, 1843, 2018, 0, 0, 3015, 3003, 2444, 1544, 6991, 550};
// Sample 4: Soja Molida
uint16_t gro_soy[12] = {458, 2318, 2371, 2675, 0, 0, 3950, 3901, 3090, 1922, 8512, 612};
// Sample 5: Maíz Molido
uint16_t gro_corn[12] = {649, 3428, 2975, 3608, 0, 0, 5411, 5039, 3736, 2199, 11145, 738};

// Rutina de inicio del microcontrolador.
void setup() {
    Wire.begin();
    lcd.init();              
    lcd.backlight();                 
    Serial.begin(115200);

    while (!Serial) {
        delay(1);
    }

    if (!sensor.begin()) {
        lcd.setCursor(0, 0);
        lcd.print("AS7341 no detectado!");
        while (1);
    }

    sensor.setATIME(100);
    sensor.setASTEP(999);
    sensor.setGain(AS7341_GAIN_256X);

    sensor.setLEDCurrent(60);   
    sensor.enableLED(true); 
    lcd.setCursor(0, 0);
    lcd.print("GrainTeller 1.0");
    lcd.setCursor(0, 1);
    lcd.print("By: Marcelo G.R.");
    delay(3000);
    lcd.clear();

    pinMode(A0, INPUT);
    pinMode(A1, INPUT);

    // Obtener el vector de luz blanca para calibrar el sensor
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrando,");
    lcd.setCursor(0, 1);
    lcd.print("Un momento.");

    uint16_t whiteReadings[12];
    double sumWhiteReadings[12] = {0.0};

    for (int i = 0; i < MEASUREMENT_COUNT; ++i) {
        if (!sensor.readAllChannels(whiteReadings)) {
            lcd.clear();
            lcd.print("Error en leer");
            lcd.print("los canales!");
            return;
        }

        for (int j = 0; j < 12; ++j) {
            sumWhiteReadings[j] += whiteReadings[j];
        }

        delay(50);
    }

    for (int i = 0; i < 12; ++i) {
        whiteReading[i] = sumWhiteReadings[i] / MEASUREMENT_COUNT;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibracion");
    lcd.setCursor(0, 1);
    lcd.print("completada!");
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
        lcd.print("Cambiando a");
        lcd.setCursor(0, 1);
        lcd.print("Modo % Mezclas");
        delay(2000);
        isFirstTime = false;
        switchedToMixtureRatioMode = false;
    }

    while (!isScanTriggered()) {
        lcd.setCursor(0, 0);
        lcd.print("Favor insertar  ");
        lcd.setCursor(0, 1);
        lcd.print("Mezcla a medir  ");
        delay(500);
    }
    lcd.clear();
  
    // Cuenta regresiva para la muestra detectada
    for (int i = 3; i > 0; --i) {
        lcd.setCursor(0, 0);
        lcd.print("Mezcla detectada");
        lcd.setCursor(0, 1);
        lcd.print("escaneando en ");
        lcd.print(i);
        delay(1000);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Escaneando...");

    uint16_t readings[12];

    double sumDistancesGroSoy = 0.0;
    double sumDistancesGroCorn = 0.0;
    double normalizedGroSoy[12];
    double normalizedGroCorn[12];

    // Obtener los vectores normalizados de las muestras de soja y maíz molidos
    for (int i = 0; i < 12; ++i) {
        normalizedGroSoy[i] = static_cast<double>(gro_soy[i]) / whiteDatabase[i];
        normalizedGroCorn[i] = static_cast<double>(gro_corn[i]) / whiteDatabase[i];
    }

    for (int i = 0; i < MEASUREMENT_COUNT; ++i) {
        if (!sensor.readAllChannels(readings)) {
            lcd.clear();
            lcd.print("Error leyendo");
            lcd.print("los canales!");
            return;
        }

        double normalizedReadings[12];


        for (int j = 0; j < 12; ++j) {
            normalizedReadings[j] = static_cast<double>(readings[j]) / whiteReading[j];
        }
        double distanceGroSoy = euclideanDistance(normalizedReadings, normalizedGroSoy, 11);
        double distanceGroCorn = euclideanDistance(normalizedReadings, normalizedGroCorn, 11);

        sumDistancesGroSoy += distanceGroSoy;
        sumDistancesGroCorn += distanceGroCorn;

        delay(50);
    }

    double averageDistanceGroSoy = sumDistancesGroSoy / MEASUREMENT_COUNT;
    double averageDistanceGroCorn = sumDistancesGroCorn / MEASUREMENT_COUNT;

 
    double totalDistance = euclideanDistance(normalizedGroSoy, normalizedGroCorn, 11);

    double grade = 100 - ((averageDistanceGroSoy / totalDistance) * 100);

    if (grade < 0) {
        grade = 0;
    } else if (grade > 100) {
        grade = 100;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Soja Molida %");
    lcd.print(grade, 0);
    lcd.setCursor(0, 1);
    lcd.print("Maiz Molido %");
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
        lcd.print("Cambiando a");
        lcd.setCursor(0, 1);
        lcd.print("Modo Det. Granos");
        delay(3000);
        isFirstTime = false;
        switchedToMixtureRatioMode = false;
    }

    while (!isScanTriggered()) {
        lcd.setCursor(0, 0);
        lcd.print("Favor insertar  ");
        lcd.setCursor(0, 1);
        lcd.print("Grano a medir   ");
        delay(500);
    }
    lcd.clear();

    // Cuenta regresiva para la muestra detectada
    for (int i = 3; i > 0; --i) {
        lcd.setCursor(0, 0);
        lcd.print("Grano detectado,");
        lcd.setCursor(0, 1);
        lcd.print("escaneando en ");
        lcd.print(i);
        delay(1000);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Escaneando...");

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
            lcd.print("Error leyendo");
            lcd.print("los canales!");
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
        lcd.print("    Muestra: ");
        lcd.setCursor(0, 1);
        switch (index) {
            case 0:
                lcd.print(" Maiz Argentino");
                break;
            case 1:
                lcd.print(" Soja Uruguaya");
                break;
            case 2:
                lcd.print("     Trigo");
                break;
            case 3:
                lcd.print("  Soja Molida");
                break;
            case 4:
                lcd.print("  Maiz Molido");
                break;
        }
    } else {
        lcd.print("Muestra no");
        lcd.setCursor(0, 1);
        lcd.print("identificada");
    }

    Serial.print("Distancia minima: ");
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

     // Esperar a que el usuario retire la muestra
    while (true) {
        if (!isScanTriggered()) {
            break;
        }
    }
}

void loop() {

    // Revisa la posición del interruptor para cambiar de modo.
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