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

void delay(void);

void delay(void)
 {
    volatile unsigned int i=0;
    for (i=1048576; i>0; i--)
    {
        i++;
        i--;
    }
      
}


int main(void)
{
	irq_setmask(0);
	irq_setie(1);
    led_r_leds_write(0xAA);
	uart_init();
    time_init();
    i2c_init();
    led_r_leds_write(0x55);
	puts("\nHDMI2USB firmware  http://timvideos.us/");
    led_r_leds_write(0xF0);
	printf("Revision %08x built "__DATE__" "__TIME__"\n", MSC_GIT_ID);
    led_r_leds_write(0x0F);
    printf("Hello Rohit!\n");
    ad9984a_init();
    led_r_leds_write(51);
    unsigned char flag = 0, flag1 = 0;
    
    unsigned char static counter = 0;
	while(1) {
		delay();
		counter++;
        printf("Counter: %d\n", counter);
        ad9984a_status();
        led_r_leds_write(counter);
        if (flag)
        {
            i2c_start_cond();
            i2c_write(0x38 << 1);
            i2c_write(0x55);
            i2c_stop_cond();
            
            i2c_start_cond();
            i2c_write(0x39 << 1);
            i2c_write(0x55);
            i2c_stop_cond();
        }
        else
        {
            i2c_start_cond();
            i2c_write(0x38 << 1);
            i2c_write(0xAA);
            i2c_stop_cond();
            
            i2c_start_cond();
            i2c_write(0x39 << 1);
            i2c_write(0xAA);
            i2c_stop_cond();
        }
        flag = !flag;
        
        if (counter==127)
        {
            flag1 = !flag1;
            if (flag1)
            {
                
                ad9984a_write(0x01, 0x54);
                ad9984a_write(0x02, 0x00);
            }
            else
            {
                ad9984a_write(0x01, 0x69);
                ad9984a_write(0x02, 0xd0);
            }
            
        }
	}

	return 0;
}


