#include "game/queenofclocks.h"

#define WALK_SPEED 3.0 /* m/s */
#define TURN_TIME 0.500
#define GRAVITY 8.0 /* m/s */

struct sprite_fryguy {
  struct sprite hdr;
  uint8_t tileid0;
  int grabbed;
  double animclock;
  int animframe;
  double turnclock;
};

#define SPRITE ((struct sprite_fryguy*)sprite)

/* Init.
 */
 
static int _fryguy_init(struct sprite *sprite) {
  SPRITE->tileid0=sprite->tileid;
  if ((sprite->arg>>24)==0x10) sprite->xform=EGG_XFORM_XREV;
  sprite->timescale=(sprite->arg>>16)&0xff;
  return 0;
}

/* Animation and motion.
 */

static void fryguy_advance_time(struct sprite *sprite,double elapsed) {
  if (SPRITE->grabbed) return;
  
  // Turnclock is not subject to relativity, and inhibits regular motion.
  int turning=0;
  if (SPRITE->turnclock>0.0) {
    SPRITE->turnclock-=elapsed;
    turning=1;
  }
  
  double kelapsed=elapsed;
  switch (sprite->timescale) {
    case NS_timescale_stop: return;
    case NS_timescale_slow: elapsed*=0.500; break;
    case NS_timescale_normal: break;
    case NS_timescale_fast: elapsed*=2.000; break;
    case NS_timescale_infinite: elapsed*=1000.0; break;
    default: return;
  }
  
  if (sprite->timescale==NS_timescale_infinite) SPRITE->animclock-=kelapsed*4.000;
  else SPRITE->animclock-=elapsed;
  if (SPRITE->animclock<=0.0) {
    if ((SPRITE->animclock+=0.200)<0.0) SPRITE->animclock=0.0;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
    sprite->tileid=SPRITE->tileid0+SPRITE->animframe;
  }
  
  if (!turning) {
    double dx=WALK_SPEED*elapsed;
    if (sprite->xform&EGG_XFORM_XREV) dx=-dx;
    if (!sprite_move(sprite,dx,0.0)) {
      SPRITE->turnclock=TURN_TIME;
      sprite->xform^=EGG_XFORM_XREV;
    }
  }
  sprite_move(sprite,0.0,GRAVITY*elapsed);
}

/* Update.
 */
 
static void _fryguy_update(struct sprite *sprite,double elapsed) {
  fryguy_advance_time(sprite,elapsed);
}

/* Get grabbed.
 */
 
static void _fryguy_grab(struct sprite *sprite,int grab,struct sprite *grabber,uint8_t dir) {
  SPRITE->grabbed=grab;
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_fryguy={
  .name="fryguy",
  .objlen=sizeof(struct sprite_fryguy),
  .init=_fryguy_init,
  .update=_fryguy_update,
  .grab=_fryguy_grab,
};
