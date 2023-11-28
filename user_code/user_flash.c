
#include <stdint.h>
#include "stm32wbxx_hal.h"
#include "user_flash.h"


flash_status_t user_flash_erase_page(uint8_t page_number, uint8_t number_of_pages)
{
  return FLASH_OK;
}

flash_status_t user_flash_read(uint32_t user_read_address, uint8_t *data, uint16_t read_length)
{
  return FLASH_OK;
}

flash_status_t user_flash_write(uint32_t user_write_address, uint64_t *data, uint16_t word_number)
{
  return FLASH_OK;
}
