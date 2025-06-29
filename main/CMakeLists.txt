idf_component_register(SRCS "controller.c" "peripheral.c" "peripheral" "main.c" "communication.c" "common.c"
                    PRIV_REQUIRES spi_flash
                    PRIV_REQUIRES esp_driver_gpio
                    PRIV_REQUIRES nvs_flash
                    PRIV_REQUIRES esp_wifi
                    PRIV_REQUIRES esp_phy
                    PRIV_REQUIRES esp_netif
                    PRIV_REQUIRES esp_event
                    PRIV_REQUIRES esp_coex
                    PRIV_REQUIRES wpa_supplicant
                    PRIV_REQUIRES mbedtls
                    PRIV_REQUIRES console
                    PRIV_REQUIRES esp_pm
                    INCLUDE_DIRS "")
