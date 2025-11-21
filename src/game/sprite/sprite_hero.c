#include "game/queenofclocks.h"

struct sprite_hero {
  struct sprite hdr;
  double animclock;
  int animframe;
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Delete.
 */
 
static void _hero_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {

  // Generic ctor centers on the center of the tile that spawned us. We want to align our foot with its bottom.
  sprite->y-=0.5;

  return 0;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.500;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
  }
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int dstx,int dsty) {
  const int ht=NS_sys_tilesize>>1;
  int bodyx=dstx+ht;
  int bodyy=dsty+ht+NS_sys_tilesize;
  uint8_t tileid=sprite->tileid;
  uint8_t xform=sprite->xform;
  #define ADDL(dx,dy,tid) graf_tile(&g.graf,bodyx+(xform?-(dx):(dx))*NS_sys_tilesize,bodyy+(dy)*NS_sys_tilesize,tid,xform);
  switch (SPRITE->animframe) {
    case 0: tileid=0x10; break;
    case 1: tileid=0x15; ADDL(1,0,0x16) break;
    case 2: tileid=0x27; ADDL(0,-2,0x07) break;
    case 3: tileid=0x18; ADDL(0,1,0x28) break;
  }
  graf_tile(&g.graf,bodyx,bodyy-NS_sys_tilesize,tileid-0x10,xform);
  graf_tile(&g.graf,bodyx,bodyy,tileid,xform);
  #undef ADDL
}

/* Type definition.
 */

const struct sprite_type sprite_type_hero={
  .name="dummy",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
};
