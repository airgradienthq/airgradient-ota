/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef ESP8266

#include "airgradientOta.h"
#include "agLogger.h"

AirgradientOTA::AirgradientOTA() {}

AirgradientOTA::~AirgradientOTA() {}

AirgradientOTA::OtaResult AirgradientOTA::updateIfAvailable(const std::string &sn,
                                                            const std::string &currentFirmware, std::string httpDomain) {
  return Skipped;
}

void AirgradientOTA::setHandlerCallback(OtaHandlerCallback_t callback) { _callback = callback; }

void AirgradientOTA::sendCallback(OtaResult result, const char *message) {
  if (_callback) {
    _callback(result, message);
  }
}

std::string AirgradientOTA::buildUrl(const std::string &sn, const std::string &currentFirmware, std::string httpDomain) {
  // "http://hw.airgradient.com/sensors/airgradient:aabbccddeeff/generic/os/firmware.bin?offset=386000&length=2000"
  // NOTE: Careful here when changing the url
  char url[150] = {0};
  sprintf(url, "http://%s/sensors/airgradient:%s/generic/os/firmware.bin?current_firmware=%s", 
          httpDomain.c_str(), sn.c_str(), currentFirmware.c_str());

  return std::string(url);
}

bool AirgradientOTA::init() {
  updatePartition_ = esp_ota_get_next_update_partition(NULL);
  if (updatePartition_ == NULL) {
    AG_LOGE(TAG, "Passive OTA partition not found");
    return false;
  }

  esp_err_t err = esp_ota_begin(updatePartition_, OTA_SIZE_UNKNOWN, &_otaHandle);
  if (err != ESP_OK) {
    AG_LOGI(TAG, "Initiating OTA failed, error=%d", err);
    return false;
  }

  imageWritten = 0;

  return true;
}

bool AirgradientOTA::write(const char *data, int size) {
  if (size == 0) {
    AG_LOGW(TAG, "No data to write");
    return false;
  }

  esp_err_t err = esp_ota_write(_otaHandle, (const void *)data, size);
  if (err != ESP_OK) {
    AG_LOGW(TAG, "OTA write failed! err=0x%d", err);
    return false;
  }

  imageWritten = imageWritten + size;
  AG_LOGD(TAG, "Written image length: %d", imageWritten);

  return true;
}

bool AirgradientOTA::finish() {
  AG_LOGI(TAG, "Finishing... Total binary data length written: %d", imageWritten);

  // Finishing fota update
  esp_err_t err = esp_ota_end(_otaHandle);
  if (err != ESP_OK) {
    AG_LOGE(TAG, "Error: OTA end failed, image invalid! err=0x%d", err);
    return false;
  }

  // Set bootloader to load app from new app partition
  err = esp_ota_set_boot_partition(updatePartition_);
  if (err != ESP_OK) {
    AG_LOGE(TAG, "Error: OTA set boot partition failed! err=0x%d", err);
    return false;
  }

  AG_LOGI(TAG, "OTA successful, MAKE SURE TO REBOOT!");

  return true;
}

void AirgradientOTA::abort() {
  // Free ota handle when ota failed midway
  esp_ota_abort(_otaHandle);
  AG_LOGI(TAG, "OTA Aborted");
}

#endif // ESP8266
