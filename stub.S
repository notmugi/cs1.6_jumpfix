.intel_syntax noprefix
.section .text.entry,"ax"
.global entry
entry:
    call 1f
1:  pop ecx
    sub ecx, offset 1b        /* ELF load bias */
    mov edx, [ecx+0xde3c88]  /* outgoing sequence */
    cmp dword ptr [ecx+0xde3c40],5
    sete al
    movzx eax,al
    cmp dword ptr [ecx+0xde9410],0  /* demo playback */
    jne inactive
    cmp dword ptr [ecx+0xde9734],0  /* spectator proxy */
    jne inactive
    cmp dword ptr [ecx+0xc5a9e0],0  /* paused */
    je args
inactive: xor eax,eax
args:
    push eax
    push dword ptr [ecx+0x94d67c]
    push dword ptr [ecx+0x94d678]
    push edx
    /* DL/EAX originally held msec; read saved integer at [original esp+16].
       Engine clamps EAX to 255 separately, so repeat its upper bound. */
    mov eax,[esp+32]
    cmp eax,255
    jle 2f
    mov eax,255
2:  push eax
    imul eax,ebx,76
    lea eax,[ecx+eax+0xd67024]
    push eax
    lea ecx,[ecx+state]
    push ecx
    call sample
    add esp,28
    test eax,eax
    js skip
    mov edx,eax
    lea ecx,[ebx+ebx*8]
    lea ecx,[ebx+ecx*2]
    jmp resume
skip: jmp done
.section .note.GNU-stack,"",@progbits
.section .text.entry,"ax"
.ifndef MOVEMENT_ONLY
.global camera
camera:
    push esi
    mov esi,[esp+8]
    sub esp,20
    mov eax,[esi+100]
    mov [esp],eax
    mov eax,[esi+104]
    mov [esp+4],eax
    mov eax,[esi+108]
    mov [esp+8],eax
    call 3f
3:  pop ecx
    sub ecx,offset 3b
    fild dword ptr [ecx+state]
    fadd qword ptr [ecx+0x35f7a0]
    sub esp,8
    fstp qword ptr [esp]
    push dword ptr [ecx+0x94d67c]
    push dword ptr [ecx+0x94d678]
    push dword ptr [ecx+0xde3c88]
    push esi
    lea eax,[ecx+state+64]
    push eax
    call blend
    add esp,28
    /* initial ESP was 12 mod 16; 24 bytes saved, then 4 for arg */
    push esi
    call original_view
    add esp,4
    mov eax,[esp]
    mov [esi+100],eax
    mov eax,[esp+4]
    mov [esi+104],eax
    mov eax,[esp+8]
    mov [esi+108],eax
    add esp,20
    pop esi
    ret
.endif
