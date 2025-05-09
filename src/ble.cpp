// ble.cpp
// Implements a basic BLE server on ESP32 to accept remote control commands
// from a BLE central device (e.g., smartphone).
// Commands supported: "next" (change mode), "auto" (enable auto patterns), "stop" (disable auto)

#include "ble.h"


// Callback for handling BLE write events
// Parses incoming string and triggers relevant actions
void MyCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
  std::string rxValue = pCharacteristic->getValue();
  if (rxValue.length() > 0) {
    if (rxValue == "next") {
      changeMode();
    } else if (rxValue == "auto") {
      startAutoMode();
    } else if (rxValue == "stop") {
      autoChangePatterns = false;
    }
  }
}

// Initializes BLE services and starts advertising
// Sets up a writable characteristic with callback to handle commands
void bleInit() {
    BLEDevice::init("SpectrumAnalyzerBLE");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                          CHARACTERISTIC_UUID,
                                          BLECharacteristic::PROPERTY_WRITE
                                        );

    pCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->start();
}

