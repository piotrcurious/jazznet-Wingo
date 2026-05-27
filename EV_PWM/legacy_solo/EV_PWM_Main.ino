#include "jazz_logic.h"

// MIDI instance is created here and used by jazz_logic.cpp
MIDI_CREATE_DEFAULT_INSTANCE();

const int ANALOG_PIN_ERROR = A0;
const int ANALOG_PIN_BASE_FREQ = A1;
const int ANALOG_PIN_SPEED = A2;
const int ANALOG_PIN_THROTTLE = A3;
const int ANALOG_PIN_BRAKE = A4;
const int LED_PIN = 13;

void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  Serial.println("Setup complete. Starting MIDI generation...");
}

void loop() {
  int sensorValue = analogRead(ANALOG_PIN_ERROR);
  int currentError = map(sensorValue, 0, ADC_RESOLUTION, 0, 127);

  int rawBaseFrequency = analogRead(ANALOG_PIN_BASE_FREQ);
  int baseNote = map(rawBaseFrequency, 0, ADC_RESOLUTION, 48, 72);

  int rawSpeed = analogRead(ANALOG_PIN_SPEED);
  int speed = map(rawSpeed, 0, ADC_RESOLUTION, 0, 127);

  int rawThrottle = analogRead(ANALOG_PIN_THROTTLE);
  int throttle = map(rawThrottle, 0, ADC_RESOLUTION, 0, 127);

  int rawBrake = analogRead(ANALOG_PIN_BRAKE);
  int brake = map(rawBrake, 0, ADC_RESOLUTION, 0, 127);

  // For real GPS integration, one would use a library like TinyGPS++
  // and read from a Serial GPS module.
  // Here we use default/dummy values or could read more analog pins for demo.
  int heading = 0;
  int altitude = 0;
  int satellites = 10;
  double latitude = 0.0;
  double longitude = 0.0;

  EVContext context = {currentError, speed, throttle, brake, heading, altitude, satellites, latitude, longitude};

  Serial.print("Base Note (Tonic): ");
  Serial.println(baseNote);
  Serial.print("Error: "); Serial.print(currentError);
  Serial.print(" Speed: "); Serial.print(speed);
  Serial.print(" Throttle: "); Serial.print(throttle);
  Serial.print(" Brake: "); Serial.println(brake);

  playChordProgression(context, baseNote);

  int predictedError = predictError(currentError);
  Serial.print("Predicted Error: ");
  Serial.println(predictedError);

  delay(500);
}
