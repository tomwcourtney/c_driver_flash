#ifndef FLASH_H
#define FLASH_H

#include "flash.h"

// PUBLIC DEFINES
#define FLASH_SPY_SIZE 100

// PUBLIC FUNCTION DECLARATIONS

/**
 * @brief Initialize the spy module.
 * 
 * @param word_size_init This is the minimum write size of the simulation flash module.
 * @param page_size_init This is the size of the page. 
 * @param flash_size_init This is the size of the total flash mem.
 */
void flash_spy_init(uint8_t word_size_init, uint16_t page_size_init, uint16_t flash_size_init);

/**
 * @brief Frees the memory allocated to the flash and sets the other state variables to 0.
 */
void flash_spy_deinit();

/**
 * @brief Erase some number of pages in simulation flash.
 * 
 * @param [in] page_number The page to erase.
 * @param [in] number_of_pages The number of pages to erase.
 * @return flash_status_t 
 */
flash_status_t flash_spy_erase_pages(uint8_t page_number, uint8_t number_of_pages);

/**
 * @brief Read some bytes from the flash.
 * 
 * @param [in] user_read_address The place to start reading.
 * @param [out] data Copy data into this.
 * @param [in] read_length The number of bytes to read.
 * @return flash_status_t 
 */
flash_status_t flash_spy_read(uint32_t user_read_address, uint8_t *data, uint16_t read_length);

/**
 * @brief Write to the simulate flash (a big array.)
 * 
 * @param [in] user_write_address The address to write to. Must be word aligned. I.e. integral multiple of word size.
 * @param [in] data The byte array of data to write.
 * @param [in] number_words The number of flash words you want to write.
 * @return flash_status_t 
 */
flash_status_t flash_spy_write(uint32_t user_write_address, uint8_t *data, uint16_t number_words);

void flash_spy_erase_all(void);

#endif