#ifndef __ModuleColliders_H__
#define __ModuleColliders_H__

#include "Module.h"
#include "Entity.h"
#include "EntitiesEnums.h"
#include <vector>

struct Collider
{
	Collider(SDL_Rect colliderRect, COLLIDER_TYPE type, Entity* owner = nullptr, iPoint offset = iPoint(0,0));
	SDL_Rect colliderRect;										// El X i Y del Rect fan de offset !!!
	COLLIDER_TYPE type;

	Entity* owner = nullptr;
	COLLIDER_TYPE collidingWith = COLLIDER_TYPE::COLLIDER_NONE;	// when isn't property of an entity
};

class ModuleColliders : public Module
{
public:
	ModuleColliders();
	bool Awake(pugi::xml_node& consoleNode);
	bool Update(float dt);
	bool PostUpdate();
	bool CleanUp();

	Collider* AddCollider(SDL_Rect colliderRect, COLLIDER_TYPE type, Entity* owner = nullptr, iPoint offset = iPoint(0,0));
	Collider* AddTemporalCollider(SDL_Rect colliderRect, COLLIDER_TYPE type, int timer);
	void deleteCollider(Collider* col);
	void CleanCollidersEntity(Entity* entity);

	bool CheckTypeCollMatrix(COLLIDER_TYPE type, COLLIDER_TYPE type2);
	bool CheckCollision(int col1, int col2);
	bool ChechCollisionTemporalCollider(int col, int colTemporal);
	void PrintColliders();

	int GetTotalUnwalkableColliders();	// es una guarrada pero funciona �\_(�u�)_/�

private:
	std::vector<Collider*> colliders;
	// Aquestes 2 llistes van en paralel
	std::vector<Collider*> temporalColliders;
	std::vector<int> temporalColliderstimer;

public:
	bool printColliders = false;

};


#endif