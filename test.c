#include <assert.h>
#include "move.c"
int main(void){
 for(int hz=100;hz<=1000;hz++){
    State s={0}; int n=0,t=0;
    for(int i=1;i<=hz;i++){
       Cmd c={0}; c.move[0]=240; c.buttons=i==1?2:0;
       int dt=i*1000/hz-(i-1)*1000/hz;
       int r=sample(&s,&c,dt,n,(double)i/hz,1);
       if(r<0)continue;
       assert(r==10 && c.move[0]==240 && c.buttons==(n?0:2)); ++n;t+=r;
    }
    assert(n==100 && t==1000);
 }
 State s={0}; Cmd c={0}; c.impulse=9; assert(sample(&s,&c,4,0,0,1)==-1);
 c.impulse=0; assert(sample(&s,&c,6,0,.006,1)==10 && c.impulse==9);
 assert(sample(&s,&c,43,1,.049,1)==43); assert(sample(&s,&c,3,2,.052,0)==3);
#ifndef MOVEMENT_ONLY
 /* Last grounded duck step: origin drops 18, eye offset rises 18.
    World eye Z is continuous at every render phase, in either hull. */
 for(int phase=0;phase<10;phase++){
    Camera cam={0}; Ref r={.dt=.004f,.entity=1,.health=100};
    r.pos[2]=36; r.viewheight[2]=-6; blend(&cam,&r,1,0,0);
    r.pos[2]=18; r.viewheight[2]=12; blend(&cam,&r,2,.01,phase);
    assert(r.pos[2]+r.viewheight[2]==30 && r.viewheight[2]==12);
 }
#endif
}
