#include <Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <DHT.h>

#define SERVICE_UUID "65052f48-6410-40d9-901f-2a2077f4aada"
#define CHARACTERISTIC_TEMP_UUID "bd0c8283-1ef4-40d5-a9cf-418913f3bc37"
#define CHARACTERISTIC_LED_UUID "bd0c8283-1ef4-40d5-a9cf-418913f3bc38"

#define DHTPIN 27
#define DHTTYPE DHT11
#define LEDPIN 12

DHT dht(DHTPIN, DHTTYPE);

int ledState = 1;
float tempValue = 1;

BLECharacteristic *tempCharacteristic;
BLECharacteristic *ledCharacteristic;

class UpdateLedValueCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *ledCharacteristic) {
        std::string rxValue = ledCharacteristic->getValue();

        if (rxValue.length() != 1) {
            return;
        }
        
        ledState = (int) rxValue[0] ?: 0;
    }
};

void setup() {
  BLEDevice::init("Capteur FSI");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  tempCharacteristic = pService->createCharacteristic(CHARACTERISTIC_TEMP_UUID, BLECharacteristic::PROPERTY_READ);
  tempCharacteristic->setValue(tempValue);
  ledCharacteristic = pService->createCharacteristic(CHARACTERISTIC_LED_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  ledCharacteristic->setValue(ledState);
  ledCharacteristic->setCallbacks(new UpdateLedValueCallbacks());
  
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  pinMode(LEDPIN, OUTPUT);

  dht.begin();
}

void loop() {
  delay(100);
  
  digitalWrite(LEDPIN, ledState == 1 ? HIGH : LOW);

  float h = dht.readHumidity();
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(f)) {
    return;
  }

  float hi = dht.computeHeatIndex(f, h);
  tempValue = dht.convertFtoC(hi);

  char txString[8]; 
  dtostrf(tempValue, 2, 2, txString);
  tempCharacteristic->setValue(txString);
}
