.section .rodata

.balign 16
.global __sm_fw_begin
__sm_fw_begin:
	.incbin "sm.bin"
__sm_fw_end:
.global __sm_fw_end
.balign 16
