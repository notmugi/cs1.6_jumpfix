/* Exercise the actual staged x86 stub at a nonzero ELF load bias without loading
 * or executing the game. Stand-ins for the two engine continuations return
 * msec or -1 so the test sees which branch the stub took. */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include "move.c"
extern int invoke(void *entry,int ms);
__asm__(".text\ninvoke: push %ebx; xor %ebx,%ebx; sub $20,%esp; mov 32(%esp),%eax; mov %eax,16(%esp); jmp *28(%esp)\n");
int main(void){
 unsigned char *b=mmap(0,0x1400000,PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);assert(b!=MAP_FAILED);
 FILE *f=fopen("hw.so","rb");assert(f && fread(b,1,0x2bae0c,f)==0x2bae0c);fclose(f);
 /* trampoline returns to invoke's caller, balancing its pretend CL_Move frame */
 memcpy(b+0x13a4b1,"\x89\xd0\x83\xc4\x14\x5b\xc3",7);
 memcpy(b+0x13a5a0,"\xb8\xff\xff\xff\xff\x83\xc4\x14\x5b\xc3",10);
 /* stand-in client copies simorg to vieworg; ret at original call return site */
 memcpy(b+0x158be0,"\x8b\x44\x24\x04\x8b\x50\x64\x89\x10\xc3",10);
 b[0x169cab]=0xc3;
 assert(mprotect(b,0x2bb000,PROT_READ|PROT_EXEC)==0);
 /* Use a nonzero snapshot slot (leave slot 0 invalid for play). */
 unsigned char *frame=b+7*17176;
 *(int*)(b+0xc5a5a8)=7; *(int*)(b+0xc5a5a0)=1;
 *(float*)(frame+0xc5d504)=100;
 *(int*)(b+0xde3c40)=5; Cmd *c=(void*)(b+0xd67024); int count=0;
 for(int i=1;i<=240;i++){
   memset(c,0,sizeof *c);c->buttons=i==1?2:0;c->move[0]=250;
   *(double*)(b+0x94d678)=i/240.; *(int*)(b+0xde3c88)=count;
   int r=invoke(b+0x13a4ab,i*1000/240-(i-1)*1000/240);
   if(r<0)continue;assert(r==10 && c->buttons==(count?0:2) && c->move[0]==250);count++;
 }
 assert(count==100); puts("staged x86 path: 240 frames, 100 commands; tap preserved");
 /* Observe through a healthy target: stock duration, no stale input on respawn. */
 c->buttons=2; assert(invoke(b+0x13a4ab,4)==-1); c->buttons=0;
 *(int*)(frame+0xc5d650)=4; assert(invoke(b+0x13a4ab,3)==3);
 *(int*)(frame+0xc5d650)=0; assert(invoke(b+0x13a4ab,6)==-1);
 assert(invoke(b+0x13a4ab,4)==10 && c->buttons==0);
 *(int*)(frame+0xc5d54c)=1; assert(invoke(b+0x13a4ab,2)==2); *(int*)(frame+0xc5d54c)=0;
 *(float*)(frame+0xc5d504)=0; assert(invoke(b+0x13a4ab,2)==2); *(float*)(frame+0xc5d504)=100;
 *(int*)(frame+0xc5aa34)=1; assert(invoke(b+0x13a4ab,2)==2); *(int*)(frame+0xc5aa34)=0;
#ifndef MOVEMENT_ONLY
 Ref r={.dt=.0025,.health=100,.entity=1};
 void (*view)(Ref*)=(void*)(b+0x169ca6);
 /* Enter through a normal aligned caller rather than the engine's in-frame call. */
 int rel;memcpy(&rel,b+0x169ca7,4);view=(void*)(b+0x169cab+rel);
 view(&r);r.pos[0]=4;(*(int*)(b+0xde3c88))++;
 for(int i=0;i<4;i++){*(int*)(b+0x13cf830)=2*i;view(&r);assert(r.pos[0]==4 && r.unused[0]>=.8f*i-.0001f && r.unused[0]<=.8f*i+.0001f);}
 *(int*)(frame+0xc5d650)=4; r.pos[0]=7; view(&r); assert(r.unused[0]==7);
 *(int*)(frame+0xc5d650)=0; r.pos[0]=9; view(&r); assert(r.unused[0]==9);
 r.pos[0]=512;view(&r);assert(r.unused[0]==512);r.paused=1;r.pos[0]=513;view(&r);assert(r.unused[0]==513);
 puts("staged camera: interpolated subframes, restored simorg, snapped teleport/pause");
#endif
}
