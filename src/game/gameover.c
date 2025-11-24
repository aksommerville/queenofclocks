#include "queenofclocks.h"

/* Private globals.
 */
 
#define LABEL_LIMIT 16
#define LABEL_TEXT_LIMIT 24
 
static struct gameover {
  struct label {
    char text[LABEL_TEXT_LIMIT];
    int textc;
    uint32_t tint; // No tint = white.
    int x,y; // Middle of first glyph. Glyphs are 8x8 pixels.
    int blink;
  } labelv[LABEL_LIMIT];
  int labelc;
  double blackout;
} gameover={0};

/* Calculate score.
 */
 
static int gameover_calculate_score(double playtime,int deathc) {

  // Must add up to at least 999999. A little over is ok.
  const int participation     =100000; // Completing the game at all is worth something. And 100k obviates the question of leading zeroes.
  const int playtime_potential=450000;
  const int deathc_potential  =450000;
  
  // Thresholds for any points and max points per criterion.
  const double playtime_worst=6.0*60.0; // A leisurely play-through, no speed tricks but no particular mistakes, took just about 5 minutes. Call it 6, throw them a bone.
  const double playtime_best =3.0*60.0; // First try, going pretty fast but certainly not optimal, 178 s. So let's call it 180, that's achievable.
  const int deathc_worst=9;
  const int deathc_best =0;
  
  int score=participation;
  if (playtime<=playtime_best) score+=playtime_potential;
  else if (playtime<playtime_worst) score+=(int)((playtime_potential*(playtime_worst-playtime))/(playtime_worst-playtime_best));
  if (deathc<=deathc_best) score+=deathc_potential;
  else if (deathc<=deathc_worst) score+=(deathc_potential*(deathc_worst-deathc+1))/(deathc_worst-deathc_best+1);

  if (score<0) score=0; else if (score>999999) score=999999;
  return score;
}

/* Add labels.
 */

static struct label *gameover_add_label() {
  if (gameover.labelc>=LABEL_LIMIT) return 0;
  struct label *label=gameover.labelv+gameover.labelc++;
  memset(label,0,sizeof(struct label));
  return label;
}

static struct label *gameover_add_static_label(int ix) {
  struct label *label=gameover_add_label();
  if (!label) return 0;
  const char *src;
  int srcc=qc_res_get_string(&src,1,ix);
  if (srcc>LABEL_TEXT_LIMIT) srcc=LABEL_TEXT_LIMIT;
  memcpy(label->text,src,srcc);
  label->textc=srcc;
  return label;
}

static struct label *gameover_add_time_label(int ix,double sf) {
  struct label *label=gameover_add_label();
  if (!label) return 0;
  
  const char *lbl;
  int lblc=qc_res_get_string(&lbl,1,ix);
  
  int ms=(int)(sf*1000.0);
  if (ms<0) ms=0;
  int sec=ms/1000; ms%=1000;
  int min=sec/60; sec%=60;
  if (min>99) { min=sec=99; ms=999; }
  char tmp[]={
    '0'+min/10,
    '0'+min%10,
    ':',
    '0'+sec/10,
    '0'+sec%10,
    '.',
    '0'+ms/100,
    '0'+(ms/10)%10,
    '0'+ms%10,
  };
  
  // Use the full buffer size with space in the middle, to force alignment.
  int minw=lblc+sizeof(tmp);
  if (minw>LABEL_TEXT_LIMIT) {
    lblc=LABEL_TEXT_LIMIT-sizeof(tmp);
    minw=LABEL_TEXT_LIMIT;
  }
  int spcc=LABEL_TEXT_LIMIT-minw;
  memcpy(label->text,lbl,lblc);
  memset(label->text+lblc,' ',spcc);
  memcpy(label->text+lblc+spcc,tmp,sizeof(tmp));
  label->textc=LABEL_TEXT_LIMIT;
  
  return label;
}

static struct label *gameover_add_int_label(int ix,int v) {
  struct label *label=gameover_add_label();
  if (!label) return 0;
  
  const char *lbl;
  int lblc=qc_res_get_string(&lbl,1,ix);
  
  char tmp[16];
  int tmpc=0;
  if (v<0) v=0; // Positive only.
  if (v>=1000000000) tmp[tmpc++]='0'+(v/1000000000)%10;
  if (v>= 100000000) tmp[tmpc++]='0'+(v/ 100000000)%10;
  if (v>=  10000000) tmp[tmpc++]='0'+(v/  10000000)%10;
  if (v>=   1000000) tmp[tmpc++]='0'+(v/   1000000)%10;
  if (v>=    100000) tmp[tmpc++]='0'+(v/    100000)%10;
  if (v>=     10000) tmp[tmpc++]='0'+(v/     10000)%10;
  if (v>=      1000) tmp[tmpc++]='0'+(v/      1000)%10;
  if (v>=       100) tmp[tmpc++]='0'+(v/       100)%10;
  if (v>=        10) tmp[tmpc++]='0'+(v/        10)%10;
                     tmp[tmpc++]='0'+(v           )%10;
  
  // Use the full buffer size with space in the middle, to force alignment.
  int minw=lblc+tmpc;
  if (minw>LABEL_TEXT_LIMIT) {
    lblc=LABEL_TEXT_LIMIT-tmpc;
    minw=LABEL_TEXT_LIMIT;
  }
  int spcc=LABEL_TEXT_LIMIT-minw;
  memcpy(label->text,lbl,lblc);
  memset(label->text+lblc,' ',spcc);
  memcpy(label->text+lblc+spcc,tmp,tmpc);
  label->textc=LABEL_TEXT_LIMIT;

  return label;
}

/* Begin.
 */
 
int gameover_begin() {
  struct label *label;
  g.gameover=1;
  gameover.blackout=1.000;

  qc_song(RID_song_cleaned_clock,0);
  
  int score=gameover_calculate_score(g.playtime,g.deathc);
  
  // Generate labels.
  gameover.labelc=0;
  if (label=gameover_add_static_label(3)) { // You win!
    label->tint=0xff8000ff;
  }
  gameover_add_label(); // blank
  gameover_add_time_label(4,g.playtime);
  gameover_add_int_label(5,g.deathc);
  gameover_add_int_label(6,score);
  gameover_add_label(); // blank
  if (score>g.hiscore) {
    if (label=gameover_add_static_label(8)) { // New high score!
      label->tint=0xffff00ff;
      label->blink=1;
    }
  } else {
    if (label=gameover_add_int_label(7,g.hiscore)) {
      label->tint=0x808080ff;
    }
  }
  gameover_add_label(); // blank
  if (label=gameover_add_static_label(9)) { // Thanks for playing!
    label->tint=0x408060ff;
  }
  
  // Position all labels.
  int h=gameover.labelc*8;
  int y=(FBH>>1)-(h>>1)+4;
  int i=gameover.labelc;
  for (label=gameover.labelv;i-->0;label++,y+=8) {
    int w=label->textc*8;
    label->x=(FBW>>1)-(w>>1)+4;
    label->y=y;
  }
  
  if (score>g.hiscore) {
    g.hiscore=score;
    qc_hiscore_save();
  }

  return 0;
}

/* Dismiss.
 */
 
static void gameover_dismiss() {
  g.gameover=0;
  if (qc_scene_load(RID_map_start)<0) egg_terminate(1);
}

/* Update.
 */
 
void gameover_update(double elapsed) {
  if (gameover.blackout>0.0) {
    gameover.blackout-=elapsed;
  } else {
    if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) gameover_dismiss();
  }
}

/* Render.
 */
 
void gameover_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  graf_set_input(&g.graf,g.texid_font);
  int blink_state=((g.framec&0x1f)<24)?1:0;
  const struct label *label=gameover.labelv;
  int i=gameover.labelc;
  for (;i-->0;label++) {
    if (label->tint) graf_set_tint(&g.graf,label->tint);
    if (!blink_state&&label->blink) graf_set_alpha(&g.graf,0x40);
    int x=label->x,y=label->y;
    const char *src=label->text;
    int srci=label->textc;
    for (;srci-->0;src++,x+=8) {
      if ((unsigned char)(*src)<=0x20) continue;
      graf_tile(&g.graf,x,y,*src,0);
    }
    if (label->tint) graf_set_tint(&g.graf,0);
    if (!blink_state&&label->blink) graf_set_alpha(&g.graf,0xff);
  }
}
