; -------------------------------------------------------------------------------------------------
; keymap.asm
; -------------------------------------------------------------------------------------------------

keymap:
        db 0            ; 0x00
        db 0            ; 0x01 - escape
        db '1'          ; 0x02
        db '2'          ; 0x03
        db '3'          ; 0x04
        db '4'          ; 0x05
        db '5'          ; 0x06
        db '6'          ; 0x07
        db '7'          ; 0x08
        db '8'          ; 0x09
        db '9'          ; 0x0a
        db '0'          ; 0x0b
        db '-'          ; 0x0c
        db '='          ; 0x0d
        db 0            ; 0x0e - backspace
        db 0            ; 0x0f - tab
        db 'q'          ; 0x10
        db 'w'          ; 0x11
        db 'e'          ; 0x12
        db 'r'          ; 0x13
        db 't'          ; 0x14
        db 'y'          ; 0x15
        db 'u'          ; 0x16
        db 'i'          ; 0x17
        db 'o'          ; 0x18
        db 'p'          ; 0x19
        db '['          ; 0x1a
        db ']'          ; 0x1b
        db 0            ; 0x1c - enter
        db 0            ; 0x1d - left control
        db 'a'          ; 0x1e
        db 's'          ; 0x1f
        db 'd'          ; 0x20
        db 'f'          ; 0x21
        db 'g'          ; 0x22
        db 'h'          ; 0x23
        db 'j'          ; 0x24
        db 'k'          ; 0x25
        db 'l'          ; 0x26
        db ';'          ; 0x27
        db "'"          ; 0x28
        db '`'          ; 0x29
        db 0            ; 0x2a - left shift
        db '\'          ; 0x2b
        db 'z'          ; 0x2c
        db 'x'          ; 0x2d
        db 'c'          ; 0x2e
        db 'v'          ; 0x2f
        db 'b'          ; 0x30
        db 'n'          ; 0x31
        db 'm'          ; 0x32
        db ','          ; 0x33
        db '.'          ; 0x34
        db '/'          ; 0x35
        db 0            ; 0x36 - right shift
        db '*'          ; 0x37
        db 0            ; 0x38 - left alt
        db ' '          ; 0x39
        times 128-($-keymap) db 0