#include "game/queenofclocks.h"

#define WMAX 9 /* Let's keep reasonable, ok? */
#define SPEED 6.0 /* m/s, at Normal scale. */
#define TURN_TIME 0.500 /* Stop for so long after a collision, before resuming the other way. */

struct sprite_platform {
  struct sprite hdr;
  int colc; // Visible in meters. It's also our physical width but let's pretend they could be different.
  int idx,idy;
  double dx,dy;
  double animclock;
  int animframe;
  double turnclock;
  int grabbed;
};

#define SPRITE ((struct sprite_platform*)sprite)

/* Init.
 */
 
static int _platform_init(struct sprite *sprite) {

  SPRITE->idx=(sprite->arg>>24)&0xff; if (SPRITE->idx&0x80) SPRITE->idx|=~0xff;
  SPRITE->idy=(sprite->arg>>16)&0xff; if (SPRITE->idy&0x80) SPRITE->idy|=~0xff;
  if (SPRITE->idx&&SPRITE->idy) {
    fprintf(stderr,"Platform with diagonal motion.\n");
    return -1;
  } else if (!SPRITE->idx&&!SPRITE->idy) {
    // Stationary is ok i guess.
  }
  SPRITE->dx=SPRITE->idx;
  SPRITE->dy=SPRITE->idy;

  // The (w,h) acquired from our resource is not used. We override (w) with a spawn-point param and (h) is constant.
  double midx=sprite->x+sprite->w*0.5;
  SPRITE->colc=(sprite->arg>>8)&0xff;
  if (SPRITE->colc<1) SPRITE->colc=1;
  else if (SPRITE->colc>WMAX) SPRITE->colc=WMAX;
  sprite->w=SPRITE->colc;
  sprite->x=midx-sprite->w*0.5;
  sprite->h=0.5;
  
  return 0;
}

/* Update.
 */
 
static void _platform_update(struct sprite *sprite,double elapsed) {

  if (SPRITE->grabbed) return;
  
  double elapsed_pfr=elapsed;
  switch (sprite->timescale) {
    case 0: return;
    case 1: elapsed*=0.500; break;
    case 2: break;
    case 3: elapsed*=2.000; break;
    case 4: elapsed*=1000.0; break;
  }

  if ((SPRITE->animclock-=elapsed)<=0.0) {
    if ((SPRITE->animclock+=0.150)<=0.0) SPRITE->animclock=0.0; // Clamp at zero, otherwise Infinite Speed would accrue a debt.
    if (++(SPRITE->animframe)>=6) SPRITE->animframe=0;
  }
  if (SPRITE->turnclock>0.0) {
    // Turnclock uses the Privileged Frame of Reference, ie real time.
    // Otherwise things get pretty weird.
    if ((SPRITE->turnclock-=elapsed_pfr)<=0.0) {
      // turnclock elapsed. Try the move again and turn around only if it fails again.
      if (!sprite_move(sprite,SPRITE->dx*SPEED*elapsed,SPRITE->dy*SPEED*elapsed)) {
        SPRITE->dx=-SPRITE->dx;
        SPRITE->dy=-SPRITE->dy;
      }
    }
  } else if (SPRITE->idx) {
    if (!sprite_move(sprite,SPRITE->dx*SPEED*elapsed,0.0)) SPRITE->turnclock=TURN_TIME;
  } else if (SPRITE->idy) {
    if (!sprite_move(sprite,0.0,SPRITE->dy*SPEED*elapsed)) SPRITE->turnclock=TURN_TIME;
  }
}

/* Get grabbed.
 */
 
static void _platform_grab(struct sprite *sprite,int grab,struct sprite *grabber,uint8_t dir) {
  if (SPRITE->grabbed=grab) {
    // If we're moving on the perpendicular axis, snap to the grabber's middle.
    /*XXX or maybe not. This introduces a lot of uncertainty.
    if (grabber) {
      switch (dir) {
        case DIR_W: case DIR_E: if (SPRITE->idy) {
            double ny=grabber->y+grabber->h*0.5-sprite->h*0.5;
            sprite_move(sprite,0.0,ny-sprite->y);
          } break;
        case DIR_N: case DIR_S: {
            double nx=grabber->x+grabber->w*0.5-sprite->w*0.5;
            sprite_move(sprite,nx-sprite->x,0.0);
          } break;
      }
    }
    /**/
  }
}

/* Render.
 */
 
static void _platform_render(struct sprite *sprite,int dstx,int dsty) {
  dsty+=NS_sys_tilesize>>1; // (x) is complicated, but (y) is constant.

  // Width of one has its own tile. Otherwise we compose from an edge and middle tile.
  if (SPRITE->colc<=1) {
    graf_tile(&g.graf,dstx+(NS_sys_tilesize>>1),dsty,sprite->tileid,0);
  } else {
    int x=dstx+(NS_sys_tilesize>>1);
    int i=SPRITE->colc;
    graf_tile(&g.graf,x,dsty,sprite->tileid+1,0);
    x+=NS_sys_tilesize;
    i--;
    for (;i>1;i--,x+=NS_sys_tilesize) graf_tile(&g.graf,x,dsty,sprite->tileid+2,0);
    graf_tile(&g.graf,x,dsty,sprite->tileid+1,EGG_XFORM_XREV);
  }
  
  // Propeller. Animated and centered horizontally.
  int propx=dstx+(int)(sprite->w*NS_sys_tilesize*0.5);
  uint8_t proptile=sprite->tileid;
  switch (SPRITE->animframe) {
    case 0: proptile+=0x03; break;
    case 1: proptile+=0x04; break;
    case 2: proptile+=0x05; break;
    case 3: proptile+=0x06; break;
    case 4: proptile+=0x05; break;
    case 5: proptile+=0x04; break;
  }
  graf_tile(&g.graf,propx,dsty,proptile,0);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_platform={
  .name="platform",
  .objlen=sizeof(struct sprite_platform),
  .carrful=1,
  .init=_platform_init,
  .update=_platform_update,
  .render=_platform_render,
  .grab=_platform_grab,
};
