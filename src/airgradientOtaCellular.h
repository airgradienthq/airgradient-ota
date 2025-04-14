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
#include "Libraries/airgradient-client/src/cellularModule.h"
#else
#include "cellularModule.h"
#endif

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
  OtaResult _performOta(int totalImageSize);
  void _buildParams(int offset, char *output);
};

#endif // ESP8266
#endif // !AIRGRADIENT_OTA_CELLULAR_H
