idf_component_register(SRCS "dro_core.c" "dro_nvs.c"
                        INCLUDE_DIRS "include"
                        REQUIRES nvs_flash)
