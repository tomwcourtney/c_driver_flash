/**
 * @file user_flash.h
 * @brief This is where you implement the hardware interface layer of the flash driver.
 * You then feed the functions you define here to the flash.h driver when you initialize
 * it. You can also put your definition for the flash page size, word size and number of pages in here.
 */

#ifndef USER_LED_H
#define USER_LED_H


#include <stdint.h>
#include "flash.h"


/* PUBLIC DEFINES */
/** @brief  This is the page size for the STM32WB flash.*/
#define USER_FLASH_PAGE_SIZE 4096
/** @brief  The is the minimum write size for the STM32WB flash. */
#define USER_FLASH_WORD_SIZE 8
/** @brief  This is the number of pages I want to use from the STM32WB flash.*/
#define USER_FLASH_NUMBER_PAGES 50
/** @brief  This is the total number of bytes from the flash I want to use. */
#define USER_FLASH_FLASH_SIZE USER_FLASH_NUMBER_PAGES * USER_FLASH_PAGE_SIZE
/** @brief The user shouldn't use the first area of flash as this is program memory. */
#define USER_FLASH_FIRST_PAGE 50


/* PUBLIC FUNCTION DECLARATIONS */

/**
 * @brief Interfaces to the STM HAL library to erase specific contiguous pages from flash.
 *
 * @param page_number The page you want to erase 0 - 255.
 * @param number_of_pages The number of pages you want to erase following page_number.
 * @return
 */
flash_status_t user_flash_erase_page(uint8_t page_number, uint8_t number_of_pages);

/**
 * @brief Implement the STM HAL library to read from flash.
 *
 * @param user_read_address Where you want to start reading.
 * @param data The byte array of data to read into.
 * @param read_length The number of bytes you want to read.
 * @return
 */
flash_status_t user_flash_read(uint32_t user_read_address, uint8_t *data, uint16_t read_length);

/**
 * @brief Implement writing to flash with the STM HAL Library.
 *
 * @param user_write_address The palce to start writing in the flash.
 * @param data The byte array of data to write.
 * @param word_number The number of words to write.
 * @return
 */
flash_status_t user_flash_write(uint32_t user_write_address, uint64_t *data, uint16_t word_number);

#endif
