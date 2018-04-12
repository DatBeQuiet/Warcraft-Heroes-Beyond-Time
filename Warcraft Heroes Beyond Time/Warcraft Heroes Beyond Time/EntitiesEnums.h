#ifndef __ENTITIESENUM_H__
#define __ENTITIESENUM_H__

enum SPRITESHEETS
{
	NON_TEXTURE = -1,
	THRALL_SHEET,
	FOOTMAN_SHEET,
	ITEMS_SHEET,
	MINES_SHEET,
	ARCHER_SHEET,
	PROJECTILE_SHEET,
	ARCHER_SMOKE_SHEET,
	GULDAN_SHEET
};

enum ENEMY_TYPE
{
	NON_ENEMY = -1,
	FOOTMAN,
	ARCHER,
	MAGE,
	DEATH_KNIGHT,
	GOBLIN,
	SKELETON,
	WEAPON			// Aixo es un recurs que faig servir per les fletxes
};

enum ENEMY_WEAPON_TYPE
{
	NON_WEAPON = -1,
	ARROW
};

enum PLAYER_TYPE
{
	NON_PLAYER = -1,
	THRALL,
	VALEERA,
	SYLVANAS
};

enum BOSS_TYPE
{
	NON_BOSS = -1,
	LICH_KING,
	ILLIDAN,
	GULDAN
};

enum CONSUMABLE_TYPE {
	NON_CONSUMABLE = -1,
	LIVE_POTION,
	ATACK_POTION,
	MOVEMENT_SPEED_POTION
};

enum CHEST_TYPE
{
	NON_CHEST = -1,
	LOW_CHEST,
	MID_CHEST,
	HIGH_CHEST
};

enum STATIC_ENTITY_TYPE
{
	NON_SENTITY = -1,
	PORTAL
};

enum FIXED_ANGLE
{
	NON_ANGLE = -1,
	UP,
	UP_RIGHT,
	RIGHT,
	DOWN_RIGHT,
	DOWN,
	DOWN_LEFT,
	LEFT,
	UP_LEFT
};

enum COLLIDER_TYPE
{
	COLLIDER_NONE = -1,
	COLLIDER_PLAYER,
	COLLIDER_ENEMY,
	COLLIDER_PLAYER_ATTACK,
	COLLIDER_ENEMY_ATTACK,
	COLLIDER_UNWALKABLE,
	COLLIDER_WALKABLE,
	COLLIDER_GULDAN,
	COLLIDER_FELBALL
};

#endif