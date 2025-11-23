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
      switch (g.physics[*p]) {
        case NS_physics_solid: return 1;
        case NS_physics_goal: return 1;
      }
    }
  }
  return 0;
}

/* Check grid for hazards etc.
 */
 
int sprite_touches_grid_physics(const struct sprite *sprite,uint8_t physics) {
  if (!sprite) return 0;
  int cola=(int)(sprite->x);
  int colz=(int)(sprite->x+sprite->w-SMALL);
  if (cola<0) cola=0;
  if (colz>=NS_sys_mapw) colz=NS_sys_mapw-1;
  if (cola>colz) return 0;
  int rowa=(int)(sprite->y);
  int rowz=(int)(sprite->y+sprite->h-SMALL);
  if (rowa<0) rowa=0;
  if (rowz>=NS_sys_maph) rowz=NS_sys_maph-1;
  if (rowa>rowz) return 0;
  const uint8_t *rowp=g.cellv+rowa*NS_sys_mapw+cola;
  for (;rowa<=rowz;rowa++,rowp+=NS_sys_mapw) {
    const uint8_t *cellp=rowp;
    int col=cola;
    for (;col<=colz;col++,cellp++) {
      if (g.physics[*cellp]==physics) return 1;
    }
  }
  return 0;
}

/* Check for generically-deliverable damage between two colliding sprites.
 * This is symmetric.
 */
 
static void sprite_check_damage(struct sprite *a,struct sprite *b) {
  if (a->type->injure) {
    if (sprite_group_has(g.grpv+NS_sprgrp_hazard,b)&&sprite_group_has(g.grpv+NS_sprgrp_fragile,a)) {
      a->type->injure(a,b);
    }
  }
  if (b->type->injure) {
    if (sprite_group_has(g.grpv+NS_sprgrp_hazard,a)&&sprite_group_has(g.grpv+NS_sprgrp_fragile,b)) {
      b->type->injure(b,a);
    }
  }
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
            fullx=nx;
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
      if (dir==DIR_N) { row=rowz; rowoob=rowa-1; drow=-1; }
      else { row=rowa; rowoob=rowz+1; drow=1; }
      for (;row!=rowoob;row+=drow) {
        if (sprite_collides_grid(sprite,cola,row,colc,1)) {
          // Collision against grid vertically.
          if (dir==DIR_N) {
            ny=row+1.0;
            if (ny>=sprite->y) return 0;
            fully=ny;
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
    
    // A collision exists. Check for generic damage.
    if (sprite->type->injure||other->type->injure) {
      sprite_check_damage(sprite,other);
    }
    
    // Collision against solid sprite.
    switch (dir) {
      case DIR_W: {
          nx=other->x+other->w;
          if (nx>=sprite->x) return 0;
          fullx=nx;
          fullw=sprite->x+sprite->w-nx;
          fullr=fullx+fullw;
        } break;
      case DIR_E: {
          nx=other->x-sprite->w;
          if (nx<=sprite->x) return 0;
          fullw=nx+sprite->w-sprite->x;
          fullr=fullx+fullw;
        } break;
      case DIR_N: {
          // If we are carrful, try pushing the other guy first. And then react as usual.
          if (sprite->type->carrful) sprite_move(other,0.0,dy);
          ny=other->y+other->h;
          if (ny>=sprite->y) return 0;
          fully=ny;
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
      // There's a little snafu here. Ride a platform and you and the platform change to the next pixel at different times, so you jitter relative to each other.
      // We might be able to mitigate it by fudging the rider's position at the moment they make contact.
      // But we don't have a strong sense of when that contact first occurs.
      // I tried mitigating it ongoingly, right here, and that was a disaster.
    }
  }

  return 1;
}

/* Line to line collision.
 * Edges don't count.
 */
 
static int lines_cross(
  double aax,double aay,double abx,double aby,
  double bax,double bay,double bbx,double bby
) {
  // We take two pairs of cross-products: Both B points from AA, and both A points from BA.
  // Both pairs must have opposite signs. Zero doesn't count as opposite, that's the edge case.
  double cpaa=((bax-aax)*(aby-aay))-((bay-aay)*(abx-aax));
  double cpab=((bbx-aax)*(aby-aay))-((bby-aay)*(abx-aax));
  double cpba=((aax-bax)*(bby-bay))-((aay-bay)*(bbx-bax));
  double cpbb=((abx-bax)*(bby-bay))-((aby-bay)*(bbx-bax));
  if (!(((cpaa<0.0)&&(cpab>0.0))||((cpaa>0.0)&&(cpab<0.0)))) return 0;
  if (!(((cpba<0.0)&&(cpbb>0.0))||((cpba>0.0)&&(cpbb<0.0)))) return 0;
  return 1;
}

/* Line to rectangle collision.
 */
 
int sprite_touches_line(const struct sprite *sprite,double ax,double ay,double bx,double by) {
  if (!sprite) return 0;
  double l=sprite->x,t=sprite->y;
  double r=l+sprite->w,b=t+sprite->h;
  
  // First, if either endpoint is in bounds, it's a collision.
  if ((ax>l)&&(ax<r)&&(ay>t)&&(ay<b)) return 1;
  if ((bx>l)&&(bx<r)&&(by>t)&&(by<b)) return 1;
  
  // Next, if the line crosses either of the sprite's diagonals, it's a collision.
  if (lines_cross(ax,ay,bx,by,l,t,r,b)) return 1;
  if (lines_cross(ax,ay,bx,by,r,t,l,b)) return 1;
  
  return 0;
}

// Same as the above, but a loose rectangle instead of a sprite.
int rect_touches_line(
  double x,double y,double w,double h,
  double ax,double ay,double bx,double by
) {
  double r=x+w,b=y+h;
  if ((ax>x)&&(ax<r)&&(ay>y)&&(ay<b)) return 1;
  if ((bx>x)&&(bx<r)&&(by>y)&&(by<b)) return 1;
  if (lines_cross(ax,ay,bx,by,x,y,r,b)) return 1;
  if (lines_cross(ax,ay,bx,by,r,y,x,b)) return 1;
  return 0;
}

/* Test line of sight.
 */
 
int sprites_have_line_of_sight(const struct sprite *a,const struct sprite *b) {
  if (!a||!b) return 0;
  double ax=a->x+a->w*0.5;
  double ay=a->y+a->h*0.5;
  double bx=b->x+b->w*0.5;
  double by=b->y+b->h*0.5;
  
  // Check for collisions against other solid sprites.
  struct sprite **otherp=g.grpv[NS_sprgrp_physics].sprv;
  int otheri=g.grpv[NS_sprgrp_physics].sprc;
  for (;otheri-->0;otherp++) {
    struct sprite *other=*otherp;
    if ((other==a)||(other==b)) continue;
    if (sprite_touches_line(other,ax,ay,bx,by)) return 0;
  }
  
  /* Next check each grid cell that the line cross. Tricky!
   * No doubt there is a graceful solution that quantizes the line iteratively.
   * But I'm dumb and in a hurry, so we'll do it the dumb way.
   * Determine the grid bounds that cover the line, then test each cell in those bounds.
   */
  int cola=(int)ax,colz=(int)bx;
  if (cola>colz) { int tmp=cola; cola=colz; colz=tmp; }
  if (cola<0) cola=0;
  if (colz>=NS_sys_mapw) colz=NS_sys_mapw-1;
  if (cola<=colz) {
    int rowa=(int)ay,rowz=(int)by;
    if (rowa>rowz) { int tmp=rowa; rowa=rowz; rowz=tmp; }
    if (rowa<0) rowa=0;
    if (rowz>=NS_sys_maph) rowz=NS_sys_maph-1;
    if (rowa<=rowz) {
      const uint8_t *gridrow=g.cellv+rowa*NS_sys_mapw+cola;
      int row=rowa;
      for (;row<=rowz;row++,gridrow+=NS_sys_mapw) {
        const uint8_t *gridp=gridrow;
        int col=cola;
        for (;col<=colz;col++,gridp++) {
          switch (g.physics[*gridp]) {
            case NS_physics_solid:
            case NS_physics_goal:
              break;
            default: continue;
          }
          // It's solid. Does the line cross it?
          if (rect_touches_line(col,row,1.0,1.0,ax,ay,bx,by)) return 0;
        }
      }
    }
  }
  
  return 1;
}

/* Extend line of sight. Complex rules.
 */
 
void extend_line_of_sight(double *dstx,double *dsty,double ax,double ay,double t) {
  /* The basic idea:
   *  - Extend first to a constant limit, NS_sys_mapw.
   *  - Clip against sprites, examining up to two faces of each, the ones with exposure to (ax,ay).
   *  - The same hacky grid logic as testing line of sight, but clipping as we do against sprites.
   * So we're trimming the line down iteratively, it can get smaller as we run.
   */
  double bx=ax-sin(t)*NS_sys_mapw;
  double by=ay+cos(t)*NS_sys_mapw;
  double d2=(bx-ax)*(bx-ax)+(by-ay)*(by-ay);
  
  #define CANDIDATE(cx,cy) { \
    double _cx=(cx),_cy=(cy); \
    double cd2=((_cx-ax)*(_cx-ax))+((_cy-ay)*(_cy-ay)); \
    if (cd2<d2) { \
      bx=_cx; \
      by=_cy; \
      d2=cd2; \
    } \
  }
  
  // Check sprites.
  struct sprite **spritep=g.grpv[NS_sprgrp_physics].sprv;
  int spritei=g.grpv[NS_sprgrp_physics].sprc;
  for (;spritei-->0;spritep++) {
    struct sprite *sprite=*spritep;
    
    // Find the line's collision point against my exposed faces.
    // Do call edges in bounds, otherwise the laser might be able to sneak thru a corner, in a wildly-improbable alignment.
    // There might not be an exposed face, if (ax,ay) is inside the sprite.
    // But that situation doesn't make sense so just ignore the sprite then.
    if (sprite->x>ax) { // Left face exposure.
      double qy=ay+((sprite->x-ax)*(by-ay))/(bx-ax);
      if ((qy>=sprite->y)&&(qy<=sprite->y+sprite->h)) {
        if (sprite_group_has(g.grpv+NS_sprgrp_detectable,sprite)) continue; // oops no, actually we don't want it
        CANDIDATE(sprite->x,qy)
        continue;
      }
    } else if (sprite->x+sprite->w<ax) { // Right face exposure.
      double qx=sprite->x+sprite->w;
      double qy=ay+((qx-ax)*(by-ay))/(bx-ax);
      if ((qy>=sprite->y)&&(qy<=sprite->y+sprite->h)) {
        if (sprite_group_has(g.grpv+NS_sprgrp_detectable,sprite)) continue;
        CANDIDATE(qx,qy)
        continue;
      }
    }
    if (sprite->y>ay) { // Top face exposure.
      double qx=ax+((sprite->y-ay)*(bx-ax))/(by-ay);
      if ((qx>=sprite->x)&&(qx<=sprite->x+sprite->w)) {
        if (sprite_group_has(g.grpv+NS_sprgrp_detectable,sprite)) continue;
        CANDIDATE(qx,sprite->y)
        continue;
      }
    } else if (sprite->y+sprite->h<ay) { // Bottom face exposure. Shouldn't come up in real life, since this is only used for the down-pointing motionsensor.
      double qy=sprite->y+sprite->h;
      double qx=ax+((qy-ay)*(bx-ax))/(by-ay);
      if ((qx>=sprite->x)&&(qx<=sprite->x+sprite->w)) {
        if (sprite_group_has(g.grpv+NS_sprgrp_detectable,sprite)) continue;
        CANDIDATE(qx,qy)
        continue;
      }
    }
  }
  
  // Map. Same cheap hack as testing line of sight, but clip like above and keep going.
  int cola=(int)ax,colz=(int)bx;
  if (cola>colz) { int tmp=cola; cola=colz; colz=tmp; }
  if (cola<0) cola=0;
  if (colz>=NS_sys_mapw) colz=NS_sys_mapw-1;
  if (cola<=colz) {
    int rowa=(int)ay,rowz=(int)by;
    if (rowa>rowz) { int tmp=rowa; rowa=rowz; rowz=tmp; }
    if (rowa<0) rowa=0;
    if (rowz>=NS_sys_maph) rowz=NS_sys_maph-1;
    if (rowa<=rowz) {
      const uint8_t *gridrow=g.cellv+rowa*NS_sys_mapw+cola;
      int row=rowa;
      for (;row<=rowz;row++,gridrow+=NS_sys_mapw) {
        const uint8_t *gridp=gridrow;
        int col=cola;
        for (;col<=colz;col++,gridp++) {
          switch (g.physics[*gridp]) {
            case NS_physics_solid:
            case NS_physics_goal:
              break;
            default: continue;
          }
          double l=col,t=row,r=col+1.0,b=row+1.0;
          if (l>ax) { // Left face exposure.
            double qy=ay+((l-ax)*(by-ay))/(bx-ax);
            if ((qy>=t)&&(qy<=b)) {
              CANDIDATE(l,qy)
              continue;
            }
          } else if (r<ax) { // Right face exposure.
            double qy=ay+((r-ax)*(by-ay))/(bx-ax);
            if ((qy>=t)&&(qy<=b)) {
              CANDIDATE(r,qy)
              continue;
            }
          }
          if (t>ay) { // Top face exposure.
            double qx=ax+((t-ay)*(bx-ax))/(by-ay);
            if ((qx>=l)&&(qx<=r)) {
              CANDIDATE(qx,t)
              continue;
            }
          } else if (b<ay) { // Bottom face exposure. Shouldn't come up in real life, since this is only used for the down-pointing motionsensor.
            double qx=ax+((b-ay)*(bx-ax))/(by-ay);
            if ((qx>=l)&&(qx<=r)) {
              CANDIDATE(qx,r)
              continue;
            }
          }
        }
      }
    }
  }
  
  #undef CANDIDATE
  *dstx=bx;
  *dsty=by;
}
