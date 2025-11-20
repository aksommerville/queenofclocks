#include "queenofclocks.h"

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

  //TODO overlay?
}
