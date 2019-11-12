    section .text

myPrint:
    push    rax
    push    rbx
    push    rcx
    push    rdx
    mov rcx,    rdi
nextChar:
    cmp byte[rcx],    0
    je  endPrint
    mov rax,    4
    mov rbx,    1
    mov rdx,    1
    int 80h
    inc rcx
    jmp nextChar
endPrint:
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ret

redPrint:
    push    rax
    push    rbx
    push    rcx
    push    rdx
    mov rax,    4
    mov rbx,    1
    mov rcx,    startRed
    mov rdx,    startRedLen
    int 80h
    mov rcx,    rdi
redNextChar:
    cmp byte[rcx],    0
    je  redEndPrint
    mov rdx,    1
    int 80h
    inc rcx
    jmp redNextChar
redEndPrint:
    mov rax,    4
    mov rbx,    1
    mov rcx,    endRed
    mov rdx,    endRedLen
    int 80h
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ret

    section .data:
startRed:   db  33, "[31m"
startRedLen equ $-startRed

endRed: db  33, "[0m"
endRedLen   equ $-endRed