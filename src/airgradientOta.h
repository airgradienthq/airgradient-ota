/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef AIRGRADIENT_OTA_H
#define AIRGRADIENT_OTA_H

#ifndef ESP8266

#include <cstdint>
#include <string>

#ifdef ARDUINO
//! Somehow if compile using pio and not include this. esp_log not come out.
#include <Arduino.h>
#endif

#include "esp_ota_ops.h"

class AirgradientOTA {
private:
  const char *const TAG = "OTA";
  esp_ota_handle_t _otaHandle = 0;
  const esp_partition_t *updatePartition_ = NULL;
  const int URL_ALLOCATION_SIZE = 200;

public:
  enum OtaResult { Starting, InProgress, Success, Failed, Skipped, AlreadyUpToDate };

  AirgradientOTA();
  virtual ~AirgradientOTA();

  // Callback function called if provided
  typedef void (*OtaHandlerCallback_t)(OtaResult result, const char *message);
  void setHandlerCallback(OtaHandlerCallback_t callback);

  // Methods that should be implemented by derived class
  virtual OtaResult updateIfAvailable(const std::string &sn, const std::string &currentFirmware);

protected:
  // OTA related implementation in base class that will only called by
  // derived class
  OtaHandlerCallback_t _callback;
  uint32_t imageWritten = 0;

  void sendCallback(OtaResult result, const char *message);
  std::string buildUrl(const std::string &sn, const std::string &currentFirmware);
  bool init();
  bool write(const char *data, int size);
  bool finish();
  void abort();
};

#endif // ESP8266
#endif // AIRGRADIENT_OTA_H
