#include "queenofclocks.h"

/* Begin.
 */
 
int ctlpan_begin(struct sprite *pumpkin) {
  sprite_group_clear(&g.ctlpan_pumpkin);
  if (sprite_group_add(&g.ctlpan_pumpkin,pumpkin)<0) return -1;
  qc_sound(RID_sound_ctlpanup,pumpkin->x+pumpkin->w*0.5);
  return 0;
}

/* Dismiss.
 */
 
void ctlpan_dismiss() {
  if (!g.ctlpan_pumpkin.sprc) return;
  struct sprite *pumpkin=g.ctlpan_pumpkin.sprv[0];
  qc_sound(RID_sound_ctlpandown,pumpkin->x+pumpkin->w*0.5);
  sprite_group_clear(&g.ctlpan_pumpkin);
}

/* Test.
 */
 
struct sprite *ctlpan_is_active() {
  if (!g.ctlpan_pumpkin.sprc) return 0;
  return g.ctlpan_pumpkin.sprv[0];
}

/* Move focus.
 */
 
static void ctlpan_move(int d) {
  struct sprite *pumpkin=g.ctlpan_pumpkin.sprv[0];
  qc_sound(RID_sound_uimotion,pumpkin->x+pumpkin->w*0.5);
  pumpkin->timescale+=d;
  if (pumpkin->timescale<0) pumpkin->timescale=0;
  else if (pumpkin->timescale>4) pumpkin->timescale=4;
}

/* Update.
 */
 
void ctlpan_update(double elapsed) {
  if (!g.ctlpan_pumpkin.sprc) return;
  if ((g.input&EGG_BTN_LEFT)&&!(g.pvinput&EGG_BTN_LEFT)) ctlpan_move(-1);
  else if ((g.input&EGG_BTN_RIGHT)&&!(g.pvinput&EGG_BTN_RIGHT)) ctlpan_move(1);
}

/* Render.
 */
 
void ctlpan_render() {
  if (!g.ctlpan_pumpkin.sprc) return;
  struct sprite *pumpkin=g.ctlpan_pumpkin.sprv[0];
  
  // Choose a bounding box.
  int bw=NS_sys_tilesize*5;
  int bx=(int)((pumpkin->x+pumpkin->w*0.5)*NS_sys_tilesize)-(bw>>1);
  if (bx<0) bx=0;
  else if (bx>FBW-bw) bx=FBW-bw;
  int by,bh=NS_sys_tilesize,tongueup=0;
  if (pumpkin->y+pumpkin->h*0.5>NS_sys_maph*0.5) {
    by=(int)(pumpkin->y*NS_sys_tilesize)-bh;
  } else {
    by=(int)((pumpkin->y+pumpkin->h)*NS_sys_tilesize);
    tongueup=1;
  }
  // (by) can't be oob because we've oriented centerward of the pumpkin.
  
  // Background tiles.
  graf_set_input(&g.graf,g.texid_sprites);
  const int halftile=NS_sys_tilesize>>1;
  int x=bx+halftile;
  int y=by+halftile;
  graf_tile(&g.graf,x,y,0x20,0); x+=NS_sys_tilesize;
  graf_tile(&g.graf,x,y,0x21,0); x+=NS_sys_tilesize;
  graf_tile(&g.graf,x,y+(tongueup?-1:0),0x22,tongueup?EGG_XFORM_YREV:0); x+=NS_sys_tilesize;
  graf_tile(&g.graf,x,y,0x21,0); x+=NS_sys_tilesize;
  graf_tile(&g.graf,x,y,0x23,0);
  
  // Timescale indicators.
  x=bx+halftile;
  int ts=0;
  for (;ts<5;ts++,x+=NS_sys_tilesize) {
    uint8_t tileid=0x30+ts;
    if (ts==pumpkin->timescale) tileid+=5;
    // There's another set of them at +10 for disabled. Not sure we'll be doing that.
    graf_tile(&g.graf,x,y,tileid,0);
  }
}
