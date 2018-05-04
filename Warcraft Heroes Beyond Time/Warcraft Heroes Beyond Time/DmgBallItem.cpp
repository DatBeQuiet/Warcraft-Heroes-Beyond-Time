#include "DMGBallItem.h"
#include "ModuleRender.h"
#include "ModuleColliders.h"
#include "Scene.h"
#include "PlayerEntity.h"
#include "ModulePrinter.h"

bool DMGBallItem::Start()
{
	angular_vel = 250.0f;
	ball_col = App->colliders->AddPlayerAttackCollider({ 0, 0, 20, 20 }, App->scene->player, ModuleItems::dmgBallDamage, PlayerAttack::P_Attack_Type::DMGBALL_ITEM);
	return true;
}

bool DMGBallItem::Act(ModuleItems::ItemEvent event, float dt)
{
	//Here you manage your item depending of the event you receive.
	switch (event)
	{
	case ModuleItems::ItemEvent::UPDATE:
		if (ball_col.expired())
			ball_col = App->colliders->AddPlayerAttackCollider({ 0, 0, 20, 20 }, App->scene->player, ModuleItems::dmgBallDamage, PlayerAttack::P_Attack_Type::DMGBALL_ITEM);
		
		ball_counter += dt;
		angle = angular_vel * ball_counter;
		if (angle > 360)
		{
			ball_counter = 0;
		}
	
		(*ball_col.lock())->rectArea.x = Ball_pos.x = cos(angle*PI/180) * radius + 0;
		(*ball_col.lock())->rectArea.y = Ball_pos.y = sin(angle*PI/180) * radius + 0;
		break;
	}
	return true;
}

bool DMGBallItem::Draw()
{
	//Use the ModulePrinter to print all the stuff.
	iPoint Draw_pos = { (int)(App->scene->player->pos.x + Ball_pos.x), (int)(App->scene->player->pos.y + Ball_pos.y) };
	App->printer->PrintSprite(Draw_pos, App->items->getItemsTexture(), RED_BALL_ICON, 0, ModulePrinter::Pivots::UPPER_LEFT, { 0,0 }, ModulePrinter::Pivots::CENTER, {0,0}, angle * 2);
	return true;
}

bool DMGBallItem::printIconOnScreen(iPoint pos)
{
	//The GUI uses this method, fill it in all the items.
	return App->render->Blit(App->items->getItemsTexture(), pos.x, pos.y, &SDL_Rect(RED_BALL_ICON), 1, 0);
}

