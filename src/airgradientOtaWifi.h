/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef AIRGRADIENT_OTA_WIFI_H
#define AIRGRADIENT_OTA_WIFI_H

#ifndef ESP8266

#include <string>

#ifdef ARDUINO
//! Somehow if compile using arduino and not include this. esp_log not come out.
#include <Arduino.h>
#endif

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

  OtaResult updateIfAvailable(const std::string &sn, const std::string &currentFirmware);

private:
  OtaResult processImage();
  void cleanupHttp(esp_http_client_handle_t client);
};

#endif // ESP8266
#endif // !AIRGRADIENT_OTA_WIFI_H
