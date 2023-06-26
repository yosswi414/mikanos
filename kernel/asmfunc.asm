; asmfunc.asm
;
; System V AMD64 Calling Convention
; Registers: RDI, RSI, RDX, RCX, R8, R9

bits 64
section .text

global IoOut32  ; void IoOut32(uint16_t addr, uint32_t data);
IoOut32:
    mov dx, di      ; dx = addr
    mov eax, esi    ; eax = data
    out dx, eax
    ret

global IoIn32  ; uint32_t IoIn32(uint16_t addr);
IoIn32:
    mov dx, di      ; dx = addr
    in eax, dx
    ret

global LoadIDT  ; void LoadIDT(uint16_t limit, uint64_t offset);
LoadIDT:
    push rbp
    mov rbp, rsp
    sub rsp, 10
    mov [rsp], di       ; limit
    mov [rsp + 2], rsi  ; offset
    lidt [rsp]
    mov rsp, rbp
    pop rbp
    ret

global LoadGDT  ; void LoadGDT(uint16_t limit, uint64_t offset)
LoadGDT:
    push rbp
    mov rbp, rsp
    sub rsp, 10
    mov [rsp], di       ; limit
    mov [rsp + 2], rsi  ; offset
    lgdt [rsp]
    mov rsp, rbp
    pop rbp
    ret

global GetCS    ; uint16_t GetCS(void);
GetCS:
    xor eax, eax
    mov ax, cs
    ret

global GetDS    ; uint16_t GetDS(void);
GetDS:
    xor eax, eax
    mov ax, ds
    ret

global GetES    ; uint16_t GetES(void);
GetES:
    xor eax, eax
    mov ax, es
    ret

global SetDSAll ; void SetDSAll(uint16_t value);
SetDSAll:
    mov ds, di
    mov es, di
    mov fs, di
    mov gs, di
    ret

global SetCSSS  ; void SetCSSS(uint16_t cs, uint16_t ss);
SetCSSS:
    push rbp
    mov rbp, rsp
    mov ss, si
    mov rax, .next
    push rdi    ; CS
    push rax    ; RIP
    o64 retf    ; o64: take 64-bit value
.next:
    mov rsp, rbp
    pop rbp
    ret

global SetCR3   ; void SetCR3(uint64_t value);
SetCR3:
    mov cr3, rdi
    ret

extern font_data
extern kernel_main_stack
extern KernelMainNewStack

global KernelMain
KernelMain:
;     push rsi
;     push rdi
;     push rax

;     ;mov rax, 19 * 128 / 8          ; KERNEL_GLYPH_HEIGHT * asciis(128) / (64 bits / KERNEL_GLYPH_WIDTH(8) bits)
;     mov rax, 19 * ('~' + 1) / 8     ; '~'(0x7e): last printable ascii character
;     mov rsi, rdx                    ; rdx: const FontBitmapData(UINT8*) font_pool
;     mov rdi, font_data
    
; .copyloop:
;     movsq
;     dec rax
;     jnz .copyloop

;     pop rax
;     pop rdi
;     pop rsi
    mov rsp, kernel_main_stack + 1024 * 1024
    call KernelMainNewStack
.fin:
    hlt
    jmp .fin

