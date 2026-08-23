idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES config camera network server leds gates fan)
