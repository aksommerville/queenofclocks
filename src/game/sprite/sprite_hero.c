#include "game/queenofclocks.h"

#define WALK_SPEED 10.0 /* m/s. Factors of 60 are less jumpy. */
#define JUMP_INITIAL -22.0 /* m/s */
#define JUMP_ACCEL    40.0 /* m/s**2. With these constants, she can jump five meters, not six. */
#define GRAVITY_ACCEL 30.0 /* m/s**2 */
#define GRAVITY_LIMIT 25.0 /* m/s */
#define GRAVITY_SOUND_THRESHOLD 10.0 /* m/s. With the above constants, a 1m fall is 8ish, and a 2m fall 11ish. Terminal velocity around 11m */

struct sprite_hero {
  struct sprite hdr;
  double animclock;
  int animframe;
  int falling; // -1,0,1 = jump,seated,fall
  int wanding; // 0,1
  int wanddir; // -1,0,1 = up,horz,down
  int walking; // -1,0,1
  double gravity; // Jump or fall power.
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Delete.
 */
 
static void _hero_del(struct sprite *sprite) {
  ctlpan_dismiss(); // Just to be safe.
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {

  // Generic ctor centers on the center of the tile that spawned us. We want to align our foot with its bottom.
  sprite->y-=0.5;
  
  // Optimistically assume that we spawn on solid ground. Should always be the case.
  SPRITE->falling=0;

  return 0;
}

/* Check victory conditions.
 */
 
int sprite_hero_in_victory_position(const struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_hero)) return 0;
  
  // Must be standing still and idle.
  if (SPRITE->falling||SPRITE->wanding||SPRITE->walking) return 0;
  
  // Examine the one cell directly below our feet. It must have "goal" physics.
  int x=(int)(sprite->x+sprite->w*0.5);
  if ((x<0)||(x>=NS_sys_mapw)) return 0;
  int y=(int)(sprite->y+sprite->h+0.5); // sic '+', and x has '*'; different things.
  if ((y<0)||(y>=NS_sys_maph)) return 0;
  if (g.physics[g.cellv[y*NS_sys_mapw+x]]!=NS_physics_goal) return 0;
  
  // Hooray!
  return 1;
}

/* Get dead.
 */
 
static void hero_die(struct sprite *sprite) {
  qc_sound(RID_sound_die,sprite->x+sprite->w*0.5);
  struct sprite *soulballs=sprite_spawn(
    sprite->x+sprite->w*0.5,sprite->y+sprite->h*0.5,
    &sprite_type_soulballs, // not required but we can spare spawn a lookup, we know it
    RID_sprite_soulballs,7, // A witch's soul has seven circles; that's an important bit of Dot lore.
    0,0
  );
  sprite_group_add(g.grpv+NS_sprgrp_deathrow,sprite);
}

/* Update walking.
 * This is called every frame, with the current dpad state, whether walking or not.
 */
 
static void hero_update_walk(struct sprite *sprite,int dx,double elapsed) {
  
  // No walking while wanding.
  if (SPRITE->wanding) dx=0;
  
  // Is it a change of state?
  if (dx!=SPRITE->walking) {
    SPRITE->walking=dx;
    if (dx>0) sprite->xform=0;
    else if (dx<0) sprite->xform=EGG_XFORM_XREV;
  }
  
  // If nonzero, apply motion and animation.
  if (SPRITE->walking) {
    sprite_move(sprite,WALK_SPEED*elapsed*SPRITE->walking,0.0);
    
    if ((SPRITE->animclock-=elapsed)<=0.0) {
      SPRITE->animclock+=0.200;
      if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
    }
  } else {
    SPRITE->animclock=0.0;
    SPRITE->animframe=0;
  }
}

/* Update gravity or jump.
 * Called every frame, with the state of the jump button.
 */
 
static void hero_update_gravity(struct sprite *sprite,int jumpreq,double elapsed) {

  // If a jump begins or continues, do it and return.
  if (jumpreq) {
    if (SPRITE->falling<0) {
      // Ongoing jump.
      SPRITE->gravity+=JUMP_ACCEL*elapsed;
      if (SPRITE->gravity>=0.0) {
        // Jump limit.
        SPRITE->falling=1;
      } else {
        double dy=SPRITE->gravity*elapsed;
        if (!sprite_move(sprite,0.0,dy)) {
          // Head bonk. Wanna use it?
        }
      }
      return;
    } else if ((SPRITE->falling==0)&&!(g.pvinput&EGG_BTN_SOUTH)) {
      if (g.input&EGG_BTN_DOWN) {
        // Begin downjump.
        //TODO Downjump. Are oneways going to be a thing?
        return;
      } else {
        // Begin regular jump.
        qc_sound(RID_sound_jump,sprite->x);
        SPRITE->falling=-1;
        SPRITE->gravity=JUMP_INITIAL;
        return;
      }
    }
    
  // Released jump but gravity still negative? Zero it before proceeding.
  } else if (SPRITE->gravity<0.0) {
    SPRITE->gravity=0.0;
  }
  
  
  // Try to move down. If it fails completely, we are seated.
  SPRITE->gravity+=GRAVITY_ACCEL*elapsed;
  if (SPRITE->gravity>=GRAVITY_LIMIT) SPRITE->gravity=GRAVITY_LIMIT;
  double dy=SPRITE->gravity*elapsed;
  if (sprite_move(sprite,0.0,dy)) {
    if (SPRITE->falling<1) {
      // Begin fall.
      SPRITE->falling=1;
    }
  } else if (SPRITE->falling) {
    // End fall.
    if (SPRITE->gravity>GRAVITY_SOUND_THRESHOLD) qc_sound(RID_sound_land,sprite->x);
    SPRITE->falling=0;
    SPRITE->gravity=0.0;
  } else {
    SPRITE->gravity=0.0;
  }
}

/* Scan bounds for wand.
 */
 
static void hero_get_wand_bounds(struct frect *dst,const struct sprite *sprite) {
  switch (SPRITE->wanddir) {
    case -1: {
        dst->l=sprite->x;
        dst->r=sprite->x+sprite->w;
        dst->t=0.0; // TODO limit per grid
        dst->b=sprite->y;
      } break;
    case 1: {
        dst->l=sprite->x;
        dst->r=sprite->x+sprite->w;
        dst->t=sprite->y+sprite->h;
        dst->b=NS_sys_maph; // TODO limit per grid
      } break;
    default: if (sprite->xform&EGG_XFORM_XREV) {
        dst->l=0.0; // TODO limit per grid
        dst->r=sprite->x;
        dst->t=sprite->y;
        dst->b=sprite->y+sprite->h;
      } else {
        dst->l=sprite->x+sprite->w;
        dst->r=NS_sys_mapw; // TODO limit per grid
        dst->t=sprite->y;
        dst->b=sprite->y+sprite->h;
      }
  }
}

/* Compare two candidate pumpkins.
 * Both are already known to be in bounds.
 * (but either may be null).
 * Return <0 if (a) is nearer or >0 if (b).
 */
 
static int hero_pumpkincmp(const struct sprite *sprite,const struct sprite *a,const struct sprite *b) {
  if (!a) return 1;
  if (!b) return -1;
  switch (SPRITE->wanddir) {
    case -1: {
        double aq=a->y+a->h,bq=b->y+b->h;
        if (aq>bq) return -1;
        if (aq<bq) return 1;
      } break;
    case 1: {
        if (a->y<b->y) return -1;
        if (a->y>b->y) return 1;
      } break;
    default: if (sprite->xform&EGG_XFORM_XREV) {
        double aq=a->x+a->w,bq=b->x+b->w;
        if (aq>bq) return -1;
        if (aq<bq) return 1;
      } else {
        if (a->x<b->x) return -1;
        if (a->x>b->x) return 1;
      }
  }
  return 0;
}

/* Generic cardinal direction bit from my wand direction.
 */
 
static uint8_t hero_dir_from_wanddir(const struct sprite *sprite) {
  switch (SPRITE->wanddir) {
    case -1: return DIR_N;
    case 1: return DIR_S;
    default: return (sprite->xform&EGG_XFORM_XREV)?DIR_W:DIR_E;
  }
}

/* Update wand, in progress.
 */
 
static void hero_update_wand_active(struct sprite *sprite,double elapsed) {

  // If we have some sprite locked, ctlpan takes over. Give it some room.
  struct sprite *pumpkin=ctlpan_is_active();
  if (pumpkin) return;
  
  // Find a sprite to lock on.
  struct frect bounds;
  hero_get_wand_bounds(&bounds,sprite);
  struct sprite **otherp=g.grpv[NS_sprgrp_motion].sprv;
  int otheri=g.grpv[NS_sprgrp_motion].sprc;
  for (;otheri-->0;otherp++) {
    struct sprite *other=*otherp;
    if (!other->type->grab) continue; // Shouldn't be in motion group.
    if (other->x>=bounds.r) continue;
    if (other->y>=bounds.b) continue;
    if (other->x+other->w<=bounds.l) continue;
    if (other->y+other->h<=bounds.t) continue;
    if (hero_pumpkincmp(sprite,other,pumpkin)<0) pumpkin=other;
  }
  
  // If we're still unattached, allow changing direction.
  if (!pumpkin) {
    switch (g.input&(EGG_BTN_UP|EGG_BTN_DOWN)) {
      case EGG_BTN_UP: SPRITE->wanddir=-1; break;
      case EGG_BTN_DOWN: SPRITE->wanddir=1; break;
      default: SPRITE->wanddir=0; switch (g.input&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
          case EGG_BTN_LEFT: sprite->xform=EGG_XFORM_XREV; break;
          case EGG_BTN_RIGHT: sprite->xform=0; break;
        } break;
    }
    return;
  }
  
  // Initiate grabbenation.
  if (ctlpan_begin(pumpkin)<0) return;
  pumpkin->type->grab(pumpkin,1,sprite,hero_dir_from_wanddir(sprite));
}

/* Update the wand.
 * Called every frame, with the state of the actuator button.
 */
 
static void hero_update_wand(struct sprite *sprite,int pressed,double elapsed) {

  // No wand in midair.
  if (SPRITE->falling) pressed=0;
  
  if (pressed==SPRITE->wanding) {
    if (pressed) hero_update_wand_active(sprite,elapsed);
    return;
  }
  SPRITE->wanding=pressed;

  // When we first enter the wand state, commit to a direction.
  if (SPRITE->wanding) {
    switch (g.input&(EGG_BTN_UP|EGG_BTN_DOWN)) {
      case EGG_BTN_UP: SPRITE->wanddir=-1; break;
      case EGG_BTN_DOWN: SPRITE->wanddir=1; break;
      default: SPRITE->wanddir=0; break;
    }
    hero_update_wand_active(sprite,elapsed);
  
  // Release wand, might need to notify the pumpkin.
  } else {
    struct sprite *pumpkin=ctlpan_is_active();
    if (pumpkin) {
      pumpkin->type->grab(pumpkin,0,sprite,hero_dir_from_wanddir(sprite));
      ctlpan_dismiss();
    }
  }
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {
  
  switch (g.input&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: hero_update_walk(sprite,-1,elapsed); break;
    case EGG_BTN_RIGHT: hero_update_walk(sprite,1,elapsed); break;
    default: hero_update_walk(sprite,0,elapsed); break;
  }
  
  if (g.input&EGG_BTN_WEST) hero_update_wand(sprite,1,elapsed);
  else hero_update_wand(sprite,0,elapsed);
  
  if (g.input&EGG_BTN_SOUTH) hero_update_gravity(sprite,1,elapsed);
  else hero_update_gravity(sprite,0,elapsed);
  
  if (sprite_touches_grid_physics(sprite,NS_physics_hazard)) hero_die(sprite);
}

/* Get hurt, generically.
 */
 
static void _hero_injure(struct sprite *sprite,struct sprite *assailant) {
  hero_die(sprite);
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
  
  // Wand takes precedence.
  if (SPRITE->wanding) {
    switch (SPRITE->wanddir) {
      case -1: tileid=0x27; ADDL(0,-2,0x07) break;
      case 0: tileid=0x15; ADDL(1,0,0x16) break;
      case 1: tileid=0x18; ADDL(0,1,0x28) break;
    }
    
  // Jumping and falling are each single frames.
  } else if (SPRITE->falling<0) {
    tileid=0x13;
  } else if (SPRITE->falling>0) {
    tileid=0x14;
  
  // Walking, apply the animation frame.
  } else if (SPRITE->walking) {
    switch (SPRITE->animframe) {
      case 1: tileid+=1; break;
      case 3: tileid+=2; break;
    }
    
  // Anything else, we're idle and the defaults are correct.
  }
  graf_tile(&g.graf,bodyx,bodyy-NS_sys_tilesize,tileid-0x10,xform);
  graf_tile(&g.graf,bodyx,bodyy,tileid,xform);
  #undef ADDL
}

/* Type definition.
 */

const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
  .injure=_hero_injure,
};
