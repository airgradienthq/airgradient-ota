/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#include "airgradientOta.h"

#include "esp_log.h"

AirgradientOTA::AirgradientOTA() {}

AirgradientOTA::~AirgradientOTA() {}

AirgradientOTA::OtaResult AirgradientOTA::updateIfAvailable(const std::string &sn,
                                                            const std::string &currentFirmware) {
  return Skipped;
}

void AirgradientOTA::setHandlerCallback(OtaHandlerCallback_t callback) { _callback = callback; }

void AirgradientOTA::sendCallback(OtaResult result, const char *message) {
  if (_callback) {
    _callback(result, message);
  }
}

std::string AirgradientOTA::buildUrl(const std::string &sn, const std::string &currentFirmware) {
  // "http://hw.airgradient.com/sensors/airgradient:aabbccddeeff/generic/os/firmware.bin?offset=386000&length=2000"
  // NOTE: Careful here when changing the url
  char url[150] = {0};
  sprintf(url,
          "http://hw.airgradient.com/sensors/airgradient:%s/generic/os/firmware.bin?current_firmware=%s",
          sn.c_str(), currentFirmware.c_str());

  return std::string(url);
}

bool AirgradientOTA::init() {
  esp_log_level_set(TAG, ESP_LOG_INFO);

  updatePartition_ = esp_ota_get_next_update_partition(NULL);
  if (updatePartition_ == NULL) {
    ESP_LOGE(TAG, "Passive OTA partition not found");
    return false;
  }

  esp_err_t err = esp_ota_begin(updatePartition_, OTA_SIZE_UNKNOWN, &_otaHandle);
  if (err != ESP_OK) {
    ESP_LOGI(TAG, "Initiating OTA failed, error=%d", err);
    return false;
  }

  imageWritten = 0;

  return true;
}

bool AirgradientOTA::write(const char *data, int size) {
  if (size == 0) {
    ESP_LOGW(TAG, "No data to write");
    return false;
  }

  esp_err_t err = esp_ota_write(_otaHandle, (const void *)data, size);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "OTA write failed! err=0x%d", err);
    return false;
  }

  imageWritten = imageWritten + size;
  ESP_LOGD(TAG, "Written image length: %d", imageWritten);

  return true;
}

bool AirgradientOTA::finish() {
  ESP_LOGI(TAG, "Finishing... Total binary data length written: %d", imageWritten);

  // Finishing fota update
  esp_err_t err = esp_ota_end(_otaHandle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: OTA end failed, image invalid! err=0x%d", err);
    return false;
  }

  // Set bootloader to load app from new app partition
  err = esp_ota_set_boot_partition(updatePartition_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: OTA set boot partition failed! err=0x%d", err);
    return false;
  }

  ESP_LOGI(TAG, "OTA successful, MAKE SURE TO REBOOT!");

  return true;
}

void AirgradientOTA::abort() {
  // Free ota handle when ota failed midway
  esp_ota_abort(_otaHandle);
  ESP_LOGI(TAG, "OTA Aborted");
}
