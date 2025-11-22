#include "queenofclocks.h"

#define FADE_OUT_TIME 0.500
#define FADE_IN_TIME  0.250

/* Render bgbits.
 */
 
static void qc_bgbits_render() {
  struct egg_render_tile vtxv[NS_sys_mapw*NS_sys_maph];
  struct egg_render_tile *vtx=vtxv;
  const uint8_t *src=g.cellv;
  int yi=NS_sys_maph,y=NS_sys_tilesize>>1;
  for (;yi-->0;y+=NS_sys_tilesize) {
    int xi=NS_sys_mapw,x=NS_sys_tilesize>>1;
    for (;xi-->0;x+=NS_sys_tilesize,vtx++,src++) {
      vtx->x=x;
      vtx->y=y;
      vtx->tileid=*src;
      vtx->xform=0;
    }
  }
  struct egg_render_uniform un={
    .mode=EGG_RENDER_TILE,
    .dsttexid=g.texid_bgbits,
    .srctexid=g.texid_terrain,
    .alpha=0xff,
  };
  egg_render(&un,vtxv,sizeof(vtxv));
}

/* Tear down current scene and replace.
 */
 
int qc_scene_load(int mapid) {

  /* Acquire the resource.
   */
  const void *serial=0;
  int serialc=qc_res_get(&serial,EGG_TID_map,mapid);
  struct map_res map={0};
  if (map_res_decode(&map,serial,serialc)<0) {
    /* Can only mean Not Found; every map is validated at load.
     * If this is the first map, fail. Otherwise try loading the first one.
     * We're not doing modals; the first map is our Hello and Gameover screen too.
     */
    if (mapid==RID_map_start) return -1;
    return qc_scene_load(RID_map_start);
  }
  
  /* Drop any volatile state.
   */
  sprite_group_kill(g.grpv+NS_sprgrp_keepalive);
  
  /* Prepare new state.
   */
  g.mapid=mapid;
  g.cellv=map.v;
  g.termclock=0.0;
  g.fadeclock=FADE_IN_TIME;
  qc_bgbits_render();
  
  /* Run commands.
   * Spawns sprites. Are there going to be other kinds of POI?
   */
  struct cmdlist_reader reader={.v=map.cmd,.c=map.cmdc};
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
    
      case CMD_map_sprite: {
          double x=cmd.arg[0]+0.5;
          double y=cmd.arg[1]+0.5;
          int rid=(cmd.arg[2]<<8)|cmd.arg[3];
          uint32_t arg=(cmd.arg[4]<<24)|(cmd.arg[5]<<16)|(cmd.arg[6]<<8)|cmd.arg[7];
          struct sprite *sprite=sprite_spawn(x,y,0,rid,arg,0,0);
        } break;
        
    }
  }
  
  return 0;
}

/* Completion.
 * This manages the whole affair, not just checking.
 */
 
static int qc_scene_check_completion(double elapsed) {

  // Acquire the hero sprite, may be null.
  struct sprite *hero=0;
  if (g.grpv[NS_sprgrp_hero].sprc>=1) hero=g.grpv[NS_sprgrp_hero].sprv[0];
  
  // If we already have a termination clock, tick it and react when it expires.
  if (g.termclock>0.0) {
    if ((g.termclock-=elapsed)>0.0) return 0; // please hold
    if (hero) {
      fprintf(stderr,"win map:%d\n",g.mapid);
      return qc_scene_load(g.mapid+1);
    } else {
      fprintf(stderr,"lose map:%d\n",g.mapid);
      return qc_scene_load(g.mapid);
    }
  }
  
  // If the hero is missing, you lose. Start ticking the clock.
  if (!hero) {
    fprintf(stderr,"hero missing!\n");
    g.termclock=FADE_OUT_TIME;
    return 0;
  }
  
  // Player can tell us whether she's standing still on the goal. If so, start the clock.
  if (sprite_hero_in_victory_position(hero)) {
    fprintf(stderr,"victory! please hold\n");
    g.termclock=FADE_OUT_TIME;
  }
  
  return 0;
}

/* Update.
 */
 
void qc_scene_update(double elapsed) {

  if (g.fadeclock>0.0) g.fadeclock-=elapsed;
  
  // Check completion. Do this at the start of the cycle, so a newly-loaded scene gets its first update before its first render.
  if (qc_scene_check_completion(elapsed)<0) {
    egg_terminate(1);
    return;
  }

  /* Update the sprites.
   * We'll make a copy of the update group every frame, and operate off that.
   * Sprites added or removed to the update group during this cycle will not take effect until the next cycle.
   */
  if (sprite_group_copy(&g.grp_updscratch,g.grpv+NS_sprgrp_update)<0) return;
  struct sprite **p=g.grp_updscratch.sprv;
  int i=g.grp_updscratch.sprc;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    if (!sprite->type->update) continue; // what is it doing in the update group?
    sprite->type->update(sprite,elapsed);
  }
  sprite_group_clear(&g.grp_updscratch);
  
  // Execute all of deathrow.
  sprite_group_kill(g.grpv+NS_sprgrp_deathrow);
}

/* Render scene.
 * Complete overwrites framebuffer.
 */

void qc_scene_render() {

  // Map. It's static so we do the real render just once when resetting the scene.
  graf_set_input(&g.graf,g.texid_bgbits);
  graf_decal(&g.graf,0,0,0,0,FBW,FBH);
  
  // Sprites.
  graf_set_input(&g.graf,g.texid_sprites);
  struct sprite_group *group=g.grpv+NS_sprgrp_render;
  sprite_group_rendersort_partial(group);
  struct sprite **p=group->sprv;
  int i=group->sprc;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->type->render) {
      int dstx=(int)(sprite->x*NS_sys_tilesize);
      int dsty=(int)(sprite->y*NS_sys_tilesize);
      sprite->type->render(sprite,dstx,dsty);
    } else {
      int dstx=(int)((sprite->x+sprite->w*0.5)*NS_sys_tilesize);
      int dsty=(int)((sprite->y+sprite->h*0.5)*NS_sys_tilesize);
      graf_tile(&g.graf,dstx,dsty,sprite->tileid,sprite->xform);
    }
  }
  
  // Fading in or out?
  int fadealpha=0;
  if (g.termclock>0.0) fadealpha=0xff-(int)((g.termclock*255.0)/FADE_OUT_TIME);
  else if (g.fadeclock>0.0) fadealpha=(int)((g.fadeclock*255.0)/FADE_IN_TIME);
  if (fadealpha>0) {
    if (fadealpha>0xff) fadealpha=0xff;
    graf_fill_rect(&g.graf,0,0,FBW,FBH,0x00000000|fadealpha);
  }

  //TODO overlay?
}
