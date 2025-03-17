/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#include <cstring>
#include <string>

#include "esp_log.h"
#include "esp_ota_ops.h"

// #include "common.h"
// #include "cellularModule.h"
#include "Libraries/airgradient-client/src/common.h"
#include "Libraries/airgradient-client/src/cellularModule.h"

#include "airgradientOtaCellular.h"
#include "airgradientOta.h"

AirgradientOTACellular::AirgradientOTACellular(CellularModule *cell) : cell_(cell) {}

AirgradientOTA::OtaResult
AirgradientOTACellular::updateIfAvailable(const std::string &sn,
                                          const std::string &currentFirmware) {
  esp_log_level_set(TAG, ESP_LOG_DEBUG);

  // Sanity check
  if (cell_ == nullptr) {
    ESP_LOGE(TAG, "Please initialize CelularCard first");
    return Skipped;
  }

  ESP_LOGI(TAG, "Start OTA using cellular");
  // Initialize ota native api
  if (!init()) {
    return Failed;
  }

  // Format the base url
  _baseUrl = buildUrl(sn, currentFirmware);

  // Initialize related variable
  OtaResult result = OtaResult::InProgress;
  char *urlBuffer = new char[URL_BUFFER_SIZE];
  int imageOffset = 0;

  // TODO: Call http HEAD method to know total image length if needed
  int totalImageSize = 1200000; // NOTE: This is assumption 1.2mb

  ESP_LOGI(TAG, "Wait OTA until finish");
  unsigned long downloadStartTime = MILLIS();
  while (true) {
    // Build build url with chunk param and attempt download chunk image
    buildParams(imageOffset, urlBuffer);
    ESP_LOGD(TAG, ">> imageOffset %d, with endpoint %s", imageOffset, urlBuffer);

    if (imageOffset == 0) {
      // Notify caller that ota is starting
      sendCallback(Starting, "");
    }

    auto response = cell_->httpGet(urlBuffer);
    if (response.status != CellReturnStatus::Ok) {
      // TODO: This can be timeout from module or error, how to handle this?
      ESP_LOGE(TAG, "Module not return OK when call httpGet()");
    }

    // Check response status code
    if (response.data.statusCode == 200) {
      if (response.data.bodyLen == 0) {
        ESP_LOGW(TAG, "Response OK but body empty");
        // TODO: What to do when status code success but body empty?
        continue; // Retry?
      }

      // Write received buffer to the app partition
      bool success = write(response.data.body.get(), response.data.bodyLen);
      if (!success) {
        // If write failed, consider it finish and fail
        result = Failed;
        break;
      }

      // Check if received chunk size is at the end of the image size, hence its complete
      if (response.data.bodyLen < CHUNK_SIZE) {
        ESP_LOGI(TAG, "Received remainder chunk (size: %d), applying image...",
                 response.data.bodyLen);
        break;
      }
    } else if (response.data.statusCode == 204) {
      ESP_LOGI(TAG, "Download image binary complete, applying image...");
      break;
    } else if (response.data.statusCode == 304) {
      ESP_LOGI(TAG, "Firmware is already up to date");
      result = AlreadyUpToDate;
      break;
    } else {
      ESP_LOGE(TAG, "Firmware update skipped, the server returned %d", response.data.statusCode);
      result = Skipped;
      break;
    }

    // Send callback
    int percent = (imageWritten * 100) / totalImageSize;
    sendCallback(InProgress, std::to_string(percent).c_str());

    // Increment image offset to download
    imageOffset = imageOffset + CHUNK_SIZE;

    // Need to feed esp32 watchdog?
    DELAY_MS(10);
  }

  ESP_LOGI(TAG, "Time taken to iterate download binaries in chunk %.2fs",
           ((float)MILLIS() - downloadStartTime) / 1000);

  delete[] urlBuffer;

  // If its not in progress then its either failed or skipped, directly return
  if (result != InProgress) {
    sendCallback(result, "");
    abort();
    return result;
  }

  // Finishing ota update
  if (!finish()) {
    sendCallback(Failed, "");
    return OtaResult::Failed;
  }

  // It is success
  sendCallback(Success, "");

  return OtaResult::Success;
}

void AirgradientOTACellular::buildParams(int offset, char *output) {
  // Clear the buffer to make sure the string later have string terminator
  memset(output, 0, URL_BUFFER_SIZE);
  // Format the enpoint
  snprintf(output, URL_BUFFER_SIZE, "%s&offset=%d&length=%d", _baseUrl.c_str(), offset, CHUNK_SIZE);
}
