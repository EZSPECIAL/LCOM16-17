#include <minix/syslib.h>
#include <minix/drivers.h>
#include <machine/int86.h>

#include "vbe.h"
#include "lmlib.h"
#include "video.h"

int vbe_get_mode_info(unsigned short mode, vbe_mode_info_t* vmi_p) {
  
	mmap_t lmblock;

	//Initialize low memory and map it, roughly 1st MiB
	lm_init();

	//Allocate a memory block within that 1st MiB
	lm_alloc(MODE_INFO_SIZE, &lmblock);

	//VBE Call function 01h - return mode info
	struct reg86u reg86;
	reg86.u.w.ax = VBE_MODE_INFO;

	//Translate the buffer linear address to a far pointer
	reg86.u.w.es = PB2BASE(lmblock.phys);		//Physical address segment base
	reg86.u.w.di = PB2OFF(lmblock.phys); 		//Physical address offset

	reg86.u.w.cx = LINEAR_FRAME | mode;	//VBE mode to get info from, assuming linear framebuffer
	reg86.u.b.intno = BIOS_VIDEO;

	if(sys_int86(&reg86) != OK) {
		printf("vga: vbe_get_mode_info: sys_int86() failed\n");
		return -1;
	}

	//Check function support in AL register
	if(reg86.u.b.al != FSUPPORTED) {
		printf("vga: vbe_get_mode_info: mode 0x%02X is not supported\n", mode);
		return -1;
	}

	//Copy struct from low memory to vbe_mode_info_t struct passed by address
	memcpy(vmi_p, lmblock.virtual, MODE_INFO_SIZE);

	//Free low memory block allocated previously
	lm_free(&lmblock);

	return 0;
}
