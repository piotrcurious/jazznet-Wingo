#include "jazz_logic.h"

// For ESP32, we often use a different MIDI library or setup.
// Here we assume a compatible MIDI interface is available.
#include <HardwareSerial.h>
HardwareSerial MIDI_Serial(2); // Use UART2 for MIDI
#include <MIDI.h>
#include <SPI.h>
#include <SD.h>
#include <esp_now.h>
#include <WiFi.h>
struct SerialMIDI {
  typedef HardwareSerial SerialType;
  SerialMIDI(SerialType& s) : serial(s) {}
  void begin() { serial.begin(31250, SERIAL_8N1, 16, 17); } // RX=16, TX=17
  void write(uint8_t b) { serial.write(b); }
  bool read(uint8_t* b) { if (serial.available()) { *b = serial.read(); return true; } return false; }
  SerialType& serial;
};
MIDI_CREATE_INSTANCE(HardwareSerial, MIDI_Serial, MIDI);

const int ANALOG_PIN_ERROR = 34;
const int ANALOG_PIN_BASE_FREQ = 35;
const int ANALOG_PIN_SPEED = 32;
const int ANALOG_PIN_THROTTLE = 33;
const int ANALOG_PIN_BRAKE = 25;
const int LED_PIN = 2; // Onboard LED for many ESP32 dev boards

// ESP-NOW structure
typedef struct struct_message {
    int currentChordIdx;
    int intensity;
} struct_message;

struct_message myData;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(struct_message)) {
        struct_message* msg = (struct_message*)incomingData;
        updateEnsemblePeer((uint8_t*)mac, msg->currentChordIdx, msg->intensity);
    }
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  }

  // Register peer (broadcast)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  for(int i=0; i<6; i++) peerInfo.peer_addr[i] = 0xFF;
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
  }

  esp_now_register_recv_cb(OnDataRecv);

  // MIDI.begin(MIDI_CHANNEL_OMNI);
  pinMode(LED_PIN, OUTPUT);
  Serial.println("ESP32 Setup complete. Starting sophisticated MIDI generation...");

  // Set ADC resolution to 12-bit
  analogReadResolution(12);

  // Initialize SD Card
  if (!SD.begin(5)) { // CS pin 5
      Serial.println("SD Card initialization failed!");
  } else {
      Serial.println("SD Card initialized.");
  }
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

  // GPS integration (dummy for now, would use Serial1/Serial2)
  int heading = 0;
  int altitude = 0;
  int satellites = 10;
  double latitude = 0.0;
  double longitude = 0.0;

  EVContext context = {currentError, speed, throttle, brake, heading, altitude, satellites, latitude, longitude};

  // Update own state for broadcast
  myData.currentChordIdx = getCurrentChordIdx();
  myData.intensity = throttle;
  esp_now_send(NULL, (uint8_t *) &myData, sizeof(myData));

  Serial.print("Base Note: "); Serial.println(baseNote);
  Serial.print("E: "); Serial.print(currentError);
  Serial.print(" S: "); Serial.print(speed);
  Serial.print(" T: "); Serial.print(throttle);
  Serial.print(" B: "); Serial.println(brake);

  playChordProgression(context, baseNote);

  int predictedError = predictError(currentError);
  Serial.print("Pred E: "); Serial.println(predictedError);

  delay(10); // Faster loop possible on ESP32
}
