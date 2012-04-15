[ORG 0x7c00]
[BITS 16]

; -------------------------------------------------------------------------------------------------
; Entrypoint for boot loader
; in: dl = bootdrive
entry:
        jmp short loader
        nop

times 0x3b db 0            ; BIOS parameter block

; Bios Parameter Block
%define boot_sector_base            0x7c00
%define bpb_oem                     boot_sector_base + 0x03
%define bpb_bytes_per_sector        boot_sector_base + 0x0b
%define bpb_sectors_per_cluster     boot_sector_base + 0x0d
%define bpb_reserved_sector_count   boot_sector_base + 0x0e
%define bpb_fat_count               boot_sector_base + 0x10
%define bpb_root_entry_count        boot_sector_base + 0x11
%define bpb_sector_count            boot_sector_base + 0x13
%define bpb_media_type              boot_sector_base + 0x15
%define bpb_sectors_per_fat         boot_sector_base + 0x16
%define bpb_sectors_per_track       boot_sector_base + 0x18
%define bpb_head_count              boot_sector_base + 0x1a
%define bpb_hidden_sector_count     boot_sector_base + 0x1c
%define bpb_large_sector_count      boot_sector_base + 0x20

; Extended Bios Parameter Block
%define ebpb_drive_number           boot_sector_base + 0x24
%define epbp_flags                  boot_sector_base + 0x25
%define epbp_signature              boot_sector_base + 0x26
%define epbp_volume_id              boot_sector_base + 0x27
%define epbp_volume_label           boot_sector_base + 0x2b
%define epbp_filesystem             boot_sector_base + 0x36

loader:
        mov [ebpb_drive_number], dl
        xor ax, ax
        mov ds, ax
        mov es, ax
        mov ss, ax
        mov sp, 0x7c00

        ; Clear Screen
        mov ax, 0x3
        int 0x10

        ; Print loading message
        mov si, msg_load
        call bios_print

        ; Find root directory
        xor eax, eax
        mov al, [bpb_fat_count]
        mul word[bpb_sectors_per_fat]
        add ax, [bpb_reserved_sector_count]

find_loader:
        mov bx, 0x8000
        mov di, bx
        call read_sector

.next_entry:
        mov cx, 0x0b
        mov si, filename_loader
        repe cmpsb                ; compare ds:si with es:di
        jz load_file

        add di, 0x20
        and di, -0x20
        cmp di, [bpb_bytes_per_sector]
        jnz .next_entry

        mov si, msg_failed
        call bios_print
        jmp $

load_file:
        mov ax, [di+0xf]
        mov bx, 0x8000

.next_cluster:
        call read_cluster
        cmp ax, 0xfff8
        jg .next_cluster

run_loader:
        mov dl, [ebpb_drive_number]
        jmp 0:0x8000

; -------------------------------------------------------------------------------------------------
; Reads a sector using LBA
; in:  eax = logical sector #
;      es:bx = destination buffer
; out: eax = next sector #
;      es:bx = destination buffer + sector size
read_sector:
        push si
        push di
        push dx

.retry:
        push eax
        mov di, sp                  ; Save stack pointer

        push byte 0                 ; High-32 bit sector to read
        push byte 0                 ; High-32 bit sector to read
        push eax                    ; Low-32 bit sector to read
        push es                     ; Destination Segment
        push bx                     ; Destination Offset
        push byte 1                 ; Number of sectors
        push byte 16                ; Size of parameter block

        mov si, sp
        mov ah, 0x42                ; Extended Read Sectors from Drive
        mov dl, [ebpb_drive_number]
        int 0x13

        mov sp, di                  ; Restore stack pointer
        pop eax

        jnc .read_ok                ; Check for success

        push ax
        xor ah, ah                  ; Reset Disk Drives
        int 0x13
        pop ax

        mov si, msg_failed
        call bios_print
        jmp .retry

.read_ok:
        ; increment to next sector
        inc eax
        add bx, 0x200
        jnc .exit

        ; increment to next segment
        mov dx, es
        add dh, 0x10
        mov es, dx

.exit:
        pop dx
        pop di
        pop si
        ret

; -------------------------------------------------------------------------------------------------
; Read a cluster using LBA
; in:  ax = cluster #
;      es:bx = destination buffer
; out: ax = next cluster #
;      es:bx = destination buffer + cluster size
read_cluster:
        push cx
        push dx
        push ax
        sub ax, 2
        mul byte[bpb_sectors_per_cluster]
        mov cx, ax

        xor eax, eax
        mov al, [bpb_fat_count]
        mul word[bpb_sectors_per_fat]
        add ax, [bpb_reserved_sector_count]
        add ax, cx

        mov cx, [bpb_root_entry_count]
        shr cx, 4                   ; (root_entry_count * 32) / 512
        add ax, cx

        xor cx, cx
        mov cl, [bpb_sectors_per_cluster]

.next_sector:
        call read_sector
        dec cx
        cmp cx, 0
        jne .next_sector

.next_cluster:
        pop ax                      ; Restore original cluster #

        push bx
        shl ax, 1
        div word[bpb_bytes_per_sector]
        add ax, [bpb_reserved_sector_count]

        mov bx, 0x7e00
        call read_sector

        mov bx, 0x7e00
        add bx, dx
        mov ax, [bx]
        pop bx

.exit:
        pop dx
        pop cx
        ret

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
msg_load db 'Booting...', 13, 10, 0
msg_failed db 'Read Failure', 13, 10, 0
filename_loader db 'LOADER  BIN'

; ----------------------------------------------------------------------------------------
; Footer
times 510-($-$$) db 0    ; Fill boot sector
dw 0xAA55                ; Boot loader signature
