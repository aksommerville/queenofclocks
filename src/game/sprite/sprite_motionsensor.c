#include "game/queenofclocks.h"

#define BLINK_CLOSE_TIME 0.250
#define NOZZLE_SPEED 0.500 /* radian/sec, at normal timescale */
#define GLOAT_TIME 0.750

struct sprite_motionsensor {
  struct sprite hdr;
  int grabbed;
  double eyet; // What direction am I looking? Radians -pi/2..pi/2. <-5 if none
  double aimt; // What direction is the nozzle pointed? Radians.
  struct sprite_group target; // Zero or one.
  double blinkclock;
  double lax,lay,lzx,lzy; // Laser line, when engaged. (lax,lay) is actually constant, it's my bottom center.
  double gloatclock; // Linger in one place, with the laser on, after killing something.
};

#define SPRITE ((struct sprite_motionsensor*)sprite)

/* Delete.
 */
 
static void _motionsensor_del(struct sprite *sprite) {
  sprite_group_cleanup(&SPRITE->target);
}

/* Select blink time randomly.
 */
 
static void motionsensor_replace_blinkclock(struct sprite *sprite) {
  SPRITE->blinkclock=0.500+((rand()&0x7fff)*2.000)/32768.0;
}

/* Init.
 */
 
static int _motionsensor_init(struct sprite *sprite) {
  SPRITE->eyet=-10.0;
  motionsensor_replace_blinkclock(sprite);
  return 0;
}

/* Return the point in (other)'s bounds which is nearest to (x,y).
 * If (x,y) is in bounds, returns the same. Otherwise something on (other)'s perimeter.
 */
 
static void motionsensor_nearest_point(double *dstx,double *dsty,double x,double y,const struct sprite *other) {
  double l=other->x;
  if (l>=x) {
    *dstx=l;
  } else {
    double r=l+other->w;
    if (r<=x) *dstx=r;
    else *dstx=x;
  }
  double t=other->y;
  if (t>=y) {
    *dsty=t;
  } else {
    double b=t+other->h;
    if (b<=y) *dsty=b;
    else *dsty=y;
  }
}

/* Scan sprites and acquire a target if one exists.
 */
 
static void motionsensor_seek_target(struct sprite *sprite) {
  if (SPRITE->target.sprc) return;
  
  /* Find the nearest sprite to us which is in the detectable group,
   * and whose foot is below our vertical middle.
   * Also filter out those where no line of sight exists.
   */
  double midx=sprite->x+sprite->w*0.5;
  double midy=sprite->y+sprite->h*0.5;
  struct sprite *best=0;
  double bestdist2;
  struct sprite **qp=g.grpv[NS_sprgrp_detectable].sprv;
  int i=g.grpv[NS_sprgrp_detectable].sprc;
  for (;i-->0;qp++) {
    struct sprite *q=*qp;
    if (q==sprite) continue; // Don't target this one, despite it being very close
    if (q->y+q->h<=midy) continue;
    double qx,qy;
    motionsensor_nearest_point(&qx,&qy,midx,midy,q);
    double dx=qx-midx,dy=qy-midy;
    double dist2=dx*dx+dy*dy;
    if (!best||(dist2<bestdist2)) {
      if (!sprites_have_line_of_sight(sprite,q)) continue;
      best=q;
      bestdist2=dist2;
    }
  }
  if (!best) return;
  
  sprite_group_add(&SPRITE->target,best);
}

/* We have a target. Confirm there is still a line of sight.
 */
 
static void motionsensor_validate_target(struct sprite *sprite) {
  if (SPRITE->target.sprc<1) return;
  struct sprite *target=SPRITE->target.sprv[0];
  if (sprites_have_line_of_sight(sprite,target)) return;
  sprite_group_remove(&SPRITE->target,target);
}

/* With (aimt) set, calculate the endpoints of my laser.
 */
 
static void motionsensor_calculate_line(struct sprite *sprite) {
  SPRITE->lax=sprite->x+sprite->w*0.5;
  SPRITE->lay=sprite->y+sprite->h;
  extend_line_of_sight(&SPRITE->lzx,&SPRITE->lzy,SPRITE->lax,SPRITE->lay,SPRITE->aimt);
}

/* With (lax,lay,lzx,lzy) set, scan fragile sprites and kill to taste.
 */
 
static void motionsensor_burninate(struct sprite *sprite) {
  struct sprite **victimp=g.grpv[NS_sprgrp_fragile].sprv;
  int victimi=g.grpv[NS_sprgrp_fragile].sprc;
  for (;victimi-->0;victimp++) {
    struct sprite *victim=*victimp;
    if (!victim->type->injure) continue; // Should not have been in fragile group in the first place.
    if (!sprite_touches_line(victim,SPRITE->lax,SPRITE->lay,SPRITE->lzx,SPRITE->lzy)) continue;
    victim->type->injure(victim,sprite);
    SPRITE->gloatclock=GLOAT_TIME;
  }
}

/* Move (aimt), the nozzle's orientation, to approach (t), respecting timescale.
 */
 
static void motionsensor_move_nozzle(struct sprite *sprite,double t,double elapsed) {
  if (sprite->timescale==NS_timescale_stop) return;
  if (sprite->timescale==NS_timescale_infinite) {
    SPRITE->aimt=t;
    return;
  }
  double speed=NOZZLE_SPEED*elapsed;
  switch (sprite->timescale) {
    case NS_timescale_slow: speed*=0.500; break;
    case NS_timescale_fast: speed*=2.000; break;
  }
  if (t<SPRITE->aimt) {
    if ((SPRITE->aimt-=speed)<=t) SPRITE->aimt=t;
  } else {
    if ((SPRITE->aimt+=speed)>=t) SPRITE->aimt=t;
  }
}

/* Update.
 */
 
static void _motionsensor_update(struct sprite *sprite,double elapsed) {
  
  if (SPRITE->grabbed) {
    if (SPRITE->target.sprc) motionsensor_burninate(sprite);
    return;
  }
  
  // If we just killed something, keep the light on and don't move. Bask in the joy of it for a sec.
  if (SPRITE->gloatclock>0.0) {
    SPRITE->eyet=-10.0;
    if ((SPRITE->blinkclock-=elapsed)<=0.0) {
      motionsensor_replace_blinkclock(sprite);
    }
    motionsensor_burninate(sprite);
    if ((SPRITE->gloatclock-=elapsed)>0.0) return;
  }

  // No target? Try to acquire one.
  if (!SPRITE->target.sprc) motionsensor_seek_target(sprite);
  else motionsensor_validate_target(sprite);
  
  // Target? Aim eye directly at it, immediately. And incline the nozzle toward it, respecting timescale.
  if (SPRITE->target.sprc) {
    if (sprite->timescale==NS_timescale_stop) {
    } else {
      struct sprite *target=SPRITE->target.sprv[0];
      double targett=atan2(sprite->x+sprite->w*0.5-(target->x+target->w*0.5),target->y+target->h*0.5-(sprite->y+sprite->h));
      if (targett<-M_PI*0.5) targett=-M_PI*0.5;
      else if (targett>M_PI*0.5) targett=M_PI*0.5;
      SPRITE->eyet=targett;
      motionsensor_move_nozzle(sprite,targett,elapsed);
      motionsensor_calculate_line(sprite);
    }
    motionsensor_burninate(sprite);
  
  // No target. Eye to the center, and blink from time to time. Move (aimt) back to zero.
  } else {
    SPRITE->eyet=-10.0;
    if ((SPRITE->blinkclock-=elapsed)<=0.0) {
      motionsensor_replace_blinkclock(sprite);
    }
    motionsensor_move_nozzle(sprite,0.0,elapsed);
  }
}

/* Get grabbed.
 */
 
static void _motionsensor_grab(struct sprite *sprite,int grab,struct sprite *grabber,uint8_t dir) {
  SPRITE->grabbed=grab;
}

/* Render.
 */
 
static void _motionsensor_render(struct sprite *sprite,int dstx,int dsty) {
  int midx=dstx+(NS_sys_tilesize>>1);
  int midy=dsty+(NS_sys_tilesize>>1);
  
  // Laser blast.
  if (SPRITE->target.sprc||(SPRITE->gloatclock>0.0)) {
    graf_set_input(&g.graf,0);
    graf_line(&g.graf,
      (int)(SPRITE->lax*NS_sys_tilesize),(int)(SPRITE->lay*NS_sys_tilesize),(g.framec&4)?0xff80ffc0:0xff4080c0,
      (int)(SPRITE->lzx*NS_sys_tilesize),(int)(SPRITE->lzy*NS_sys_tilesize),(g.framec&4)?0xff00ffc0:0xff0080c0
    );
    graf_set_input(&g.graf,g.texid_sprites);
  }
  
  // Eyeball.
  uint8_t eyetile=sprite->tileid+2;
  if (SPRITE->eyet>-5.0) {
    int frame=(int)(((SPRITE->eyet+M_PI*0.5)*5.0)/M_PI);
    if (frame<0) frame=0; else if (frame>4) frame=4;
    eyetile=sprite->tileid+3+frame;
  }
  graf_tile(&g.graf,midx,midy,eyetile,0);
  
  // Skin.
  uint8_t skintile=sprite->tileid;
  if ((SPRITE->eyet<-5.0)&&(SPRITE->blinkclock<BLINK_CLOSE_TIME)) skintile+=1;
  graf_tile(&g.graf,midx,midy,skintile,0);
  
  // Nozzle.
  int rot8=((int)((SPRITE->aimt*256.0)/(M_PI*2.0)))&0xff;
  graf_fancy(
    &g.graf,midx,dsty+NS_sys_tilesize,sprite->tileid+8,0,
    rot8,NS_sys_tilesize,0,0x808080ff
  );
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_motionsensor={
  .name="motionsensor",
  .objlen=sizeof(struct sprite_motionsensor),
  .del=_motionsensor_del,
  .init=_motionsensor_init,
  .update=_motionsensor_update,
  .grab=_motionsensor_grab,
  .render=_motionsensor_render,
};
