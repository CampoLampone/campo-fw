idf_component_register(SRCS "fan.c"
                    INCLUDE_DIRS "."
                    REQUIRES config esp_driver_ledc esp_driver_tsens)
