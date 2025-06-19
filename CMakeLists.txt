set(srcs 
  "src/airgradientOta.cpp"
  "src/airgradientOtaCellular.cpp"
  "src/airgradientOtaWifi.cpp"
)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "src"
		    REQUIRES esp_timer app_update AirgradientClient
                    )
