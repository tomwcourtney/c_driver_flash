/**
 * @file flash.h
 * @brief For creating and accessing user areas of flash memory. Note that it is not thread safe so either don't use it with multi-threading
 * or if you need to use it with multi-threading use a dedicated thread that is in charge of interfacing to the flash and queue actions to 
 * this thread. This will serialize flash operations and prevent race conditions.
 */

#ifndef INC_FLASH_H_
#define INC_FLASH_H_

#include <stdint.h>

/* PUBLIC DEFINES */

/**
 * @brief This is the largest amount of data you can read at a time. 
 * It's limited because an internal buffer is used for padding. 
 * 
 * Maybe this should just be the flash_user page size.
 */
#define FLASH_MAX_WRITE_SIZE 256

/**
 * @brief This is the value that's used for padding in the flash.
 */
#define FLASH_EMPTY_VALUE 0xFF


/* PUBLIC TYPES */

/**
 * @brief Status codes for function execution.
 */
//typedef enum{
//	FLASH_OK,
//	FLASH_ERROR,
//	FLASH_NOT_ERASED_ERROR,
//	FLASH_DATA_NOT_FOUND
//}flash_status_t;

/**
 * @brief For specifying the endianess of a given flash.
 */
typedef enum{
	FLASH_ENDIANESS_BIG,
	FLASH_ENDIANESS_LITTLE	
}flash_endianess_t;

/**
 * @brief Function pointer type for programming flash. Depending on the size of word_size your user implementation 
 * of this function might need to reconstruct a larger number to write than uint8_t data. Hence it's a pointer to an
 * array.
 * @param write_address The address to write at.
 * @param data The data to be written into memory.
 * @param number_of_words The number of words you want to write to mem.
 * @return flash_status_t status of the program operation.
 */
typedef flash_status_t (*flash_write_ptr)( uint32_t write_address, uint8_t * data, uint16_t number_of_words);

/**
 * @brief Read data from flash mem. 
 * @param user_read_address The address to start reading from.
 * @param data Byte array to read data into.
 * @param read_length The number of bytes to read.
 * @return flash_status_t status of the program operation.
 */
typedef flash_status_t (*flash_read_ptr)(uint32_t user_read_address, uint8_t *data, uint16_t read_length);

/**
 * @brief Pointer to function that can be used to erase data on a page of flash mem.
 * @param page The page to start erasing from.
 * @param number_of_pages The number of pages to erase.
 * @return flash_status_t status of the program operation.
 */
typedef flash_status_t (*erase_ptr)(uint8_t start_page, uint8_t number_of_pages);

/**
 * @brief Holds state information for the flash module.
 * @param start_page The starting page of the section of flash dedicated to the user.
 * @param page_size The size of the a single page in flash mem.
 * @param number_of_pages The number of flash pages dedicated to the user. 
 * @param end_page The final page of user flash.
 * @param word_size The minimum number of bytes that you can write to flash.
 * @param read_size The minimum number of bytes that you can read from flash.
 * @param flash_size The number of bytes allocated to the user for flash.
 * @param endianess The endianess of the flash.
 */
typedef struct{
	uint8_t start_page;
	uint16_t page_size;
	uint8_t number_of_pages;
	uint8_t end_page;
	uint8_t word_size;
	uint8_t read_size;
	uint32_t flash_size;
	uint32_t base_address;
	flash_endianess_t endianess;
}flash_area_t;

/**
 * @brief A given area of flash that is used as a circular buffer. The minimum size of an area in flash
 * is one page.
 *
 * @param head Next location to write to in the memory.
 * @param tail Next location to read from in the memory.
 * @param start_page The start page of the flash user area.
 * @param end_page The end page of the flash user area.
 * @param index_page The page that this index gets stored in.
 * @param max_data_address The maximum address of the data space
 * @param min_data_addres The minimum address of the data space
 * @param max_index_address The maximum address of the index page
 * @param min_index_addres The minimum address of the index page
 * @param min_index_addres The minimum address of the index page
 * @param index_data_size The number of bytes to represent the head and tail of the index.
 */
typedef struct{
	uint32_t head;
	uint32_t tail;
	uint8_t start_page;
	uint8_t end_page;
	uint8_t index_page;
	uint32_t max_data_address;
	uint32_t min_data_address;
	uint32_t max_index_address;
	uint32_t min_index_address;
	uint8_t index_data_size;
}flash_index_t;


/* PUBLIC FUNCTION DECLARATIONS */

/**
 * @brief Initialize the flash module
 * 
 * @param write_fn User implemented write function. 
 * @param read_fn User implemented read function.
 * @param erase_fn User implemented erase function.
 * @param word_size Minimum number of bytes for a write.
 * @param page_size Minimum number of bytes for a write.
 * @param number_of_pages Minimum number of bytes for a write.
 * @param start_page The page of mem that the user can start on.
 * @param endianess Specifies the endianess of the flash.
 */
void flash_init(flash_write_ptr write_fn, flash_read_ptr read_fn, erase_ptr erase_fn, uint8_t word_size, uint16_t page_size, uint8_t number_of_pages, uint8_t start_page, uint32_t base_address, flash_endianess_t endianess);

/**
 * @fn flash_status_t flash_write(uint32_t, uint8_t*, uint16_t)
 * @brief Write some bytes out to flash
 *
 * @param [in] user_address Where to start writing relative to the user defined flash start.
 * @param [in] data Write bytes from here.
 * @param [in] length The number of double words to read out.
 * @return Status of execution.
 */
flash_status_t flash_write(uint32_t user_address, uint8_t * data, uint16_t write_length);

/**
 * @fn flash_status_t flash_read(uint32_t, uint8_t*, uint16_t)
 * @brief Read some bytes out of flash.
 *
 * @param [in] user_read_address Where to start writing relative to the user defined flash start.
 * @param [out] data Read data into this buffer.
 * @param [in] length The number of bytes to read out.
 * @return Status of execution.
 */
flash_status_t flash_read(uint32_t user_read_address, uint8_t * data, uint16_t length);

/**
 * @fn flash_status_t flash_erase_page(uint32_t)
 * @brief Erases the page that the given user address is located on.
 *
 * @param page_number The number of the start page.
 * @param number_of_pages The number of pages to erase.
 * @return flash_status_t
 */
flash_status_t flash_erase_pages(uint8_t page_number, uint8_t number_of_pages);

/**
 * @brief Register a new index for the user's flash area. 
 * 
 * @param start_page
 * @param end_page 
 * @return int Id of the new index
 */
int flash_index_register(uint8_t start_page, uint8_t end_page);

/**
 * @brief Write some data to flash using an index as a guide of where to write.
 * 
 * @param id The id of the index
 * @param data The data to be written.
 * @param word_count The number of words to be written.
 * @return flash_status_t 
 */
flash_status_t flash_index_write(uint8_t id, uint8_t * data, uint16_t byte_count);

/**
 * @brief Read from flash using index object.
 * 
 * @param id The index to use.
 * @param data Read into this array.
 * @param data_length Number of bytes to read.
 * @return flash_status_t 
 */
flash_status_t flash_index_read(uint8_t id, uint8_t * data, uint16_t data_length);

/**
 * @brief Returns the value of head of given index.
 * 
 * @param id The index to get head from.
 * @return uint32_t -1 if error.
 */
uint32_t flash_index_get_head(uint8_t id);

/**
 * @brief User can request to read data from the index space relative to the head of the index.
 * Instead of from the tail.
 * 
 * @param id The id of the index to read from.
 * @param position Positive or negative number relative to the head.
 * @param data Array to read data in to.
 * @param data_length Number of bytes you want to read.
 * @return Status
 */
flash_status_t flash_index_read_rel_head(uint8_t id, int position, uint8_t * data, uint16_t data_length);

/**
 * @brief Erase all data pages in an index.
 * 
 * @param id Index to erase.
 * @return flash_status_t 
 */
flash_status_t flash_index_erase_all_data(uint8_t id);

/**
 * @brief Erase all index pages for an index.
 *
 * @param id Index to erase.
 * @return flash_status_t
 */
flash_status_t flash_index_erase_index(uint8_t id);

/**
 * @brief Writes the given index head and tail to flash page assigned for index
 * as specified by the object.
 * 
 * @param id The index object id.
 * @return flash_status_t 
 */
flash_status_t flash_index_write_index(uint8_t id);

/**
 * @brief Retrieves the address of first byte of the last written head and tail for a given index. 
 *
 * @param id The given index obj id.
 * @return status - FLASH_DATA_NOT_FOUND if no index data found on page.
 */
flash_status_t flash_index_get_index_address(uint8_t id, uint32_t *address);

/**
 * @brief Reset the value of the head and tail of the index to
 * the start of the data page(s).
 * 
 * @param id The index you want to reset.
 * @return flash_status_t 
 */
flash_status_t flash_index_reset(uint8_t id);

/**
 * @brief Load the index data stored in flash into the given index object.
 * 
 * @param id The index you want to load.
 * @return flash_status_t 
 */
flash_status_t flash_index_load(uint8_t id);

#endif /* INC_FLASH_H_ */
