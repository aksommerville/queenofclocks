#include "game/queenofclocks.h"

#define GRAVITY 8.0 /* m/s */
#define CRY_TIME 1.000
#define CRY_PERIOD_MIN 4.000
#define CRY_PERIOD_MAX 6.000

struct sprite_princess {
  struct sprite hdr;
  double cryclock;
};

#define SPRITE ((struct sprite_princess*)sprite)

static void princess_reset_cryclock(struct sprite *sprite) {
  SPRITE->cryclock=CRY_PERIOD_MIN+((rand()&0x7fff)*(CRY_PERIOD_MAX-CRY_PERIOD_MIN))/32768.0;
}

static int _princess_init(struct sprite *sprite) {
  princess_reset_cryclock(sprite);
  return 0;
}

static void _princess_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->cryclock-=elapsed)<0.0) {
    princess_reset_cryclock(sprite);
  }
  sprite_move(sprite,0.0,GRAVITY*elapsed);
}

static void _princess_injure(struct sprite *sprite,struct sprite *assailant) {
  sprite_group_add(g.grpv+NS_sprgrp_deathrow,sprite);
  qc_sound(RID_sound_lady_die,sprite->x+sprite->w*0.5);
  struct sprite *soulballs=sprite_spawn(
    sprite->x+sprite->w*0.5,sprite->y+sprite->h*0.5,
    &sprite_type_soulballs, // not required but we can spare spawn a lookup, we know it
    RID_sprite_soulballs,7, // A witch's soul has seven circles; that's an important bit of Dot lore.
    0,0
  );
  // Debatable: We could fail the level when the princess dies. I kind of think it's funnier to force the player to kill themselves.
}

static void _princess_interact(struct sprite *sprite,struct sprite *hero) {
  if (g.termclock>0.0) return;
  qc_sound(RID_sound_rescue,sprite->x+sprite->w*0.5);
  sprite_hero_force_victory(hero);
}

static void _princess_render(struct sprite *sprite,int dstx,int dsty) {
  dstx+=NS_sys_tilesize>>1;
  dsty+=NS_sys_tilesize>>1;
  if (SPRITE->cryclock<CRY_TIME) {
    graf_tile(&g.graf,dstx,dsty,sprite->tileid+1,0);
    graf_tile(&g.graf,dstx,dsty-NS_sys_tilesize,sprite->tileid+2,0);
  } else {
    graf_tile(&g.graf,dstx,dsty,sprite->tileid,0);
  }
}

const struct sprite_type sprite_type_princess={
  .name="princess",
  .objlen=sizeof(struct sprite_princess),
  .init=_princess_init,
  .update=_princess_update,
  .injure=_princess_injure,
  .interact=_princess_interact,
  .render=_princess_render,
};
