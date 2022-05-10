; winsupa.asm - support code for C programs for Win32
;
; This program written by Paul Edwards
; Released to the public domain

.386

.model flat,c

_DATA   segment dword 'DATA'
_DATA   ends
_BSS    segment dword 'BSS'
_BSS    ends

_TEXT segment 'CODE'


public __setj
__setj proc env:dword
        mov eax, env
        push ebx
        mov ebx, esp
        push ebx               ; esp

        mov [eax + 4], ecx
        mov [eax + 8], edx
        mov [eax + 12], edi
        mov [eax + 16], esi

        pop ebx
        mov [eax + 20], ebx    ; esp
        mov ebx, [ebp + 0]
        mov [eax + 24], ebx    ; ebp

        mov ebx, [ebp + 4]     ; return address
        mov [eax + 28], ebx    ; return address

        pop ebx
        mov [eax + 0], ebx
        mov eax, 0

        ret
__setj endp


public __longj
__longj proc env:dword
        mov eax, env
        mov ebp, [eax + 20]
        mov esp, ebp
        pop ebx                ; position of old ebx
        pop ebx                ; position of old ebp
        pop ebx                ; position of old return address

        mov ebx, [eax + 28]    ; return address
        push ebx

        mov ebx, [eax + 24]    ; ebp saved as normal
        push ebx
        mov ebp, esp

        mov ebx, [eax + 0]
        mov ecx, [eax + 4]
        mov edx, [eax + 8]
        mov edi, [eax + 12]
        mov esi, [eax + 16]

        mov eax, [eax + 32]    ; return value

        ret
__longj endp


; For compiling with GCC 4 we don't want to
; have to link the GCC library in
public __chkstk_ms
__chkstk_ms proc
        ret
__chkstk_ms endp


_TEXT ends

end
