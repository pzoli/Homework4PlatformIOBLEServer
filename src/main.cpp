#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
 
// Egyedi UUID-k (online generátorral, pl. uuidgenerator.net-tel létrehozva)
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
 
BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;
uint32_t counter = 0;
 
// Csatlakozási események figyelése
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
      deviceConnected = true;
    }
 
     void onDisconnect(BLEServer* pServer) override {
        deviceConnected = false;
        // Rövid késleltetés adása vagy a stack újraélesztése
        delay(100);
        BLEDevice::startAdvertising();
        Serial.println("Kliens lekapcsolódott, hirdetés újraindítva.");
    }
};
 
// Írási esemény figyelése (amikor a kliens adatot küld a szervernek)
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        Serial.print("Klienstől érkezett: ");
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        Serial.println();
      }
    }
};
 
void setup() {
  Serial.begin(115200);
 
  // 1. BLE stack inicializálása az eszköz nevével
  BLEDevice::init("ESP32_GATT_Példa");
 
  // 2. GATT Szerver létrehozása és callback bekötése
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
 
  // 3. Szolgáltatás (Service) definiálása
  BLEService *pService = pServer->createService(SERVICE_UUID);
 
  // 4. Karakterisztika létrehozása a kívánt jogokkal
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
 
  // Értesítések engedélyezéséhez elengedhetetlen leíró (CCCD)
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
 
  // 5. Szolgáltatás indítása
  pService->start();
 
  // 6. Hirdetés (Advertising) beállítása, hogy a kliens rátaláljon
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // iPhone/Linux gyors újracsatlakozáshoz
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
 
  Serial.println("GATT szerver elindult, várakozás csatlakozásra...");
}
 
void loop() {
  // Ha csatlakozva van a kliens, másodpercenként értesítést (Notify) küldünk
  if (deviceConnected) {
    pCharacteristic->setValue(String(counter).c_str());
    pCharacteristic->notify();
    Serial.printf("Érték elküldve (Notify): %d\n", counter);
    counter++;
    delay(1000);
  }
}