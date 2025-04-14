/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef ESP8266

#include <cstring>
#include <string>

#include "airgradientOtaCellular.h"
#include "airgradientOta.h"
#include "esp_ota_ops.h"
#include "agLogger.h"

#ifdef ARDUINO
#include "Libraries/airgradient-client/src/common.h"
#include "Libraries/airgradient-client/src/cellularModule.h"
#else
#include "common.h"
#include "cellularModule.h"
#endif

AirgradientOTACellular::AirgradientOTACellular(CellularModule *cell) : cell_(cell) {}

AirgradientOTA::OtaResult
AirgradientOTACellular::updateIfAvailable(const std::string &sn,
                                          const std::string &currentFirmware) {
  // Sanity check
  if (cell_ == nullptr) {
    AG_LOGE(TAG, "Please initialize CelularCard first");
    return Skipped;
  }
  AG_LOGI(TAG, "Start OTA using cellular");

  // Format the base url
  _baseUrl = buildUrl(sn, currentFirmware);

  // Download with expected body length empty, just want to know if available or not
  int totalImageSize = 1400000; // NOTE: This is assumption 1.4mb
  std::string url = _baseUrl + "&offset=0&length=0";
  AG_LOGI(TAG, "First check if firmware update available");
  AG_LOGI(TAG, "%s", url.c_str());

  // TODO: Future improvement should use http HEAD method to also know the total image length
  //   without receive response body
  // Problem is i2c to serial bridge limitation, response header can go up to 500 bytes in 1 serial dump
  auto response = cell_->httpGet(url);
  if (response.status != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Module does not return OK to httpGet()");
    sendCallback(OtaResult::Failed, "");
    return OtaResult::Failed;
  }

  if (response.data.statusCode == 304) {
    AG_LOGI(TAG, "Firmware is already up to date");
    sendCallback(OtaResult::AlreadyUpToDate, "");
    return OtaResult::AlreadyUpToDate;
  } else if (response.data.statusCode != 200) {
    // Perhaps status code 400
    AG_LOGE(TAG, "Firmware update skipped, the server returned %d", response.data.statusCode);
    sendCallback(OtaResult::Skipped, "");
    return OtaResult::Skipped;
  }

  // Notify caller that ota is starting
  sendCallback(Starting, "");

  return _performOta(totalImageSize);
}

AirgradientOTA::OtaResult AirgradientOTACellular::_performOta(int totalImageSize) {
  // Initialize related variable
  OtaResult result = OtaResult::InProgress;
  char *urlBuffer = new char[URL_BUFFER_SIZE];
  int imageOffset = 0;

  // Initialize ota native api
  if (!init()) {
    sendCallback(OtaResult::Failed, "");
    return OtaResult::Failed;
  }

  // Notify caller that ota is starting
  sendCallback(InProgress, "0");

  AG_LOGI(TAG, "Wait until OTA has finished");
  unsigned long downloadStartTime = MILLIS();
  while (true) {
    // Build url with chunk param and attempt download chunk image
    _buildParams(imageOffset, urlBuffer);
    AG_LOGI(TAG, ">> imageOffset %d, with endpoint %s", imageOffset, urlBuffer);

    auto response = cell_->httpGet(urlBuffer);
    if (response.status != CellReturnStatus::Ok) {
      AG_LOGE(TAG, "Module does not return OK to httpGet()");
      result = Failed;
      break;
    }

    // Check response status code
    if (response.data.statusCode == 200) {
      if (response.data.bodyLen == 0) {
        AG_LOGW(TAG, "Response OK but body empty");
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
        AG_LOGI(TAG, "Received remainder chunk (size: %d), applying image...",
                response.data.bodyLen);
        sendCallback(InProgress, "100"); // Send finish indicatation
        break;
      }
    } else if (response.data.statusCode == 204) {
      AG_LOGI(TAG, "Download image binary complete, applying image...");
      sendCallback(InProgress, "100"); // Send finish indicatation
      break;
    } else {
      AG_LOGE(TAG, "Download of image chunk failed, the server returned %d", response.data.statusCode);
      result = Failed;
      break;
    }

    // Send callback
    int percent = (imageWritten * 100) / totalImageSize;
    if (percent < 96) {
      // Because not knowing the exact image, then only send callback < 96 to
      // avoid send percentage more than 100%
      sendCallback(InProgress, std::to_string(percent).c_str());
    }

    // Increment image offset to download
    imageOffset = imageOffset + CHUNK_SIZE;

    // Need to feed esp32 watchdog?
    DELAY_MS(10);
  }

  AG_LOGI(TAG, "Time taken to iterate download binaries in chunk %.2fs",
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

void AirgradientOTACellular::_buildParams(int offset, char *output) {
  // Clear the buffer to make sure the string later have string terminator
  memset(output, 0, URL_BUFFER_SIZE);
  // Format the enpoint
  snprintf(output, URL_BUFFER_SIZE, "%s&offset=%d&length=%d", _baseUrl.c_str(), offset, CHUNK_SIZE);
}

#endif // ESP8266
