#include <generated/csr.h>
#include <irq.h>
#include <uart.h>

void isr(void);
void isr(void)
{
	unsigned int irqs;

	irqs = irq_pending() & irq_getmask();
    
    /* IMPORTANT:
     * Do NOT miss UART Interrupt! I had mistakenly commented out
     * below lines including uart ones too. Which resulted in code
     * not working, and I spent almost 8 hours debugging the code,
     * disassembling, memory maps and what not, only to find that,
     * UART handler has been disabled! #Sigh
     * 
     * DO NOT repeat my mistake!! 
     */ 
	if(irqs & (1 << UART_INTERRUPT))
		uart_isr();
	
    /*if(irqs & (1 << HDMI_IN0_INTERRUPT))
		hdmi_in0_isr();
	if(irqs & (1 << HDMI_IN1_INTERRUPT))
		hdmi_in1_isr();
    */
}
