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
  //TODO sprites
  
  /* Prepare new state.
   */
  g.mapid=mapid;
  g.cellv=map.v;
  qc_bgbits_render();
  
  //TODO load sprites
  //TODO other scene-start concerns?
  
  return 0;
}

/* Render scene.
 * Complete overwrites framebuffer.
 */

void qc_scene_render() {
  graf_set_input(&g.graf,g.texid_bgbits);
  graf_decal(&g.graf,0,0,0,0,FBW,FBH);
  //TODO sprites
  //TODO overlay?
}
