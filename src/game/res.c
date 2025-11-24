#include "queenofclocks.h"

/* Receive a tilesheet.
 * We're only interested in "terrain", and only its "physics" table.
 */
 
static int qc_tilesheet_welcome(int rid,const void *v,int c) {
  if (rid!=RID_tilesheet_terrain) return 0;
  struct tilesheet_reader reader;
  if (tilesheet_reader_init(&reader,v,c)<0) return -1;
  struct tilesheet_entry entry;
  while (tilesheet_reader_next(&entry,&reader)>0) {
    if (entry.tableid!=NS_tilesheet_physics) continue;
    memcpy(g.physics+entry.tileid,entry.v,entry.c); // tilesheet_reader_next verifies bounds.
  }
  return 0;
}

/* Receive a map.
 * These are going to be recorded in TOC, and mostly we read them on demand.
 * But during load, fail if any map is not the expected size.
 */
 
static int qc_map_welcome(int rid,const void *v,int c) {
  struct map_res map={0};
  if (map_res_decode(&map,v,c)<0) return -1;
  if ((map.w!=NS_sys_mapw)||(map.h!=NS_sys_maph)) {
    fprintf(stderr,"map:%d has bounds (%d,%d), expected (%d,%d)\n",rid,map.w,map.h,NS_sys_mapw,NS_sys_maph);
    return -2;
  }
  return 1;
}

/* Receive a resource, during the initial read.
 * Return >0 to record it in the global TOC.
 * Beware that the TOC currently only contains resources before this one.
 */
 
static int qc_res_welcome(int tid,int rid,const void *v,int c) {
  switch (tid) {
    case EGG_TID_strings: return 1;
    case EGG_TID_tilesheet: return qc_tilesheet_welcome(rid,v,c);
    case EGG_TID_map: return qc_map_welcome(rid,v,c);
    case EGG_TID_sprite: return 1;
  }
  return 0;
}

/* Second pass over resources after TOC build-up.
 * When this begins, the TOC is already final.
 */
 
static int qc_res_link() {
  return 0;
}

/* Initialize resource store.
 * Main prepares (g.rom) and textures; we do the rest.
 */
 
int qc_res_init() {
  struct rom_reader reader={0};
  if (rom_reader_init(&reader,g.rom,g.romc)<0) return -1;
  struct rom_entry res;
  while (rom_reader_next(&res,&reader)>0) {
    int err=qc_res_welcome(res.tid,res.rid,res.v,res.c);
    if (err<0) {
      fprintf(stderr,"Error processing resource %d:%d, %d bytes.\n",res.tid,res.rid,res.c);
      return -2;
    }
    if (!err) continue;
    if (g.resc>=g.resa) {
      int na=g.resa+64;
      if (na>INT_MAX/sizeof(struct rom_entry)) return -1;
      void *nv=realloc(g.resv,sizeof(struct rom_entry)*na);
      if (!nv) return -1;
      g.resv=nv;
      g.resa=na;
    }
    g.resv[g.resc++]=res;
  }
  return qc_res_link();
}

/* Search registry.
 */
 
static int qc_res_search(int tid,int rid) {
  int lo=0,hi=g.resc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct rom_entry *q=g.resv+ck;
         if (tid<q->tid) hi=ck;
    else if (tid>q->tid) lo=ck+1;
    else if (rid<q->rid) hi=ck;
    else if (rid>q->rid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Get resource.
 */
 
int qc_res_get(void *dstpp,int tid,int rid) {
  int p=qc_res_search(tid,rid);
  if (p<0) return 0;
  const struct rom_entry *res=g.resv+p;
  *(const void**)dstpp=res->v;
  return res->c;
}

/* Last id for resource.
 */
 
int qc_res_last_id(int tid) {
  int p=qc_res_search(tid+1,0);
  if (p<0) p=-p-1;
  p--;
  if (p<0) return 0;
  const struct rom_entry *res=g.resv+p;
  if (res->tid!=tid) return 0;
  return res->rid;
}
