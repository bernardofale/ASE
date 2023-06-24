idf_component_register(SRCS "lcdlib.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver
                    REQUIRES freertos)