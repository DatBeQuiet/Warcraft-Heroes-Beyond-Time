#include "AngelsGuardItem.h"
#include "ModuleRender.h"
#include "ModuleColliders.h"
#include "Scene.h"
#include "PlayerEntity.h"
#include "ModulePrinter.h"

bool AngelsGuardItem::Start()
{
	return true;
}

bool AngelsGuardItem::Act(ModuleItems::ItemEvent event, float dt)
{
	//Here you manage your item depending of the event you receive.
	switch (event)
	{
		case ModuleItems::ItemEvent::PLAYER_DIED:
		{
			App->scene->player->state = PlayerEntity::states::PL_RELIVE;
			App->items->unequipItem(this);
			break;
		}
	}
	return true;
}

bool AngelsGuardItem::Draw()
{
	//Use the ModulePrinter to print all the stuff.
	return true;
}

bool AngelsGuardItem::printIconOnScreen(iPoint pos)
{
	//The GUI uses this method, fill it in all the items.
	return App->render->Blit(App->items->getItemsTexture(), pos.x, pos.y, &SDL_Rect(ANGEL_ICON), 1, 0);
}

