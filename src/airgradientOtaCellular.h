/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef AIRGRADIENT_OTA_CELLULAR_H
#define AIRGRADIENT_OTA_CELLULAR_H

#ifndef ESP8266
#include <string>

#ifdef ARDUINO
//! Somehow if compile using arduino and not include this. esp_log not come out.
#include <Arduino.h>
#endif

#include "Libraries/airgradient-client/src/cellularModule.h"
// #include "cellularModule.h"
#include "airgradientOta.h"

class AirgradientOTACellular : public AirgradientOTA {
private:
  const char *const TAG = "OTACell";
  const int CHUNK_SIZE = 64000; // bytes
  const int URL_BUFFER_SIZE = 200;

  CellularModule *cell_ = nullptr;
  std::string _baseUrl;

public:
  AirgradientOTACellular(CellularModule *cell);
  ~AirgradientOTACellular() {};

  OtaResult updateIfAvailable(const std::string &sn, const std::string &currentFirmware);

private:
  void buildParams(int offset, char *output);
};

#endif // ESP8266
#endif // !AIRGRADIENT_OTA_CELLULAR_H
