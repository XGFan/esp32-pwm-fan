#include <Arduino.h>

// the number of the LED pin
const int PWM_PIN = 12;
const int BTN = 9;
const int TACH_PIN = 18;

// setting PWM properties
const int freq = 25000;
const int ledChannel = 1;
const int resolution = 8;

int level = 3;

// Define variables for storing the last tachometer value and the current tachometer value
volatile unsigned long lastTach = 0;
volatile unsigned long currentTach = 0;
volatile unsigned long lastRPM = 0;
volatile unsigned long currentRPM = 0;

// Define a variable for storing the time of the last tachometer reading
volatile unsigned long lastTachTime = 0;

void IRAM_ATTR tachISR()
{
  currentTach++;
  long currentTime = millis();
  long deltaTime = currentTime - lastTachTime;
  if (deltaTime >= 1000)
  {
    currentRPM = currentTach * 30;
    lastTach = currentTach;
    currentTach = 0;
    lastTachTime = currentTime;
  }
}

void printRPMIfChanged()
{
  if ((currentRPM > lastRPM && currentRPM - lastRPM > 30) || (currentRPM < lastRPM && lastRPM - currentRPM > 30))
  {
    Serial.printf("Fan RPM: %d\n", currentRPM);
    lastRPM = currentRPM;
  }
}

void setup()
{
  Serial.begin(115200); /* prepare for possible serial debug */

  pinMode(BTN, INPUT);

  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(PWM_PIN, ledChannel);
  ledcWrite(ledChannel, 0);

  attachInterrupt(digitalPinToInterrupt(TACH_PIN), tachISR, RISING);
}

void loop()
{
  int value = digitalRead(BTN);
  if (value == LOW)
  {
    level += 1;
    if (level > 8)
    {
      level -= 8;
    }
    ledcWrite(ledChannel, level * 32 - 1);
    Serial.printf("Fan level: %d, %d\n", level * 32 - 1, level);
    delay(300);
  }
  printRPMIfChanged();
}

// 256
// 224
// 192
// 160
// 128
// 96
// 64
// 32
// 0
