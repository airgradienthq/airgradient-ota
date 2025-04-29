/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifdef ARDUINO
#ifndef ESP8266
#include "Libraries/airgradient-client/src/common.h"

#include "airgradientOtaWifi.h"
#include "airgradientOta.h"
#include "agLogger.h"

AirgradientOTAWifi::AirgradientOTAWifi() {}

AirgradientOTAWifi::~AirgradientOTAWifi() {}

AirgradientOTA::OtaResult
AirgradientOTAWifi::updateIfAvailable(const std::string &sn, const std::string &currentFirmware, std::string httpDomain) {
  // Format the base url
  std::string url = buildUrl(sn, currentFirmware, httpDomain);
  AG_LOGI(TAG, "%s", url.c_str());

  // Initialize http configuration
  _httpConfig.url = url.c_str();

  // Initialize http client
  _httpClient = esp_http_client_init(&_httpConfig);
  if (_httpClient == NULL) {
    AG_LOGE(TAG, "Failed to initialize http connection");
    return Failed;
  }

  // Attempt open connection to the http server
  esp_err_t err = esp_http_client_open(_httpClient, 0);
  if (err != ESP_OK) {
    esp_http_client_cleanup(_httpClient);
    AG_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    return Failed;
  }
  esp_http_client_fetch_headers(_httpClient);

  int statusCode = esp_http_client_get_status_code(_httpClient);
  if (statusCode == 304) {
    AG_LOGI(TAG, "Firmware is already up to date");
    cleanupHttp(_httpClient);
    sendCallback(AlreadyUpToDate, "");
    return AlreadyUpToDate;
  } else if (statusCode != 200) {
    AG_LOGW(TAG, "Firmware update skipped, the server returned %d", statusCode);
    cleanupHttp(_httpClient);
    sendCallback(Skipped, "");
    return Skipped;
  }

  // Initialize ota native api
  if (!init()) {
    // When failed, close http connection and delete http client memory
    // sendCallback(Failed, ""); // TODO: Send callback here?
    cleanupHttp(_httpClient);
    return Failed;
  }

  // Notify caller that ota is starting
  sendCallback(Starting, "");

  return processImage();
}

AirgradientOTA::OtaResult AirgradientOTAWifi::processImage() {
  // Initialize buffer to hold data from client socket
  char *buf = new char[OTA_BUF_SIZE];
  int totalImageSize = esp_http_client_get_content_length(_httpClient);
  AG_LOGI(TAG, "Image size %d bytes", totalImageSize);

  uint32_t lastCbCall = MILLIS();
  OtaResult result = InProgress;
  while (1) {
    int recvSize = esp_http_client_read(_httpClient, buf, OTA_BUF_SIZE);
    if (recvSize == 0) {
      sendCallback(InProgress, "100");
      AG_LOGI(TAG, "Download iamge binary complete, applying image...");
      break;
    } else if (recvSize < 0) {
      AG_LOGE(TAG, "HTTP data read error");
      result = Failed;
      break;
    }

    // Write received buffer to the app partition
    if (!write(buf, recvSize)) {
      // If write failed, consider it finish and fail
      result = Failed;
      break;
    }

    // Send callback only every 250ms
    if ((MILLIS() - lastCbCall) > 250) {
      int percent = (imageWritten * 100) / totalImageSize;
      sendCallback(InProgress, std::to_string(percent).c_str());
      lastCbCall = MILLIS();
    }
  }

  // Do clean up
  delete[] buf;
  cleanupHttp(_httpClient);

  // If its not in progress then its failed
  if (result == Failed) {
    abort();
    sendCallback(Failed, "");
    return Failed;
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

void AirgradientOTAWifi::cleanupHttp(esp_http_client_handle_t client) {
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
}

#endif // ESP8266
#endif // ARDUINO
