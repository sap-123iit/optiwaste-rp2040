#include <Wire.h> 
#include <SparkFun_VL53L5CX_Library.h>
#include "HX711.h"

// -------------------- HX711 (Load Cell) --------------------
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN  = 3;

HX711 scale;

#define LED2    4   // LED
#define BUZZER  6   // Buzzer (HIGH = ON)

// -------------------- VL53L5CX (ToF) ----------------------
SparkFun_VL53L5CX myImager;
VL53L5CX_ResultsData measurementData; // ~1.3KB

#define OUT Serial

int imageResolution = 0;
int imageWidth      = 0;
int buzzerState     = 0;  // 1 if ToF condition met, 0 otherwise

// -------------------- ToF Trigger Logic --------------------
#define TOF_THRESHOLD_MIN 150  // 300 - 50 mm
#define TOF_THRESHOLD_MAX 450  // 300 + 50 mm

// Convert 1-based (row,col) to 0-based array index for 8x8 grid
int getToFIndex(int row, int col) {
  return (row - 1) * 8 + (col - 1);
}

bool checkToFCondition() {
  // Positions inside the rectangle (row, col)
  int positions[][2] = {
    {6, 2}, {6, 3}, {6, 4}, {6, 5}, {6, 6},  // Row 6
    {7, 2}, {7, 3}, {7, 4}, {7, 5}, {7, 6}   // Row 7
  };

  int withinRangeCount = 0;
  int totalPositions = sizeof(positions) / sizeof(positions[0]);

  for (int i = 0; i < totalPositions; i++) {
    int index = getToFIndex(positions[i][0], positions[i][1]);
    int distance = measurementData.distance_mm[index];
    if (distance >= TOF_THRESHOLD_MIN && distance <= TOF_THRESHOLD_MAX) {
      withinRangeCount++;
    }
  }

  // Example: at least 20% of positions in range → true
  return withinRangeCount >= (totalPositions * 0.2);
}


// -------------------- Helpers -----------------------------
void handleSerialCommands() {
  while (Serial.available() > 0) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'a':   // LED ON + short beep
        digitalWrite(LED2, HIGH);
        digitalWrite(BUZZER, HIGH); // Pin 6
        delay(200);
        digitalWrite(BUZZER, LOW);
        scale.tare(10);

        break;

      case 'b':   // LED ON, buzzer OFF
        digitalWrite(LED2, HIGH);
        digitalWrite(BUZZER, LOW);
        break;

      case 'd':   // LED OFF, buzzer OFF
        scale.tare(10);
        digitalWrite(LED2, LOW);
        digitalWrite(BUZZER, LOW);
        break;

      case 't':   // Tare scale
        scale.tare(10);
       
        break;

      default:
        break;
    }
  }
}

void setup() {
  // ----- Serial -----
  Serial.begin(115200);
  while (!Serial) {}

  // ----- GPIO -----
  pinMode(LED2, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(LED2, LOW);
  digitalWrite(BUZZER, LOW);  // LOW = off, HIGH = on

  // ----- HX711 -----
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(12.95f);  // Calibration factor
 delay(400);
  scale.tare(10);           // Zero the scale
  delay(10);
  // ----- I2C (VL53L5CX) -----
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();
  Wire.setClock(400000); // 400kHz

  OUT.println(F("Initializing VL53L5CX... (can take several seconds)"));
  if (!myImager.begin()) {
    OUT.println(F("Sensor not found. Check wiring (SDA=GP0, SCL=GP1) and PWREN."));
    while (1) { delay(1000); }
  }

  myImager.setResolution(8 * 8);              // 64 zones
  imageResolution = myImager.getResolution();  // 64 or 16
  imageWidth = (imageResolution == 64) ? 8 : 4;

  myImager.startRanging();


}



void loop() {
  // ---- Handle serial commands for LED/Buzzer/Tare ----
  handleSerialCommands();

  // ---- HX711: read & print weight with buzzer state ----
  float weight_kg = scale.get_units(5) / 1000.0f;
  OUT.print(F("Weight: "));
  OUT.print(weight_kg, 2);
  OUT.print(F(", "));
  OUT.print(buzzerState);
  OUT.println();

  // Power cycle HX711 like old.ino
//  scale.power_down(); 
//  delay(4);
//  scale.power_up();

  // ---- ToF: collect data, set buzzerState if needed ----
  if (myImager.isDataReady()) {
    if (myImager.getRangingData(&measurementData)) {
      buzzerState = checkToFCondition() ? 1 : 0;
    }
  }

  // Small delay to prevent tight looping
  //delay(5);
}