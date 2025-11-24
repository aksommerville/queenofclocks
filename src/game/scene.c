#include "queenofclocks.h"

#define FADE_OUT_TIME_WIN 0.500
#define FADE_OUT_TIME_LOSE 1.000
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

/* Add high score to bgbits.
 */
 
static void qc_bgbits_add_hiscore(int hiscore) {
  if (hiscore<0) hiscore=0; else if (hiscore>999999) hiscore=999999;
  
  char tmp[6];
  int tmpc=0;
  if (hiscore>=100000) tmp[tmpc++]='0'+(hiscore/100000)%10;
  if (hiscore>= 10000) tmp[tmpc++]='0'+(hiscore/ 10000)%10;
  if (hiscore>=  1000) tmp[tmpc++]='0'+(hiscore/  1000)%10;
  if (hiscore>=   100) tmp[tmpc++]='0'+(hiscore/   100)%10;
  if (hiscore>=    10) tmp[tmpc++]='0'+(hiscore/    10)%10;
                       tmp[tmpc++]='0'+(hiscore       )%10;
  
  const char *lbl=0;
  int lblc=qc_res_get_string(&lbl,1,7);
  
  const char *sep=": ";
  int sepc=2;
  
  int chc=lblc+sepc+tmpc;
  int w=8*chc;
  int x=(FBW>>1)-(w>>1)+(NS_sys_tilesize>>1);
  int y=7;
  int i;
  const char *src;
  graf_reset(&g.graf);
  graf_set_output(&g.graf,g.texid_bgbits);
  graf_set_input(&g.graf,g.texid_font);
  graf_set_tint(&g.graf,0x808080ff);
  for (i=lblc,src=lbl;i-->0;src++,x+=8) if (*src>0x20) graf_tile(&g.graf,x,y,*src,0);
  for (i=sepc,src=sep;i-->0;src++,x+=8) if (*src>0x20) graf_tile(&g.graf,x,y,*src,0);
  for (i=tmpc,src=tmp;i-->0;src++,x+=8) if (*src>0x20) graf_tile(&g.graf,x,y,*src,0);
  graf_flush(&g.graf);
  graf_set_output(&g.graf,1);
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
     * If this is the first map, fail. Otherwise start game-over.
     */
    if (mapid==RID_map_start) return -1;
    /* Loop after the last level. Need this if you're going backward. *
    if (NEXT_MAP<0) return qc_scene_load(qc_res_last_id(EGG_TID_map));
    return qc_scene_load(RID_map_start);
    /**/
    return gameover_begin();
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
  g.fake_map=0;
  qc_bgbits_render();
  
  /* Run commands.
   * Spawns sprites. Are there going to be other kinds of POI?
   */
  struct cmdlist_reader reader={.v=map.cmd,.c=map.cmdc};
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
    
      case CMD_map_fake: {
          g.playtime=0.0;
          g.deathc=0;
          g.fake_map=1;
          if (g.hiscore) qc_bgbits_add_hiscore(g.hiscore);
        } break;
    
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
      return qc_scene_load(g.mapid+NEXT_MAP);
    } else {
      return qc_scene_load(g.mapid);
    }
  }
  
  // If the hero is missing, you lose. Start ticking the clock.
  if (!hero) {
    if (!g.fake_map) g.deathc++;
    g.termclock=g.termtime=FADE_OUT_TIME_LOSE;
    return 0;
  }
  
  // Player can tell us whether she's standing still on the goal. If so, start the clock.
  if (sprite_hero_in_victory_position(hero)) {
    g.termclock=g.termtime=FADE_OUT_TIME_WIN;
  }
  
  return 0;
}

/* Update.
 */
 
void qc_scene_update(double elapsed) {

  if (g.fadeclock>0.0) g.fadeclock-=elapsed;
  if (!g.fake_map) g.playtime+=elapsed;
  
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
  if (g.termclock>0.0) fadealpha=0xff-(int)((g.termclock*255.0)/g.termtime);
  else if (g.fadeclock>0.0) fadealpha=(int)((g.fadeclock*255.0)/FADE_IN_TIME);
  if (fadealpha>0) {
    if (fadealpha>0xff) fadealpha=0xff;
    graf_fill_rect(&g.graf,0,0,FBW,FBH,0x00000000|fadealpha);
  }

  // On the hello map, show hiscore if nonzero. Others, show play time and death count.
  if (g.fake_map) {
    // Actually, we write the high score directly onto (g.bgbits). It can't change during play.
  } else {
    graf_set_input(&g.graf,g.texid_font);
    
    int y=4;
    graf_tile(&g.graf,4,y,0x80,0); // skull
    int x=15;
    int v=g.deathc; if (v>999) v=999; else if (v<0) v=0;
    if (v>=100) { graf_tile(&g.graf,x,y,'0'+v/100,0); x+=8; }
    if (v>=10) { graf_tile(&g.graf,x,y,'0'+(v/10)%10,0); x+=8; }
    graf_tile(&g.graf,x,y,'0'+v%10,0);
    
    graf_tile(&g.graf,FBW-4,y,0x81,0); // hourglass
    int ms=(int)(g.playtime*1000.0);
    if (ms<0) ms=0;
    int sec=ms/1000; ms%=1000;
    int min=sec/60; sec%=60;
    if (min>99) min=sec=99;
    x=FBW-15;
    graf_tile(&g.graf,x,y,'0'+sec%10,0); x-=8;
    graf_tile(&g.graf,x,y,'0'+sec/10,0); x-=8;
    if (ms<800) graf_tile(&g.graf,x,y,':',0); x-=8;
    graf_tile(&g.graf,x,y,'0'+min%10,0); x-=8;
    if (min>=10) graf_tile(&g.graf,x,y,'0'+min/10,0);
  }
}
