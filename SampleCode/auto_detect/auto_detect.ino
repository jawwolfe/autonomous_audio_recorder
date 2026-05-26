#include <SD.h>
#include <driver/i2s.h>

#define I2S_WS 25
#define I2S_SCK 33
#define I2S_SD 32
#define PIN_SPI_CS 5

// Recording constraints
const int recordingTimeLimit = 30000; // 30 seconds limit
const float soundThresholdMultiplier = 1.5; // Starts recording if 1.5x louder than quiet
const int silenceTimeout = 5000; // Stop if silent for 5 seconds

unsigned long recordingStartTime = 0;
unsigned long lastSoundTime = 0;
bool isRecording = false;
unsigned long recordingStartTime = 0;
unsigned long lastSoundTime = 0;


void setup() {
  Serial.begin(115200);
  // Initialize SD and I2S here...
  calibrateNoiseFloor();
}

void loop() {
  float currentVolume = readMicrophoneVolume();
  
  if (!isRecording) {
    if (currentVolume > (baselineNoise * soundThresholdMultiplier)) {
      startRecording();
    }
  } else {
    // We are recording, check stop conditions
    unsigned long elapsed = millis() - recordingStartTime;
    float silenceDuration = millis() - lastSoundTime;

    // Check time limit OR silence limit
    if (elapsed >= recordingTimeLimit || silenceDuration >= silenceTimeout) {
      stopRecording();
    } else {
      appendAudioToSD(); // Read from I2S and write to WAV
      if (currentVolume > (baselineNoise * soundThresholdMultiplier)) {
        lastSoundTime = millis(); // Reset silence timer if sound continues
      }
    }
  }
}

void calibrateNoiseFloor() {
  float sum = 0;
  for (int i = 0; i < 100; i++) {
    sum += readMicrophoneVolume();
    delay(10);
  }
  baselineNoise = sum / 100.0;
}

// Helper to calculate RMS or Peak volume from raw PCM data
float readMicrophoneVolume() {
  int32_t raw_samples[512];
  size_t bytes_read;
  i2s_read(I2S_NUM_0, (void**)raw_samples, sizeof(raw_samples), &bytes_read, portMAX_DELAY);
  
  // Calculate RMS
  float sum_squares = 0;
  int samples_count = bytes_read / sizeof(int32_t);
  for (int i = 0; i < samples_count; i++) {
    float sample = (float)raw_samples[i];
    sum_squares += sample * sample;
  }
  return sqrt(sum_squares / samples_count);
}