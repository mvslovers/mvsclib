; stringa.asm - string assembler functions
;
; This program written by Paul Edwards, kerravon@w3.to
; Released to the public domain
;
; Mods by Rowan Crowe

.386p

.model flat

_DATA   segment dword public use32 'DATA'
_DATA   ends
_BSS    segment dword public use32 'BSS'
_BSS    ends

DGROUP  group   _DATA,_BSS
    assume cs:_TEXT,ds:DGROUP

_TEXT segment 'CODE'

public memcpy
memcpy proc
push ebp
mov ebp, esp
pushf
push edi
push esi
push ecx
mov edi, [ebp+8]
mov eax, edi    ; used for return value
mov esi, [ebp+12]
mov ecx, [ebp+16]
cld
shr ecx, 2
rep movsd
mov cl, byte ptr [ebp+16] ; ECX = 0 so we can safely manipulate
and cl, 3                 ; the bottom byte only
rep movsb
pop ecx
pop esi
pop edi
popf
pop ebp
ret
memcpy endp

_TEXT ends

end
