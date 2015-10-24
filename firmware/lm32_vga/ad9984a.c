#include <stdio.h>
#include <stdlib.h>

#include <irq.h>
#include <uart.h>
#include <time.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include <hw/flags.h>
#include <console.h>

#include "i2c.h"
#include "ad9984a.h"


const unsigned char regReadAddress[NUM_REG_READ] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 
0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 
0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 
0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 
0x2D, 0x2E, 0x34, 0x36, 0x3C};

const unsigned char regWriteAddress[NUM_REG_WRITE] = {0x01, 0x02, 0x03, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 
0x18, 0x1B, 0x1C, 0x1D, 0x20, 0x21, 0x22, 0x28, 0x29, 0x2C, 0x2D, 0x2E, 
0x3C};

const unsigned char regWriteValue[NUM_REG_WRITE] =   {0x54, 0x00, 0xA0, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 
0x20, 0x73, 0xFF, 0x7F, 0x0B, 0x20, 0x32, 0xBF, 0x02, 0x00, 0xE8, 0xE0, 
0x0E};

unsigned char ad9984a_read(unsigned char reg)
{
    unsigned char chip_addr = AD9984A;  // 7-bit, Other possible adr=0x4D
    unsigned char data = 0x00;
    
    // For read sequence, see AD9984A datasheet page 23
    i2c_start_cond();
    i2c_write(chip_addr << 1 | 0x00); // write instruction
    i2c_write(reg);
    i2c_start_cond(); 
    i2c_write(chip_addr << 1 | 0x01); // read instruction
    data = i2c_read(0);
    i2c_stop_cond();
    
    return data;
}

void ad9984a_write(unsigned char reg, unsigned char value)
{
    unsigned char chip_addr = AD9984A; // 7-bit
    
    // For write sequence, see AD9984A datasheet page 23
    i2c_start_cond();
    i2c_write(chip_addr << 1 | 0x00); // write instruction
    i2c_write(reg);
    i2c_write(value);
    i2c_stop_cond();
}
    
void ad9984a_status(void)
{
    printf("[AD9984A] Register : 0x%02x, Value : 0x%02x\n", 0x01, ad9984a_read(0x01));
    printf("[AD9984A] Register : 0x%02x, Value : 0x%02x\n", 0x02, ad9984a_read(0x02));
    unsigned int hsync_count = 0;
    hsync_count = (ad9984a_read(regReadAddress[38]) << 4) + (ad9984a_read(regReadAddress[39]) >> 4);
    printf("[AD9984A] No. of hsync per vsync: %d\n", hsync_count);
}

void ad9984a_debug(void)
{
    unsigned char i=0;
    for (i=0; i < NUM_REG_READ; i++)
    {
        printf("Register : 0x%02x, Value : 0x%02x\n", regReadAddress[i], ad9984a_read(regReadAddress[i]));
    }
}

void ad9984a_init(void)
{
    printf("\nAD9984A :: Current register values\n\n");
    ad9984a_debug();
    
    printf("\nAD9984A :: Writing new values...");
    unsigned char i=0;
    for (i=0; i < NUM_REG_WRITE; i++)
    {
        ad9984a_write(regWriteAddress[i], regWriteValue[i]);
    }
    printf("Done...\n");
    
    printf("\nAD9984A :: New register values\n\n");
    ad9984a_debug();
    puts("");
}
    
