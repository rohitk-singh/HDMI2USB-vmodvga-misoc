#ifndef __AD9984A_H
#define __AD9984A_H

/* AD9984A 7-bit i2c address */
#define AD9984A 0x4C

#define NUM_REG_READ 50
#define NUM_REG_WRITE 22

const unsigned char regReadAddress[NUM_REG_READ];
const unsigned char regWriteAddress[NUM_REG_WRITE];
const unsigned char regWriteValue[NUM_REG_WRITE];

unsigned char ad9984a_read(unsigned char reg);
void ad9984a_write(unsigned char reg, unsigned char value);
void ad9984a_debug(void);
void ad9984a_init(void);
void ad9984a_status(void);

#endif /* __AD9984A_H */
