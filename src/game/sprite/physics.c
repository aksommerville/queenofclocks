#include "game/queenofclocks.h"

#define SMALL 0.001

/* Vector conveniences.
 */
 
uint8_t dir_from_vector(double dx,double dy) {
  if (dx<0.0) {
    if (dy<0.0) return DIR_NW;
    if (dy>0.0) return DIR_SW;
    return DIR_W;
  }
  if (dx>0.0) {
    if (dy<0.0) return DIR_NE;
    if (dy>0.0) return DIR_SE;
    return DIR_E;
  }
  if (dy<0.0) return DIR_N;
  if (dy>0.0) return DIR_S;
  return DIR_MID;
}

void vector_from_dir(double *dx,double *dy,uint8_t dir) {
  switch (dir) {
    case DIR_NW: *dx=-1.0; *dy=-1.0; break;
    case DIR_N:  *dx= 0.0; *dy=-1.0; break;
    case DIR_NE: *dx= 1.0; *dy=-1.0; break;
    case DIR_W:  *dx=-1.0; *dy= 0.0; break;
    case DIR_MID:*dx= 0.0; *dy= 0.0; break;
    case DIR_E:  *dx= 1.0; *dy= 0.0; break;
    case DIR_SW: *dx=-1.0; *dy= 1.0; break;
    case DIR_S:  *dx= 0.0; *dy= 1.0; break;
    case DIR_SE: *dx= 1.0; *dy= 1.0; break;
  }
}

/* Check grid for solids.
 */
 
int sprite_collides_grid(const struct sprite *sprite,int x,int y,int w,int h) {
  if (x<0) { w+=x; x=0; }
  if (y<0) { h+=y; y=0; }
  if (x>NS_sys_mapw-w) w=NS_sys_mapw-x;
  if (y>NS_sys_maph-h) h=NS_sys_maph-y;
  if ((w<1)||(h<1)) return 0;
  const uint8_t *row=g.cellv+y*NS_sys_mapw+x;
  int yi=h;
  for (;yi-->0;row+=NS_sys_mapw) {
    const uint8_t *p=row;
    int xi=w;
    for (;xi-->0;p++) {
      // For now, all sprites are alike and the only physics value we care about is "solid".
      if (g.physics[*p]==NS_physics_solid) return 1;
    }
  }
  return 0;
}

/* Move sprite, public entry point.
 */
 
int sprite_move(struct sprite *sprite,double dx,double dy) {
  
  // Record the direction as a scalar. Recur if diagonal, and terminate if zero.
  uint8_t dir=dir_from_vector(dx,dy);
  switch (dir) {
    case DIR_NW: case DIR_NE: case DIR_SW: case DIR_SE: {
        return sprite_move(sprite,dx,0.0)||sprite_move(sprite,0.0,dy);
      }
    case DIR_MID: return 0;
  }
  
  // Take the combined bounds of current and desired position.
  double nx=sprite->x+dx;
  double ny=sprite->y+dy;
  double fullx,fully,fullw,fullh;
  switch (dir) {
    case DIR_W: fully=sprite->y; fullh=sprite->h; fullx=nx; fullw=sprite->x+sprite->w-nx; break;
    case DIR_E: fully=sprite->y; fullh=sprite->h; fullx=sprite->x; fullw=nx+sprite->w-sprite->x; break;
    case DIR_N: fullx=sprite->x; fullw=sprite->w; fully=ny; fullh=sprite->y+sprite->h-ny; break;
    case DIR_S: fullx=sprite->x; fullw=sprite->w; fully=sprite->y; fullh=ny+sprite->h-sprite->y; break;
  }
  
  // Read the grid in the direction of travel.
  // We can stop at the first collision.
  int cola=(int)(fullx);
  int colz=(int)(fullx+fullw-SMALL);
  int rowa=(int)(fully);
  int rowz=(int)(fully+fullh-SMALL);
  if (cola<0) cola=0;
  if (colz>=NS_sys_mapw) colz=NS_sys_mapw-1;
  if (rowa<0) rowa=0;
  if (rowz>=NS_sys_maph) rowz=NS_sys_maph-1;
  if ((cola<=colz)&&(rowa<=rowz)) {
    int colc=colz-cola+1,rowc=rowz-rowa+1;
    if (dir&(DIR_W|DIR_E)) { // Moving horizontally, read one column at a time.
      int coloob,col,dcol;
      if (dir==DIR_W) { col=colz; coloob=cola-1; dcol=-1; }
      else { col=cola; coloob=colz+1; dcol=1; }
      for (;col!=coloob;col+=dcol) {
        if (sprite_collides_grid(sprite,col,rowa,1,rowc)) {
          // Collision against grid horizontally.
          if (dir==DIR_W) {
            nx=col+1.0;
            if (nx>=sprite->x) return 0;
            fullw=sprite->x+sprite->w-nx;
          } else {
            nx=col-sprite->w;
            if (nx<=sprite->x) return 0;
            fullw=nx+sprite->w-sprite->x;
          }
          goto _done_grid_;
        }
      }
    } else { // Moving vertically, read one row at a time.
      int rowoob,row,drow;
      if (dir==DIR_S) { row=rowz; rowoob=rowa-1; drow=-1; }
      else { row=rowa; rowoob=rowz+1; drow=1; }
      for (;row!=rowoob;row+=drow) {
        if (sprite_collides_grid(sprite,cola,row,colc,1)) {
          // Collision against grid vertically.
          if (dir==DIR_N) {
            ny=row+1.0;
            if (ny>=sprite->y) return 0;
            fullh=sprite->y+sprite->h-ny;
          } else {
            ny=row-sprite->h;
            if (ny<=sprite->y) return 0;
            fullh=ny+sprite->h-sprite->y;
          }
          goto _done_grid_;
        }
      }
    }
   _done_grid_:;
  }
  
  // Check other solid sprites.
  double fullr=fullx+fullw;
  double fullb=fully+fullh;
  struct sprite **otherp=g.grpv[NS_sprgrp_physics].sprv;
  int otheri=g.grpv[NS_sprgrp_physics].sprc;
  for (;otheri-->0;otherp++) {
    struct sprite *other=*otherp;
    
    // First the basic culling.
    if (other==sprite) continue; // This collision is ok ;)
    if (other->x>=fullr) continue;
    if (other->y>=fullb) continue;
    if (other->x+other->w<=fullx) continue;
    if (other->y+other->h<=fully) continue;
    
    // If we are carrful and moving up, try moving the other first.
    if (sprite->type->carrful&&(dir==DIR_N)&&0) {
      if (sprite_move(other,0.0,dy)) continue;
    }
    
    // Collision against solid sprite.
    switch (dir) {
      case DIR_W: {
          nx=other->x+other->w;
          if (nx>=sprite->x) return 0;
          fullw=sprite->x+sprite->w-nx;
          fullr=fullx+fullw;
        } break;
      case DIR_E: {
          nx=other->x-sprite->w;
          if (nx<=sprite->x) return 0;
          fullw=nx+sprite->x-sprite->x;
          fullr=fullx+fullw;
        } break;
      case DIR_N: {
          // If we are carrful, try pushing the other guy first. And then react as usual.
          if (sprite->type->carrful) sprite_move(other,0.0,dy);
          ny=other->y+other->h;
          if (ny>=sprite->y) return 0;
          fullh=sprite->y+sprite->h-ny;
          fullb=fully+fullh;
        } break;
      case DIR_S: {
          ny=other->y-sprite->h;
          if (ny<=sprite->y) return 0;
          fullh=ny+sprite->h-sprite->y;
          fullb=fully+fullh;
        } break;
    }
  }
  
  // OK let's keep it.
  dx=nx-sprite->x; // Freshen (dx,dy) in case we're partial and carrful.
  dy=ny-sprite->y;
  double oy=sprite->y; // Need for detecting pumpkins.
  sprite->x=nx;
  sprite->y=ny;
  
  // If we are carrful and moving anything but up, look for sprites aligned with our top edge and apply the same move to them.
  // The up case is handled differently, under sprite collisions, because we do want to collide normally sometimes when up.
  if (sprite->type->carrful&&(dir!=DIR_N)) {
    for (otherp=g.grpv[NS_sprgrp_physics].sprv,otheri=g.grpv[NS_sprgrp_physics].sprc;otheri-->0;otherp++) {
      struct sprite *other=*otherp;
      double e=other->y+other->h-oy;
      if ((e<-SMALL)||(e>SMALL)) continue;
      if (other->x+other->w<=fullx) continue;
      if (other->x>=fullx+fullw) continue;
      sprite_move(other,dx,dy);
    }
  }

  return 1;
}
