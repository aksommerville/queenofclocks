/* shared_symbols.h
 * This file is consumed by eggdev and editor, in addition to compiling in with the game.
 */

#ifndef SHARED_SYMBOLS_H
#define SHARED_SYMBOLS_H

#define NS_sys_tilesize 8
#define NS_sys_mapw 40
#define NS_sys_maph 22
#define EGGDEV_ignoreData "" /* Comma-delimited glob patterns for editor to ignore under src/data/ */

#define CMD_map_image     0x20 /* u16:imageid */
#define CMD_map_sprite    0x61 /* u16:position, u16:spriteid, u32:arg */

#define CMD_sprite_image    0x20 /* u16:imageid ; Always image:sprites for us. */
#define CMD_sprite_tile     0x21 /* u8:tileid, u8:xform */
#define CMD_sprite_type     0x22 /* u16:sprtype */
#define CMD_sprite_layer    0x23 /* u16:layer ; default 256 */
#define CMD_sprite_groups   0x40 /* b32:sprgrp */
#define CMD_sprite_size     0x41 /* u8.8:width, u8.8:height ; both in meters */

#define NS_tilesheet_physics 1
#define NS_tilesheet_family 0
#define NS_tilesheet_neighbors 0
#define NS_tilesheet_weight 0

#define NS_physics_vacant 0
#define NS_physics_solid 1
#define NS_physics_goal 2
#define NS_physics_hazard 3

#define NS_timescale_stop 0
#define NS_timescale_slow 1
#define NS_timescale_normal 2
#define NS_timescale_fast 3
#define NS_timescale_infinite 4

// Editor uses the comment after a 'sprtype' symbol as a prompt in the new-sprite modal.
// Should match everything after 'spriteid' in the CMD_map_sprite args.
#define NS_sprtype_dummy 0 /* (u32)0 */
#define NS_sprtype_hero 1 /* (u32)0 */
#define NS_sprtype_platform 3 /* (u8)dx (u8)dy (u8)w (u8:timescale)normal */
#define NS_sprtype_soulballs 4 /* (u32)0 */
#define NS_sprtype_flamethrower 5 /* (u8)dir (u8:timescale)normal (u8)period (u8)phase */
#define FOR_EACH_SPRTYPE \
  _(dummy) \
  _(hero) \
  _(platform) \
  _(soulballs) \
  _(flamethrower)
  
#define NS_sprgrp_keepalive 0 /* Programmatic access only. */
#define NS_sprgrp_deathrow  1 /* Programmatic access only. */
#define NS_sprgrp_render    2 /* All should join. */
#define NS_sprgrp_update    3 /* Anything that needs updates, typically everything. */
#define NS_sprgrp_hero      4 /* Should always contain just one hero sprite. Or empty if she's dead. */
#define NS_sprgrp_motion    5 /* Anything with a speed that Dot can control. */
#define NS_sprgrp_physics   6 /* Participates in physics. */
#define NS_sprgrp_hazard    7 /* Hurts Dot on contact. */
#define NS_sprgrp_fragile   8 /* Probably just Dot, but maybe there's damageable pumpkins too. */

#endif
