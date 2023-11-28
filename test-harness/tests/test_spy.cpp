#include "CppUTest/TestHarness.h"

extern "C"
{
#include <string.h>
#include "../spies/flash_spy.h"
}

TEST_GROUP(TestSpy)
{
    #define WORD_SIZE 8
    #define PAGE_SIZE 16
    #define FLASH_SIZE 1024

    void setup()
    {
        flash_spy_init(WORD_SIZE, PAGE_SIZE, FLASH_SIZE);
    }

    void teardown()
    {
        flash_spy_deinit();
    }

    #define REINIT_FLASH_CUSTOM(word_size) \
        flash_spy_deinit(); \
        flash_spy_init(word_size, PAGE_SIZE, FLASH_SIZE);

    #define FLASH_READ_OK(address, data_ptr, size) \
        CHECK_EQUAL_TEXT(FLASH_OK, flash_spy_read(address, data_ptr, size), "Flash read error");

    #define READ_OK_TEXT(address, data_ptr, size, text) \
        CHECK_EQUAL_TEXT(FLASH_OK, flash_spy_read(address, data_ptr, size), text);
    

    #define WRITE_OK(address, data_ptr, size) \
        CHECK_EQUAL_TEXT(FLASH_OK, flash_spy_write(address, data_ptr, size), "Flash write error");

    #define WRITE_OK_TEXT(address, data_ptr, size, text) \
        CHECK_EQUAL_TEXT(FLASH_OK, flash_spy_write(address, data_ptr, size), text);

   #define ERASE_OK_TEXT(start_page, quantity, text) \
        CHECK_EQUAL_TEXT(FLASH_OK, flash_spy_erase_pages(start_page, quantity), text);


};

// /* ZERO */

/* If you try to write to spy flash and the module isn't initialized then error. */
TEST(TestSpy, initial_state_zero)
{
    teardown();
    uint32_t user_address = 0;
    uint8_t data_length = 1;
    uint8_t data[data_length] = {0};
    CHECK_EQUAL_TEXT(FLASH_ERROR, flash_spy_write(user_address, data, data_length), "Uninitialized module should error");
    setup();
}

/*
Entire flash value needs to be empty (PADDING_VALUE) initially.
*/
TEST(TestSpy, initial_flash_value)
{
    // Read out all of the spy flash
    uint8_t read_data[FLASH_SIZE] = {0};
    CHECK_EQUAL_TEXT(FLASH_OK, flash_spy_read(0x00, read_data, FLASH_SIZE), "Uninitialized module should error");

    // Check that all of read data is equal to PADDING VALUE
    uint8_t expected_data[FLASH_SIZE] = {0};
    memset(expected_data, FLASH_EMPTY_VALUE, FLASH_SIZE);
    MEMCMP_EQUAL_TEXT(expected_data, read_data, FLASH_SIZE, "Initialized flash not all padding value.");
}

/* ONE */

/*
The smallest number of bytes that can be written to flash is the word_size of the spy flash.
Test this by writing a word and seeing that not just 1 byte is written but all the bytes in the
word.
*/
TEST(TestSpy, minimum_write_size_is_word)
{
    // Init with word size 8
    uint8_t word_size = 8;
    REINIT_FLASH_CUSTOM(word_size);

    // Write some data
    uint8_t number_words = 1;
    uint8_t write_data[word_size] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    CHECK_EQUAL_TEXT(FLASH_OK, flash_spy_write(0x00, write_data, number_words), "Failed to write to flash");

    // Read back the data
    uint8_t read_data[word_size] = {0};
    flash_spy_read(0x00, read_data, word_size);

    // Check that it matches what was written
    MEMCMP_EQUAL(write_data, read_data, word_size);
}

/*
User should not be able to write to cells that aren't erased. I.e. if the value of a word is not all 0xFF then error.
It won't check which cell failed it'll just return error if any cell is not ready to be written to.
*/
TEST(TestSpy, cannot_write_to_non_erased_cell)
{
    // Write to cell
    uint8_t number_words = 1;
    uint8_t write_data[WORD_SIZE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    CHECK_EQUAL_TEXT(FLASH_OK, flash_spy_write(0x00, &write_data[0], number_words), "Failed to write to flash");

    // Try to write to same cell and ERROR
    CHECK_EQUAL_TEXT(FLASH_ERROR, flash_spy_write(0x00, &write_data[0], number_words), "Did not fail to write to flash");
}


/*
Erasing a page should turn all the values on that page to empty (0xFF)
*/
TEST(TestSpy, erase_pages)
{
    uint8_t write_data[WORD_SIZE] = {0};
    // Write to page 0
    CHECK_EQUAL(FLASH_OK, flash_spy_write(0, write_data, 1));

    // Erase page
    CHECK_EQUAL(FLASH_OK, flash_spy_erase_pages(0, 1));;

    // Check that the value of page 0
    uint8_t read_data[PAGE_SIZE] = {0};
    CHECK_EQUAL_TEXT(FLASH_OK, flash_spy_read(0, &read_data[0], PAGE_SIZE), "Flash read error");
    uint8_t expected_value[PAGE_SIZE] = {0};
    memset(expected_value, FLASH_EMPTY_VALUE, PAGE_SIZE);
    MEMCMP_EQUAL(expected_value, read_data, PAGE_SIZE);
}

/*
Write to multiple pages only erase 1
*/
TEST(TestSpy, write_page_0_and_1_erase_0)
{
    // Write to page 0 and page 1
    uint8_t write_data[WORD_SIZE] = {0};
    WRITE_OK_TEXT(0, write_data, 1, "Failed to write to page 0");
    WRITE_OK_TEXT(PAGE_SIZE, write_data, 1, "Failed to write to page 1");

    // Erase page 0
    ERASE_OK_TEXT(0,1, "Failed to erase page 0");

    // Expected value of page 0
    uint8_t expected_value0[PAGE_SIZE] = {0};
    memset(expected_value0, FLASH_EMPTY_VALUE, PAGE_SIZE);

    // Expected value of page 1
    uint8_t expected_value1[PAGE_SIZE] = {0};
    memset(expected_value1, FLASH_EMPTY_VALUE, PAGE_SIZE);
    memset(expected_value1, 0, WORD_SIZE);                   

    // Read page 0
    uint8_t read_data0[PAGE_SIZE] = {0};
    READ_OK_TEXT(0, read_data0, PAGE_SIZE, "Read page 0");

    // Read page 1
    uint8_t read_data1[PAGE_SIZE] = {0};
    READ_OK_TEXT(PAGE_SIZE, read_data1, PAGE_SIZE, "Read page 1");

    // Check page 0 is empty
    MEMCMP_EQUAL_TEXT(expected_value0, read_data0, PAGE_SIZE, "Page 0 not equal to expected");

    // Check that page 1 is not empty
    MEMCMP_EQUAL_TEXT(expected_value1, read_data1, PAGE_SIZE, "Page 1 not equal to expected");
}

/*
Test writing to the guts of the flash
*/

/*
Write over 3 pages and erase the second one.
*/
TEST(TestSpy, write_pages_0_to_2_erase_1)
{
    uint32_t page0 = 0;
    uint32_t page1 = PAGE_SIZE;
    uint32_t page2 = PAGE_SIZE*2;

    // Write to pages
    uint8_t write_data[WORD_SIZE] = {0};
    WRITE_OK_TEXT(page0, write_data, 1, "Failed to write to page 0");
    WRITE_OK_TEXT(page1, write_data, 1, "Failed to write to page 1");
    WRITE_OK_TEXT(page2, write_data, 1, "Failed to write to page 2");

    // Set expected value before erase
    uint16_t area_size = 3 * PAGE_SIZE;
    uint8_t expected_before_erase[area_size] = {0};
    memset(expected_before_erase, FLASH_EMPTY_VALUE, area_size);
    memcpy(&expected_before_erase[page0], write_data, WORD_SIZE);
    memcpy(&expected_before_erase[page1], write_data, WORD_SIZE);
    memcpy(&expected_before_erase[page2], write_data, WORD_SIZE);

    // Check flash value
    uint8_t read_data_before_erase[area_size] = {0};
    READ_OK_TEXT(0, read_data_before_erase, area_size, "Failed to read pages before erase");
    MEMCMP_EQUAL_TEXT(expected_before_erase, read_data_before_erase, area_size, "Mem compare before erase.");

    // Erase page 1
    ERASE_OK_TEXT(1, 1, "Erase page 1 failed");

    // Expected value of 3 flash  pages after erase.
    uint8_t expected_value[area_size] = {0};
    memset(expected_value, FLASH_EMPTY_VALUE, area_size);
    memcpy(&expected_value[page0], write_data, WORD_SIZE);
    memcpy(&expected_value[page2], write_data, WORD_SIZE);

    // Read the data out of flash
    uint8_t read_data[area_size] = {0};
    READ_OK_TEXT(0, read_data, area_size, "Read pages 0 - 2 failed.");
    MEMCMP_EQUAL_TEXT(expected_value, read_data, area_size, "Mem compare after erase.");
}

