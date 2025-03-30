idf_component_register(
        SRC_DIRS
            "."
        INCLUDE_DIRS
            "."
        PRIV_REQUIRES spi_flash
        REQUIRES mqtt esp_wifi esp_event nvs_flash driver
)