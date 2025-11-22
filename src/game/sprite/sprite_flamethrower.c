#include "game/queenofclocks.h"

/* If it were any longer, its power would be unfathomable.
 */
#define THATS_WHAT_SHE_SAID 12

struct sprite_flamethrower {
  struct sprite hdr;
  uint8_t dir;
  double period; // s
  double clock; // Counts up to (period).
  int grabbed;
  int length;
  int state;
  double animclock;
  int animframe;
};

#define SPRITE ((struct sprite_flamethrower*)sprite)

/* Init.
 */

static int _flamethrower_init(struct sprite *sprite) {
  SPRITE->dir=(sprite->arg>>24)&0xff;
  sprite->timescale=(sprite->arg>>16)&0xff;
  SPRITE->period=((((sprite->arg>>8)&0xff)+1)*4.0)/256.0;
  //SPRITE->clock=((sprite->arg&0xff)*SPRITE->period)/256.0; // TODO Do we need to indicate phase at the spawn point too?
  SPRITE->length=(sprite->arg&0xff);
  if (SPRITE->length<1) SPRITE->length=1;
  else if (SPRITE->length>THATS_WHAT_SHE_SAID) SPRITE->length=THATS_WHAT_SHE_SAID;
  
  // (dir) overrides (xform). Tiles are oriented right by default.
  // Tiles are symmetric, or close enough to that I'm not worried about it.
  switch (SPRITE->dir) {
    case DIR_N: sprite->xform=EGG_XFORM_SWAP|EGG_XFORM_XREV; break;
    case DIR_S: sprite->xform=EGG_XFORM_SWAP; break;
    case DIR_W: sprite->xform=EGG_XFORM_XREV; break;
    case DIR_E: sprite->xform=0; break;
    default: fprintf(stderr,"Invalid direction 0x%02x for flamethrower.\n",SPRITE->dir); return -1;
  }
  
  return 0;
}

/* Set state. It's ok and good to spam these, call every frame.
 */
 
static void flamethrower_on(struct sprite *sprite) {
  if (SPRITE->state) return;
  qc_sound(RID_sound_flameon,sprite->x+sprite->w*0.5);
  SPRITE->state=1;
}

static void flamethrower_off(struct sprite *sprite) {
  if (!SPRITE->state) return;
  qc_sound(RID_sound_flameoff,sprite->x+sprite->w*0.5);
  SPRITE->state=0;
}

/* Advance time. May change state.
 */

static void flamethrower_advance_time(struct sprite *sprite,double elapsed) {

  // The flames animation runs in immutable time; it's only the machine that's subject to Relativity.
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.125;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
  }

  // Stop dead if grabbed or paused. Either state.
  if (SPRITE->grabbed) return;
  if (sprite->timescale==NS_timescale_stop) return;
  
  // Infinite speed, it's tricky to define what ought to happen.
  // My feeling is it's effectively in both the On and Off states simultaneously, and since the Off state is noop, it's On.
  if (sprite->timescale==NS_timescale_infinite) {
    flamethrower_on(sprite);
    return;
  }
  
  // Slow, Normal, and Fast all work the same way, just different scales on (elapsed).
  switch (sprite->timescale) {
    case NS_timescale_slow: elapsed*=0.500; break;
    case NS_timescale_normal: break;
    case NS_timescale_fast: elapsed*=2.000; break;
    default: return;
  }
  
  if ((SPRITE->clock+=elapsed)>=SPRITE->period) {
    // Clock might advance so fast or slow that it crosses multiple periods. Reset to zero if oob.
    SPRITE->clock-=SPRITE->period;
    if (SPRITE->clock<0.0) SPRITE->clock=0.0;
    else if (SPRITE->clock>=SPRITE->period) SPRITE->clock=0.0;
  }
  if (SPRITE->clock>=SPRITE->period*0.5) flamethrower_on(sprite);
  else flamethrower_off(sprite);
}

/* Hurt any witches in our flame range.
 * Caller asserts state.
 */
 
static void flamethrower_check_damage(struct sprite *sprite) {
  struct sprite_group *grp=g.grpv+NS_sprgrp_fragile;
  if (grp->sprc<1) return;
  
  double l=sprite->x,t=sprite->y,r=sprite->x+sprite->w,b=sprite->y+sprite->h;
  switch (SPRITE->dir) {
    case DIR_N: t-=SPRITE->length; b-=1.0; break;
    case DIR_S: b+=SPRITE->length; t+=1.0; break;
    case DIR_W: l-=SPRITE->length; r-=1.0; break;
    case DIR_E: r+=SPRITE->length; l+=1.0; break;
    default: return;
  }
  
  struct sprite **otherp=grp->sprv;
  int otheri=grp->sprc;
  for (;otheri-->0;otherp++) {
    struct sprite *other=*otherp;
    if (!other->type->injure) continue; // Shouldn't be in fragile group!
    if (other->x>=r) continue;
    if (other->y>=b) continue;
    if (other->x+other->w<=l) continue;
    if (other->y+other->h<=t) continue;
    other->type->injure(other,sprite);
  }
}

/* Update.
 */

static void _flamethrower_update(struct sprite *sprite,double elapsed) {
  flamethrower_advance_time(sprite,elapsed);
  if (SPRITE->state) flamethrower_check_damage(sprite);
}

/* Get grabbed.
 */

static void _flamethrower_grab(struct sprite *sprite,int grab,struct sprite *grabber,uint8_t dir) {
  SPRITE->grabbed=grab;
}

/* Render.
 */

static void _flamethrower_render(struct sprite *sprite,int dstx,int dsty) {
  const int halftile=NS_sys_tilesize>>1;
  int x0=dstx+halftile,y0=dsty+halftile;
  
  // Base tile, constant.
  graf_tile(&g.graf,x0,y0,sprite->tileid,sprite->xform);
  
  /* Clock image above that.
   * It's two tiles (tileid+1=Off,tileid+2=On), and we tick it via xform. It's fully symmetric.
   * The natural clock tile has its indicator on the left side of the top edge.
   * At infinite timescale, just don't draw it.
   */
  if (sprite->timescale!=NS_timescale_infinite) {
    double halfperiod=SPRITE->period*0.5;
    uint8_t clocktile=sprite->tileid+(SPRITE->state?2:1);
    uint8_t clockxform;
    int clockt=(int)((SPRITE->clock*8.0)/halfperiod);
    switch (clockt&7) {
      case 0: clockxform=0; break;
      case 1: clockxform=EGG_XFORM_XREV; break;
      case 2: clockxform=EGG_XFORM_SWAP|EGG_XFORM_YREV; break;
      case 3: clockxform=EGG_XFORM_SWAP|EGG_XFORM_YREV|EGG_XFORM_XREV; break;
      case 4: clockxform=EGG_XFORM_YREV|EGG_XFORM_XREV; break;
      case 5: clockxform=EGG_XFORM_YREV; break;
      case 6: clockxform=EGG_XFORM_SWAP|EGG_XFORM_XREV; break;
      case 7: clockxform=EGG_XFORM_SWAP; break;
    }
    graf_tile(&g.graf,x0,y0,clocktile,clockxform);
  }
  
  /* Flames, if On.
   */
  if (SPRITE->state) {
    int dx=0,dy=0;
    switch (SPRITE->dir) {
      case DIR_N: dy=-NS_sys_tilesize; break;
      case DIR_S: dy=NS_sys_tilesize; break;
      case DIR_W: dx=-NS_sys_tilesize; break;
      case DIR_E: dx=NS_sys_tilesize; break;
    }
    int x=x0+dx,y=y0+dy;
    uint8_t tileid=sprite->tileid+3+2*SPRITE->animframe;
    int i=SPRITE->length;
    for (;i-->0;x+=dx,y+=dy) {
      if (!i) tileid++;
      graf_tile(&g.graf,x,y,tileid,sprite->xform);
    }
  }
}

/* Type definition.
 */

const struct sprite_type sprite_type_flamethrower={
  .name="flamethrower",
  .objlen=sizeof(struct sprite_flamethrower),
  .init=_flamethrower_init,
  .update=_flamethrower_update,
  .render=_flamethrower_render,
  .grab=_flamethrower_grab,
};
