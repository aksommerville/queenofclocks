#ifndef EGG_GAME_MAIN_H
#define EGG_GAME_MAIN_H

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/res/res.h"
#include "egg_res_toc.h"
#include "shared_symbols.h"
#include "sprite/sprite.h"

#define FBW 320
#define FBH 176

// Track so many recent sounds. Don't play it again within some brief interval.
#define SNDPLAY_LIMIT 8

// Normally 1, but use (-1) to cycle backward, while designing maps.
#define NEXT_MAP 1

struct frect { double l,r,t,b; };

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  struct rom_entry *resv;
  int resc,resa;
  int input,pvinput;
  
  // We'll use just two tilesheets. Load them in advance, globally.
  int texid_terrain;
  int texid_sprites;
  uint8_t physics[256];
  struct sndplay { double time; int rid; } sndplayv[SNDPLAY_LIMIT];
  
  int texid_bgbits;
  int mapid;
  const uint8_t *cellv; // NS_sys_mapw*NS_sys_maph, never null except during init
  struct sprite_group grpv[32];
  struct sprite_group grp_updscratch;
  double termclock; // If nonzero, counts down to termination.
  double fadeclock; // '' fading in.
  double termtime; // Range for termclock (fadeclock's is constant).
  int framec; // Global.
  
  struct sprite_group ctlpan_pumpkin;
} g;

void qc_sound(int rid,double x); // (x<0) if non-spatial, otherwise in 0..NS_sys_mapw and we pan accordingly

// res.c
int qc_res_init();
int qc_res_get(void *dstpp,int tid,int rid); // Not all resources are recorded.
int qc_res_last_id(int tid);

// scene.c
int qc_scene_load(int mapid);
void qc_scene_update(double elapsed);
void qc_scene_render();

// ctlpan.c
int ctlpan_begin(struct sprite *pumpkin);
void ctlpan_dismiss();
struct sprite *ctlpan_is_active(); // Fine to update, render, whatever, when we're dismissed. But call this if you actually need to know. Returns the pumpkin if active.
void ctlpan_update(double elapsed);
void ctlpan_render();

#endif
