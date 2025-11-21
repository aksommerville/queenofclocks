#include "game/queenofclocks.h"

/* Delete.
 */
 
void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->refc-->1) return;
  if (sprite->type->del) sprite->type->del(sprite);
  if (sprite->grpc) fprintf(stderr,"WARNING: Deleting sprite %p(%s) with %d groups still attached.\n",sprite,sprite->type->name,sprite->grpc);
  if (sprite->grpv) free(sprite->grpv);
  free(sprite);
}

/* Retain.
 */
 
int sprite_ref(struct sprite *sprite) {
  if (!sprite) return -1;
  if (sprite->refc<1) return -1;
  if (sprite->refc>=INT_MAX) return -1;
  sprite->refc++;
  return 0;
}

/* New.
 */
 
struct sprite *sprite_new(double x,double y,const struct sprite_type *type,int rid,uint32_t arg,const void *cmd,int cmdc) {
  if (rid&&!cmdc) cmdc=qc_res_get(&cmd,EGG_TID_sprite,rid);
  if (!type) {
    if (!(type=sprite_type_from_commands(cmd,cmdc))) return 0;
  }
  struct sprite *sprite=calloc(1,type->objlen);
  if (!sprite) return 0;
  
  sprite->refc=1;
  sprite->type=type;
  sprite->w=1.0;
  sprite->h=1.0;
  sprite->layer=256;
  sprite->rid=rid;
  sprite->arg=arg;
  sprite->cmd=cmd;
  sprite->cmdc=cmdc;
  
  // Join keepalive, all sprites must.
  if (sprite_group_add(g.grpv+NS_sprgrp_keepalive,sprite)<0) {
    sprite_del(sprite);
    return 0;
  }
  
  // Update with known generic commands.
  if (cmdc) {
    uint32_t grpmask=0;
    struct cmdlist_reader reader;
    if (sprite_reader_init(&reader,cmd,cmdc)<0) {
      sprite_kill(sprite);
      sprite_del(sprite);
      return 0;
    }
    struct cmdlist_entry cmd;
    while (cmdlist_reader_next(&cmd,&reader)>0) {
      switch (cmd.opcode) {
        case CMD_sprite_tile: sprite->tileid=cmd.arg[0]; sprite->xform=cmd.arg[1]; break;
        case CMD_sprite_groups: grpmask=(cmd.arg[0]<<24)|(cmd.arg[1]<<16)|(cmd.arg[2]<<8)|cmd.arg[3]; break;
        case CMD_sprite_size: sprite->w=cmd.arg[0]+cmd.arg[1]/256.0; sprite->h=cmd.arg[2]+cmd.arg[3]/256.0; break;
      }
    }
    grpmask&=~((1<<NS_sprgrp_keepalive)|(1<<NS_sprgrp_deathrow)); // Not allowed to join keepalive or deathrow via resource.
    if (grpmask) {
      uint32_t bit=1,p=0;
      for (;p<32;p++,bit<<=1) {
        if (grpmask&bit) {
          if (sprite_group_add(g.grpv+p,sprite)<0) {
            sprite_kill(sprite);
            sprite_del(sprite);
            return 0;
          }
        }
      }
    } else { // Omitting grpmask is not technically an error, but realistically it must always be set.
      fprintf(stderr,"sprite:%d did not set grpmask\n",rid);
    }
  }
  
  // We now have the correct size, hopefully, so set (x,y). The (x,y) we receive is the center, but sprite records the corner.
  sprite->x=x-sprite->w*0.5;
  sprite->y=y-sprite->h*0.5;
  
  // Give type's initializer a crack at it.
  if (type->init&&(type->init(sprite)<0)) {
    sprite_kill(sprite);
    sprite_del(sprite);
  }
  
  return sprite;
}

/* New sprite, WEAK.
 */
 
struct sprite *sprite_spawn(
  double x,double y,
  const struct sprite_type *type,
  int rid,uint32_t arg,
  const void *cmd,int cmdc
) {
  struct sprite *sprite=sprite_new(x,y,type,rid,arg,cmd,cmdc);
  if (!sprite) return 0;
  if (sprite->grpc<1) {
    sprite_del(sprite);
    return 0;
  }
  sprite_del(sprite);
  return sprite;
}

/* Sprite type lookup.
 */
 
const struct sprite_type *sprite_type_from_id(int id) {
  switch (id) {
    #define _(tag) case NS_sprtype_##tag: return &sprite_type_##tag;
    FOR_EACH_SPRTYPE
    #undef _
  }
  return 0;
}

const struct sprite_type *sprite_type_from_commands(const void *v,int c) {
  struct cmdlist_reader reader;
  if (sprite_reader_init(&reader,v,c)<0) return 0;
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    if (cmd.opcode!=CMD_sprite_type) continue;
    return sprite_type_from_id((cmd.arg[0]<<8)|cmd.arg[1]);
  }
  return 0;
}
