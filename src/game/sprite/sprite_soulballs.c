/* sprite_soulballs.c
 * arg: (u24)unused (u8)ballc
 * Spawn one at the midpoint of the dead sprite.
 * Use the resource.
 * One soulballs manages the entire set.
 */

#include "game/queenofclocks.h"

#define SPEED 100.0 /* px/s of radius. NB pixels not meters. */
#define BALL_LIMIT 16 /* for sanity's sake */

struct sprite_soulballs {
  struct sprite hdr;
  double ttl;
  double animclock;
  int animframe;
  double radius;
  int ballc;
};

#define SPRITE ((struct sprite_soulballs*)sprite)

static int _soulballs_init(struct sprite *sprite) {
  SPRITE->ttl=1.000;
  SPRITE->ballc=sprite->arg&0xff;
  if (SPRITE->ballc<1) SPRITE->ballc=1;
  else if (SPRITE->ballc>BALL_LIMIT) SPRITE->ballc=BALL_LIMIT;
  return 0;
}

static void _soulballs_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->ttl-=elapsed)<=0.0) {
    sprite_group_add(g.grpv+NS_sprgrp_deathrow,sprite);
    return;
  }
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.125;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
  }
  SPRITE->radius+=SPEED*elapsed;
}

static void _soulballs_render(struct sprite *sprite,int dstx,int dsty) {
  double midx=dstx,midy=dsty;
  uint8_t tileid=sprite->tileid;
  switch (SPRITE->animframe) {
    case 1: case 3: tileid+=1; break;
    case 2: tileid+=2; break;
  }
  double t=0.0,dt=(M_PI*2.0)/SPRITE->ballc;
  int i=SPRITE->ballc;
  for (;i-->0;t+=dt) {
    int x=(int)(midx+SPRITE->radius*cos(t));
    int y=(int)(midy-SPRITE->radius*sin(t));
    graf_tile(&g.graf,x,y,tileid,0);
  }
}

const struct sprite_type sprite_type_soulballs={
  .name="soulballs",
  .objlen=sizeof(struct sprite_soulballs),
  .init=_soulballs_init,
  .update=_soulballs_update,
  .render=_soulballs_render,
};
