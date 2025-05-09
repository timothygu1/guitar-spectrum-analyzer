// ble.h
// Header for BLE control interface on ESP32 Spectrum Analyzer
// Defines BLE UUIDs, callback class, and initialization method

#pragma once

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// BLE service and characteristic UUIDs (custom-defined)
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Externally defined functions and variables controlling LED modes
extern void changeMode();
extern void startAutoMode();
extern bool autoChangePatterns;

class MyCallbacks : public BLECharacteristicCallbacks {
public:
  void onWrite(BLECharacteristic *pCharacteristic) override;
};


void bleInit();