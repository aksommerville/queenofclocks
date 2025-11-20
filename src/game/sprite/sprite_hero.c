#include "game/queenofclocks.h"

struct sprite_hero {
  struct sprite hdr;
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
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int dstx,int dsty) {
  const int ht=NS_sys_tilesize>>1;
  graf_tile(&g.graf,dstx+ht,dsty+ht,sprite->tileid-0x10,sprite->xform);
  graf_tile(&g.graf,dstx+ht,dsty+ht+NS_sys_tilesize,sprite->tileid,sprite->xform);
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
