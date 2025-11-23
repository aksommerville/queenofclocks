#include "game/queenofclocks.h"

struct sprite_passgate {
  struct sprite hdr;
  int password;
  int open;
};

#define SPRITE ((struct sprite_passgate*)sprite)

static int _passgate_init(struct sprite *sprite) {
  SPRITE->password=sprite->arg;
  SPRITE->open=0; // Begin closed, since the password sprites might not exist yet. Depend on getting an update before our first render.
  return 0;
}

static int passgate_should_open(const struct sprite *sprite) {
  //fprintf(stderr,"%d %s %d %d (open=%d)\n",g.framec,__func__,SPRITE->password,sprite_password_get(),SPRITE->open);
  return (SPRITE->password==sprite_password_get());
}

static void _passgate_update(struct sprite *sprite,double elapsed) {
  // To open and close, we just add/remove render and physics. The sprite remains extant throughout.
  if (passgate_should_open(sprite)) {
    if (SPRITE->open) return;
    SPRITE->open=1;
    qc_sound(RID_sound_passgate_open,sprite->x+sprite->w*0.5);
    sprite_group_remove(g.grpv+NS_sprgrp_render,sprite);
    sprite_group_remove(g.grpv+NS_sprgrp_physics,sprite);
  } else {
    if (!SPRITE->open) return;
    SPRITE->open=0;
    qc_sound(RID_sound_passgate_close,sprite->x+sprite->w*0.5);
    sprite_group_add(g.grpv+NS_sprgrp_render,sprite);
    sprite_group_add(g.grpv+NS_sprgrp_physics,sprite);
    sprite_force_out_collisions(sprite);
  }
}

const struct sprite_type sprite_type_passgate={
  .name="passgate",
  .objlen=sizeof(struct sprite_passgate),
  .init=_passgate_init,
  .update=_passgate_update,
};
