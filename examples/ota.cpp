#include <Arduino.h>

// #define USE_CELLULAR

#include "airgradientClient.h"

#ifdef USE_CELLULAR
#include "airgradientCellularClient.h"
#include "cellularModuleA7672xx.h"
#include "agSerial.h"
#include "airgradientOtaCellular.h"
AgSerial agSerial(Wire);
CellularModule *cell;
#else
#include "WiFi.h"
#include "airgradientWifiClient.h"
#include "airgradientOtaWifi.h"
#endif

AirgradientClient *agClient;

static const std::string serialNumber = "aabbccddeeff";

#define GPIO_IIC_RESET 3
#define EXPANSION_CARD_POWER 4
#define GPIO_POWER_CELLULAR 5

void cellularCardOn() {
  pinMode(EXPANSION_CARD_POWER, OUTPUT);
  digitalWrite(EXPANSION_CARD_POWER, HIGH);
}

void otaCallback(AirgradientOTA::OtaResult result, const char *msg) {
  switch (result) {
  case AirgradientOTA::Starting:
    Serial.println("Starting");
    break;
  case AirgradientOTA::InProgress:
    Serial.printf("InProgress %s\n", msg);
    break;
  case AirgradientOTA::Success:
    Serial.println("success");
    break;
  case AirgradientOTA::Failed:
    Serial.println("Failed");
    break;
  default:
    break;
  }
}

void setup() {
  Serial.begin(115200);
  cellularCardOn();
  delay(3000);

#ifdef USE_CELLULAR
  Wire.begin(7, 6);
  Wire.setClock(1000000);
  agSerial.init(GPIO_IIC_RESET);
  if (!agSerial.open()) {
    Serial.println("AgSerial failed, or cellular module just not found. restarting in 10s");
    delay(10000);
    esp_restart();
  }

  cell = new CellularModuleA7672XX(&agSerial, GPIO_POWER_CELLULAR);
  agClient = new AirgradientCellularClient(cell);

#else // WIFI
  agClient = new AirgradientWifiClient();

  // Connect wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin("ssid", "password");
  Serial.println("\nConnecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
#endif

  if (!agClient->begin()) {
    Serial.println("Start Airgradient Client FAILED, restarting in 10s");
    delay(10000);
    esp_restart();
  }

  /* Create the task, storing the handle. */
  BaseType_t xReturned =
      xTaskCreate(feedWatchdogTimer, /* Function that implements the task. */
                  "WD",              /* Text name for the task. */
                  1024,              /* Stack size in words, not bytes. */
                  NULL,              /* Parameter passed into the task. */
                  5,                 /* Priority at which the task is created. */
                  &handleWdTask);    /* Used to pass out the created task's handle. */

  if (xReturned == pdPASS) {
    Serial.println("Watchdog feed task created");
  } else {
    Serial.println("Watchdog feed task create failed");
  }

  // URL goes to production for this test
#ifdef USE_CELLULAR
  AirgradientOTACellular agOta(cell);
#else
  AirgradientOTAWifi agOta;
#endif
  agOta.setHandlerCallback(otaCallback);
  auto otaStatus = agOta.updateIfAvailable(serialNumber, "3.1.21");
  if (otaStatus == AirgradientOTA::Success) {
    Serial.println("OTA success, restarting..");
    delay(1000);
    esp_restart();
  } else if (otaStatus == AirgradientOTA::AlreadyUpToDate) {
    Serial.println("Current version already up to date. Forever loop no action!");
    while (1) {
      delay(100);
    }
  } else {
    Serial.println("OTA failed, restarting..");
    while (1) {
      delay(100);
    }
  }
}

void loop() {}
