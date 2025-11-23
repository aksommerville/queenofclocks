#include "game/queenofclocks.h"

#define CONVEY_SPEED 6.0 /* m/s at normal timescale */

struct sprite_conveyor {
  struct sprite hdr;
  uint8_t dir; // DIR_W or DIR_E
  double animclock;
  int animframe;
  int w; // meters, >=2
  int grabbed;
};

#define SPRITE ((struct sprite_conveyor*)sprite)

static int _conveyor_init(struct sprite *sprite) {
  SPRITE->w=(sprite->arg>>24);
  if (SPRITE->w<2) SPRITE->w=2;
  else if (SPRITE->w>NS_sys_mapw) SPRITE->w=NS_sys_mapw;
  SPRITE->dir=(sprite->arg>>16)&0xff;
  sprite->timescale=(sprite->arg>>8)&0xff;
  sprite->w=SPRITE->w;
  return 0;
}

static void _conveyor_update(struct sprite *sprite,double elapsed) {
  if (SPRITE->grabbed||(sprite->timescale==NS_timescale_stop)) return;
  switch (sprite->timescale) {
    case NS_timescale_slow: elapsed*=0.500; break;
    case NS_timescale_normal: break;
    case NS_timescale_fast: elapsed*=2.000; break;
    case NS_timescale_infinite: elapsed*=1000.0; break;
    default: return;
  }
  if ((SPRITE->animclock-=elapsed)<0.0) {
    if ((SPRITE->animclock+=0.200)<0.0) SPRITE->animclock=0.0;
    if (SPRITE->dir==DIR_W) {
      if (--(SPRITE->animframe)<0) SPRITE->animframe=3;
    } else {
      if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
    }
  }
  
  // Convey sprites.
  double dx=((SPRITE->dir==DIR_W)?-1.0:1.0)*CONVEY_SPEED*elapsed;
  double l=sprite->x;
  double r=sprite->x+sprite->w;
  double t=sprite->y;
  struct sprite **otherp=g.grpv[NS_sprgrp_physics].sprv;
  int otheri=g.grpv[NS_sprgrp_physics].sprc;
  for (;otheri-->0;otherp++) {
    struct sprite *other=*otherp;
    // Any horizontal overlap. Conveyors should drive pumpkins all the way off their edge.
    if (other->x>r) continue;
    if (other->x+other->w<l) continue;
    // And its foot must be within a smidgeon of my top.
    double qy=other->y+other->h;
    double dy=qy-t;
    if ((dy<-0.001)||(dy>0.001)) continue;
    // If we're at infinite speed, move the sprite all the way to my edge (or as far as it will go).
    if (sprite->timescale==NS_timescale_infinite) {
      if (dx<0.0) sprite_move(other,l-(other->x+other->w),0.0);
      else sprite_move(other,r-other->x,0.0);
    // Regular motion (slow,normal,fast).
    } else {
      sprite_move(other,dx,0.0);
    }
  }
}

static void _conveyor_grab(struct sprite *sprite,int grab,struct sprite *grabber,uint8_t dir) {
  SPRITE->grabbed=grab;
}

static void _conveyor_render(struct sprite *sprite,int dstx,int dsty) {
  dstx+=NS_sys_tilesize>>1;
  dsty+=NS_sys_tilesize>>1;
  uint8_t tileid=sprite->tileid+0x10*SPRITE->animframe;
  graf_tile(&g.graf,dstx,dsty,tileid,0); dstx+=NS_sys_tilesize; tileid++;
  int i=SPRITE->w-2;
  for (;i-->0;dstx+=NS_sys_tilesize) graf_tile(&g.graf,dstx,dsty,tileid,0);
  tileid++;
  graf_tile(&g.graf,dstx,dsty,tileid,0);
}

const struct sprite_type sprite_type_conveyor={
  .name="conveyor",
  .objlen=sizeof(struct sprite_conveyor),
  .init=_conveyor_init,
  .update=_conveyor_update,
  .grab=_conveyor_grab,
  .render=_conveyor_render,
};
