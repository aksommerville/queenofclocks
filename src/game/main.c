#include "queenofclocks.h"

struct g g={0};

void egg_client_quit(int status) {
}

/* Init.
 ******************************************************************/

int egg_client_init() {

  int fbw=0,fbh=0;
  egg_texture_get_size(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"Framebuffer size mismatch! metadata=%dx%d header=%dx%d\n",fbw,fbh,FBW,FBH);
    return -1;
  }

  g.romc=egg_rom_get(0,0);
  if (!(g.rom=malloc(g.romc))) return -1;
  egg_rom_get(g.rom,g.romc);
  if (qc_res_init()<0) return -1;
  
  if (egg_texture_load_image(g.texid_terrain=egg_texture_new(),RID_image_terrain)<0) return -1;
  if (egg_texture_load_image(g.texid_sprites=egg_texture_new(),RID_image_sprites)<0) return -1;
  if (egg_texture_load_raw(g.texid_bgbits=egg_texture_new(),FBW,FBH,FBW<<2,0,0)<0) return -1;

  srand_auto();
  
  g.grpv[NS_sprgrp_render].order=SPRITE_GROUP_ORDER_RENDER;
  
  // We only have one song and it plays forever.
  egg_play_song(1,1,1,1.0f,0.0f);
  
  if (qc_scene_load(RID_map_start)<0) return -1;

  return 0;
}

/* Update and render basically just defer to the scene.
 * I'm trying not to implement a modal stack for this one.
 *********************************************************************/

void egg_client_update(double elapsed) {

  g.pvinput=g.input;
  g.input=egg_input_get_one(0);
  if ((g.input&EGG_BTN_AUX3)&&!(g.pvinput&EGG_BTN_AUX3)) {
    egg_terminate(0);
    return;
  }
  
  //XXX TEMP: AUX1 to restart, for when I get trapped.
  if ((g.input&EGG_BTN_AUX1)&&!(g.pvinput&EGG_BTN_AUX1)) {
    qc_scene_load(g.mapid);
  }

  qc_scene_update(elapsed);
  
  ctlpan_update(elapsed);
}


void egg_client_render() {
  g.framec++;
  graf_reset(&g.graf);
  qc_scene_render();
  ctlpan_render();
  graf_flush(&g.graf);
}

/* Audio events.
 */

void qc_sound(int rid,double x) {
  
  double now=egg_time_real();
  struct sndplay *rewrite=g.sndplayv;
  struct sndplay *sndplay=g.sndplayv;
  int i=SNDPLAY_LIMIT;
  for (;i-->0;sndplay++) {
    if (sndplay->time<rewrite->time) rewrite=sndplay; // Find the oldest.
    if (sndplay->rid==rid) {
      double since=now-sndplay->time;
      if (since<=0.050) return; // Drop sounds if within 50 ms. Otherwise we're repeating PCM at greater than 20 Hz and could get some weird artifacts.
    }
  }
  rewrite->time=now;
  rewrite->rid=rid;
  
  double pan=0.0;
  if (x>=0.0) {
    pan=(x*2.0)/NS_sys_mapw-1.0;
  }
  egg_play_sound(rid,1.0,pan);
}
