#include "BluetoothA2DPSource.h"
#include "LittleFS.h"
#include <WiFi.h>

BluetoothA2DPSource a2dp_source;
File audioFile;
const int buttonPin = 5;
bool isPlaying = false;
bool lastButtonState;

int32_t get_audio_data(Frame *data, int32_t len) {
    if (!isPlaying || !audioFile) return 0;

    int samplesToRead = len / 2;

    if (samplesToRead <= 0) return 0;

    uint8_t monoBuffer[samplesToRead];

    size_t bytesRead = audioFile.read(monoBuffer, samplesToRead);
    int samplesRead = bytesRead; 

    if (samplesRead == 0) {
        Serial.println("Fileend reached");
        audioFile.close();
        isPlaying = false;
        return 0;
    }

    int outIndex = 0;

    for (int i = 0; i < samplesRead && outIndex < len; i++) {

        // 8 Bit unsigned → 16 Bit signed
        int16_t sample16 = ((int16_t)monoBuffer[i] - 128) << 8;

        // Mono => Stereo
        data[outIndex].channel1 = sample16;
        data[outIndex].channel2 = sample16;
        outIndex++;

        // Upsampling 22k → 44k
        if (outIndex < len) {
            data[outIndex].channel1 = sample16;
            data[outIndex].channel2 = sample16;
            outIndex++;
        }
    }

    return outIndex;
}

void playFile() {
    int fileNumber = random(1, 12); 
    String fileName = "/" + String(fileNumber) + ".wav";
    
    audioFile = LittleFS.open(fileName, "r");
    if (!audioFile) {
        Serial.println("Cant open file " + fileName);
        return;
    }
    
    // skip wav header
    audioFile.seek(44); 
    isPlaying = true;
    Serial.println("Playing: " + fileName);
}

void setup() {
    Serial.begin(115200);
    pinMode(buttonPin, INPUT_PULLUP);
    
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }

    // Deactivate WIFI
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    lastButtonState = digitalRead(buttonPin);

    a2dp_source.start("Creative MUVO FREE", get_audio_data);
    a2dp_source.set_auto_reconnect(true);
}

void loop() {
    bool currentButtonState = digitalRead(buttonPin);

    if (currentButtonState == LOW && lastButtonState == HIGH) {
        delay(300);

        if (!isPlaying) {
            playFile();
            isPlaying = true; 
            Serial.println("Starting playback");
        } else {
            audioFile.close();
            isPlaying = false;
            Serial.println("Stopping");
        }
        
        delay(50);
    }

    lastButtonState = currentButtonState;
}