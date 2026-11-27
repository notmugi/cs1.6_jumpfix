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
}
