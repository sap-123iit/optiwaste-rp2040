#include <Wire.h>
#include <SparkFun_VL53L5CX_Library.h>

// -------------------- VL53L5CX (ToF) ----------------------
SparkFun_VL53L5CX myImager;
VL53L5CX_ResultsData measurementData; // ~1.3KB

#define OUT Serial

int imageResolution = 0;
int imageWidth      = 0;

#define LED2    4   // LED
#define BUZZER  6   // Buzzer (HIGH = ON)

// -------------------- Timing ------------------------------
unsigned long lastPrintMs = 0;
const unsigned long printIntervalMs = 250; // 4 Hz

// -------------------- Helpers -----------------------------
void printToFGrid() {
  // Pretty print with increasing y, decreasing x
  OUT.println(F("TOF(mm) 8x8:"));
  for (int y = 0; y <= imageWidth * (imageWidth - 1); y += imageWidth) {
    for (int x = imageWidth - 1; x >= 0; x--) {
      OUT.print(measurementData.distance_mm[x + y]);
      if (x) OUT.print('\t');
    }
    OUT.println();
  }
  OUT.println();
}

void handleSerialCommands() {
  while (Serial.available() > 0) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'a':   // LED ON + short beep
        digitalWrite(LED2, HIGH);
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
        break;

      case 'b':   // LED ON, buzzer OFF
        digitalWrite(LED2, HIGH);
        digitalWrite(BUZZER, LOW);
        break;

      case 'd':   // LED OFF, buzzer OFF
        digitalWrite(LED2, LOW);
        digitalWrite(BUZZER, LOW);
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
  // ---- Handle serial commands for LED/Buzzer ----
  handleSerialCommands();

  // ---- ToF: print grid each time a new frame is ready at ~4 Hz ----
  unsigned long now = millis();
  if (now - lastPrintMs >= printIntervalMs) {
    lastPrintMs = now;
    if (myImager.isDataReady()) {
      if (myImager.getRangingData(&measurementData)) {
        printToFGrid();
      }
    }
  }

  // Small delay to prevent tight looping
  delay(5);
}