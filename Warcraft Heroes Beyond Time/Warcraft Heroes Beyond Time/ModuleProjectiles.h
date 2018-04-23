#ifndef __MODULEPROJECTILES_H__
#define __MODULEPROJECTILES_H__

#include "Module.h"
#include "SDL/include/SDL.h"

#include <list>

struct Projectile;
struct ProjectileInfo;

enum Projectile_type
{
	no_type = -1,
	projectile_type,
	fel_ball
};

class ModuleProjectiles : public Module
{
public:

	ModuleProjectiles();
	~ModuleProjectiles();

	void Init();
	bool Awake(pugi::xml_node&);
	bool Start();
	bool PreUpdate();
	bool Update(float dt);
	bool PostUpdate();
	bool CleanUp();

	void AddProjectile(const ProjectileInfo& projectile, Projectile_type type);
	void DestroyProjectile(Projectile* toDelete);
	const SDL_Texture* GetProjectileAtlas() const;

private:
	std::list<Projectile*> projectilesList;
	std::list<Projectile*> toSpawnProjectilesList;
	std::list<Projectile*> toKillProjectilesList;

public:
	SDL_Texture* projectilesAtlas = nullptr;
};

#endif // __MODULEPROJECTILES_H__