#include "i8042.h"

.text
.global _kbd_asm_handler
_kbd_asm_handler:
	movl $0, %ecx
waitonbuf:
	xor %eax, %eax
	cmpl $TIMEOUT, %ecx			#checks if number of tries has reached max
	je timeout					#jumps to error state
	inc %ecx
	inb $STATUS_PORT, %al		#read input from the status register to %al
	testb $OUT_BUF_STATUS, %al 	#bitwise & to test bit 0 of status byte
	jz waitonbuf				#check zero flag

	xor %eax, %eax
	inb $KBD_OUT_BUF, %al		#read scancode from output port to %al
	ret
timeout:
	xor %eax, %eax
	movl $KBD_ERROR, %eax
	ret
