[ORG 0x7c00]
[BITS 16]

; -------------------------------------------------------------------------------------------------
; Entrypoint for boot loader
; in: dl = bootdrive
entry:
        jmp short loader
        nop

times 0x3b db 0			; BIOS parameter block

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
