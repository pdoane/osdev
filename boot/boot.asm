[ORG 0x7c00]
[BITS 16]

; -------------------------------------------------------------------------------------------------
; Entrypoint for boot loader
; in: dl = bootdrive
entry:
		jmp short loader
		nop

; Bios Parameter Block
bpb:
.oem:							db "POS     "
.bytes_per_sector:				dw 0x0200
.sectors_per_cluster:			db 0x40
.reserved_sector_count:			dw 0x0001
.fat_count:						db 0x02
.root_entry_count:				dw 0x0200
.sector_count:					dw 0x0000
.media_type:					db 0xf8
.sectors_per_fat:				dw 0x00fb
.sectors_per_track:				dw 0x003f
.head_count:					dw 0x00ff
.hidden_sector_count:			dd 0x00000000
.large_sector_count:			dd 0x003ea000

; Extended Bios Parameter Block
epbp:
.drive_number:					db 0x80
.flags:							db 0x00
.signature:						db 0x29
.volume_id:						dd 0xa0a1a2a3
.volume_label:					db "Volume Name"
.filesystem:					db "FAT16   "

; -------------------------------------------------------------------------------------------------
; Code starts here
loader:
		cli
		xor ax, ax
		mov ds, ax
		mov es, ax
		mov ss, ax
		mov sp, 0x7c00
		sti

		; Clear Screen
		mov ax, 0x3
		int 0x10

		; Print loading message
		mov si, msg_load
		call bios_print

; -------------------------------------------------------------------------------------------------
; Prints a string to the screen
; in: si = address of string
bios_print:
		push ax
		mov ah, 0x0e
.loop:
		lodsb
		cmp al, 0
		je .done
		int 0x10
		jmp .loop
.done:
		pop ax
		ret

; ----------------------------------------------------------------------------------------
msg_load db 'Welcome!', 13, 10, 0
		
; ----------------------------------------------------------------------------------------
; Footer
times 510-($-$$) db 0	; Fill boot sector
dw 0xAA55				; Boot loader signature
