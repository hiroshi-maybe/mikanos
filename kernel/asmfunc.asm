; asmfunc.asm
;
; System V AMD64 Calling Convention
; Registers: RDI, RSI, RDX, RCX, R8, R9

bits 64
section .text

; void IoOut32(
;    uint16_t addr /* in RDI register */,
;    uint32_t data /* in RSI register */);
global IoOut32
IoOut32:
    mov dx, di  ; dx = addr
    mov eax, esi    ; eax = data
    out dx, eax ; OUT instruction writes to an I/O device
    ret

; uint32_t IoIn32(
;    uint16_t addr /* in RDI register */);
global IoIn32
IoIn32:
    mov dx, di  ; dx = addr
    in eax, dx  ; IN instruction reads from an I/O device
    ret

; uint16_t GetCS(void);
global GetCS
GetCS:
    xor eax, eax ; clears upper 32 bits of rax
    mov ax, cs
    ret

; void LoadIDT(uint16_t limit, uint64_t offset);
global LoadIDT
LoadIDT:
    push rbp
    mov rbp, rsp
    sub rsp, 10 ; make 10 bytes memory space (rsp = rbp - 10)
    mov [rsp], di   ; move limit (first arg) in the RDI register
    mov [rsp + 2], rsi  ; move offset (second arg) in the RSI register
    lidt [rsp]
    mov rsp, rbp
    pop rbp
    ret

; void LoadGDT(uint16_t limit, uint64_t offset);
global LoadGDT
LoadGDT:
    push rbp
    mov rbp, rsp
    sub rsp, 10 ; make 10 bytes memory space
    mov [rsp], di ; move limit (first arg) in the RDI register
    mov [rsp + 2], rsi ; move offset (second arg) in the RSI register
    lgdt [rsp] ; move offset and limit to GDT register
    mov rsp, rbp
    pop rbp
    ret

; void SetDSAll(uint16_t value);
global SetDSAll
SetDSAll:
    mov ds, di
    mov es, di
    mov fs, di
    mov gs, di
    ret

; void SetCSSS(uint16_t cs, uint16_t ss);
global SetCSSS
SetCSSS:
    push rbp
    mov rbp, rsp
    mov ss, si ; SS register points to gdt[2]
    mov rax, .next
    push rdi ; CS register points to gdt[1]
    push rax
    o64 retf
.next:
    mov rsp, rbp
    pop rbp
    ret

extern kernel_main_stack
extern KernelMainNewStack

global KernelMain
KernelMain:
    mov rsp, kernel_main_stack + 1024 * 1024
    call KernelMainNewStack
.fin:
    hlt
    jmp .fin
