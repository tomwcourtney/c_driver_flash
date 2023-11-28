/**
 *  flash.c
 *
 *  Created on: Sep 28, 2023
 *      Author: xolop
 */

#include "flash.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// PRIVATE DEFINES

/* Convert an address in the user flash space to an address in the total flash space. */
#define USER_TO_FLASH_ADDRESS(start_address, user_address) \
  start_address + user_address

/* This is the total number of distinct areas of flash you can have*/
#define MAX_INDICES 4

// PRIVATE VARIABLES
/* This holds the user flash information*/
static flash_area_t user_flash = {0};
/* Function to write to flash. Set at init*/
static flash_write_ptr write = 0;
/* Function to erase page of flash. */
static erase_ptr erase = 0;
/* Function to read from flash. */
static flash_read_ptr read = 0;
/* Buffer for padding out what's written to flash. */
static uint8_t padding_buffer[FLASH_MAX_WRITE_SIZE] = {0x00};
/* Array of index trackers. */
static flash_index_t indices[MAX_INDICES] = {0};
/* The quantity of trackers you have. */
static uint8_t index_count = 0;

// PRIVATE FUNCTION DECLARATIONS

/**
 * @brief Is the flash initialized.
 *
 * @return true User flash is initialized.
 * @return false User flash is not initialized.
 */
static bool initialized();

/**
 * @brief Check if an index exists.
 *
 * @param id The id of the index.
 * @return true Index exists.
 * @return false Index doesn't exist.
 */
static bool index_exists(uint8_t id);

/**
 * @brief Convert a number of bytes to a number of whole words.
 * @param number_of_bytes The number of bytes to convert.
 * @return The number of words to represent the bytes.
 */
uint16_t bytes_to_words(uint32_t number_of_bytes);

/**
 * @brief Convert a number of words to bytes.
 *
 * @param number_of_words Number of words to convert.
 * @return uint16_t The number of bytes.
 */
uint16_t words_to_bytes(uint16_t number_of_words);

/**
 * @brief Determine the number of bytes required to write some
 * data as a word aligned quantity.
 *
 * @param num_bytes Quantity of bytes to write.
 * @return uint16_t The number of bytes required.
 */
uint16_t bytes_to_byte_aligned(uint16_t num_bytes);

// PRIVATE FUNCTION DEFINITIONS

static bool initialized()
{
  if (user_flash.number_of_pages == 0)
  {
    return false;
  }

  return true;
}

static bool index_exists(uint8_t id)
{
  // Check if the index exists
  if (id >= index_count || index_count == 0)
  {
    return false;
  }

  return true;
}
uint16_t bytes_to_words(uint32_t number_of_bytes)
{
  uint16_t quotient = number_of_bytes / user_flash.word_size;
  uint16_t remainder = number_of_bytes % user_flash.word_size;

  return (remainder == 0) ? quotient : quotient + 1;
}

uint16_t words_to_bytes(uint16_t number_of_words)
{
  return number_of_words * user_flash.word_size;
}

uint16_t bytes_to_byte_aligned(uint16_t num_bytes)
{
  return words_to_bytes(bytes_to_words(num_bytes));
}

// PUBLIC FUNCTION DEFINITIONS

void flash_init(flash_write_ptr write_fn, flash_read_ptr read_fn, erase_ptr erase_fn, uint8_t word_size, uint16_t page_size, uint8_t number_of_pages, uint8_t start_page, uint32_t base_address, flash_endianess_t endianess)
{
  write = write_fn;
  read = read_fn;
  erase = erase_fn;
  user_flash.word_size = word_size;
  user_flash.page_size = page_size;
  user_flash.start_page = start_page;
  user_flash.number_of_pages = number_of_pages;
  user_flash.end_page = start_page + number_of_pages - 1;
  user_flash.endianess = endianess;
  user_flash.base_address = base_address;
  index_count = 0;
  memset(indices, 0, sizeof(indices));
}

flash_status_t flash_write(uint32_t user_address, uint8_t *data, uint16_t data_length)
{
  if (user_address >= ((user_flash.start_page + user_flash.number_of_pages) * user_flash.page_size))
  {
    return FLASH_ERROR;
  }

  if (user_flash.word_size == 0)
  {
    return FLASH_ERROR;
  }

  // Not a word aligned address.
  if (user_address % user_flash.word_size != 0)
  {
    return FLASH_ERROR;
  }

  if (data_length == 0)
  {
    return FLASH_ERROR;
  }

  if (data_length > FLASH_MAX_WRITE_SIZE)
  {
    return FLASH_ERROR;
  }

  if (write == 0 || data == NULL)
  {
    return FLASH_ERROR;
  }

  memset(padding_buffer, 0xFF, FLASH_MAX_WRITE_SIZE); // Reset padding
  memcpy(padding_buffer, data, data_length);          // Copy data into padding
  uint16_t words_in_data = data_length / user_flash.word_size;
  uint8_t remainder = data_length % user_flash.word_size;

  if (remainder != 0)
  {
    words_in_data += 1; // Add one as there's a partial word in the write data.
  }

  return write(user_address + user_flash.base_address, padding_buffer, words_in_data);
}

flash_status_t flash_read(uint32_t user_read_address, uint8_t *data, uint16_t length)
{
  if (user_read_address >= (user_flash.start_page + user_flash.number_of_pages) * user_flash.page_size)
  {
    return FLASH_ERROR;
  }

  if (read == 0 || data == NULL)
  {
    return FLASH_ERROR;
  }
  else
  {
    return read(user_read_address + user_flash.base_address, data, length);
  }
}

flash_status_t flash_erase_pages(uint8_t page_number, uint8_t number_of_pages)
{
  if (page_number < user_flash.start_page || page_number + number_of_pages >= user_flash.end_page)
  {
    return FLASH_ERROR;
  }

  if (erase == 0)
  {
    return FLASH_ERROR;
  }
  else
  {
    return erase(page_number, number_of_pages);
  }
}

int flash_index_register(uint8_t start_page, uint8_t end_page)
{
  if (!initialized())
  {
    // printf("Use flash not initialized\n");
    return -1;
  }

  // Minimum number of pages is 2 as you need 1 for the index. Index is alwasy page_start.
  if ((end_page - start_page) < 1)
  {
    // printf("Not enough pages\n");
    return -1;
  }

  // End page must be greater than start page
  if (start_page > end_page)
  {
    // printf("Start page is > end page\n");
    return -1;
  }

  // Page numbers cannot exceed total available pages numbers
  if (start_page < user_flash.start_page || end_page >= user_flash.start_page + user_flash.number_of_pages)
  {
    // printf("Start or end page outside limit\n");
    return -1;
  }

  if (index_count > 0)
  {
    // printf("Checking index count\n");
    // Check all of the existing indices for overlap
    for (uint8_t idx = 0; idx < index_count; idx++)
    {
      if ((start_page >= indices[idx].start_page && start_page <= indices[idx].end_page) || (end_page >= indices[idx].start_page && end_page <= indices[idx].end_page))
      {
        // printf("Overlap with existing index\n");
        return -1;
      }
    }
  }

  flash_index_t new_index = {
      .index_page = start_page,
      .start_page = start_page + 1,
      .end_page = end_page,
      .head = (start_page + 1) * user_flash.page_size,
      .tail = (start_page + 1) * user_flash.page_size,
      .min_data_address = (start_page + 1) * user_flash.page_size,
      .max_data_address = (end_page + 1) * user_flash.page_size,
      .min_index_address = start_page * user_flash.page_size,
      .max_index_address = (start_page + 1) * user_flash.page_size};

  new_index.index_data_size = sizeof(new_index.head) + sizeof(new_index.tail);

  indices[index_count] = new_index;

  uint8_t id = index_count;
  index_count++;
  return id;
}
// TODO: Add printf back to this to see the write address 
flash_status_t flash_index_write(uint8_t id, uint8_t *data, uint16_t data_length)
{
  // Check if the index exists
  if (!index_exists(id))
  {
    return FLASH_ERROR;
  }

  flash_index_t *index = &indices[id];

  // printf("\nWrite address (head) is %d", index->head);

  // uint16_t words_before_wrap = (index->max_data_address - index->head) / user_flash.word_size; // The number of words that can be written before reaching the end of the flash space for this index.
  // uint16_t bytes_before_wrap = words_before_wrap * user_flash.word_size;                       // The number of bytes that can be written before reaching the end of flash space. Integral multiple of words before wrap.

  uint16_t words_before_wrap = bytes_to_words(index->max_data_address - index->head); // The number of words that can be written before reaching the end of the flash space for this index.
  uint16_t bytes_before_wrap = words_to_bytes(words_before_wrap);                       // The number of bytes that can be written before reaching the end of flash space. Integral multiple of words before wrap.

  // If there are more bytes to write than left before the end of flash then wrap
  // printf("\nData length %d > bytes_to_wrap %d  ?", data_length, bytes_before_wrap);
  if (data_length >= bytes_before_wrap)
  {
    // printf("\n Wrap Section");
    // If there's only one page then don't break up the data just write to the start of the page.
    if ((index->end_page - index->start_page) == 0)
    {
      if (data_length > user_flash.page_size)
      {
        data = &data[data_length - data_length % user_flash.word_size];
        data_length %= user_flash.word_size;
      }

      // Have to erase the wrap page to write to it again.
      if (flash_erase_pages(index->start_page, 1) != FLASH_OK)
      {
        // printf("\nFailed to erase page.");
        return FLASH_ERROR;
      }
      
      // printf("\nErased 1 page data.");

      // Return head to start of page.
      index->head = index->start_page * user_flash.page_size;

      if (flash_write(index->head, data, data_length) == FLASH_OK)
      {
        uint16_t bytes_written = (data_length % user_flash.word_size == 0) ? data_length : data_length + (user_flash.word_size - data_length % user_flash.word_size);
        index->head += bytes_written;           // Increment the head by bytes written
        index->head %= index->max_data_address; // Wrap the head by the max address value. Must add start_page address or head will go all the way back to 0.
        if (index->head < index->min_data_address)
        {
          index->head += index->min_data_address;
        }
      }
      else
      {
        // printf("\nFailed to write page.");
        return FLASH_ERROR;
      }
    }
    else
    {

      // Write as many bytes as you can before the wrap from the start of the data array.
      if (flash_write(index->head, data, bytes_before_wrap) != FLASH_OK)
      {
        // printf("\nFailed to write.");
        return FLASH_ERROR;
      }
      index->head += bytes_before_wrap;                        // Increment the head by bytes written. This will necessarily be word aligned quantity.
      index->head %= index->max_data_address;                  // Wrap the head by the max address value. Must add start_page address or head will go all the way back to 0.
      index->head += index->start_page * user_flash.page_size; // Have to add the start page address or it will go back to 0.

      // Have to erase the wrap page to write to it again.
      if (flash_erase_pages(index->start_page, 1) != FLASH_OK)
      {
        // printf("\nFailed to erase page.");
        return FLASH_ERROR;
      }

      // Write the remaining bytes to the new head position
      uint16_t bytes_after_wrap = data_length - bytes_before_wrap; // Must be word aligned increment.
      if (flash_write(index->head, &data[bytes_before_wrap], bytes_after_wrap) != FLASH_OK)
      {
        // printf("\nFailed to write.");
        return FLASH_ERROR;
      }
      // Integral number of words? yes - bytes_after_wrap else bytes_after_wrap + byte to form whole word.
      uint16_t bytes_written = (bytes_after_wrap % user_flash.word_size == 0) ? bytes_after_wrap : bytes_after_wrap + (user_flash.word_size - bytes_after_wrap % user_flash.word_size);
      index->head += bytes_written;
      index->head %= index->max_data_address; // Wrap the head by the max address value. Must add start_page address or head will go all the way back to 0.
      if (index->head < index->min_data_address)
      {
        index->head += index->min_data_address;
      }
    }
  }
  // If there aren't more bytes to be written then room remaining just write out all of the bytes
  else
  {
    if (flash_write(index->head, data, data_length) == FLASH_OK)
    {
      uint16_t bytes_written = (data_length % user_flash.word_size == 0) ? data_length : data_length + (user_flash.word_size - data_length % user_flash.word_size);
      index->head += bytes_written;           // Increment the head by bytes written
      index->head %= index->max_data_address; // Wrap the head by the max address value. Must add start_page address or head will go all the way back to 0.
      if (index->head < index->min_data_address)
      {
        index->head += index->min_data_address;
      }
    }
    else
    {
      // printf("\nFailed to write. No wrap.");
      return FLASH_ERROR;
    }
  }

  if (flash_index_write_index(id) != FLASH_OK)
  {
    // printf("\nFailed to write index.");
    return FLASH_ERROR;
  }

  return FLASH_OK;
}

flash_status_t flash_index_read(uint8_t id, uint8_t *data, uint16_t data_length)
{
  if (!index_exists(id))
  {
    return FLASH_ERROR;
  }

  if (flash_read(indices[id].tail + user_flash.base_address, data, data_length) == FLASH_OK)
  {
    // Update tail
    indices[id].tail += data_length;
    return FLASH_OK;
  }
  else
  {
    return FLASH_ERROR;
  }
}

uint32_t flash_index_get_head(uint8_t id)
{
  if (!index_exists(id))
  {
    return -1;
  }

  return indices[id].head;
}

flash_status_t flash_index_read_rel_head(uint8_t id, int position, uint8_t *data, uint16_t data_length)
{
  if (!index_exists(id))
  {
    return FLASH_ERROR;
  }

  // Can't read ahead of head
  if (position > 0)
  {
    return FLASH_ERROR;
  }

  flash_index_t *index = &indices[id];
  uint32_t read_address = index->head + position;

  // Check if we reverse wrap
  if (read_address < index->min_data_address)
  {
    read_address = index->max_data_address - (index->min_data_address - read_address);
  }

  // Check for forward read wrap
  if ((read_address + data_length) > index->max_data_address)
  {
    // Determine how many bytes can be read
    uint16_t bytes_before_wrap = index->max_data_address - read_address;

    // Read the bytes
    if (flash_read(read_address, data, bytes_before_wrap) != FLASH_OK)
    {
      return FLASH_ERROR;
    }

    read_address += bytes_before_wrap;
    read_address %= index->max_data_address;
    read_address += index->min_data_address;

    // Read the bytes
    if (flash_read(read_address, &data[bytes_before_wrap], data_length - bytes_before_wrap) != FLASH_OK)
    {
      return FLASH_ERROR;
    }
  }
  else
  {
    if (flash_read(read_address, data, data_length) != FLASH_OK)
    {
      return FLASH_ERROR;
    }
  }

  return FLASH_OK;
}

flash_status_t flash_index_erase_all_data(uint8_t id)
{

  if (!index_exists(id))
  {
    return FLASH_ERROR;
  }

  flash_index_t *index = &indices[id];

  index->head = index->min_data_address;

  return flash_erase_pages(index->start_page, index->end_page - index->start_page + 1);
}

flash_status_t flash_index_erase_index(uint8_t id)
{

  if (!index_exists(id))
  {
    return FLASH_ERROR;
  }

  flash_index_t *index = &indices[id];

  index->head = index->min_data_address;

  return flash_erase_pages(index->index_page, 1);
}

flash_status_t flash_index_write_index(uint8_t id)
{
  // Check if the index exists
  if (!index_exists(id))
  {
    return FLASH_ERROR;
  }

  // Search for the next spot to write to in the index page.
  uint32_t read_address = 0;
  uint32_t write_address = 0;
  flash_status_t status = flash_index_get_index_address(id, &read_address);
  // printf("\nRead address %d", read_address);

  flash_index_t *index = &indices[id];
  uint8_t index_data_size = sizeof(index->head) * 2;

  if (status == FLASH_ERROR)
  {
    return FLASH_ERROR;
  }

  if (status == FLASH_DATA_NOT_FOUND)
  {
    write_address = index->min_index_address;
    // printf(" Write address %d", write_address);
  }
  else 
  {
    write_address = read_address + bytes_to_byte_aligned(index_data_size);
    // printf(" Write address %d", write_address);

    if(write_address >= index->max_index_address)
    {
      if (flash_erase_pages(index->index_page, 1) != FLASH_OK)
      {
        return FLASH_ERROR;
      }

      write_address = index->min_index_address;
      // printf(" Write address %d", write_address);
    }
  }

  // Write the index data to flash
  uint8_t write_data[sizeof(index->head) * 2];
  memcpy(write_data, &(index->head), index_data_size / 2);
  memcpy(&write_data[index_data_size / 2], &(index->tail), index_data_size / 2);

  if (flash_write(write_address, write_data, index_data_size) != FLASH_OK)
  {
    return FLASH_ERROR;
  }
  else
  {
    return FLASH_OK;
  }
}

flash_status_t flash_index_get_index_address(uint8_t id, uint32_t *address)
{
  if (!index_exists(id))
  {
    return FLASH_ERROR;
  }

  if (user_flash.word_size == 0)
  {
    return FLASH_ERROR;
  }

  flash_index_t *index = &indices[id];

  uint32_t read_address = index->min_index_address;
  uint8_t index_data_size = sizeof(index->head) + sizeof(index->tail);
  uint8_t index_data_bytes_aligned = bytes_to_words(index_data_size) * user_flash.word_size;

  // Search for an empty word in flash. This will indicate that the end of written data has been found.
  uint8_t empty_word[user_flash.word_size];
  memset(empty_word, FLASH_EMPTY_VALUE, user_flash.word_size);

  uint8_t index_page_data[user_flash.word_size];
  memset(index_page_data, 0x00, user_flash.word_size);

  while (read_address < index->max_index_address)
  {
    flash_read(read_address, index_page_data, user_flash.word_size);

    // If empty word found
    if (memcmp(empty_word, index_page_data, user_flash.word_size) == 0)
    {
      // If empty word at flash start
      if (read_address == index->min_index_address)
      {
        return FLASH_DATA_NOT_FOUND;
      }
      else
      {
        // Address is the spot where empty word was found - the index data size
        *address = read_address - index_data_bytes_aligned;
        return FLASH_OK;
      }
    }
    else
    {
      // Increment by word as index data writes will be word aligned.
      read_address += user_flash.word_size;
    }
  }

  // If we made it to here no empty word was found so the last data is located at the end of the page
  *address = index->max_index_address - index_data_bytes_aligned;
  return FLASH_OK;
}

flash_status_t flash_index_reset(uint8_t id)
{
  if(!index_exists(id))
  {
    return FLASH_ERROR;
  }

  flash_index_t * index = &indices[id];

  index->head = index->start_page*user_flash.page_size;
  index->tail = index->head;

  return FLASH_OK;
}

flash_status_t flash_index_load(uint8_t id)
{
  if(!index_exists(id))
  {
    return FLASH_ERROR;
  }

  flash_index_t * index = &(indices[id]);
  uint32_t index_address = 0;

  // Get the address of the flash index data
  flash_status_t status = flash_index_get_index_address(id, &index_address);
  if(status != FLASH_OK)
  {
    // Could be error or index data not found.
    return status;
  }

  uint8_t data_size = index->index_data_size;
  uint8_t read_data[data_size];

  if(flash_read(index_address, read_data, data_size) != FLASH_OK)  
  {
    return FLASH_ERROR;
  }

  memcpy(&(index->head), &read_data[0], data_size/2);
  memcpy(&(index->tail), &read_data[data_size/2], data_size/2);

  return FLASH_OK;
}
