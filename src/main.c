
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("Perspective Awakens",0,1,0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER|THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);

#define W 512
#define SW 480
#define SH 272
#define ROOMS 7
#define PLAYER_RADIUS .38f

typedef struct {unsigned int c;float x,y,z;} V;
typedef struct {const char *name,*hint;unsigned int wall,floor;} Room;
typedef struct {float x,z,hx,hz;} Obstacle;

static unsigned int __attribute__((aligned(16))) list[262144];
static SceCtrlData pad,old;

static const Room rooms[ROOMS]={
 {"THE WHITE ROOM","A room with no history. Yet.",0xffeeeeee,0xffd8d8d8},
 {"THE LONG GALLERY","The walls seem farther apart when you look away.",0xffd5d5d5,0xffbcbcbc},
 {"THE OBSERVATION ROOM","Something is watching from the doorway.",0xffeeeeee,0xffc9c9c9},
 {"THE IMPOSSIBLE HALL","The end of this hall is closer than it appears.",0xffcfcfcf,0xffb2b2b2},
 {"THE CLOCK ROOM","Time passes differently when you stand still.",0xffe5e5e5,0xffc4c4c4},
 {"THE DEVELOPER ROOM","This room was not meant to be found.",0xffbdbdbd,0xff999999},
 {"THE QUESTION ROOM","You were always part of the experiment.",0xffe8e8e8,0xffcccccc}
};

static int room=0,title=1,titleTicks=0,solved=0,secret=0;
static int encounters=0,secrets=0,msg=0,msgTicks=0,still=0,backwards=0;
static float x=0,y=1.65f,z=-3,yaw=0,pitch=0,cubeSize=.65f;

static const char *messages[]={
 "The cube remembers the size you gave it.",
 "Chatterbox: Oh! You found me. I wasn't supposed to be in this build.",
 "Chatterbox: You're getting pretty good at seeing things differently.",
 "Chatterbox: I think I know who's been changing this world...",
 "A hidden passage has appeared.",
 "The room is listening.",
 "You found a geometric memory.",
 "The chair is facing a different direction now.",
 "Developer note: this was definitely intentional.",
 "You stood still long enough for the world to notice.",
 "The ending is not where you think it is."
};

static int hit(unsigned int b){
 return (pad.Buttons&b)&&!(old.Buttons&b);
}

static void message(int n,int t){
 msg=n;
 msgTicks=t;
}

static V cv[]={
{0xffffffff,-1,-1,1},{0xffffffff,1,-1,1},{0xffffffff,1,1,1},{0xffffffff,-1,-1,1},{0xffffffff,1,1,1},{0xffffffff,-1,1,1},
{0xffffffff,1,-1,-1},{0xffffffff,-1,-1,-1},{0xffffffff,-1,1,-1},{0xffffffff,1,-1,-1},{0xffffffff,-1,1,-1},{0xffffffff,1,1,-1},
{0xffffffff,-1,-1,-1},{0xffffffff,-1,-1,1},{0xffffffff,-1,1,1},{0xffffffff,-1,-1,-1},{0xffffffff,-1,1,1},{0xffffffff,-1,1,-1},
{0xffffffff,1,-1,1},{0xffffffff,1,-1,-1},{0xffffffff,1,1,-1},{0xffffffff,1,-1,1},{0xffffffff,1,1,-1},{0xffffffff,1,1,1},
{0xffffffff,-1,1,1},{0xffffffff,1,1,1},{0xffffffff,1,1,-1},{0xffffffff,-1,1,1},{0xffffffff,1,1,-1},{0xffffffff,-1,1,-1},
{0xffffffff,-1,-1,-1},{0xffffffff,1,-1,-1},{0xffffffff,1,-1,1},{0xffffffff,-1,-1,-1},{0xffffffff,1,-1,1},{0xffffffff,-1,-1,1}
};

/* Retro PS1-style textures */
typedef struct {float u,v;unsigned int c;float x,y,z;} TV;

static unsigned int __attribute__((aligned(16))) wallTex[64*64];
static unsigned int __attribute__((aligned(16))) floorTex[64*64];

static void makeTextures(void){
 int tx,ty;

 for(ty=0;ty<64;ty++)
 for(tx=0;tx<64;tx++){

  int n=(tx*37+ty*19+tx*ty*3+(tx^ty)*11)%23;
  int shade=151+n-11;

  if(ty%16==0 || tx%32==0)shade-=22;

  if((tx==11&&ty>8&&ty<25)||
     (tx==12&&ty>25&&ty<31)||
     (tx==45&&ty>39&&ty<55))
   shade-=42;

  if((tx+ty*3)%47==0)shade-=25;

  if(shade<55)shade=55;
  if(shade>205)shade=205;

  wallTex[ty*64+tx]=
   0xff000000u|
   ((unsigned int)shade<<16)|
   ((unsigned int)shade<<8)|
   (unsigned int)shade;

  {
   int tile=((tx/16)+(ty/16))&1;

   int fshade=(tile?132:151)+
    (int)((tx*13+ty*7)%13)-6;

   if(tx%16<2||ty%16<2)fshade-=35;

   if(fshade<50)fshade=50;
   if(fshade>190)fshade=190;

   floorTex[ty*64+tx]=
    0xff000000u|
    ((unsigned int)fshade<<16)|
    ((unsigned int)fshade<<8)|
    (unsigned int)fshade;
  }
 }

 sceKernelDcacheWritebackRange(wallTex,sizeof(wallTex));
 sceKernelDcacheWritebackRange(floorTex,sizeof(floorTex));
}

static void texturedQuad(const TV *v,int isFloor){

 sceGuEnable(GU_TEXTURE_2D);

 sceGuTexMode(GU_PSM_8888,0,0,GU_FALSE);

 sceGuTexImage(
  0,64,64,64,
  isFloor?floorTex:wallTex
 );

 sceGuTexSync();

 sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
 sceGuTexFilter(GU_NEAREST,GU_NEAREST);
 sceGuTexWrap(GU_REPEAT,GU_REPEAT);

 sceGuColor(0xffffffff);

 sceGumDrawArray(
  GU_TRIANGLES,
  GU_TEXTURE_32BITF|
  GU_COLOR_8888|
  GU_VERTEX_32BITF|
  GU_TRANSFORM_3D,
  6,0,v
 );

 sceGuDisable(GU_TEXTURE_2D);
}

static void texturedPlane(
 float x1,float y1,float z1,
 float x2,float y2,float z2,
 float x3,float y3,float z3,
 float x4,float y4,float z4,
 float ru,float rv,int isFloor
){

 TV v[6]={
  {0,0,0xffffffff,x1,y1,z1},
  {ru,0,0xffffffff,x2,y2,z2},
  {ru,rv,0xffffffff,x3,y3,z3},
  {0,0,0xffffffff,x1,y1,z1},
  {ru,rv,0xffffffff,x3,y3,z3},
  {0,rv,0xffffffff,x4,y4,z4}
 };

 texturedQuad(v,isFloor);
}

static void box(
 float a,float b,float c,
 float sx,float sy,float sz,
 unsigned int col
){

 V v[36];
 int i;

 for(i=0;i<36;i++){
  v[i]=cv[i];
  v[i].c=col;
 }

 sceGumPushMatrix();

 ScePspFVector3 p={a,b,c};
 ScePspFVector3 s={sx,sy,sz};

 sceGumTranslate(&p);
 sceGumScale(&s);

 sceGumDrawArray(
  GU_TRIANGLES,
  GU_COLOR_8888|
  GU_VERTEX_32BITF|
  GU_TRANSFORM_3D,
  36,0,v
 );

 sceGumPopMatrix();
}

/* Object collision */
static int blockedAt(float px,float pz){

 Obstacle o;
 float r=PLAYER_RADIUS;

 if(room<2){
  o=(Obstacle){0,1.7f,.85f,.85f};

  if(fabsf(px-o.x)<o.hx+r &&
     fabsf(pz-o.z)<o.hz+r)
   return 1;
 }

 if(room==1){

  o=(Obstacle){-3.8f,-2.5f,.7f,.7f};

  if(fabsf(px-o.x)<o.hx+r &&
     fabsf(pz-o.z)<o.hz+r)
   return 1;

  o=(Obstacle){3.8f,-2.5f,.7f,.7f};

  if(fabsf(px-o.x)<o.hx+r &&
     fabsf(pz-o.z)<o.hz+r)
   return 1;
 }

 if(room==2){

  o=(Obstacle){0,-2,.42f,.42f};

  if(fabsf(px-o.x)<o.hx+r &&
     fabsf(pz-o.z)<o.hz+r)
   return 1;
 }

 if(room==3){

  int i;

  for(i=0;i<4;i++){

   o=(Obstacle){0,-4.5f+i*1.6f,1.8f,.75f};

   if(fabsf(px-o.x)<o.hx+r &&
      fabsf(pz-o.z)<o.hz+r)
    return 1;
  }
 }

 if(room==4){

  o=(Obstacle){0,-5,1.6f,.2f};

  if(fabsf(px-o.x)<o.hx+r &&
     fabsf(pz-o.z)<o.hz+r)
   return 1;
 }

 if(room==5){

  o=(Obstacle){-4,0,.8f,.8f};

  if(fabsf(px-o.x)<o.hx+r &&
     fabsf(pz-o.z)<o.hz+r)
   return 1;

  o=(Obstacle){4,0,.8f,.8f};

  if(fabsf(px-o.x)<o.hx+r &&
     fabsf(pz-o.z)<o.hz+r)
   return 1;
 }

 if(room==6){

  o=(Obstacle){0,0,.45f,.45f};

  if(fabsf(px-o.x)<o.hx+r &&
     fabsf(pz-o.z)<o.hz+r)
   return 1;
 }

 /* Chair */

 o=(Obstacle){-4.7f,-4,.75f,.75f};

 if(fabsf(px-o.x)<o.hx+r &&
    fabsf(pz-o.z)<o.hz+r)
  return 1;

 o=(Obstacle){-4.7f,-4.65f,.75f,.12f};

 if(fabsf(px-o.x)<o.hx+r &&
    fabsf(pz-o.z)<o.hz+r)
  return 1;

 o=(Obstacle){-5.25f,-4,.12f,.12f};

 if(fabsf(px-o.x)<o.hx+r &&
    fabsf(pz-o.z)<o.hz+r)
  return 1;

 o=(Obstacle){-4.15f,-4,.12f,.12f};

 if(fabsf(px-o.x)<o.hx+r &&
    fabsf(pz-o.z)<o.hz+r)
  return 1;

 return 0;
}

static void roomDraw(void){

 unsigned int a=rooms[room].wall;

 /* Textured walls, floor and ceiling */

 texturedPlane(
  -8,0,-9, 8,0,-9,
  8,0,9, -8,0,9,
  8,9,1
 );

 texturedPlane(
  -8,4.02f,9, 8,4.02f,9,
  8,4.02f,-9, -8,4.02f,-9,
  8,9,0
 );

 texturedPlane(
  -8,0,-9, -8,4.2f,-9,
  -8,4.2f,9, -8,0,9,
  9,2,0
 );

 texturedPlane(
  8,0,9, 8,4.2f,9,
  8,4.2f,-9, 8,0,-9,
  9,2,0
 );

 texturedPlane(
  -8,0,-9, -8,4.2f,-9,
  8,4.2f,-9, 8,0,-9,
  8,2,0
 );

 /* Front wall */

 box(-2.7f,2,8.9f,5.3f,2.2f,.18f,a);
 box(2.7f,2,8.9f,5.3f,2.2f,.18f,a);
 box(0,3.75f,8.9f,2.7f,.45f,.18f,a);

 box(0,1.65f,9.02f,2.35f,1.65f,.035f,0xff080808);

 /* Door frame */

 box(-2.48f,1.8f,8.72f,.12f,1.9f,.16f,0xff333333);
 box(2.48f,1.8f,8.72f,.12f,1.9f,.16f,0xff333333);
 box(0,3.68f,8.72f,2.6f,.12f,.16f,0xff333333);

 /* Cube and pedestal */

 if(room<2){
  box(0,.65f,1.7f,.85f,.12f,.85f,0xff555555);
  box(0,1.45f,1.7f,cubeSize,cubeSize,cubeSize,0xff222222);
 }

 /* Gallery */

 if(room==1){
  box(-3.8f,1.1f,-2.5f,.7f,1.1f,.7f,0xff777777);
  box(3.8f,1.1f,-2.5f,.7f,1.1f,.7f,0xff777777);
 }

 /* Chatterbox */

 if(room==2){
  box(0,2.7f,-2,.38f,.38f,.38f,0xff222222);
  box(0,2.7f,-1.59f,.25f,.2f,.035f,0xffeeeeee);
 }

 /* Staircase */

 if(room==3){
  int i;

  for(i=0;i<4;i++)
   box(0,.45f+i*.65f,-4.5f+i*1.6f,
       1.8f,.18f,.75f,0xff777777);
 }

 /* Clock room */

 if(room==4){
  box(0,2.1f,-5,1.6f,1.6f,.12f,0xff333333);
  box(0,2.1f,-4.82f,1.3f,1.3f,.035f,0xffeeeeee);
 }

 /* Developer room */

 if(room==5){
  box(-4,.6f,0,.8f,.6f,.8f,0xff444444);
  box(4,.6f,0,.8f,.6f,.8f,0xff444444);
 }

 /* Final room */

 if(room==6){
  box(0,2.3f,0,.45f,.45f,.45f,0xff111111);
  box(0,2.3f,0,.22f,.22f,.22f,0xffeeeeee);
 }

 /* Recurring chair */

 box(-4.7f,.65f,-4,.75f,.12f,.75f,0xff444444);
 box(-4.7f,1.35f,-4.65f,.75f,.75f,.12f,0xff444444);
 box(-5.25f,.95f,-4,.12f,.45f,.12f,0xff444444);
 box(-4.15f,.95f,-4,.12f,.45f,.12f,0xff444444);
}

static void draw3d(void){

 ScePspFVector3 eye={x,y,z};

 ScePspFVector3 at={
  x+sinf(yaw)*cosf(pitch),
  y+sinf(pitch),
  z+cosf(yaw)*cosf(pitch)
 };

 ScePspFVector3 up={0,1,0};

 sceGuStart(GU_DIRECT,list);

 sceGuClearColor(0xffeeeeee);
 sceGuClearDepth(0);
 sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);

 sceGuEnable(GU_DEPTH_TEST);
 sceGuDepthFunc(GU_GEQUAL);
 sceGuDisable(GU_CULL_FACE);

 sceGumMatrixMode(GU_PROJECTION);
 sceGumLoadIdentity();
 sceGumPerspective(65,16.0f/9.0f,.5f,100);

 sceGumMatrixMode(GU_VIEW);
 sceGumLoadIdentity();
 sceGumLookAt(&eye,&at,&up);

 sceGumMatrixMode(GU_MODEL);
 sceGumLoadIdentity();

 roomDraw();

 sceGuFinish();
 sceGuSync(0,0);
 sceGuSwapBuffers();
}

static void resetPos(void){
 x=0;
 y=1.65f;
 z=-3;
 yaw=0;
 pitch=0;
}

static void enter(int r){

 room=(r+ROOMS)%ROOMS;
 resetPos();

 if(room==2&&encounters==0){
  message(1,360);
  encounters=1;
 }
 else if(room==4&&encounters==1){
  message(2,360);
  encounters=2;
 }
 else if(room==6&&encounters==2){
  message(3,420);
  encounters=3;
  secret=1;
 }
 else if(room==5)
  message(8,240);
 else if(room==6)
  message(10,360);
}

static void interact(void){

 if(room<2&&fabsf(x)<2&&fabsf(z-1.7f)<2.2f){

  cubeSize+=.22f;

  if(cubeSize>1.75f)
   cubeSize=.43f;

  if(cubeSize>1.3f){
   solved=1;
   secret=1;
   message(0,300);
  }
  else
   message(6,180);

  return;
 }

 if(room==2&&fabsf(x)<2&&z>-3&&z<0){
  message(encounters<2?1:2,300);
  return;
 }

 if(fabsf(x+4.7f)<1.3f&&fabsf(z+4)<1.4f){
  message(7,220);
  secrets++;
  return;
 }

 if(room==5&&fabsf(x)<2&&z>5){
  enter(6);
  return;
 }

 message(5,150);
}

static void update(void){

 float ax=(pad.Lx-128)/128.0f;
 float ay=(pad.Ly-128)/128.0f;

 const float dead=.15f;
 const float speed=.075f;
 const float turn=.045f;

 if(fabsf(ax)<dead)ax=0;
 if(fabsf(ay)<dead)ay=0;

 float len=sqrtf(ax*ax+ay*ay);

 if(len>1.0f){
  ax/=len;
  ay/=len;
 }

 /* Camera controls */

 if(pad.Buttons&PSP_CTRL_SQUARE)
  yaw+=turn;

 if(pad.Buttons&PSP_CTRL_CIRCLE)
  yaw-=turn;

 if(pad.Buttons&PSP_CTRL_TRIANGLE)
  pitch+=.025f;

 if(pad.Buttons&PSP_CTRL_CROSS)
  pitch-=.025f;

 if(pitch>.8f)pitch=.8f;
 if(pitch<-.8f)pitch=-.8f;

 /* Movement */

 float dx=sinf(yaw)*(-ay)*speed-cosf(yaw)*ax*speed;
 float dz=cosf(yaw)*(-ay)*speed+sinf(yaw)*ax*speed;

 if(!blockedAt(x+dx,z))
  x+=dx;

 if(!blockedAt(x,z+dz))
  z+=dz;

 /* Room boundaries */

 if(x>7.1f)x=7.1f;
 if(x< -7.1f)x=-7.1f;

 if(z< -8.1f)z=-8.1f;

 if(z>8.5f){

  if((room==0&&!solved)||(room==5&&!secret)){
   z=8.4f;
   message(5,120);
  }
  else{
   enter(room+1);
  }
 }

 if(z< -8)z=-8;

 /* Easter eggs */

 if(!ax&&!ay)still++;
 else still=0;

 if(ay>.5f)backwards++;
 else backwards=0;

 if(still==900)message(9,240);
 if(backwards==600)message(8,240);

 if(hit(PSP_CTRL_RTRIGGER))
  interact();

 if(msgTicks>0)
  msgTicks--;
}

static void textScreen(void){

 pspDebugScreenSetXY(0,0);

 if(title){

  pspDebugScreenPrintf("P E R S P E C T I V E   A W A K E N S\n\n\n");
  pspDebugScreenPrintf("              [  ]\n\n\n");
  pspDebugScreenPrintf("       PRESS START TO WAKE UP\n");

  if(titleTicks>180)
   pspDebugScreenPrintf("\n                 . .\n");

  pspDebugScreenPrintf("\n\n  A world shaped by the way you see it.\n");

  return;
 }

 pspDebugScreenPrintf(
  "%s\nROOM %d / %d     MEMORIES %d\n\n",
  rooms[room].name,room+1,ROOMS,secrets
 );

 pspDebugScreenPrintf("CONTROLS\n");
 pspDebugScreenPrintf("------------------------------\n");

 pspDebugScreenPrintf("Analog: Walk / Strafe\n");
 pspDebugScreenPrintf("Square: Turn Right\n");
 pspDebugScreenPrintf("Circle: Turn Left\n");
 pspDebugScreenPrintf("Triangle: Look Up\n");
 pspDebugScreenPrintf("Cross: Look Down\n");
 pspDebugScreenPrintf("R Trigger: Interact\n\n");

 pspDebugScreenPrintf(
  "%s\n",
  msgTicks>0?messages[msg]:rooms[room].hint
 );

 if(room==0&&!solved)
  pspDebugScreenPrintf("\nTry changing the cube's size.\n");

 if(room==6)
  pspDebugScreenPrintf("\nTHE QUESTION IS: WHO IS PLAYING WHOM?\n");
}

int main(void){

 sceCtrlSetSamplingCycle(0);
 sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

 pspDebugScreenInit();
 sceGuInit();

 makeTextures();

 sceGuStart(GU_DIRECT,list);

 sceGuDrawBuffer(GU_PSM_8888,(void*)0,W);
 sceGuDispBuffer(SW,SH,(void*)0x88000,W);
 sceGuDepthBuffer((void*)0x110000,W);

 sceGuOffset(2048-SW/2,2048-SH/2);
 sceGuViewport(2048,2048,SW,SH);
 sceGuDepthRange(65535,0);

 sceGuScissor(0,0,SW,SH);
 sceGuEnable(GU_SCISSOR_TEST);

 sceGuFinish();
 sceGuSync(0,0);

 sceDisplayWaitVblankStart();
 sceGuDisplay(GU_TRUE);

 sceCtrlReadBufferPositive(&old,1);

 while(1){

  sceCtrlReadBufferPositive(&pad,1);

  if(title){

   titleTicks++;

   if(hit(PSP_CTRL_START)||hit(PSP_CTRL_SELECT)){
    title=0;
    enter(0);
    message(5,180);
   }

   pspDebugScreenClear();
   textScreen();

   sceDisplayWaitVblankStart();
  }
  else{

   update();
   draw3d();

   pspDebugScreenClear();
   textScreen();

   sceDisplayWaitVblankStart();
  }

  old=pad;
 }

 return 0;
}
