/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef AIRGRADIENT_OTA_WIFI_H
#define AIRGRADIENT_OTA_WIFI_H

#ifdef ARDUINO
#ifndef ESP8266

#ifdef ARDUINO
/**
 * If compile using arduino, and Arduino.h no included, below error occur
 * ~/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32c3/include/lwip/lwip/src/include/lwip/ip4_addr.h:63:37: error: expected ')' before numeric constant
 * #define IPADDR_NONE         ((u32_t)0xffffffffUL)
 */
#include <Arduino.h>
#endif

#include <string>
#include "esp_http_client.h"
#include "airgradientOta.h"

class AirgradientOTAWifi : public AirgradientOTA {
private:
  const char *const TAG = "OTAWifi";
  esp_http_client_handle_t _httpClient = NULL;
  esp_http_client_config_t _httpConfig = {0};
  const int OTA_BUF_SIZE = 1024;

public:
  AirgradientOTAWifi();
  ~AirgradientOTAWifi();

  OtaResult updateIfAvailable(const std::string &sn,
                              const std::string &currentFirmware,
                              std::string httpDomain = AIRGRADIENT_HTTP_DOMAIN);

private:
  OtaResult processImage();
  void cleanupHttp(esp_http_client_handle_t client);
};

#endif // ESP8266
#endif // ARDUINO
#endif // !AIRGRADIENT_OTA_WIFI_H
