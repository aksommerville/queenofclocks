#include "game/queenofclocks.h"

static int gpassword_dirty=1;
static int gpassword=0;

struct sprite_password {
  struct sprite hdr;
  uint8_t v; // 0..9
  uint8_t tileid0; // The background. Next 10 tiles are digits 0..9
  // You'd think we don't need to record tileid0 but we do: Our actual (tileid) is the naked digit, so passgate can read it generically.
  int grabbed;
  double clock;
};

#define SPRITE ((struct sprite_password*)sprite)

static void _password_del(struct sprite *sprite) {
  gpassword_dirty=1;
}

static int _password_init(struct sprite *sprite) {
  gpassword_dirty=1;
  SPRITE->tileid0=sprite->tileid;
  SPRITE->v=(sprite->arg>>24)&0xff;
  if (SPRITE->v>=10) SPRITE->v=0;
  sprite->timescale=(sprite->arg>>16)&0xff;
  if (sprite->timescale!=NS_timescale_infinite) { // Infinite timescale, we do not display a digit.
    sprite->tileid=SPRITE->tileid0+1+SPRITE->v;
  }
  return 0;
}

static void _password_update(struct sprite *sprite,double elapsed) {
  if (SPRITE->grabbed) return; // No change while grabbed.
  switch (sprite->timescale) {
    case NS_timescale_slow: elapsed*=0.500; break;
    case NS_timescale_normal: break;
    case NS_timescale_fast: elapsed*=2.000; break;
    case NS_timescale_infinite: sprite->tileid=SPRITE->tileid0; return;
    default: return; // stop or invalid: Don't change.
  }
  if ((SPRITE->clock-=elapsed)<=0.0) {
    SPRITE->clock+=0.500;
    if (++(SPRITE->v)>9) SPRITE->v=0;
    gpassword_dirty=1;
  }
  sprite->tileid=SPRITE->tileid0+1+SPRITE->v;
}

static void _password_grab(struct sprite *sprite,int grab,struct sprite *grabber,uint8_t dir) {
  SPRITE->grabbed=grab;
}

static void _password_render(struct sprite *sprite,int dstx,int dsty) {
  dstx+=NS_sys_tilesize>>1;
  dsty+=NS_sys_tilesize>>1;
  graf_tile(&g.graf,dstx,dsty,SPRITE->tileid0,0);
  if (sprite->timescale!=NS_timescale_infinite) {
    graf_tile(&g.graf,dstx,dsty,sprite->tileid,0);
  }
}

const struct sprite_type sprite_type_password={
  .name="password",
  .objlen=sizeof(struct sprite_password),
  .init=_password_init,
  .update=_password_update,
  .grab=_password_grab,
  .render=_password_render,
};

/* Some extra global support to acquire and cache the currently-visible password.
 * A map may only contain one password. The sprites must be arranged in a contiguous row.
 */
 
static int sprite_password_read() {

  // Collect sprite values into a list of columns. Can't have more than one per column.
  int v_by_col[NS_sys_mapw];
  memset(v_by_col,0xff,sizeof(v_by_col));
  struct sprite **p=g.grpv[NS_sprgrp_motion].sprv;
  int i=g.grpv[NS_sprgrp_motion].sprc;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->type!=&sprite_type_password) continue;
    int col=(int)(sprite->x+sprite->w*0.5);
    if ((col<0)||(col>=NS_sys_mapw)) return -1; // Call it invalid if a password digit is offscreen. Won't happen.
    if (v_by_col[col]>=0) {
      fprintf(stderr,"map:%d contains multiple password sprites in the same column\n",g.mapid);
      return -1;
    }
    if (sprite->timescale==NS_timescale_infinite) return -1; // Invalid if any digit has infinite speed -- nothing gets displayed.
    v_by_col[col]=SPRITE->v;
  }
  
  // Read columns right to left.
  int col=NS_sys_mapw-1;
  while ((col>=0)&&(v_by_col[col]<0)) col--;
  if (col<0) {
    fprintf(stderr,"map:%d contains no password sprites but somebody tried to read the password\n",g.mapid);
    return -1;
  }
  int v=0,p10=1;
  while ((col>=0)&&(v_by_col[col]>=0)) {
    v+=v_by_col[col]*p10;
    p10*=10;
    if (p10>=1000000000) { // Cut off at 8 digits, to stay well clear of overflow.
      fprintf(stderr,"map:%d contains a password longer than 8 digits\n",g.mapid);
      return -1;
    }
    col--;
  }
  
  // Continue reading to verify that all remaining columns are empty.
  while (col>=0) {
    if (v_by_col[col]>=0) {
      fprintf(stderr,"map:%d contains disjoint password sprites\n",g.mapid);
      return -1;
    }
    col--;
  }
  
  return v;
}

int sprite_password_get() {
  if (gpassword_dirty) {
    gpassword=sprite_password_read();
    gpassword_dirty=0;
  }
  return gpassword;
}
