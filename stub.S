.intel_syntax noprefix
.section .text.entry,"ax"
.global entry
entry:
    call 1f
1:  pop ecx
    sub ecx, offset 1b        /* ELF load bias */
    call local_alive
    mov edx, [ecx+0xde3c88]  /* outgoing sequence */
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
/* Shared by movement and view. ECX is the load bias; return boolean in EAX.
   Use the local snapshot's observer mode, not the spectated target's health. */
local_alive:
    cmp dword ptr [ecx+0xde3c40],5      /* ca_active */
    jne inactive
    mov eax,[ecx+0xde9410]             /* demo playback */
    or eax,[ecx+0xde9734]              /* spectator proxy */
    or eax,[ecx+0xc5a9e0]              /* paused */
    or eax,[ecx+0xc5aa00]              /* intermission */
    jne inactive
    cmp dword ptr [ecx+0xc5a5a0],0     /* cl.validsequence */
    je inactive
    mov eax,[ecx+0xc5a5a8]             /* cl.parsecountmod */
    cmp eax,64
    jae inactive
    imul eax,eax,17176                 /* sizeof(frame_t) */
    add eax,ecx
    mov edx,[eax+0xc5aa34]             /* frame.invalid */
    or edx,[eax+0xc5d650]              /* clientdata.iuser1 != OBS_NONE */
    or edx,[eax+0xc5d54c]              /* clientdata.deadflag != DEAD_NO */
    jne inactive
    xorps xmm0,xmm0
    movss xmm1,[eax+0xc5d504]          /* clientdata.health */
    ucomiss xmm1,xmm0
    jbe inactive
    mov eax,1
    ret
inactive:
    /* Either callback may see the transition first; discard both histories. */
    push edi
    push ecx
    lea edi,[ecx+state]
    xor eax,eax
    mov ecx,16                         /* reserved movement state: 64 bytes */
    rep stosd
.ifndef MOVEMENT_ONLY
    mov [edi],eax                      /* Camera.valid */
.endif
    pop ecx
    pop edi
    ret
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
    call local_alive
    test eax,eax
    jz stock_view
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
stock_view:
    add esp,20
    pop esi
    jmp original_view
.endif
