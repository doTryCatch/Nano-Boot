idf_component_register(SRCS "bootloader_start.c" "pre_boot_condition_checkup.c" "find_partition.c" "flash_reader.c" "banner.c" "handleUpload.c" "read_ota_flag.c"
   INCLUDE_DIRS 
        "." 
  REQUIRES bootloader bootloader_support esp_partition esp_rom spi_flash

)


# Use the default linker scripts

idf_build_get_property(scripts BOOTLOADER_LINKER_SCRIPT)

target_linker_script(${COMPONENT_LIB} INTERFACE "${scripts}") 

