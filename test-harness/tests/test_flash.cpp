#include "CppUTest/TestHarness.h"

extern "C"
{
#include <string.h>
#include "../../inc/flash.h"
#include "../spies/flash_spy.h"
}

TEST_GROUP(Test)
{
#define WORD_SIZE 8
#define PAGE_SIZE 32
#define FLASH_SIZE 1024
#define START_PAGE 1
#define NUMBER_PAGES FLASH_SIZE/PAGE_SIZE
#define BASE_ADDRESS 0

    void setup()
    {
        flash_init((flash_write_ptr)flash_spy_write, (flash_read_ptr)flash_spy_read, (erase_ptr)flash_spy_erase_pages, WORD_SIZE, PAGE_SIZE, NUMBER_PAGES, START_PAGE, BASE_ADDRESS, FLASH_ENDIANESS_LITTLE);
        flash_spy_init(WORD_SIZE, PAGE_SIZE, FLASH_SIZE);
    }

    void teardown()
    {
        flash_init(0, 0, 0, 0, 0, 0, 0, 0, FLASH_ENDIANESS_BIG);
        flash_spy_deinit();
    }

#define REINIT_FLASH_CUSTOM(word_size, endianess) \
    flash_spy_deinit(); \
    flash_init((flash_write_ptr)flash_spy_write, (flash_read_ptr)flash_spy_read, (erase_ptr)flash_spy_erase_pages, word_size, PAGE_SIZE, NUMBER_PAGES, START_PAGE, BASE_ADDRESS, FLASH_ENDIANESS_LITTLE); \
    flash_spy_init(word_size, PAGE_SIZE, FLASH_SIZE);

#define FLASH_READ_OK(address, data_ptr, size) \
    CHECK_EQUAL_TEXT(FLASH_OK, flash_read(address, data_ptr, size), "Flash read error");

#define READ_OK_TEXT(address, data_ptr, size, text) \
    CHECK_EQUAL_TEXT(FLASH_OK, flash_read(address, data_ptr, size), text);

#define READ_ERROR_TEXT(address, data_ptr, size, text) \
    CHECK_EQUAL_TEXT(FLASH_ERROR, flash_read(address, data_ptr, size), text);

#define WRITE_OK(address, data_ptr, size) \
    CHECK_EQUAL_TEXT(FLASH_OK, flash_write(address, data_ptr, size), "Flash write error");

#define WRITE_OK_TEXT(address, data_ptr, size, text) \
    CHECK_EQUAL_TEXT(FLASH_OK, flash_write(address, data_ptr, size), text);

#define WRITE_ERROR_TEXT(address, data_ptr, size, text) \
    CHECK_EQUAL_TEXT(FLASH_ERROR, flash_write(address, data_ptr, size), text);

#define ERASE_OK_TEXT(start_page, quantity, text) \
    CHECK_EQUAL_TEXT(FLASH_OK, flash_erase_pages(start_page, quantity), text);

#define ERASE_ERROR_TEXT(start_page, quantity, text) \
    CHECK_EQUAL_TEXT(FLASH_ERROR, flash_erase_pages(start_page, quantity), text);

#define REGISTER_ID_OK_TEXT(start_page, end_page, id, text) \
    id = flash_index_register(start_page, end_page); \
    CHECK_COMPARE_TEXT(id, >=, 0, text);

#define REGISTER_ID_ERROR_TEXT(start_page, end_page, id, text) \
    id = flash_index_register(start_page, end_page); \
    CHECK_COMPARE_TEXT(id, <, 0, text);

#define WRITE_INDEX_OK_TEXT(id, data, byte_count, text) \
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_write(id, data, byte_count), text);

#define READ_INDEX_OK_TEXT(id, data, byte_count, text) \
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_read(id, data, byte_count), text);

#define BYTES_TO_WORDS(bytes) \
    (bytes % WORD_SIZE == 0) ? bytes/WORD_SIZE : bytes/WORD_SIZE + 1

#define ALIGNED_BYTES(bytes) \
    (BYTES_TO_WORDS(bytes)) * WORD_SIZE
};

/** ZERO **/

// Nothing should work
TEST(Test, uninitialized_module_should_not_work)
{
    teardown();

    // Write should not work
    CHECK_EQUAL(FLASH_ERROR, flash_write(0, NULL, 0));
    // Read should not work
    CHECK_EQUAL(FLASH_ERROR, flash_read(0, NULL, 0));
    // Erase should not work
    CHECK_EQUAL(FLASH_ERROR, flash_erase_pages(0, 0));

    setup();
}

/** ONE */

TEST(Test, initialized_module_should_write_read_erase)
{
    uint8_t write_data[] = {0};
    uint8_t read_data[] = {0};

    // Read should work
    CHECK_EQUAL_TEXT(FLASH_OK, flash_read(START_PAGE * PAGE_SIZE, read_data, 1), "Flash read failed");

    // Write should work
    CHECK_EQUAL_TEXT(FLASH_OK, flash_write(START_PAGE * PAGE_SIZE, write_data, 1), "Flash write failed");

    // Erase should work
    CHECK_EQUAL_TEXT(FLASH_OK, flash_erase_pages(START_PAGE, 0), "Erase failed");
}

TEST(Test, write_a_byte_to_memory)
{
    // 1. Initialize flash with implemented flash_write and flash_read.

    // 2. Use flash write to modify a byte of memory.
    uint8_t write_data[1] = {0x01};
    CHECK_EQUAL(FLASH_OK, flash_write(0, &write_data[0], 1));

    // 3. Use flash read to check the value of the byte.
    uint8_t read_data[1] = {0};
    CHECK_EQUAL(FLASH_OK, flash_read(0, &read_data[0], 1));

    // 4. Fake it till you make it bb.
    LONGS_EQUAL(read_data[0], write_data[0]);
}

/* Check that you can write a byte and erase the page.*/
TEST(Test, write_a_byte_erase_the_page)
{
    // 1. Initialize flash with implemented flash_write and flash_read.

    // 2. Use flash write to modify a byte of memory.
    uint8_t write_data[1] = {0x01};
    WRITE_OK_TEXT(START_PAGE*PAGE_SIZE, &write_data[0], 1, "Modify a byte of data");

    // 3. Use flash read to check the value of the byte.
    uint8_t read_data[1] = {0};
    READ_OK_TEXT(START_PAGE*PAGE_SIZE, &read_data[0], 1, "Read back the byte of data");

    // 4. Check that the bytes are equal.
    LONGS_EQUAL(read_data[0], write_data[0]);

    // 5. Wipe the page
    ERASE_OK_TEXT(START_PAGE, 1, "Wipe the page with the byte written.");

    // 6. Read the value that should have been wiped
    CHECK_EQUAL(FLASH_OK, flash_read(0, &read_data[0], 1));

    // 7. Check that the value is wiped
    LONGS_EQUAL_TEXT(FLASH_EMPTY_VALUE, read_data[0], "Value not wiped");
}

/* Write a byte. Erase it. Write it again.*/
TEST(Test, overwrite_a_byte)
{
    // 1. Initialize flash with proper functions.

    // 2. Write a byte.
    uint8_t write_data[1] = {0x01};
    CHECK_EQUAL(FLASH_OK, flash_write(START_PAGE*PAGE_SIZE, &write_data[0], 1));

    // 3. Erase the page the byte was written to.
    CHECK_EQUAL(FLASH_OK, flash_erase_pages(START_PAGE, 1));

    // 4. Write a different byte to the same address.
    write_data[0] = 0x02;
    CHECK_EQUAL(FLASH_OK, flash_write(START_PAGE*PAGE_SIZE, &write_data[0], 1));

    // 5. Read back the byte at the address that was written to.
    uint8_t read_data[1] = {0};
    CHECK_EQUAL(FLASH_OK, flash_read(START_PAGE*PAGE_SIZE, &read_data[0], 1));

    // 6. Check that the value matches the new write
    LONGS_EQUAL_TEXT(write_data[0], read_data[0], "Read value does not match new write value");
}

/*
 Check that the data it being written to flash in the right order of bytes
 according to endianess.
 */
TEST(Test, write_to_double_word_flash_little_endian)
{
    // 1. Initialize the flash structure with a double word (8) write size.
    uint8_t word_size = 8;
    REINIT_FLASH_CUSTOM(8, FLASH_ENDIANESS_LITTLE);

    // 2. Write a value to flash that is less than the write length.
    uint8_t write_data[1] = {0x01};
    CHECK_EQUAL(FLASH_OK, flash_write(0, &write_data[0], 1));

    // 3. Read out the first #word_length of flash and check that there's padding.
    uint8_t read_data[word_size] = {0};
    CHECK_EQUAL(FLASH_OK, flash_read(0, &read_data[0], word_size));

    // 4. Check that the value of the read data
    uint8_t expected_data[word_size] = {0};
    memset(expected_data, FLASH_EMPTY_VALUE, word_size);
    expected_data[0] = {0x01};
    MEMCMP_EQUAL_TEXT(expected_data, read_data, word_size, "Data is not padded correctly");
}

/*
There is a maximum amount of data that you can write at a time.
Return error if the maximum write size is exceeded.
The limit exists because there's a buffer internal to the drive that's for padding.
*/
TEST(Test, maximum_write_size_enforced)
{
    // 1. Initialize the flash structure with a double word (8) write size.
    uint8_t word_size = 8;
    REINIT_FLASH_CUSTOM(word_size, FLASH_ENDIANESS_LITTLE);

    // 2. Try to write more than the maximum number of bytes to the array.
    uint16_t write_size = 1025;
    uint8_t write_data[1025] = {0x00};
    CHECK_EQUAL(FLASH_ERROR, flash_write(0, write_data, write_size));
}

/*
You cannot write a 0 length amount of data to flash.
*/
TEST(Test, length_0_data)
{
    // 1. Initialize the flash structure with a double word (8) write size.
    uint8_t word_size = 8;
    REINIT_FLASH_CUSTOM(word_size, FLASH_ENDIANESS_LITTLE);

    // 2. Try to write more than the maximum number of bytes to the array.
    uint16_t write_size = 0;
    uint8_t write_data[1] = {0x00};
    CHECK_EQUAL(FLASH_ERROR, flash_write(0, write_data, write_size));
}

/*
The libary will pad out your write request with 0xFFs if its not an integral multiple of the word_size.
Pad with 0xFF as this doesn't require a change on the flash.
*/
TEST(Test, write_request_are_padded)
{
    // 1. Initialize the flash structure with a double word (8) write size.
    uint8_t word_size = 8;
    REINIT_FLASH_CUSTOM(word_size, FLASH_ENDIANESS_LITTLE);

    // 2. Write a value to flash that is less than the word size.
    uint8_t write_data[word_size - 1] = {0x01};
    CHECK_EQUAL(FLASH_OK, flash_write(0, &write_data[0], 1));

    // 3. Read back the word from the write address
    uint8_t read_data[word_size] = {0};
    CHECK_EQUAL(FLASH_OK, flash_read(0, &read_data[0], word_size));

    // 4. Check that the last byte of the read back word is padding (0xFF).
    CHECK_EQUAL_TEXT(FLASH_EMPTY_VALUE, read_data[word_size - 1], "Read back data padding incorrect.");
}

/*
The address that is being written to must be word_size aligned. That is address % word_size == 0 or error.
*/
TEST(Test, write_size_aligned)
{
    // Automatically initialized with word size 8

    // Try to write with non aligned address. Should get error.
    uint8_t write_data[WORD_SIZE] = {0};
    uint32_t user_address = WORD_SIZE - 1;
    CHECK_EQUAL_TEXT(FLASH_ERROR, flash_write(user_address, &write_data[0], 1), "Address not word aligned");
}

/*
If you try to write over an area of memory that has isn't clear (all set to empty) you get an error.
*/
TEST(Test, write_to_non_empty_flash)
{
    // Write to some flash
    uint8_t write_data[WORD_SIZE] = {0};
    uint32_t user_address = 0;
    uint8_t number_of_words = 1;
    CHECK_EQUAL(FLASH_OK, flash_write(user_address, write_data, number_of_words));

    // Try to write to the same spot again. Should be error.
    CHECK_EQUAL_TEXT(FLASH_ERROR, flash_write(user_address, write_data, number_of_words), "Write over written data should fail.");
}

/*
If a cell is written to you can't write to it again without erasing it. So erasing it should fix this.
So write. Write again and error. Erase. Write and OK.
*/
TEST(Test, can_write_to_erased_cell_after_write)
{
    // Write to page 0
    uint8_t write_data[WORD_SIZE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    WRITE_OK_TEXT(START_PAGE * PAGE_SIZE, write_data, 1, "Write the first time.");

    // Try to write to the same address again
    WRITE_ERROR_TEXT(START_PAGE * PAGE_SIZE, write_data, 1, "Write the second time.");

    // Erase the page
    ERASE_OK_TEXT(START_PAGE, 1, "Erase page 0");

    // Try to write again
    WRITE_OK_TEXT(START_PAGE * PAGE_SIZE, write_data, 1, "Write the third time after erase.");
}

/* Cannot try to erase a page > than the max page */
TEST(Test, cannot_erase_a_page_exceeding_flash_size)
{
    // Try to erase page > flash size
    uint8_t pages_in_flash = FLASH_SIZE / PAGE_SIZE;
    // Because the pages are zero based you can't try to erase the nth page.
    ERASE_ERROR_TEXT(pages_in_flash, 1, "Failed to fail at erasing too many pages.");
}

/* Cannot erase page count that exceed limit*/
TEST(Test, cannot_erase_more_pages_than_proceeding_start_pages)
{
    // Try to erase page > flash size
    uint8_t pages_in_flash = FLASH_SIZE / PAGE_SIZE;
    // Because the pages are zero based you can't try to erase the nth page.
    ERASE_ERROR_TEXT(pages_in_flash - 1, 2, "Failed to fail at erasing too many pages.");
}

/* Cannot try to enter an address > than the max address */
TEST(Test, cannot_write_read_to_address_exceeding_flash_size)
{
    uint32_t max_address = (START_PAGE + NUMBER_PAGES)*PAGE_SIZE;

    // Try to read to address should fail.
    uint8_t data = 0;
    READ_ERROR_TEXT(max_address, &data, 1, "Read failed to fail");
    
    // Try to write to address should fail.
    uint8_t write_data = 0;
    WRITE_ERROR_TEXT(max_address, &write_data, 1, "Write failed to fail");
}

/* Register a new flash index and store the id */
TEST(Test, register_a_new_index)
{
    int id = 0;
    REGISTER_ID_OK_TEXT(1, 2, id, "Failed to register a new tracker.");
}

/* 
When you register a new user flash you can't have any overlap in the memory that they occupy.
*/
TEST(Test, register_indices_check_overlap)
{
    int id = 0;
    REGISTER_ID_OK_TEXT(1,5, id,  "Failed to register a new index.");
    REGISTER_ID_ERROR_TEXT(1,2, id, "Did not fail to register a new index.");
}

/* 
When you register a new user flash you can't have any overlap in the memory that they occupy.
*/
TEST(Test, register_indices_check_start_page_overlap_with_end_page)
{
    int id = 0;
    REGISTER_ID_OK_TEXT(1,5, id,  "Failed to register a new index.");
    REGISTER_ID_ERROR_TEXT(5,2, id, "Did not fail to register a new index.");
}


/*
Should not be able to register an index that exceeds the user space.
*/
TEST(Test, cannot_register_index_with_out_of_bounds_page)
{
    // Pages are zero based counting so if you say end_page is number of pages it won't work.
    int id = 0;
    REGISTER_ID_ERROR_TEXT(0,NUMBER_PAGES, id, "Failed to fail to register a new index.");
}

/*
Should not be able to register an index if the user space isn't initalized.
*/
TEST(Test, cannot_register_index_if_user_flash_not_initialized)
{
    teardown();
    int id;
    REGISTER_ID_ERROR_TEXT(0,1, id, "Failed to fail to register a new index.");
    setup();
}

/*
Write to some user flash using the index read back the data.
*/
TEST(Test, write_with_index)
{
    // Register
    uint8_t index_id;
    REGISTER_ID_OK_TEXT(1,2,index_id,"Register index");

    // Write some data
    uint8_t write_data[WORD_SIZE] = {0x01, 0x02, 0x03, 0x04};
    WRITE_INDEX_OK_TEXT(index_id, write_data, WORD_SIZE, "Failned to write using index.");

    // Read the data
    uint8_t read_data[WORD_SIZE] = {0};
    READ_INDEX_OK_TEXT(index_id, read_data, WORD_SIZE, "Failed to read back data");
    
    // Check the value of the read data
    MEMCMP_EQUAL_TEXT(write_data, read_data, WORD_SIZE, "Write and read do not match");
}

/*
What happens when you want to write data beyond the edge of your area.
Wrap Write
*/
TEST(Test, write_data_over_edge)
{
    uint16_t write_size = PAGE_SIZE + 1;

    // Register an index
    int id = 0;
    REGISTER_ID_OK_TEXT(START_PAGE,START_PAGE+1,id, "Failed to register new index");

    // Try to write beyond the end it needs to wrap. Note that start page is dedicated to the index object.
    uint8_t write_data[write_size] = {0};
    write_data[write_size-1] = 0x01;    // Last byte to be 0x01 so that it's written to start of page.
    WRITE_INDEX_OK_TEXT(id,write_data, write_size, "Failed to write data");

    // Read back the data it should be all 0 with a 0x01 at address 0 because we are writing
    // a page size + 1 and the total size of the data area is 1 page.
    uint8_t read_data[PAGE_SIZE] = {0};
    READ_INDEX_OK_TEXT(id, read_data, PAGE_SIZE, "Read back data failed"); 

    // Expected data
    uint8_t expected_data[PAGE_SIZE] = {0};
    memset(expected_data, FLASH_EMPTY_VALUE, PAGE_SIZE);    // Set to empty val as page should have been erased to accomadate wrap. 
    expected_data[0] = 0x01;

    // Compare
    MEMCMP_EQUAL_TEXT(expected_data, read_data, PAGE_SIZE, "Written doesn't match expected");
}

/*
You shouldn't be able to register an index that's outside of the allowed page allocated area. Added an allowable start page parameter to the flash init./
*/
TEST(Test, cannot_register_index_out_of_user_area)
{
    int id = 0;
    REGISTER_ID_ERROR_TEXT(0, 1, id, "Failed to fail to register");
}

/* 
Test that function to retrieve the head is working correctly.
*/
TEST(Test, retrieve_head_of_index)
{
    // Register index
    int id = 0;
    REGISTER_ID_OK_TEXT(START_PAGE,START_PAGE+1, id, "Index was not registered");

    // Compare to expected. Remember that the 1st page of an index belongs to the index itself.
    uint32_t expected_head = (START_PAGE+1)*PAGE_SIZE;
    LONGS_EQUAL_TEXT(expected_head, flash_index_get_head(id), "Head doesn't match expected.");

    // Write to index
    uint8_t write_data[1] = {0};
    WRITE_INDEX_OK_TEXT(id, write_data, 1, "failed to write");
    
    // Retrieve head again
    expected_head += WORD_SIZE;

    // Compare to expected
    LONGS_EQUAL_TEXT(expected_head, flash_index_get_head(id), "Head doesn't match expected after write");
}


/*
Cannot set position relative to head ahead of head, therefore position always < 0.
*/
TEST(Test, position_ahead_of_head)
{
    // Register index
    int id = 0;
    REGISTER_ID_OK_TEXT(START_PAGE,START_PAGE+1, id, "Index was not registered");
    
    // Read from -5 of page to +5
    uint8_t read_data[10] = {0};
    CHECK_EQUAL(FLASH_ERROR, flash_index_read_rel_head(id, 5, read_data, 10));
}

/*
When you request read over head position then the read address needs to wrap back to the beginning of the index.
*/
TEST(Test, read_over_head_wrap)
{
    // Register index
    int id = 0;
    REGISTER_ID_OK_TEXT(START_PAGE,START_PAGE+1, id, "Index was not registered");
    
    // Write complete page. This will cause a wrap as doing a full page.
    uint8_t write_data[PAGE_SIZE] = {0};
    for(uint8_t idx = 0; idx < PAGE_SIZE; idx++)
    {
        write_data[idx] = idx; 
    } 
    WRITE_INDEX_OK_TEXT(id, write_data, PAGE_SIZE, "Failed to write page");

    // Check head is wrapped back to the start of the page
    CHECK_EQUAL_TEXT((START_PAGE+1)*PAGE_SIZE, flash_index_get_head(id), "Head not located at right spot");

    // Read from -5 of page to +5. 
    uint8_t read_data[10] = {0};
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_read_rel_head(id, -5, read_data, 10), "Read relative to head failed.");

    // Compare read values with expected
    uint8_t expected_data[10] = {PAGE_SIZE - 5, PAGE_SIZE - 4, PAGE_SIZE - 3, PAGE_SIZE - 2, PAGE_SIZE - 1, 0, 1, 2, 3, 4};
    MEMCMP_EQUAL_TEXT(expected_data, read_data, 10, "Data not equivalent");
}

/*
Make sure after write that index head is word aligned
*/
TEST(Test, make_sure_head_is_word_aligned)
{
    int id = 0;

    uint8_t start_page = START_PAGE+1;
    uint8_t end_page = START_PAGE + 1;
    uint32_t min_address = start_page * PAGE_SIZE;
    uint32_t max_address = (end_page + 1) * PAGE_SIZE;

    REGISTER_ID_OK_TEXT(START_PAGE, end_page, id, "Failed to reg index");

    // Write some portion of a word to flash
    uint8_t write_data[WORD_SIZE + 1] = {0};
    WRITE_INDEX_OK_TEXT(id, write_data, WORD_SIZE/2, "Failed to write");
    // The head should have advanded an entire word even though only a partial word was written.
    uint32_t head = flash_index_get_head(id);
    CHECK_EQUAL_TEXT(min_address + WORD_SIZE, head, "Head didn't match expected 1");

    // Write a word and a byte advancing the head by 2 words
    WRITE_INDEX_OK_TEXT(id, write_data, WORD_SIZE + 1 , "Failed to write");

    // Check that the head is pointed at where it should be.
    uint32_t new_head = (head + 2*WORD_SIZE)%max_address;

    if(new_head < min_address)
    {
        new_head += min_address;
    }

    CHECK_EQUAL_TEXT(new_head, flash_index_get_head(id), "Head didn't match expected 2");
}   

/*
Test function that erases all memory in an index.
*/
TEST(Test, erase_entire_index)
{
    // Register
    int id = 0;
    REGISTER_ID_OK_TEXT(START_PAGE, START_PAGE + 1, id, "failed to register an index");

    // Write entire index
    uint8_t write_data[PAGE_SIZE] = {0};
    WRITE_INDEX_OK_TEXT(id, write_data, PAGE_SIZE, "failed to write page to index");

    // Erase entire index
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_erase_all_data(id), "Failed to erase index");

    // Read back page
    uint8_t read_data[PAGE_SIZE] = {0};
    READ_INDEX_OK_TEXT(id, read_data, PAGE_SIZE, "Failed to read data");

    // Check entire index empty
    uint8_t expected_data[PAGE_SIZE] = {0};
    memset(expected_data, FLASH_EMPTY_VALUE, PAGE_SIZE);
    MEMCMP_EQUAL_TEXT(expected_data, read_data, PAGE_SIZE, "Page isn't empty");
}

/*
When you write data over the edge of the memory space and there's only one page of memory for data then you want the whole data to be
written at the start of the page, not partially at the end of the last page and then the rest at the start of the page because the
portion written at the end of the page will be erased.
*/
TEST(Test, write_data_over_edge_single_page)
{

    int id = 0;
    REGISTER_ID_OK_TEXT(1,2,id, "Failed to register new index");

    // Fill the page minus a few words
    uint16_t write_size = PAGE_SIZE - 3*WORD_SIZE;
    uint8_t write_data[write_size] = {0};
    WRITE_INDEX_OK_TEXT(id,write_data, write_size, "Failed to write data");

    // Write something that's greater than the remaining space in the page
    uint16_t write_size2 = 4*WORD_SIZE;
    uint8_t write_data2[write_size2] = {0};
    memset(write_data2, 0x0A, write_size2);
    WRITE_INDEX_OK_TEXT(id, write_data2, write_size2, "Failed to write second byte array.");

    // The final write should be present and complete at the start of the page
    uint8_t read_data[write_size2] = {0};
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_read_rel_head(id, -write_size2, read_data, write_size2), "Read relative to head failed.");

    // Expected data 
    uint8_t expected_data[write_size2] = {0};
    memset(expected_data, 0x0A, write_size2);    // Set to empty val as page should have been erased to accomadate wrap. 

    // Compare
    MEMCMP_EQUAL_TEXT(expected_data, read_data, write_size2, "Written doesn't match expected");
}

/* 
Read back the index data position before write, index data shouldn't be written.
*/
TEST(Test, index_position_not_found)
{
    // Register index
    int id = 0;
    REGISTER_ID_OK_TEXT(START_PAGE, START_PAGE + 1, id, "Failed to reg");

    // The index page should initially have no data written.
    uint32_t index_address = 0;
    CHECK_EQUAL(FLASH_DATA_NOT_FOUND, flash_index_get_index_address(id, &index_address));
}

/* 
 Write index data to flash.
 Verify the index data is in the write place.
*/
TEST(Test, write_index_check_position)
{
    // 1. Register index
    int id = 0;
    uint8_t index_start_page = START_PAGE;
    uint32_t index_start_address = index_start_page * PAGE_SIZE;
    REGISTER_ID_OK_TEXT(index_start_page, START_PAGE + 1, id, "Failed to reg");

    // 2. Write the index data to flash. This will increment the position of the index data.
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_write_index(id), "Failed to write index data to flash.");

    // 3. Read the last position of the index data on the index page
    uint32_t last_index_address = 0;
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_get_index_address(id, &last_index_address), "Last index position either error or not found.");

    // 4. Check that the address of the last index data is expected
    uint32_t expected_address = index_start_address;
    CHECK_EQUAL_TEXT(expected_address, last_index_address, "Position read from index flash page doesn't match expected.");

    /** DO IT AGAIN **/

    // 5. Write the index data to flash. This will increment the position of the index data.
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_write_index(id), "Failed to write index data to flash. 2");

    // 6. Read the last position of the index data on the index page
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_get_index_address(id, &last_index_address), "Last index position either error or not found.");

    // 7. Check that the address of the last index data is expected
    expected_address = index_start_address + WORD_SIZE;
    CHECK_EQUAL_TEXT(expected_address, last_index_address, "Position read from index flash page doesn't match expected.");
}

/* Write index data n times */
TEST(Test, write_index_n_times)
{
    // Register index
    int id = 0;
    uint8_t index_start_page = START_PAGE;
    REGISTER_ID_OK_TEXT(index_start_page, START_PAGE + 1, id, "Failed to reg");

    uint8_t n = 113;
    for(uint8_t i = 0; i < n; i++)
    {
        // Write the index data to flash. This will increment the position of the index data.
        CHECK_EQUAL_TEXT(FLASH_OK, flash_index_write_index(id), "Failed to write index data to flash.");
    }

    // Get the last position of the index data on the index page
    uint32_t index_start_address = index_start_page * PAGE_SIZE;
    uint32_t index_end_address = (index_start_page+1) * PAGE_SIZE;
    uint32_t last_index_address = 0;
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_get_index_address(id, &last_index_address), "Last index position either error or not found.");

    // Check that the address of the last index data is expected
    uint16_t bytes_written = (n-1)*(ALIGNED_BYTES(8));

    uint32_t expected_address = bytes_written % index_end_address;
    if(expected_address < index_start_address)
    {
        expected_address += index_start_address;
    }

    CHECK_EQUAL_TEXT(expected_address, last_index_address, "Position read from index flash page doesn't match expected.");
}

/* 
When you write data to flash with an index the index head and tail save location should move up in index' index page of flash.
*/
TEST(Test, write_to_flash_index_data_position_increases_when_you_write_with_index)
{
    // 1. Register index
    int id = 0;
    uint8_t index_page = START_PAGE;
    uint32_t index_start_address = index_page * PAGE_SIZE;
    uint32_t index_end_address = (index_page +1)* PAGE_SIZE;
    REGISTER_ID_OK_TEXT(index_page, START_PAGE + 1, id, "Failed to reg");

    // 2. Write data to flash to advance the index position
    uint8_t write_size = WORD_SIZE;
    uint8_t write_data[WORD_SIZE] = {0};
    WRITE_INDEX_OK_TEXT(id, write_data, write_size, "Failed to write data");

    // 3. Get the address of the index
    uint32_t last_index_address = 0;
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_get_index_address(id, &last_index_address), "Failed to read index");

    // 4. We only wrote once so the address should be at the beggining of the page
     // Check that the address of the last index data is expected
    uint16_t bytes_written = 0;

    uint32_t expected_address = bytes_written % index_end_address;
    if(expected_address < index_start_address)
    {
        expected_address += index_start_address;
    }

    CHECK_EQUAL_TEXT(expected_address, last_index_address, "Position read from index flash page doesn't match expected.");
}

/*
Reset the local index head and tail
*/
TEST(Test, reset_local_index_head_and_tail)
{
    // 1. Register an index
    int id = 0;
    REGISTER_ID_OK_TEXT(START_PAGE, START_PAGE + 1, id, "Failed to register new index");

    // 2. Write to the index data. This will modify the head.
    uint8_t write_data[WORD_SIZE] = {0};
    CHECK_EQUAL(FLASH_OK,flash_index_write(id, write_data, WORD_SIZE));
    
    // 2. Store the index head
    uint32_t old_head = flash_index_get_head(id);
    
    // 3. Reset the index
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_reset(id), "Failed to reset the index head and tail");

    // 4. Store the new index head
    uint32_t new_head = flash_index_get_head(id);
    
    // 5. Compare the old index head to the new index head. They should not be equal.
    CHECK_TEXT(old_head != new_head, "Old head does not equal new head");
}

/*
Need to be able to load index information into an index object from flash from the index page. 
*/
TEST(Test, load_index_head_and_tail_from_index_page_in_flash)
{
    // 1. Register the index
    int id = 0;
    REGISTER_ID_OK_TEXT(START_PAGE, START_PAGE + 1, id, "Failed to register new index");

    // 2. Write some data to modify the flash index
    uint8_t write_data[WORD_SIZE] = {0};
    CHECK_EQUAL(FLASH_OK,flash_index_write(id, write_data, WORD_SIZE));

    // 3. Store the current register head
    uint32_t old_head = flash_index_get_head(id);

    // 4. Reset the index
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_reset(id), "Failed to reset the index head and tail");

    // 5. Load the index from flash
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_load(id),"Failed to load index from flash");

    // 6. Store the head
    uint32_t new_head = flash_index_get_head(id);

    // 7. Compare the new head to the old head
    CHECK_EQUAL_TEXT(old_head, new_head, "Old head does not equal new head");
}

/*
Check that I can write to the index heaps of times, then reset the local index data, then load the index
head and tail from the flash.
*/
TEST(Test, load_index_after_index_reset_and_many_writes)
{
    // 1. Register the index
    int id = 0;
    REGISTER_ID_OK_TEXT(START_PAGE, START_PAGE + 1, id, "Failed to register new index");

    // 2. Write some data to modify the flash index
    for(uint8_t i = 0; i < 100; i++)
    {
        uint8_t write_data[WORD_SIZE] = {0};
        CHECK_EQUAL_TEXT(FLASH_OK,flash_index_write(id, write_data, WORD_SIZE), "Failed to write to flash");
    }

    // 3. Store the current register head
    uint32_t old_head = flash_index_get_head(id);

    // 4. Reset the index
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_reset(id), "Failed to reset the index head and tail");

    // 5. Load the index from flash
    CHECK_EQUAL_TEXT(FLASH_OK, flash_index_load(id),"Failed to load index from flash");

    // 6. Store the head
    uint32_t new_head = flash_index_get_head(id);

    // 7. Compare the new head to the old head
    CHECK_EQUAL_TEXT(old_head, new_head, "Old head does not equal new head");
}