#ifndef __VGA_IN_H
#define __VGA_IN_H

extern int vga_in_debug;
extern int vga_in_fb_index;

unsigned int vga_in_framebuffer_base(char n);

void vga_in_isr(void);
void vga_in_init_video(int hres, int vres);
void vga_in_disable(void);
void vga_in_clear_framebuffers(void);
void vga_in_print_status(void);
int vga_in_calibrate_delays(void);
int vga_in_adjust_phase(void);
int vga_in_init_phase(void);
int vga_in_phase_startup(void);
void vga_in_service(void);

#endif /* __VGA_IN_H */
