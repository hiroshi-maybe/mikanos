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
    mov [rsp], di   ; move limit in the RDI register (first arg)
    mov [rsp + 2], rsi  ; move offset in the RSI register (second arg)
    lidt [rsp]
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
