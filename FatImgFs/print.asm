

    section .text

global	_Z7myPrintPKc
global	_Z8redPrintPKc

_Z7myPrintPKc:
    push    rax
    push    rdi
    push    rsi
    push    rdx
    mov rsi,    rdi
nextChar:
    cmp byte[rsi],    0
    je  endPrint
    mov rax,    1
    mov rdi,    1
    mov rdx,    1
    syscall
    inc rsi
    jmp nextChar
endPrint:
    pop rdx
    pop rsi
    pop rdi
    pop rax
    ret

_Z8redPrintPKc:
    push    rax
    push    rdx
    push    rsi
    push    rdi
    mov rax,    1
    mov rdi,    1
    mov rsi,    startRed
    mov rdx,    startRedLen
    syscall
    pop rdi
    mov rsi,    rdi
redNextChar:
    cmp byte[rsi],    0
    je  redEndPrint
    mov rax,    1
    mov rdi,    1
    mov rdx,    1
    syscall
    inc rsi
    jmp redNextChar
redEndPrint:
    mov rax,    1
    mov rdi,    1
    mov rsi,    endRed
    mov rdx,    endRedLen
    syscall
    pop rsi
    pop rdx
    pop rax
    ret

    section .data:
startRed:   db  1Bh,"[31m"
startRedLen equ $-startRed

endRed: db  1Bh,"[0m"
endRedLen   equ $-endRed
