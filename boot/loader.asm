; -------------------------------------------------------------------------------------------------
; loader.asm
; -------------------------------------------------------------------------------------------------

[ORG 0x8000]
[BITS 16]

%include "boot/defines.asm"

; -------------------------------------------------------------------------------------------------
; Entrypoint for OS loader
loader:
        cli
        cld
        xor ax, ax
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        mov sp, boot_sector_base

        lgdt [gdt.desc]

        mov si, msg_load            ; Print loading message
        call bios_print

        call enable_a20_gate
        call enable_unreal_mode
        call load_kernel
        call build_memory_map
        call build_vm_tables
        call enable_long_mode

        cli                         ; Jump to 64-bit code
        jmp gdt.code:long_mode

; -------------------------------------------------------------------------------------------------
; Enable A20 Gate
enable_a20_gate:
        in al, 0x92
        or al, 0x02
        out 0x92, al

        ret

; -------------------------------------------------------------------------------------------------
; Enable Unreal Mode
enable_unreal_mode:
        push ds                     ; Save real mode

        mov eax, cr0                ; Activate protected mode
        or al, 0x01
        mov cr0, eax

        mov bx, gdt.unreal          ; Load unreal descriptor
        mov ds, bx

        and al, 0xfe                ; Disable protected mode
        mov cr0, eax

        pop ds                      ; Restore real mode

        ret

; -------------------------------------------------------------------------------------------------
; Load Kernel
load_kernel:
        call find_root_file

        mov ax, [di+0xf]
        mov edi, kernel_base

.next_cluster:
        call transfer_cluster
        cmp ax, 0xfff8
        jg .next_cluster

        ret

; -------------------------------------------------------------------------------------------------
; Builds the e820 Memory map
build_memory_map:
    mov di, memory_map              ; Destination for memory map storage
    xor ebx, ebx                    ; State for BIOS call, set to 0 initially

.loop:
    mov eax, 0xe820                 ; Call int 0x15, 0xe820 memory map
    mov edx, 0x534D4150
    mov ecx, 24
    int 0x15

    jc .done                        ; Carry means unsupported or end of list

    cmp eax, 0x534D4150             ; EAX should match EDX
    jne .done

    jcxz .next_entry                ; Skip zero-length entries

    cmp cl, 20                      ; Test for ACPI 3.X entry
    jbe .good_entry

    test byte [es:di + 20], 1       ; Test ACPI 3.X ignore bit
    je .next_entry

.good_entry:
    add di, 24                      ; Found a valid entry

.next_entry:
    test ebx, ebx                   ; Go to next entry
    jne .loop

.done:
    xor ax, ax                      ; Write terminating entry
    mov cx, 12
    rep stosw
    ret

; ----------------------------------------------------------------------------------------
; Build 64-bit VM tables
build_vm_tables:
        ; Define one 2MB page at memory address 0.

        mov di, vm_pml4             ; PML4
        mov ax, vm_pdp + 0xf
        stosw
        xor ax, ax
        mov cx, 0x07ff
        rep stosw

        mov ax, vm_pd + 0xf         ; PDP
        stosw
        xor ax, ax
        mov cx, 0x07ff
        rep stosw

        mov ax, 0x018f              ; PD
        stosw
        xor ax, ax
        mov cx, 0x07ff
        rep stosw

        ret

; -------------------------------------------------------------------------------------------------
; Enable Long Mode
enable_long_mode:
        mov eax, 0x000000a0         ; Set PAE and PGE
        mov cr4, eax

        mov eax, vm_pml4            ; Assign PML4
        mov cr3, eax

        mov ecx, 0xc0000080         ; Read from EFER MSR
        rdmsr

        or eax, 0x00000100          ; Set LME
        wrmsr

        mov eax, cr0                ; Activate paging and protected mode
        or eax, 0x80000001
        mov cr0, eax

        ret

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
        inc eax                     ; Increment to next sector
        add bx, 0x200
        jnc .exit

        mov dx, es                  ; Increment to next segment
        add dh, 0x10
        mov es, dx

.exit:
        pop dx
        pop di
        pop si
        ret

; -------------------------------------------------------------------------------------------------
; Transfer a cluster using LBA
; in:  ax = cluster #
;      edi = destination buffer
; out: ax = next cluster #
;      edi = destination buffer + cluster size
transfer_cluster:
        push esi
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

        xor dx, dx
        mov dl, [bpb_sectors_per_cluster]

.next_sector:
        mov bx, temp_sector
        call read_sector

        mov esi, temp_sector
        mov ecx, 0x80
        a32 rep movsd

        dec dx
        jne .next_sector

.next_cluster:
        pop ax                      ; Restore original cluster #

        push bx
        shl ax, 1
        div word[bpb_bytes_per_sector]
        add ax, [bpb_reserved_sector_count]

        mov bx, temp_sector
        call read_sector

        mov bx, temp_sector
        add bx, dx
        mov ax, [bx]
        pop bx

.exit:
        pop dx
        pop cx
        pop esi
        ret

; -------------------------------------------------------------------------------------------------
; Find root file
; destroys: eax, bx, cx, si, di
find_root_file:
        xor eax, eax
        mov al, [bpb_fat_count]
        mul word[bpb_sectors_per_fat]
        add ax, [bpb_reserved_sector_count]

        mov bx, temp_sector
        mov di, bx
        call read_sector

.next_entry:
        mov si, filename
        mov cx, 0x0b
        repe cmpsb                ; compare ds:si with es:di
        jz .done

        add di, 0x20
        and di, -0x20
        cmp di, [bpb_bytes_per_sector]
        jnz .next_entry

        mov si, msg_failed
        call bios_print
        jmp $

.done:
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

; -------------------------------------------------------------------------------------------------
; Data
msg_load db 'Loading Kernel...', 13, 10, 0
msg_failed db 'Read Failure', 13, 10, 0
filename db 'KERNEL  BIN'

; -------------------------------------------------------------------------------------------------
gdt:
        dq 0x0000000000000000               ; Null Descriptor
.code equ $ - gdt                           ; Code segment
        dq 0x0020980000000000
.data equ $ - gdt                           ; Data segment
        dq 0x0000920000000000
.unreal equ $ - gdt                         ; Unreal segment
        db 0xff, 0xff, 0, 0, 0, 10010010b, 11001111b, 0

.desc:
        dw $ - gdt - 1                      ; 16-bit Size (Limit)
        dq gdt                              ; 64-bit Base Address

; -------------------------------------------------------------------------------------------------
[BITS 64]
long_mode:
        lgdt [gdt.desc]
        mov ax, gdt.data
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

        mov rsp, kernel_stack               ; Execute kernel
        jmp kernel_base
