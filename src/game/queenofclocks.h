#ifndef EGG_GAME_MAIN_H
#define EGG_GAME_MAIN_H

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/res/res.h"
#include "egg_res_toc.h"
#include "shared_symbols.h"

#define FBW 320
#define FBH 176

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  struct rom_entry *resv;
  int resc,resa;
  
  // We'll use just two tilesheets. Load them in advance, globally.
  int texid_terrain;
  int texid_sprites;
  uint8_t physics[256];
  
  int texid_bgbits;
  int mapid;
  const uint8_t *cellv; // NS_sys_mapw*NS_sys_maph, never null except during init
} g;

// res.c
int qc_res_init();
int qc_res_get(void *dstpp,int tid,int rid); // Not all resources are recorded.

// scene.c
int qc_scene_load(int mapid);
void qc_scene_render();

#endif
