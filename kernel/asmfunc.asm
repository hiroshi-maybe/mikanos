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
