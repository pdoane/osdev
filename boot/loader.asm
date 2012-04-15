[ORG 0x8000]
[BITS 16]

; -------------------------------------------------------------------------------------------------
; Entrypoint for OS loader
loader:
        ; Print loading message
        mov si, msg_load
        call bios_print

        jmp test

; -------------------------------------------------------------------------------------------------
; Filler to test multi-cluster loading
times 2048 db 0

; -------------------------------------------------------------------------------------------------
test:
        mov si, msg_success
        call bios_print

        jmp $

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
msg_load db 'Loading Kernel...', 13, 10, 0
msg_success db 'Welcome!', 13, 10, 0