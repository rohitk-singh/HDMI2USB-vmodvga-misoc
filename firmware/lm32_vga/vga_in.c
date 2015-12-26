#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include <irq.h>
#include <uart.h>
#include <time.h>
#include <system.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include <hw/flags.h>

#include "vga_in.h"

int vga_in_debug;
int vga_in_fb_index;

#define FRAMEBUFFER_COUNT 4
#define FRAMEBUFFER_MASK (FRAMEBUFFER_COUNT - 1)

#define BYTES_PER_PIXEL 2 // should be changed for 32-bits per pixel

#define VGA_IN_FRAMEBUFFERS_BASE 0x00000000
#define VGA_IN_FRAMEBUFFERS_SIZE 80*80*BYTES_PER_PIXEL  //intially was 1280*720*2


unsigned int vga_in_framebuffer_base(char n) {
	return VGA_IN_FRAMEBUFFERS_BASE + n*VGA_IN_FRAMEBUFFERS_SIZE;
}

static int vga_in_fb_slot_indexes[2];
static int vga_in_next_fb_index;
static int vga_in_hres, vga_in_vres;

void vga_in_isr(void)
{
	int fb_index = -1;
	int length;
	int expected_length;
	unsigned int address_min, address_max;

	address_min = VGA_IN_FRAMEBUFFERS_BASE & 0x0fffffff;
	address_max = address_min + VGA_IN_FRAMEBUFFERS_SIZE*FRAMEBUFFER_COUNT;
	if((vga_in_dma_slot0_status_read() == DVISAMPLER_SLOT_PENDING)
		&& ((vga_in_dma_slot0_address_read() < address_min) || (vga_in_dma_slot0_address_read() > address_max)))
		printf("vga_in: slot0: stray DMA\n");
	if((vga_in_dma_slot1_status_read() == DVISAMPLER_SLOT_PENDING)
		&& ((vga_in_dma_slot1_address_read() < address_min) || (vga_in_dma_slot1_address_read() > address_max)))
		printf("vga_in: slot1: stray DMA\n");
    
    /*
     * NOT implemented right now, since we are testing only DMA
     * 
     * 
	if((vga_in_resdetection_hres_read() != vga_in_hres)
	  || (vga_in_resdetection_vres_read() != vga_in_vres)) {
		// Dump frames until we get the expected resolution 
		if(vga_in_dma_slot0_status_read() == DVISAMPLER_SLOT_PENDING) {
			vga_in_dma_slot0_address_write(vga_in_framebuffer_base(vga_in_fb_slot_indexes[0]));
			vga_in_dma_slot0_status_write(DVISAMPLER_SLOT_LOADED);
		}
		if(vga_in_dma_slot1_status_read() == DVISAMPLER_SLOT_PENDING) {
			vga_in_dma_slot1_address_write(vga_in_framebuffer_base(vga_in_fb_slot_indexes[1]));
			vga_in_dma_slot1_status_write(DVISAMPLER_SLOT_LOADED);
		}
		return;
	}
    */
    

	expected_length = vga_in_hres*vga_in_vres*BYTES_PER_PIXEL;
	if(vga_in_dma_slot0_status_read() == DVISAMPLER_SLOT_PENDING) {
		length = vga_in_dma_slot0_address_read() - (vga_in_framebuffer_base(vga_in_fb_slot_indexes[0]) & 0x0fffffff);
		if(length == expected_length) {
			fb_index = vga_in_fb_slot_indexes[0];
			vga_in_fb_slot_indexes[0] = vga_in_next_fb_index;
			vga_in_next_fb_index = (vga_in_next_fb_index + 1) & FRAMEBUFFER_MASK;
		} else
			printf("vga_in: slot0: unexpected frame length: %d\n", length);
		vga_in_dma_slot0_address_write(vga_in_framebuffer_base(vga_in_fb_slot_indexes[0]));
		vga_in_dma_slot0_status_write(DVISAMPLER_SLOT_LOADED);
	}
	if(vga_in_dma_slot1_status_read() == DVISAMPLER_SLOT_PENDING) {
		length = vga_in_dma_slot1_address_read() - (vga_in_framebuffer_base(vga_in_fb_slot_indexes[1]) & 0x0fffffff);
		if(length == expected_length) {
			fb_index = vga_in_fb_slot_indexes[1];
			vga_in_fb_slot_indexes[1] = vga_in_next_fb_index;
			vga_in_next_fb_index = (vga_in_next_fb_index + 1) & FRAMEBUFFER_MASK;
		} else
			printf("vga_in: slot1: unexpected frame length: %d\n", length);
		vga_in_dma_slot1_address_write(vga_in_framebuffer_base(vga_in_fb_slot_indexes[1]));
		vga_in_dma_slot1_status_write(DVISAMPLER_SLOT_LOADED);
	}

	if(fb_index != -1)
		vga_in_fb_index = fb_index;
	/* processor_update(); Not needed now */
}

static int vga_in_connected;
static int vga_in_locked;

void vga_in_init_video(int hres, int vres)
{
	unsigned int mask;
    
    printf("\nVGA_in: Initializing...\n");
    
	//hdmi_in0_clocking_pll_reset_write(1);  //Unimplemented as of now
	vga_in_connected = vga_in_locked = 0;
	vga_in_hres = hres; vga_in_vres = vres;

	vga_in_dma_frame_size_write(hres*vres*BYTES_PER_PIXEL); // Initially it was hres*vres*2, but since we re using only 1024*768 counter, 
	vga_in_fb_slot_indexes[0] = 0;
	vga_in_dma_slot0_address_write(vga_in_framebuffer_base(0));
	vga_in_dma_slot0_status_write(DVISAMPLER_SLOT_LOADED);
	vga_in_fb_slot_indexes[1] = 1;
	vga_in_dma_slot1_address_write(vga_in_framebuffer_base(1));
	vga_in_dma_slot1_status_write(DVISAMPLER_SLOT_LOADED);
	vga_in_next_fb_index = 2;

	vga_in_dma_ev_pending_write(vga_in_dma_ev_pending_read());
	vga_in_dma_ev_enable_write(0x3);
	mask = irq_getmask();
	mask |= 1 << VGA_IN_INTERRUPT;
	irq_setmask(mask);

	vga_in_fb_index = 3;
}

void vga_in_disable(void)
{
	unsigned int mask;

	mask = irq_getmask();
	mask &= ~(1 << VGA_IN_INTERRUPT);
	irq_setmask(mask);

    vga_in_frame_start_counter_write(0); // stop counter
	vga_in_dma_slot0_status_write(DVISAMPLER_SLOT_EMPTY);
	vga_in_dma_slot1_status_write(DVISAMPLER_SLOT_EMPTY);
	//vga_in_clocking_pll_reset_write(1); Not implemented as of now
}

void vga_in_clear_framebuffers(void)
{
    printf("\nVGA_in: Clearing FBs...\n");
	int i;
	flush_l2_cache();
	volatile unsigned int *framebuffer = (unsigned int *)(MAIN_RAM_BASE + VGA_IN_FRAMEBUFFERS_BASE);
	
    /*
     * IMPORTANT: Review this code: "i<(VGA_IN_FRAMEBUFFERS_SIZE*FRAMEBUFFER_COUNT)/4;"
     * 
     * VERY VERY IMPORTANT
     * 
     * UPDATE: Would work fine for now. Since BYTES_PER_PIXEL=4 ie 32-bits per pixel. 
     *         Hence, each framebuffer[i] refers to each pixel. Just set it to zeros.
     */
    for(i=0; i<(VGA_IN_FRAMEBUFFERS_SIZE*FRAMEBUFFER_COUNT)/8; i++) { //4
		//framebuffer[i] = 0x80108010; /* black in YCbCr 4:2:2*/
        framebuffer[i] = 0x00000000; /* black*/
	}
}

static void vga_in_check_overflow(void)
{
	if(vga_in_frame_overflow_read()) {
		printf("vga_in: FIFO overflow\n");
		vga_in_frame_overflow_write(1);
	}
}

void vga_in_dump_fb(void)
{
    printf("\nDumping vga_in fb data and registers\n");
    int i;
    unsigned int upper=0, lower=0;
    flush_l2_cache();
    volatile unsigned int *framebuffer = (unsigned int *)(MAIN_RAM_BASE + VGA_IN_FRAMEBUFFERS_BASE);
    volatile unsigned int temp =0 ;
    
    for (i=0; i<(VGA_IN_FRAMEBUFFERS_SIZE*FRAMEBUFFER_COUNT)/4; i++) { //4 initially
        lower = framebuffer[i] >> 16;
        upper = framebuffer[i] & 0x0000ffff;
        printf("%u %u ", lower, upper);
    }  
    
    printf("\n\nRegisters:\n");
    printf("vga_in_dma_frame_size: %d\n", vga_in_dma_frame_size_read());
    printf("vga_in_frame_start_counter: %d\n", vga_in_frame_start_counter_read());
    printf("vga_in_frame_overflow: %d\n", vga_in_frame_overflow_read());
    printf("Sizeof unsigned int: %d\n\n", sizeof(unsigned int));
}

void vga_in_start(void)
{
    printf("\nStarting vga_in...\n");
    vga_in_disable();
    vga_in_clear_framebuffers();
    vga_in_frame_start_counter_write(1); // Start counter
    vga_in_init_video(80, 80);
}

void vga_in_test(void)
{
    
    
    
}

void vga_in_service(void)
{
	static int last_event;
    
    /* NOT IMPLEMENTED AS OF NOW
     * 
	if(hdmi_in0_connected) {
		if(!hdmi_in0_edid_hpd_notif_read()) {
			if(hdmi_in0_debug)
				printf("dvisampler0: disconnected\n");
			hdmi_in0_connected = 0;
			hdmi_in0_locked = 0;
			hdmi_in0_clocking_pll_reset_write(1);
			hdmi_in0_clear_framebuffers();
		} else {
			if(hdmi_in0_locked) {
				if(hdmi_in0_clocking_locked_filtered()) {
					if(elapsed(&last_event, identifier_frequency_read()/2)) {
						hdmi_in0_adjust_phase();
						if(hdmi_in0_debug)
							hdmi_in0_print_status();
					}
				} else {
					if(hdmi_in0_debug)
						printf("dvisampler0: lost PLL lock\n");
					hdmi_in0_locked = 0;
					hdmi_in0_clear_framebuffers();
				}
			} else {
				if(hdmi_in0_clocking_locked_filtered()) {
					if(hdmi_in0_debug)
						printf("dvisampler0: PLL locked\n");
					hdmi_in0_phase_startup();
					if(hdmi_in0_debug)
						hdmi_in0_print_status();
					hdmi_in0_locked = 1;
				}
			}
		}
	} else {
		if(hdmi_in0_edid_hpd_notif_read()) {
			if(hdmi_in0_debug)
				printf("dvisampler0: connected\n");
			hdmi_in0_connected = 1;
			hdmi_in0_clocking_pll_reset_write(0);
		}
	}
    */
	vga_in_check_overflow();
}


/* NOT IMPLEMENTED Functions
 * 
 *
static int hdmi_in0_d0, hdmi_in0_d1, hdmi_in0_d2;

void hdmi_in0_print_status(void)
{
	hdmi_in0_data0_wer_update_write(1);
	hdmi_in0_data1_wer_update_write(1);
	hdmi_in0_data2_wer_update_write(1);
	printf("dvisampler0: ph:%4d %4d %4d // charsync:%d%d%d [%d %d %d] // WER:%3d %3d %3d // chansync:%d // res:%dx%d\n",
		hdmi_in0_d0, hdmi_in0_d1, hdmi_in0_d2,
		hdmi_in0_data0_charsync_char_synced_read(),
		hdmi_in0_data1_charsync_char_synced_read(),
		hdmi_in0_data2_charsync_char_synced_read(),
		hdmi_in0_data0_charsync_ctl_pos_read(),
		hdmi_in0_data1_charsync_ctl_pos_read(),
		hdmi_in0_data2_charsync_ctl_pos_read(),
		hdmi_in0_data0_wer_value_read(),
		hdmi_in0_data1_wer_value_read(),
		hdmi_in0_data2_wer_value_read(),
		hdmi_in0_chansync_channels_synced_read(),
		hdmi_in0_resdetection_hres_read(),
		hdmi_in0_resdetection_vres_read());
}

static int wait_idelays(void)
{
	int ev;

	ev = 0;
	elapsed(&ev, 1);
	while(hdmi_in0_data0_cap_dly_busy_read()
	  || hdmi_in0_data1_cap_dly_busy_read()
	  || hdmi_in0_data2_cap_dly_busy_read()) {
		if(elapsed(&ev, identifier_frequency_read() >> 6) == 0) {
			printf("dvisampler0: IDELAY busy timeout\n");
			return 0;
		}
	}
	return 1;
} 
 
int hdmi_in0_calibrate_delays(void)
{
	hdmi_in0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_CAL|DVISAMPLER_DELAY_SLAVE_CAL);
	hdmi_in0_data1_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_CAL|DVISAMPLER_DELAY_SLAVE_CAL);
	hdmi_in0_data2_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_CAL|DVISAMPLER_DELAY_SLAVE_CAL);
	if(!wait_idelays())
		return 0;
	hdmi_in0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_RST|DVISAMPLER_DELAY_SLAVE_RST);
	hdmi_in0_data1_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_RST|DVISAMPLER_DELAY_SLAVE_RST);
	hdmi_in0_data2_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_RST|DVISAMPLER_DELAY_SLAVE_RST);
	hdmi_in0_data0_cap_phase_reset_write(1);
	hdmi_in0_data1_cap_phase_reset_write(1);
	hdmi_in0_data2_cap_phase_reset_write(1);
	hdmi_in0_d0 = hdmi_in0_d1 = hdmi_in0_d2 = 0;
	return 1;
}

int hdmi_in0_adjust_phase(void)
{
	switch(hdmi_in0_data0_cap_phase_read()) {
		case DVISAMPLER_TOO_LATE:
			hdmi_in0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_DEC);
			if(!wait_idelays())
				return 0;
			hdmi_in0_d0--;
			hdmi_in0_data0_cap_phase_reset_write(1);
			break;
		case DVISAMPLER_TOO_EARLY:
			hdmi_in0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_INC);
			if(!wait_idelays())
				return 0;
			hdmi_in0_d0++;
			hdmi_in0_data0_cap_phase_reset_write(1);
			break;
	}
	switch(hdmi_in0_data1_cap_phase_read()) {
		case DVISAMPLER_TOO_LATE:
			hdmi_in0_data1_cap_dly_ctl_write(DVISAMPLER_DELAY_DEC);
			if(!wait_idelays())
				return 0;
			hdmi_in0_d1--;
			hdmi_in0_data1_cap_phase_reset_write(1);
			break;
		case DVISAMPLER_TOO_EARLY:
			hdmi_in0_data1_cap_dly_ctl_write(DVISAMPLER_DELAY_INC);
			if(!wait_idelays())
				return 0;
			hdmi_in0_d1++;
			hdmi_in0_data1_cap_phase_reset_write(1);
			break;
	}
	switch(hdmi_in0_data2_cap_phase_read()) {
		case DVISAMPLER_TOO_LATE:
			hdmi_in0_data2_cap_dly_ctl_write(DVISAMPLER_DELAY_DEC);
			if(!wait_idelays())
				return 0;
			hdmi_in0_d2--;
			hdmi_in0_data2_cap_phase_reset_write(1);
			break;
		case DVISAMPLER_TOO_EARLY:
			hdmi_in0_data2_cap_dly_ctl_write(DVISAMPLER_DELAY_INC);
			if(!wait_idelays())
				return 0;
			hdmi_in0_d2++;
			hdmi_in0_data2_cap_phase_reset_write(1);
			break;
	}
	return 1;
}

int hdmi_in0_init_phase(void)
{
	int o_d0, o_d1, o_d2;
	int i, j;

	for(i=0;i<100;i++) {
		o_d0 = hdmi_in0_d0;
		o_d1 = hdmi_in0_d1;
		o_d2 = hdmi_in0_d2;
		for(j=0;j<1000;j++) {
			if(!hdmi_in0_adjust_phase())
				return 0;
		}
		if((abs(hdmi_in0_d0 - o_d0) < 4) && (abs(hdmi_in0_d1 - o_d1) < 4) && (abs(hdmi_in0_d2 - o_d2) < 4))
			return 1;
	}
	return 0;
}

int hdmi_in0_phase_startup(void)
{
	int ret;
	int attempts;

	attempts = 0;
	while(1) {
		attempts++;
		hdmi_in0_calibrate_delays();
		if(hdmi_in0_debug)
			printf("dvisampler0: delays calibrated\n");
		ret = hdmi_in0_init_phase();
		if(ret) {
			if(hdmi_in0_debug)
				printf("dvisampler0: phase init OK\n");
			return 1;
		} else {
			printf("dvisampler0: phase init failed\n");
			if(attempts > 3) {
				printf("dvisampler0: giving up\n");
				hdmi_in0_calibrate_delays();
				return 0;
			}
		}
	}
}

static int hdmi_in0_clocking_locked_filtered(void)
{
	static int lock_start_time;
	static int lock_status;

	if(hdmi_in0_clocking_locked_read()) {
		switch(lock_status) {
			case 0:
				elapsed(&lock_start_time, -1);
				lock_status = 1;
				break;
			case 1:
				if(elapsed(&lock_start_time, identifier_frequency_read()/4))
					lock_status = 2;
				break;
			case 2:
				return 1;
		}
	} else
		lock_status = 0;
	return 0;
}

*/

