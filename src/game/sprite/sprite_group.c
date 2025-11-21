#include "game/queenofclocks.h"

/* Object lifecycle.
 */
 
void sprite_group_del(struct sprite_group *group) {
  if (!group) return;
  if (!group->refc) return; // Immortal.
  if (group->refc-->1) return;
  if (group->sprc) fprintf(stderr,"WARNING: Deleting group %p with %d sprites attached.\n",group,group->sprc);
  if (group->sprv) free(group->sprv);
  free(group);
}
 
int sprite_group_ref(struct sprite_group *group) {
  if (!group) return -1;
  if (!group->refc) return 0; // Immortal.
  if (group->refc<1) return -1;
  if (group->refc>=INT_MAX) return -1;
  group->refc++;
  return 0;
}
 
struct sprite_group *sprite_group_new() {
  struct sprite_group *group=calloc(1,sizeof(struct sprite_group));
  if (!group) return 0;
  group->refc=1;
  return group;
}

/* Primitive membership ops, private.
 */
 
static int group_sprv_search(const struct sprite_group *group,const struct sprite *sprite) {
  switch (group->order) {
    case SPRITE_GROUP_ORDER_ADDR: {
        int lo=0,hi=group->sprc;
        while (lo<hi) {
          int ck=(lo+hi)>>1;
          const struct sprite *q=group->sprv[ck];
               if (sprite<q) hi=ck;
          else if (sprite>q) lo=ck+1;
          else return ck;
        }
        return -lo-1;
      }
    case SPRITE_GROUP_ORDER_RENDER: {
        // Render group is not guaranteed to remain in order at all times.
        // So we have to search linear. But do record the correct insertion point optimistically.
        int insp=-1;
        struct sprite **p=group->sprv;
        int i=0;
        for (;i<group->sprc;i++,p++) {
          const struct sprite *q=*p;
          if (q==sprite) return i;
          if ((insp<0)&&(q->layer>sprite->layer)) insp=i;
        }
        if (insp<0) return -group->sprc-1;
        return -insp-1;
      }
    default: { // Anything else, eg EXPLICIT, it's a linear search and insertion point is the end.
        struct sprite **p=group->sprv;
        int i=0;
        for (;i<group->sprc;i++,p++) {
          if (*p==sprite) return i;
        }
        return -group->sprc-1;
      }
  }
  return -1;
}

static int sprite_grpv_search(const struct sprite *sprite,const struct sprite_group *group) {
  int lo=0,hi=sprite->grpc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct sprite_group *q=sprite->grpv[ck];
         if (group<q) hi=ck;
    else if (group>q) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static int group_sprv_insert(struct sprite_group *group,int p,struct sprite *sprite) {
  if (!group||!sprite||(p<0)||(p>group->sprc)) return -1;
  if (group->sprc>=group->spra) {
    int na=group->spra+16;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(group->sprv,sizeof(void*)*na);
    if (!nv) return -1;
    group->sprv=nv;
    group->spra=na;
  }
  if (sprite_ref(sprite)<0) return -1;
  memmove(group->sprv+p+1,group->sprv+p,sizeof(void*)*(group->sprc-p));
  group->sprv[p]=sprite;
  group->sprc++;
  return 0;
}

static int sprite_grpv_insert(struct sprite *sprite,int p,struct sprite_group *group) {
  if (!sprite||!group||(p<0)||(p>sprite->grpc)) return -1;
  if (sprite->grpc>=sprite->grpa) {
    int na=sprite->grpa+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(sprite->grpv,sizeof(void*)*na);
    if (!nv) return -1;
    sprite->grpv=nv;
    sprite->grpa=na;
  }
  if (sprite_group_ref(group)<0) return -1;
  memmove(sprite->grpv+p+1,sprite->grpv+p,sizeof(void*)*(sprite->grpc-p));
  sprite->grpv[p]=group;
  sprite->grpc++;
  return 0;
}

static void group_sprv_remove(struct sprite_group *group,int p) {
  if ((p<0)||(p>=group->sprc)) return;
  struct sprite *sprite=group->sprv[p];
  group->sprc--;
  memmove(group->sprv+p,group->sprv+p+1,sizeof(void*)*(group->sprc-p));
  sprite_del(sprite);
}

static void sprite_grpv_remove(struct sprite *sprite,int p) {
  if ((p<0)||(p>=sprite->grpc)) return;
  struct sprite_group *group=sprite->grpv[p];
  sprite->grpc--;
  memmove(sprite->grpv+p,sprite->grpv+p+1,sizeof(void*)*(sprite->grpc-p));
  sprite_group_del(group);
}

/* Single membership ops, public.
 */

int sprite_group_has(const struct sprite_group *group,const struct sprite *sprite) {
  if (!group||!sprite) return 0;
  if ((group->sprc<sprite->grpc)&&(group->order==SPRITE_GROUP_ORDER_ADDR)) {
    return (group_sprv_search(group,sprite)>=0);
  } else {
    return (sprite_grpv_search(sprite,group)>=0);
  }
}

int sprite_group_add(struct sprite_group *group,struct sprite *sprite) {
  if (!group||!sprite) return -1;
  
  // Locate each in the other.
  int grpp=sprite_grpv_search(sprite,group);
  if (grpp>=0) return 0; // Already added (assume that group's ref exists too).
  int sprp=group_sprv_search(group,sprite);
  if (sprp>=0) return -1; // Lists inconsistent! This must never happen.
  grpp=-grpp-1;
  sprp=-sprp-1;
  
  // Add each, and if the second add fails, back out carefully.
  if (sprite_grpv_insert(sprite,grpp,group)<0) return -1;
  if (group_sprv_insert(group,sprp,sprite)<0) {
    sprite_grpv_remove(sprite,grpp);
    return -1;
  }
  return 0;
}

int sprite_group_remove(struct sprite_group *group,struct sprite *sprite) {
  if (!group||!sprite) return 0; // Nulls are effectively "not attached", which is not an error.
  int grpp=sprite_grpv_search(sprite,group);
  if (grpp<0) return 0; // Not attached. (assume the group doesn't list the sprite either).
  int sprp=group_sprv_search(group,sprite);
  if (sprp<0) return -1; // Lists inconsistent! This must never happen.
  // Usually we're dealing with immortal groups, but if (group) is mortal and on its last life, retain across the operation.
  if (group->refc==1) {
    if (sprite_group_ref(group)<0) return -1;
    sprite_grpv_remove(sprite,grpp);
    group_sprv_remove(group,sprp);
    sprite_group_del(group);
  } else {
    sprite_grpv_remove(sprite,grpp);
    group_sprv_remove(group,sprp);
  }
  return 0;
}

/* Bulk membership ops.
 */

void sprite_kill(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->grpc<1) return;
  if (sprite_ref(sprite)<0) return;
  while (sprite->grpc>0) {
    sprite->grpc--;
    struct sprite_group *group=sprite->grpv[sprite->grpc]; // Temporarily unlist one group without deleting it.
    int sprp=group_sprv_search(group,sprite);
    if (sprp>=0) {
      group_sprv_remove(group,sprp);
    }
    sprite_group_del(group);
  }
  sprite_del(sprite);
}

void sprite_group_clear(struct sprite_group *group) {
  if (!group) return;
  if (group->sprc<1) return;
  if (sprite_group_ref(group)<0) return;
  while (group->sprc>0) {
    group->sprc--;
    struct sprite *sprite=group->sprv[group->sprc];
    int grpp=sprite_grpv_search(sprite,group);
    if (grpp>=0) {
      sprite_grpv_remove(sprite,grpp);
    }
    sprite_del(sprite);
  }
  sprite_group_del(group);
}

void sprite_group_kill(struct sprite_group *group) {
  if (!group) return;
  if (group->sprc<1) return;
  if (sprite_group_ref(group)<0) return;
  while (group->sprc>0) {
    group->sprc--;
    struct sprite *sprite=group->sprv[group->sprc];
    sprite_kill(sprite);
    sprite_del(sprite); // sprite_kill() will miss the reference we just popped.
  }
  sprite_group_del(group);
}

/* Two passes of a cocktail sort, for the render group.
 * Render order can change on the fly. In this game, I don't think that will be happening much.
 * But we're equipped to catch changes to (sprite->layer) on the fly, just in case.
 * When they change, we might be out of order for a few frames until the sorting shakes out.
 */
 
static inline int sprite_rendercmp(const struct sprite *a,const struct sprite *b) {
  return a->layer-b->layer;
}

void sprite_group_rendersort_partial(struct sprite_group *group) {
  if (group->sprc<2) return;
  
  // Up.
  {
    int stopp=group->sprc-1;
    int i=0,done=1;
    for (;i<stopp;i++) {
      struct sprite *a=group->sprv[i];
      struct sprite *b=group->sprv[i+1];
      if (sprite_rendercmp(a,b)>0) {
        group->sprv[i]=b;
        group->sprv[i+1]=a;
        done=0;
      }
    }
    if (done) return;
  }
  
  // Down.
  {
    int i=group->sprc-1;
    for (;i>0;i--) {
      struct sprite *a=group->sprv[i];
      struct sprite *b=group->sprv[i-1];
      if (sprite_rendercmp(a,b)<0) {
        group->sprv[i]=b;
        group->sprv[i-1]=a;
      }
    }
  }
}

/* Copy one group to another.
 */
 
int sprite_group_copy(struct sprite_group *dst,struct sprite_group *src) {
  if (!dst||!src) return -1;
  if (dst->sprc) sprite_group_clear(dst);
  if (!src->sprc) return 0;
  if (dst->spra<src->sprc) {
    int na=src->sprc;
    if (na<INT_MAX-32) na+=32;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(dst->sprv,sizeof(void*)*na);
    if (!nv) return -1;
    dst->sprv=nv;
    dst->spra=na;
  }
  // Retain all the sprites in (src). If one fails, ay yi yi, back it out.
  int i=0;
  for (;i<src->sprc;i++) {
    if (sprite_ref(src->sprv[i])<0) {
      while (i-->0) sprite_del(src->sprv[i]);
      return -1;
    }
  }
  // And then just memcpy them over.
  memcpy(dst->sprv,src->sprv,sizeof(void*)*src->sprc);
  dst->sprc=src->sprc;
  return 0;
}
