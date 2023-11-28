#include "flash_spy.h"
#include <string.h>
#include <stdio.h>
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"

// Public Defines

// Private Variables
/* The size of a word in the simulation flash. */
uint8_t word_size = 0;
/* The page size of the flash. */
uint16_t page_size = 0;
/* The total size of the flash. */
uint16_t flash_size = 0;

/* This is the simulated flash that will be written to, read from and erased. */
uint8_t * flash = 0;

// Private Function Declarations

// Private Function Definitions

// Public Function Definitions

void flash_spy_init(uint8_t word_size_init, uint16_t page_size_init, uint16_t flash_size_init)
{
    word_size = word_size_init;
    page_size = page_size_init;
    flash_size = flash_size_init;
    flash = (uint8_t*)malloc(flash_size);
    memset(flash, FLASH_EMPTY_VALUE, flash_size);
}

void flash_spy_deinit()
{
    word_size = 0;
    page_size = 0;
    flash_size = 0;
    free(flash);
}

flash_status_t flash_spy_erase_pages(uint8_t page_number, uint8_t number_of_pages)
{
    memset(&flash[page_number*page_size], 0xFF, page_size);
    return FLASH_OK;
}

flash_status_t flash_spy_read(uint32_t user_read_address, uint8_t *data, uint16_t read_length)
{
    memcpy(data, &flash[user_read_address], read_length);
    return FLASH_OK;
}

flash_status_t flash_spy_write(uint32_t user_write_address, uint8_t *data, uint16_t number_words)
{
    if(word_size == 0 || number_words == 0)
    {
        return FLASH_ERROR;
    }

    // Determine what page the address is on
    uint8_t page = user_write_address/page_size;

    // Check if the rest of the page that is to be written to is clear (all value empty 0xFF)
    for(uint32_t idx = user_write_address; idx < (page+1) * page_size; idx++)
    {
        if(flash[idx] != FLASH_EMPTY_VALUE)
        {
            return FLASH_ERROR;
        }
    }

    memcpy(&flash[user_write_address], data, number_words*word_size);
    return FLASH_OK;
}

void flash_spy_erase_all(void)
{
    memset(flash, FLASH_EMPTY_VALUE, flash_size);
}
