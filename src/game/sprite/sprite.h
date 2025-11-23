/* sprite.h
 */
 
#ifndef SPRITE_H
#define SPRITE_H

struct sprite;
struct sprite_type;
struct sprite_group;

struct sprite {
  const struct sprite_type *type;
  int refc;
  struct sprite_group **grpv;
  int grpc,grpa;
  double x,y,w,h; // m. (x,y) are the top-left corner, and bounds are not always square.
  int layer; // Render order, 0..0xffff. Default 256.
  uint8_t tileid,xform;
  uint32_t arg; // From spawn point.
  int rid;
  const void *cmd; // From resource.
  int cmdc;
  int timescale; // 0,1,2,3,4 = stop,slow,normal,fast,infinite. ctlpan may change it behind your back.
};

struct sprite_type {
  const char *name;
  int objlen;
  
  int carrful; // Hint to physics that a physical sprite resting on my head should move with me if possible.
  
  void (*del)(struct sprite *sprite);
  int (*init)(struct sprite *sprite);
  void (*update)(struct sprite *sprite,double elapsed);
  
  /* If you don't implement, the wrapper will draw (tileid,xform) at the middle of your physical bounds.
   * (dstx,dsty) are your (x,y) quantized to framebuffer pixels.
   * Sprites may assume that (g.texid_sprites) is active in (g.graf). If you set it otherwise, put it back before returning.
   */
  void (*render)(struct sprite *sprite,int dstx,int dsty);
  
  /* Notify of beginning or end of wand inspection.
   * (grab) nonzero if we're starting the grab, zero if releasing.
   * (grabber) is the hero.
   * (dir) is a cardinal direction bit, which direction is the grabber facing.
   * Everything in motion group must implement this.
   */
  void (*grab)(struct sprite *sprite,int grab,struct sprite *grabber,uint8_t dir);
  
  /* Receive damage from (assailant).
   * If these sprites are in the physics, fragile, and hazard groups, this can be delivered generically during collision detection.
   * DO NOT DELETE either sprite! (but then that is standing policy for all hooks too).
   */
  void (*injure)(struct sprite *sprite,struct sprite *assailant);
};

struct sprite_group {
  int refc; // 0 if immortal, eg the global groups
  struct sprite **sprv;
  int sprc,spra;
  int order;
};

#define SPRITE_GROUP_ORDER_ADDR     0 /* Default: Sort sprites by address. Efficient but unpredictable order. */
#define SPRITE_GROUP_ORDER_EXPLICIT 1 /* Retain the order added. Searches are expensive. */
#define SPRITE_GROUP_ORDER_RENDER   2 /* For the render group only. */

/* Generic sprite.
 ****************************************************************************/

/* Sprite allocation and deletion.
 * These should not be used by the program at large.
 * Prefer "spawn" and "deathrow".
 */
void sprite_del(struct sprite *sprite);
struct sprite *sprite_new(double x,double y,const struct sprite_type *type,int rid,uint32_t arg,const void *cmd,int cmdc);
int sprite_ref(struct sprite *sprite);

/* Create, initialize, and register a new sprite with midpoint at (x,y) in meters.
 * (type) may be null if you provide (cmd), and (cmd) maybe empty if you provide (rid).
 * Returns WEAK.
 */
struct sprite *sprite_spawn(
  double x,double y,
  const struct sprite_type *type,
  int rid,uint32_t arg,
  const void *cmd,int cmdc
);

/* Sprite types.
 ***************************************************************************/
 
#define _(tag) extern const struct sprite_type sprite_type_##tag;
FOR_EACH_SPRTYPE
#undef _

const struct sprite_type *sprite_type_from_id(int id);
const struct sprite_type *sprite_type_from_commands(const void *v,int c);

int sprite_hero_in_victory_position(const struct sprite *sprite);

/* Examines all the password sprites and returns the displayed value, or <0 if invalid.
 * There's some magic caching in the background, it's fine to spam this.
 */
int sprite_password_get();

/* Sprite group.
 *************************************************************************/
 
void sprite_group_del(struct sprite_group *group); // No effect on immortals.
void sprite_group_cleanup(struct sprite_group *group); // Immortals only.
struct sprite_group *sprite_group_new();
int sprite_group_ref(struct sprite_group *group);

/* All sprite<->group links are mutual.
 * So we only phrase them with group first; "sprite_add_group" would be redundant.
 */
int sprite_group_has(const struct sprite_group *group,const struct sprite *sprite);
int sprite_group_add(struct sprite_group *group,struct sprite *sprite);
int sprite_group_remove(struct sprite_group *group,struct sprite *sprite);

/* "Kill" removes a sprite immediately from all groups, including keepalive, which usually deletes the sprite too.
 * "Clear" is the equivalent operation on a group.
 * Or you can kill a group, ie remove all groups from all members of this group. That's what deathrow does.
 */
void sprite_kill(struct sprite *sprite);
void sprite_group_clear(struct sprite_group *group);
void sprite_group_kill(struct sprite_group *group);

void sprite_group_rendersort_partial(struct sprite_group *group);

/* Drop all sprites in (dst), then add all from (src).
 */
int sprite_group_copy(struct sprite_group *dst,struct sprite_group *src);

/* Physics.
 * We'll use an on-demand physics model.
 * When a participating sprite wants to move, it should call sprite_move(), one axis at a time.
 * All interactions are applied immediately during that call.
 *********************************************************************/

/* Move a sprite by (dx,dy) meters, applying physics.
 * We assume that (sprite) is a member of physics group, and we don't check.
 * Physics for real happens one-dimensionally. If both (dx,dy) are nonzero, we apply (dx) first, then (dy).
 * You will never move backward with respect to the requested vector, on either axis.
 * Returns nonzero if moved, or zero if fully blocked.
 * Partial moves are not distinguishable from full ones.
 */
int sprite_move(struct sprite *sprite,double dx,double dy);

/* Nonzero if any cell in (x,y,w,h) is solid, by (sprite)'s reckoning.
 * We don't care whether (x,y,w,h) overlaps (sprite)'s bounds.
 */
int sprite_collides_grid(const struct sprite *sprite,int x,int y,int w,int h);

/* Nonzero if it overlaps any cell of the given physics type.
 * eg checking for hazards.
 */
int sprite_touches_grid_physics(const struct sprite *sprite,uint8_t physics);

/* Nonzero if there is a straight line between their centers which doesn't cross
 * any other solid sprite or solid grid cell.
 * Using midpoints instead of corner-to-corner favors hiding and is a bit more efficient.
 */
int sprites_have_line_of_sight(const struct sprite *a,const struct sprite *b);

/* Line segment starting at (ax,ay) with angle (t), zero is down, clockwise.
 * Fill (dstx,dsty) with the far endpoint, on the perimeter of some solid.
 * May exceed the screen's bounds to some sane but arbitrary distance, if there's no collision.
 * Sprites in physics group do count as solids but not those in detectable group.
 */
void extend_line_of_sight(double *dstx,double *dsty,double ax,double ay,double t);

/* Nonzero if (ax,ay .. bx,by) terminates in the sprite or passes through it.
 * Edges do not count.
 */
int sprite_touches_line(const struct sprite *sprite,double ax,double ay,double bx,double by);

/* If any sprite collides with (sprite), move it by the minimum escapement.
 * (sprite) itself will not move.
 * For passgate. But generally, anything that becomes solid suddenly should use this.
 */
void sprite_force_out_collisions(const struct sprite *sprite);

/* Nonzero if a collision exists against (sprite). We don't say with what, or how deep.
 */
int sprite_collides_anything(const struct sprite *sprite);

/* vector_from_dir() only returns -1,0,1. Diagonals are not normal; they're sqrt(2) longer than cardinals.
 */
#define DIR_NW  0x80
#define DIR_N   0x40
#define DIR_NE  0x20
#define DIR_W   0x10
#define DIR_MID 0x00
#define DIR_E   0x08
#define DIR_SW  0x04
#define DIR_S   0x02
#define DIR_SE  0x01
uint8_t dir_from_vector(double dx,double dy);
void vector_from_dir(double *dx,double *dy,uint8_t dir);

#endif
