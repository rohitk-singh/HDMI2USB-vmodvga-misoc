#include <stdio.h>
#include <stdlib.h>

#include <irq.h>
#include <uart.h>
#include <time.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include <hw/flags.h>
#include <console.h>

#include "ci.h"
#include "i2c.h"
#include "ad9984a.h"

int main(void)
{
	irq_setmask(0);
	irq_setie(1);
	uart_init();
    time_init();
    i2c_init();
    
	puts("\nHDMI2USB-vModVGA firmware  http://hdmi2usb.tv/");
	printf("Revision %08x built "__DATE__" "__TIME__"\n", MSC_GIT_ID);
    
    ad9984a_init();
    ci_prompt();
    
	while(1) {
		ci_service();
	}

	return 0;
}


