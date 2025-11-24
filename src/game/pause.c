#include "queenofclocks.h"

#define STRIX_DISMISS 10
#define STRIX_RESTART 11
#define STRIX_END 12
#define STRIX_QUIT 13

/* Extra globals.
 */
 
#define LABEL_LIMIT 4
#define LABEL_TEXT_LIMIT 16
 
static struct pause {
  struct label {
    char text[LABEL_TEXT_LIMIT];
    int textc;
    int strix;
  } labelv[LABEL_LIMIT];
  int labelc;
  int labelp;
} pause={0};

/* Add label.
 */
 
static struct label *pause_add_label(int strix) {
  if (pause.labelc>=LABEL_LIMIT) return 0;
  struct label *label=pause.labelv+pause.labelc++;
  memset(label,0,sizeof(struct label));
  const char *src=0;
  int srcc=qc_res_get_string(&src,1,strix);
  if (srcc>LABEL_TEXT_LIMIT) srcc=LABEL_TEXT_LIMIT;
  memcpy(label->text,src,srcc);
  label->textc=srcc;
  label->strix=strix;
  return label;
}

/* Begin.
 */
 
void pause_begin() {
  qc_sound(RID_sound_uimotion,-1.0);
  g.pause=1;
  pause.labelc=0;
  pause.labelp=0;
  
  pause_add_label(STRIX_DISMISS);
  pause_add_label(STRIX_RESTART);
  pause_add_label(STRIX_END);
  pause_add_label(STRIX_QUIT);
}

/* Dismiss.
 */
 
void pause_dismiss() {
  qc_sound(RID_sound_uimotion,-1.0);
  g.pause=0;
}

/* Activate selection.
 */
 
static void pause_activate() {
  if ((pause.labelp<0)||(pause.labelp>=pause.labelc)) return;
  switch (pause.labelv[pause.labelp].strix) {
    case STRIX_DISMISS: pause_dismiss(); break;
    case STRIX_RESTART: g.deathc++; qc_scene_load(g.mapid); pause_dismiss(); break;
    case STRIX_END: qc_scene_load(RID_map_start); pause_dismiss(); break;
    case STRIX_QUIT: egg_terminate(0); break;
  }
}

/* Move selection.
 */
 
static void pause_move(int d) {
  if (pause.labelc<1) return;
  pause.labelp+=d;
  if (pause.labelp<0) pause.labelp=pause.labelc-1;
  else if (pause.labelp>=pause.labelc) pause.labelp=0;
  qc_sound(RID_sound_uimotion,-1.0);
}

/* Update.
 * Main manages dismissal.
 * But we do include a Dismiss option for completeness.
 */
 
void pause_update(double elapsed) {
  if ((g.input&EGG_BTN_UP)&&!(g.pvinput&EGG_BTN_UP)) pause_move(-1);
  else if ((g.input&EGG_BTN_DOWN)&&!(g.pvinput&EGG_BTN_DOWN)) pause_move(1);
  else if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) pause_activate();
}

/* Render.
 * The scene also renders, we go on top of it.
 */
 
void pause_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x000000c0);
  
  struct label *label;
  int i;
  
  int w=0;
  for (label=pause.labelv,i=pause.labelc;i-->0;label++) {
    if (label->textc>w) w=label->textc;
  }
  w+=4; // 1 for the indicator, 2 for outer margins, 1 for inner margin
  w*=8;
  int h=(pause.labelc+2)*8;
  int x=(FBW>>1)-(w>>1);
  int y=(FBH>>1)-(h>>1);
  graf_fill_rect(&g.graf,x,y,w,h,0x000000ff);
  
  x+=12;
  y+=12;
  graf_set_input(&g.graf,g.texid_font);
  for (i=0,label=pause.labelv;i<pause.labelc;i++,label++,y+=8) {
    if (i==pause.labelp) graf_tile(&g.graf,x,y,0x82,0);
    int chx=x+16;
    const char *src=label->text;
    int srci=label->textc;
    for (;srci-->0;src++,chx+=8) graf_tile(&g.graf,chx,y,*src,0);
  }
}
