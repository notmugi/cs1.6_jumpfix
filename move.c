/* Build 10211: called after stock input sampling and stock integer-ms accounting. */
typedef unsigned char u8;
typedef struct { short lerp; u8 ms, pad; float angles[3], move[3]; u8 light, unused; unsigned short buttons; u8 impulse, weapon; } Cmd;
typedef struct { int debt, seq; double last; unsigned short buttons; u8 impulse, weapon; float movement[3]; int sampled; } State;
_Static_assert(__builtin_offsetof(Cmd, buttons)==30 && __builtin_offsetof(Cmd, weapon)==33,"cmd ABI");
/* Returns -1 to keep sampling the same unsent engine command slot. */
int sample(State *s, Cmd *c, int ms, int seq, double now, int active) {
    if (!active || (s->sampled && (s->seq != seq || now < s->last || now-s->last > .25))) *s=(State){0};
    if (!active) return ms;
    s->seq=seq; s->last=now; s->debt+=ms; s->sampled+=ms;
    s->buttons |= c->buttons;
    if (c->impulse) s->impulse=c->impulse;
    if (c->weapon) s->weapon=c->weapon;
    for(int i=0;i<3;i++) s->movement[i]+=c->move[i]*ms;
    if(s->debt<10) return -1;
    /* At low FPS/hitches, pass elapsed time instead of bursting catch-up packets. */
    int out = s->debt>=20 ? s->debt : 10;
    if(out>255) out=255;
    c->buttons=s->buttons; c->impulse=s->impulse; c->weapon=s->weapon;
    for(int i=0;i<3;i++) c->move[i]=s->movement[i]/s->sampled;
    int remainder=s->debt-out; *s=(State){0}; s->debt=remainder; return out;
}
#ifndef MOVEMENT_ONLY
/* Presentation only. Positions are consecutive predicted command endpoints;
 * the movement scheduler's unsimulated milliseconds give the blend fraction. */
typedef struct { int valid, seq; double last; float prev[3], cur[3]; } Camera;
typedef struct { float unused[15],dt,time; int intermission,paused,spectating,ground,water; float vel[3],pos[3],viewheight[3],unused2[4]; int health; float unused3[7]; int clients,entity,player,maxent,demo,hw,smoothing; void *cmd,*vars; int viewport[4],next,only; } Ref;
_Static_assert(__builtin_offsetof(Ref,pos)==100 && __builtin_offsetof(Ref,viewheight)==112 && __builtin_offsetof(Ref,demo)==188 && __builtin_offsetof(Ref,next)==224,"ref ABI");
void blend(Camera *s,Ref *r,int seq,double now,double phase){
 if(r->next || r->only) return;
 if(r->demo || r->paused || r->spectating || r->intermission || r->health<=0 || r->entity!=r->player+1 || !(r->dt>0 && r->dt<.010f)){s->valid=0;return;}
 int reset=!s->valid || now<s->last || now-s->last>.05 || ((unsigned)seq-(unsigned)s->seq)>1;
 /* Blend world eye Z: at the crouch hull swap, origin and eye offset jump
  * in opposite directions. Blending only the origin creates a false bob. */
 float eye[3]={r->pos[0],r->pos[1],r->pos[2]+r->viewheight[2]};
 for(int i=0;i<3;i++){float d=eye[i]-s->cur[i]; if(!(d>=-32 && d<=32))reset=1;}
 for(int i=0;i<3;i++){
   float raw=eye[i];
   if(reset)s->prev[i]=raw;
   else if(seq!=s->seq)s->prev[i]=s->cur[i];
   else s->prev[i]+=raw-s->cur[i]; /* keep same-step prediction corrections */
   s->cur[i]=raw;
   if(phase>=0 && phase<10){
     float shown=s->prev[i]+(raw-s->prev[i])*(float)(phase*.1);
     r->pos[i]=i==2 ? shown-r->viewheight[2] : shown;
   }
 }
 s->valid=1;s->seq=seq;s->last=now;
}
#endif
