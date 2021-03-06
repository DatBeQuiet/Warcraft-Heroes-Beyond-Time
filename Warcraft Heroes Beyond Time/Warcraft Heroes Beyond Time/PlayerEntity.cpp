#include "Application.h"
#include "PlayerEntity.h"
#include "ModuleInput.h"
#include "ModuleRender.h"
#include "Scene.h"
#include "ModuleMapGenerator.h"
#include "ModuleEntitySystem.h"
#include "ModulePrinter.h"
#include "ModuleColliders.h"
#include "ModuleAudio.h"
#include "Application.h"
#include "ModuleTextures.h"
#include "ModuleItems.h"
#include "GUIWindow.h"
#include "GUIImage.h"
#include "ModuleTransitions.h"
#include "EffectsElem.h"
#include "FileSystem.h"
#include "ParticleSystem.h"

#include "AngelsGuardItem.h"

PlayerEntity::PlayerEntity(fPoint coor, PLAYER_TYPE type, SDL_Texture* texture) : DynamicEntity(coor, texture, DynamicType::PLAYER), type(type)
{
	App->entities->LoadCDs(DashConfigCD, damagedConfigCD, deadinfloorConfigCD);
}

bool PlayerEntity::Start()
{
	anim = &idleDown;
	state = states::PL_IDLE;


	InitCulling();

	if (dashEmitter == nullptr)
	{
		dashEmitter = App->psystem->AddEmiter({ (pos.x - App->render->camera.x) / App->winScale, (pos.y - App->render->camera.y) / App->winScale }, EMITTER_TYPE_DASH, -1);
		dashEmitter->StopEmission();
	}

	return true;
}

bool PlayerEntity::Update(float dt)
{
	


	return true;
}

bool PlayerEntity::Finish() 
{ 


	return true; 
}

Collider* PlayerEntity::GetDamageCollider() const
{
	return damageCol;
}

fPoint PlayerEntity::CalculatePosFromBezier(fPoint startPos, fPoint handleA, float t, fPoint handleB, fPoint endPos)
{
	float t2 = pow(t, 2.0f);
	float t3 = pow(t, 3.0f);
	float subT = 1.0f - t;
	float subT2 = pow(subT, 2);
	float subT3 = pow(subT, 3);

	fPoint firstArgument;
	firstArgument.x = subT3 * startPos.x;
	firstArgument.y = subT3 * startPos.y;

	fPoint secondArgument;
	secondArgument.x = 3.0f * t * subT2 * handleA.x;
	secondArgument.y = 3.0f * t * subT2 * handleA.y;

	fPoint thirdArgument;
	thirdArgument.x = 3.0f * t2 * subT * handleB.x;
	thirdArgument.y = 3.0f * t2 * subT * handleB.y;

	fPoint fourthArgument;
	fourthArgument.x = t3 * endPos.x;
	fourthArgument.y = t3 * endPos.y;

	fPoint res;
	res.x = firstArgument.x + secondArgument.x + thirdArgument.x + fourthArgument.x;
	res.y = firstArgument.y + secondArgument.y + thirdArgument.y + fourthArgument.y;

	return res;
}

void PlayerEntity::ResetDash()
{
	state = states::PL_IDLE;
	t = 0.0f;
	DashCD = DashConfigCD;
	vCollision = hCollision = false;

	if (anim == &dashRight)
		anim = &idleRight;
	else if (anim == &dashLeft)
		anim = &idleLeft;
	else if (anim == &dashUp)
		anim = &idleUp;
	else if (anim == &dashDown)
		anim = &idleDown;
	else if (anim == &dashUpRight)
		anim = &idleUpRight;
	else if (anim == &dashUpLeft)
		anim = &idleUpLeft;
	else if (anim == &dashDownRight)
		anim = &idleDownRight;
	else if (anim == &dashDownLeft)
		anim = &idleDownLeft;
}

void PlayerEntity::PlayerStates(float dt)
{
	if (move && !App->transitions->IsFading())
	{
		if (App->input->IsKeyboardAvailable())
			KeyboardStates(dt);
		else
			JoyconStates(dt);

		//CheckMapLimits();
		CheckCulling();

		if (drawFZ)
			App->printer->PrintQuad(freeZone, { 255, 0, 0, 50 }, true, true);
	}
	else if(!App->transitions->IsFading())
	{
		CheckIddleStates();
		if (drawFZ)
			App->printer->PrintQuad(freeZone, { 255, 0, 0, 50 }, true, true);

	}
}

void PlayerEntity::KeyboardStates(float dt)
{

	if (dashEmitter != nullptr && App->scene->player->state == PlayerEntity::states::PL_DASH)
		GenerateDashParticles();

	switch (state)
	{
		case states::PL_IDLE:
		{
			if ((App->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT && App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT))
			{
				state = states::PL_UP_RIGHT;
				anim = &upRight;
			}
			else if (App->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT && App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
			{
				state = states::PL_UP_LEFT;
				anim = &upLeft;
			}
			else if (App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT && App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)
			{
				state = states::PL_DOWN_RIGHT;
				anim = &downRight;
			}
			else if (App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT && App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
			{
				state = states::PL_DOWN_LEFT;
				anim = &downLeft;
			}
			else if (App->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT)
			{
				state = states::PL_UP;
				anim = &up;
			}
			else if (App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT)
			{
				state = states::PL_DOWN;
				anim = &down;
			}
			else if (App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
			{
				state = states::PL_LEFT;
				anim = &left;
			}
			else if (App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)
			{
				state = states::PL_RIGHT;
				anim = &right;
			}
			else if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN && t == 0.0f && DashCD == 0.0f)
			{
				App->audio->PlayFx(App->audio->Thrall_Dash_FX);
				state = states::PL_DASH;
				startPos = pos;
				animBefore = anim;
			}
			else if (App->input->GetKey(SDL_SCANCODE_J) == KEY_DOWN)
			{
				state = states::PL_ATTACK;
				animBefore = anim;
				Attack();
			}
			else if (App->input->GetKey(SDL_SCANCODE_K) == KEY_DOWN && numStats.energy == 100)
			{
				state = states::PL_SKILL;
				animBefore = anim;
				anim = &skill;
				numStats.energy = 0;
				UseSkill();
			}
			break;
		}

		case states::PL_DASH:
		{
			if (t <= 1.0f && t >= 0.0f)
			{
				if (animBefore == &idleRight || animBefore == &right)
				{
					SDL_Rect copyWallcol = wallCol->rectArea;
					SDL_Rect otherCol;
					float distance = (startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance) - pos.x;
					copyWallcol.x += pos.x + distance;
					copyWallcol.y += pos.y;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{
						pos.x = startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance;
					}
					else
					{
						distance -= (copyWallcol.x + copyWallcol.w) - otherCol.x;

						pos.x += distance;
						ResetDash();
						break;
					}

					anim = &dashRight;
					float x = 0.1f / dt;
					t += (x * dt);
				}
				else if (animBefore == &idleLeft || animBefore == &left)
				{
					SDL_Rect copyWallcol = wallCol->rectArea;
					copyWallcol.x += pos.x;
					copyWallcol.y += pos.y;

					SDL_Rect otherCol;
					float distance = -((startPos.x - (CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance)) - pos.x);

					copyWallcol.x -= distance;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{

						pos.x -= distance;
					}
					else if (App->colliders->collideWithWalls(copyWallcol, otherCol))
					{
						distance -= (otherCol.x + otherCol.w) - copyWallcol.x;
						pos.x -= distance;
						ResetDash();
						break;
					}

					anim = &dashLeft;
					float x = 0.1f / dt;
					t += (x * dt);
				}
				else if (animBefore == &idleUp || animBefore == &up)
				{
					anim = &dashUp;
					SDL_Rect copyWallcol = wallCol->rectArea;
					copyWallcol.x += pos.x;
					copyWallcol.y += pos.y;

					SDL_Rect otherCol;
					float distance = -((startPos.y - (CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance)) - pos.y);

					copyWallcol.y -= distance;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{

						pos.y = startPos.y - CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance;
					}
					else
					{
						distance -= (otherCol.y + otherCol.h) - copyWallcol.y;
						pos.y -= distance;
						ResetDash();
						break;
					}

					anim = &dashUp;
					float x = 0.1f / dt;
					t += (x * dt);
				}
				else if (animBefore == &idleDown || animBefore == &down)
				{
					anim = &dashDown;
					SDL_Rect copyWallcol = wallCol->rectArea;
					SDL_Rect otherCol;
					float distance = (startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance) - pos.y;
					copyWallcol.x += pos.x;
					copyWallcol.y += pos.y + distance;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{
						pos.y = startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance;
					}
					else
					{
						distance -= (copyWallcol.y + copyWallcol.h) - otherCol.y;

						pos.y += distance;
						ResetDash();
						break;
					}

					anim = &dashDown;
					float x = 0.1f / dt;
					t += (x * dt);
				}
				else if (animBefore == &idleUpRight || animBefore == &upRight)
				{
					if (!vCollision)
					{
						anim = &dashUpRight;
						SDL_Rect copyWallcol = wallCol->rectArea;
						copyWallcol.x += pos.x;
						copyWallcol.y += pos.y;

						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });

						SDL_Rect otherCol;
						float distance = abs(pos.y - (startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(315.0f))));

						copyWallcol.y -= distance;

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{

							pos.y = startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(315.0f));
						}
						else
						{
							vCollision = true;
							distance -= (otherCol.y + otherCol.h) - copyWallcol.y;
							pos.y -= distance;
						}
					}

					if (!hCollision)
					{
						anim = &dashUpRight;
						SDL_Rect copyWallcol = wallCol->rectArea;
						SDL_Rect otherCol;
						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });
						float distance = abs(pos.x - (startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(315.0f))));
						copyWallcol.x += pos.x + distance;
						copyWallcol.y += pos.y;

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{
							pos.x = startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(315.0f));
						}
						else
						{
							distance -= (copyWallcol.x + copyWallcol.w) - otherCol.x;
							hCollision = true;
							pos.x += distance;
						}
					}

					if (vCollision && hCollision)
					{
						ResetDash();
						break;
					}

					anim = &dashUpRight;
					float x = 0.1f / dt;
					t += (x * dt);
				}
				else if (animBefore == &idleDownRight || animBefore == &downRight)
				{
					anim = &dashDownRight;

					if (!vCollision)
					{
						SDL_Rect copyWallcol = wallCol->rectArea;
						copyWallcol.x += pos.x;
						copyWallcol.y += pos.y;

						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });

						SDL_Rect otherCol;
						float distance = abs(pos.y - (startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(45.0f))));

						copyWallcol.y += distance;

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{
							pos.y = startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(45.0f));
						}
						else
						{
							vCollision = true;
							distance -= (copyWallcol.y + copyWallcol.h) - otherCol.y;
							pos.y += distance;
						}
					}

					if (!hCollision)
					{
						SDL_Rect copyWallcol = wallCol->rectArea;
						SDL_Rect otherCol;
						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });
						float distance = abs(pos.x - (startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(45.0f))));
						copyWallcol.x += pos.x + distance;
						copyWallcol.y += pos.y;



						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{
							pos.x = startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(45.0f));
						}
						else
						{
							distance -= (copyWallcol.x + copyWallcol.w) - otherCol.x;
							hCollision = true;
							pos.x += distance;
						}
					}

					if (vCollision && hCollision)
					{
						ResetDash();
						break;
					}
					float x = 0.1f / dt;
					t += (x * dt);
				}
				else if (animBefore == &idleDownLeft || animBefore == &downLeft)
				{
					anim = &dashDownLeft;

					if (!vCollision)
					{
						SDL_Rect copyWallcol = wallCol->rectArea;
						copyWallcol.x += pos.x;
						copyWallcol.y += pos.y;

						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });

						SDL_Rect otherCol;
						float distance = abs(pos.y - (startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(135.0f))));

						copyWallcol.y += distance;

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{
							pos.y = startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(135.0f));
						}
						else
						{
							vCollision = true;
							distance -= (copyWallcol.y + copyWallcol.h) - otherCol.y;
							pos.y += distance;
						}
					}

					if (!hCollision)
					{
						SDL_Rect copyWallcol = wallCol->rectArea;
						copyWallcol.x += pos.x;
						copyWallcol.y += pos.y;
						SDL_Rect otherCol;

						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });
						float distance = abs(pos.x - (startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(135.0f))));
						copyWallcol.x -= distance;

						if (distance < 0)
							int a = 0;

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{
							pos.x = startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(135.0f));
						}
						else
						{
							distance -= (otherCol.x + otherCol.w) - copyWallcol.x;
							hCollision = true;
							pos.x -= distance;
						}
					}

					if (vCollision && hCollision)
					{
						ResetDash();
						break;
					}

					float x = 0.1f / dt;
					t += (x * dt);
				}
				else if (animBefore == &idleUpLeft || animBefore == &upLeft)
				{
					anim = &dashUpLeft;

					if (!vCollision)
					{
						SDL_Rect copyWallcol = wallCol->rectArea;
						copyWallcol.x += pos.x;
						copyWallcol.y += pos.y;

						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });

						SDL_Rect otherCol;
						float distance = abs(pos.y - (startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(225.0f))));

						copyWallcol.y -= distance;

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{

							pos.y = startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(225.0f));
						}
						else
						{
							vCollision = true;
							distance -= (otherCol.y + otherCol.h) - copyWallcol.y;
							pos.y -= distance;
						}
					}

					if (!hCollision)
					{
						SDL_Rect copyWallcol = wallCol->rectArea;
						copyWallcol.x += pos.x;
						copyWallcol.y += pos.y;
						SDL_Rect otherCol;

						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });
						float distance = abs(pos.x - (startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(225.0f))));
						copyWallcol.x -= distance;

						if (distance < 0)
							int a = 0;

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{
							pos.x = startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(225.0f));
						}
						else
						{
							distance -= (otherCol.x + otherCol.w) - copyWallcol.x;
							hCollision = true;
							pos.x -= distance;
						}
					}

					if (vCollision && hCollision)
					{
						ResetDash();
						break;
					}

					float x = 0.1f / dt;
					t += (x * dt);
				}
			}
			else
			{
				ResetDash();
			}
			break;
		}

		case states::PL_UP:
		{
			pos.y -= numStats.speed * dt;

			if ((App->input->GetKey(SDL_SCANCODE_W) == KEY_UP))
			{
				state = states::PL_IDLE;
				anim = &idleUp;
			}
			else if (App->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT && App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)
			{
				state = states::PL_UP_RIGHT;
				anim = &upRight;
			}
			else if (App->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT && App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
			{
				state = states::PL_UP_LEFT;
				anim = &upLeft;
			}
			else if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN && DashCD == 0.0f)
			{
				App->audio->PlayFx(App->audio->Thrall_Dash_FX);
				startPos = pos;
				state = states::PL_DASH;
				animBefore = anim;
			}
			else if (App->input->GetKey(SDL_SCANCODE_J) == KEY_DOWN)
			{
				state = states::PL_ATTACK;
				animBefore = anim;
				Attack();
			}
			else if (App->input->GetKey(SDL_SCANCODE_K) == KEY_DOWN && numStats.energy == 100)
			{
				state = states::PL_SKILL;
				animBefore = anim;
				anim = &skill;
				numStats.energy = 0;
				UseSkill();
			}
			break;
		}

		case states::PL_DOWN:
		{
			pos.y += numStats.speed * dt;
			if (App->input->GetKey(SDL_SCANCODE_S) == KEY_UP)
			{
				state = states::PL_IDLE;
				anim = &idleDown;
			}
			else if (App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT && App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)
			{
				state = states::PL_DOWN_RIGHT;
				anim = &downRight;
			}
			else if (App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT && App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
			{
				state = states::PL_DOWN_LEFT;
				anim = &downLeft;
			}
			else if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN && DashCD == 0.0f)
			{
				App->audio->PlayFx(App->audio->Thrall_Dash_FX);
				startPos = pos;
				state = states::PL_DASH;
				animBefore = anim;
			}
			else if (App->input->GetKey(SDL_SCANCODE_J) == KEY_DOWN)
			{
				state = states::PL_ATTACK;
				animBefore = anim;
				Attack();
			}
			else if (App->input->GetKey(SDL_SCANCODE_K) == KEY_DOWN && numStats.energy == 100)
			{
				state = states::PL_SKILL;
				animBefore = anim;
				anim = &skill;
				numStats.energy = 0;
				UseSkill();
			}
			break;
		}

		case states::PL_LEFT:
		{
			pos.x -= numStats.speed * dt;
			if (App->input->GetKey(SDL_SCANCODE_A) == KEY_UP)
			{
				state = states::PL_IDLE;
				anim = &idleLeft;
			}
			else if (App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT && App->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT)
			{
				state = states::PL_UP_LEFT;
				anim = &upLeft;
			}
			else if (App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT && App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT)
			{
				state = states::PL_DOWN_LEFT;
				anim = &downLeft;
			}
			else if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN && DashCD == 0.0f)
			{
				App->audio->PlayFx(App->audio->Thrall_Dash_FX);
				startPos = pos;
				state = states::PL_DASH;
				animBefore = anim;
			}
			else if (App->input->GetKey(SDL_SCANCODE_J) == KEY_DOWN)
			{
				state = states::PL_ATTACK;
				animBefore = anim;
				Attack();
			}
			else if (App->input->GetKey(SDL_SCANCODE_K) == KEY_DOWN && numStats.energy == 100)
			{
				state = states::PL_SKILL;
				animBefore = anim;
				anim = &skill;
				numStats.energy = 0;
				UseSkill();
			}
			break;
		}

		case states::PL_RIGHT:
		{
			pos.x += numStats.speed * dt;
			if (App->input->GetKey(SDL_SCANCODE_D) == KEY_UP)
			{
				state = states::PL_IDLE;
				anim = &idleRight;
			}
			else if (App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT && App->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT)
			{
				state = states::PL_UP_RIGHT;
				anim = &upRight;
			}
			else if (App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT && App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT)
			{
				state = states::PL_DOWN_RIGHT;
				anim = &downRight;
			}
			else if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN && DashCD == 0.0f)
			{
				App->audio->PlayFx(App->audio->Thrall_Dash_FX);
				startPos = pos;
				state = states::PL_DASH;
				animBefore = anim;
			}
			else if (App->input->GetKey(SDL_SCANCODE_J) == KEY_DOWN)
			{
				state = states::PL_ATTACK;
				animBefore = anim;
				Attack();
			}
			else if (App->input->GetKey(SDL_SCANCODE_K) == KEY_DOWN && numStats.energy == 100)
			{
				state = states::PL_SKILL;
				animBefore = anim;
				anim = &skill;
				numStats.energy = 0;
				UseSkill();
			}
			break;
		}

		case states::PL_UP_LEFT:
		{
			pos.x -= numStats.speed * 0.75f * dt;
			pos.y -= numStats.speed * 0.75f * dt;

			if (App->input->GetKey(SDL_SCANCODE_W) == KEY_UP && App->input->GetKey(SDL_SCANCODE_A) == KEY_UP)
			{
				state = states::PL_IDLE;
				anim = &idleUpLeft;
			}
			else if (App->input->GetKey(SDL_SCANCODE_W) == KEY_UP)
			{
				state = states::PL_LEFT;
				anim = &left;
			}
			else if (App->input->GetKey(SDL_SCANCODE_A) == KEY_UP)
			{
				state = states::PL_UP;
				anim = &up;
			}
			else if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN && DashCD == 0.0f)
			{
				App->audio->PlayFx(App->audio->Thrall_Dash_FX);
				startPos = pos;
				state = states::PL_DASH;
				animBefore = anim;
			}
			else if (App->input->GetKey(SDL_SCANCODE_J) == KEY_DOWN)
			{
				state = states::PL_ATTACK;
				animBefore = anim;
				Attack();
			}
			else if (App->input->GetKey(SDL_SCANCODE_K) == KEY_DOWN && numStats.energy == 100)
			{
				state = states::PL_SKILL;
				animBefore = anim;
				anim = &skill;
				numStats.energy = 0;
				UseSkill();
			}
			break;
		}

		case states::PL_UP_RIGHT:
		{
			pos.x += numStats.speed * 0.75f * dt;
			pos.y -= numStats.speed * 0.75f * dt;
			if (App->input->GetKey(SDL_SCANCODE_W) == KEY_UP && App->input->GetKey(SDL_SCANCODE_D) == KEY_UP)
			{
				state = states::PL_IDLE;
				anim = &idleUpRight;
			}
			else if (App->input->GetKey(SDL_SCANCODE_W) == KEY_UP)
			{
				state = states::PL_RIGHT;
				anim = &right;
			}
			else if (App->input->GetKey(SDL_SCANCODE_D) == KEY_UP)
			{
				state = states::PL_UP;
				anim = &up;
			}
			else if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN && DashCD == 0.0f)
			{
				App->audio->PlayFx(App->audio->Thrall_Dash_FX);
				startPos = pos;
				state = states::PL_DASH;
				animBefore = anim;
			}
			else if (App->input->GetKey(SDL_SCANCODE_J) == KEY_DOWN)
			{
				state = states::PL_ATTACK;
				animBefore = anim;
				Attack();
			}
			else if (App->input->GetKey(SDL_SCANCODE_K) == KEY_DOWN && numStats.energy == 100)
			{
				state = states::PL_SKILL;
				animBefore = anim;
				anim = &skill;
				numStats.energy = 0;
				UseSkill();
			}
			break;
		}

		case states::PL_DOWN_LEFT:
		{
			pos.x -= numStats.speed * 0.75f * dt;
			pos.y += numStats.speed * 0.75f * dt;
			if (App->input->GetKey(SDL_SCANCODE_S) == KEY_UP && App->input->GetKey(SDL_SCANCODE_A) == KEY_UP)
			{
				state = states::PL_IDLE;
				anim = &idleDownLeft;
			}
			else if (App->input->GetKey(SDL_SCANCODE_S) == KEY_UP)
			{
				state = states::PL_LEFT;
				anim = &left;
			}
			else if (App->input->GetKey(SDL_SCANCODE_A) == KEY_UP)
			{
				state = states::PL_DOWN;
				anim = &down;
			}
			else if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN && DashCD == 0.0f)
			{
				App->audio->PlayFx(App->audio->Thrall_Dash_FX);
				startPos = pos;
				state = states::PL_DASH;
				animBefore = anim;
			}
			else if (App->input->GetKey(SDL_SCANCODE_J) == KEY_DOWN)
			{
				state = states::PL_ATTACK;
				animBefore = anim;
				Attack();
			}
			else if (App->input->GetKey(SDL_SCANCODE_K) == KEY_DOWN && numStats.energy == 100)
			{
				state = states::PL_SKILL;
				animBefore = anim;
				anim = &skill;
				numStats.energy = 0;
				UseSkill();
			}
			break;
		}

		case states::PL_DOWN_RIGHT:
		{
			pos.x += numStats.speed * 0.75f * dt;
			pos.y += numStats.speed * 0.75f * dt;
			if (App->input->GetKey(SDL_SCANCODE_S) == KEY_UP && App->input->GetKey(SDL_SCANCODE_D) == KEY_UP)
			{
				state = states::PL_IDLE;
				anim = &idleDownRight;
			}
			else if (App->input->GetKey(SDL_SCANCODE_S) == KEY_UP)
			{
				state = states::PL_RIGHT;
				anim = &right;
			}
			else if (App->input->GetKey(SDL_SCANCODE_D) == KEY_UP)
			{
				state = states::PL_DOWN;
				anim = &down;
			}
			else if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN && DashCD == 0.0f)
			{
				App->audio->PlayFx(App->audio->Thrall_Dash_FX);
				startPos = pos;
				state = states::PL_DASH;
				animBefore = anim;
			}
			else if (App->input->GetKey(SDL_SCANCODE_J) == KEY_DOWN)
			{
				state = states::PL_ATTACK;
				animBefore = anim;
				Attack();
			}
			else if (App->input->GetKey(SDL_SCANCODE_K) == KEY_DOWN && numStats.energy == 100)
			{
				state = states::PL_SKILL;
				animBefore = anim;
				anim = &skill;
				numStats.energy = 0;
				UseSkill();
			}
			break;
		}

		case states::PL_ATTACK:
		{
			if (animBefore == &idleDown || animBefore == &down)
				anim = &attackDown;

			else if (animBefore == &idleUp || animBefore == &up)
				anim = &attackUp;

			else if (animBefore == &idleUpRight || animBefore == &upRight)
				anim = &attackUpRight;

			else if (animBefore == &idleRight || animBefore == &right)
				anim = &attackRight;

			else if (animBefore == &idleDownRight || animBefore == &downRight)
				anim = &attackDownRight;

			else if (animBefore == &idleDownLeft || animBefore == &downLeft)
				anim = &attackDownLeft;

			else if (animBefore == &idleLeft || animBefore == &left)
				anim = &attackLeft;

			else if (animBefore == &idleUpLeft || animBefore == &upLeft)
				anim = &attackUpLeft;

			if (anim->Finished())
			{
				anim->Reset();

				if (animBefore == &left)
					anim = &idleLeft;
				else if (animBefore == &up)
					anim = &idleUp;
				else if (animBefore == &down)
					anim = &idleDown;
				else if (animBefore == &right)
					anim = &idleRight;
				else if (animBefore == &upRight)
					anim = &idleUpRight;
				else if (animBefore == &upLeft)
					anim = &idleUpLeft;
				else if (animBefore == &downLeft)
					anim = &idleDownLeft;
				else if (animBefore == &downRight)
					anim = &idleDownRight;
				else
					anim = animBefore;

				state = states::PL_IDLE;
			}

			break;
		}

		case states::PL_SKILL:
		{
			if (anim->Finished())
			{
				anim->Reset();
				anim = animBefore;

				if (animBefore == &left)
					anim = &idleLeft;
				else if (animBefore == &up)
					anim = &idleUp;
				else if (animBefore == &down)
					anim = &idleDown;
				else if (animBefore == &right)
					anim = &idleRight;
				else if (animBefore == &upRight)
					anim = &idleUpRight;
				else if (animBefore == &upLeft)
					anim = &idleUpLeft;
				else if (animBefore == &downLeft)
					anim = &idleDownLeft;
				else if (animBefore == &downRight)
					anim = &idleDownRight;
				else
					anim = animBefore;

				state = states::PL_IDLE;
			}
			break;
		}

		case states::PL_DEAD:
		{
			if (anim != &deadDownRight)
			{
				anim->Reset();
				animBefore = anim;
				anim = &deadDownRight;
			}
			else if (anim->Finished())
			{
				App->items->newEvent(ModuleItems::ItemEvent::PLAYER_DIED);
				if (state == states::PL_DEAD)
				{
					deadinfloorcd += dt;
					// PlayFX, Go to the main menu.
					if (deadinfloorcd > deadinfloorConfigCD)
					{
						deadinfloorcd = 0.0f;
						App->scene->GoMainMenu();
					}
				}
				else
				{
					deadDownRight.Reset();
				}

			}
			break;
		}

		case states::PL_RELIVE:
		{
			if (reliveCounter == 0.0f)
				App->effects->CreateEffect({ pos.x-10, pos.y - 100 }, timeReliving, App->effects->playerReliveAnim);

			anim = &idleDown;

			reliveCounter += dt;

			if (reliveCounter > timeReliving)
			{
				state = states::PL_IDLE;
				reliveCounter = 0.0f;
				numStats.hp = numStats.maxhp;
				App->audio->PlayMusic(App->audio->InGameBSO.data());
			}

			break;
		}

		case states::PL_WIN:
		{
			afterWinCounter += dt;
			if (afterWinCounter > 20)
			{
				App->fs->deleteSavedGame();
				App->scene->GoMainMenu();
			}
			else if (afterWinCounter > 2)
			{
				App->scene->CreateGratitudeScreen();
			}
			break;
		}
	}

	if (DashCD > 0.0f)
	{
		DashCD -= dt;
		if (DashCD < 0.0f)
			DashCD = 0.0f;
	}

	if (damaged)
	{
		damagedCD += dt;
		
		uint percent = damagedCD * 100 / damagedConfigCD;
		float alpha = 255 - (percent * 255 / 100);
		App->scene->blood->setOpacity(alpha);

		if (damagedCD > damagedConfigCD)
		{
			SDL_SetTextureColorMod(App->entities->spritesheetsEntities[THRALL_SHEET], 255, 255, 255);
			damaged = false;
			damagedCD = 0.0f;
			
			App->gui->DestroyElem(App->scene->blood);
			App->scene->blood = nullptr;
		}
	}
}

void PlayerEntity::JoyconStates(float dt)
{
	if (dashEmitter != nullptr && App->scene->player->state == PlayerEntity::states::PL_DASH)
		GenerateDashParticles();


	switch (state)
	{
	case states::PL_IDLE:
	{
		if (App->input->GetXAxis() != 0 || App->input->GetYAxis() != 0)
		{
			state = states::PL_MOVE;
		}

		else if (App->input->GetAction("Dash") == KEY_DOWN && t == 0.0f && move)
		{
			App->audio->PlayFx(App->audio->Thrall_Dash_FX);
			App->input->PlayJoyRumble(0.85f, 100);
			startPos = pos;
			state = states::PL_DASH;
			animBefore = anim;
		}

		else if (App->input->GetAction("Attack") == KEY_DOWN)
		{
			animBefore = anim;
			state = states::PL_ATTACK;
			Attack();
		}

		else if (App->input->GetAction("Skill") == KEY_DOWN && numStats.energy == 100)
		{
			animBefore = anim;
			anim = &skill;
			numStats.energy = 0;
			state = states::PL_SKILL;
			UseSkill();
		}
		break;
	}


	case states::PL_DASH:
	{
		if (App->input->GetAction("Attack") == KEY_DOWN)
		{
			attackWhileDash = true;
		}
		if (t <= 1.0f && t >= 0.0f)
		{
			if (animBefore == &idleRight)
			{
				anim = &dashRight;
				SDL_Rect copyWallcol = wallCol->rectArea; 
				SDL_Rect otherCol;
				float distance = (startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance) - pos.x;
				copyWallcol.x += pos.x + distance;
				copyWallcol.y += pos.y;

				if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
				{
					pos.x = startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance;
				}
				else
				{
					distance -= (copyWallcol.x + copyWallcol.w) - otherCol.x;
					
					pos.x += distance;
					ResetDash();
					break;
				}

			}
			else if (animBefore == &idleLeft)
			{
				anim = &dashLeft;
				SDL_Rect copyWallcol = wallCol->rectArea; 
				copyWallcol.x += pos.x;
				copyWallcol.y += pos.y;

				SDL_Rect otherCol;
				float distance = -((startPos.x - (CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance)) - pos.x);

				copyWallcol.x -= distance;

				if (!App->colliders->collideWithWalls(copyWallcol, otherCol) )
				{
					
					pos.x -= distance;
				}
				else if(App->colliders->collideWithWalls(copyWallcol, otherCol))
				{
					distance -= (otherCol.x + otherCol.w) - copyWallcol.x;
					pos.x -= distance;        
					ResetDash();
					break;
				}
				
			}
			else if (animBefore == &idleUp)
			{
				anim = &dashUp;
				SDL_Rect copyWallcol = wallCol->rectArea;
				copyWallcol.x += pos.x;
				copyWallcol.y += pos.y;

				SDL_Rect otherCol;
				float distance = -((startPos.y - (CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance)) - pos.y);

				copyWallcol.y -= distance;

				if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
				{

					pos.y = startPos.y - CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance;
				}
				else
				{
					distance -= (otherCol.y + otherCol.h) - copyWallcol.y;
					pos.y -= distance;
					ResetDash();
					break;
				}
			
			}
			else if (animBefore == &idleDown)
			{
				anim = &dashDown;
				SDL_Rect copyWallcol = wallCol->rectArea;
				SDL_Rect otherCol;
				float distance = (startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance) - pos.y;
				copyWallcol.x += pos.x;
				copyWallcol.y += pos.y + distance;

				if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
				{
					pos.y = startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance;
				}
				else
				{
					distance -= (copyWallcol.y + copyWallcol.h) - otherCol.y;

					pos.y += distance;
					ResetDash();
					break;
				}
			}
			else if (animBefore == &idleUpRight)
			{
				if (!vCollision)
				{
					anim = &dashUpRight;
					SDL_Rect copyWallcol = wallCol->rectArea;
					copyWallcol.x += pos.x;
					copyWallcol.y += pos.y;

					fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });

					SDL_Rect otherCol;
					float distance = abs(pos.y - (startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(315.0f))));

					copyWallcol.y -= distance;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{

						pos.y = startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(315.0f));
					}
					else
					{
						vCollision = true;
						distance -= (otherCol.y + otherCol.h) - copyWallcol.y;
						pos.y -= distance;
					}
				}

				if (!hCollision)
				{
					anim = &dashUpRight;
					SDL_Rect copyWallcol = wallCol->rectArea;
					SDL_Rect otherCol;
					fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });
					float distance = abs(pos.x - (startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(315.0f))));
					copyWallcol.x += pos.x + distance;
					copyWallcol.y += pos.y;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{
						pos.x = startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(315.0f));
					}
					else
					{
						distance -= (copyWallcol.x + copyWallcol.w) - otherCol.x;
						hCollision = true;
						pos.x += distance;
					}
				}

				if (vCollision && hCollision)
				{
					ResetDash();
					break;
				}
			}
			else if (animBefore == &idleUpLeft)
			{
				anim = &dashUpLeft;

				if (!vCollision)
				{
					SDL_Rect copyWallcol = wallCol->rectArea;
					copyWallcol.x += pos.x;
					copyWallcol.y += pos.y;

					fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });

					SDL_Rect otherCol;
					float distance = abs(pos.y - (startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(225.0f))));

					copyWallcol.y -= distance;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{

						pos.y = startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(225.0f));
					}
					else
					{
						vCollision = true;
						distance -= (otherCol.y + otherCol.h) - copyWallcol.y; 
						pos.y -= distance;
					}
				}

				if (!hCollision)
				{
					SDL_Rect copyWallcol = wallCol->rectArea;
					copyWallcol.x += pos.x;
					copyWallcol.y += pos.y;
					SDL_Rect otherCol;

					fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });
					float distance = abs(pos.x - (startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(225.0f))));
					copyWallcol.x -= distance;

					if (distance < 0)
						int a = 0;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{
						pos.x = startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(225.0f));
					}
					else
					{
						distance -= (otherCol.x + otherCol.w) - copyWallcol.x;
						hCollision = true;
						pos.x -= distance;
					}
				}

				if (vCollision && hCollision)
				{
					ResetDash();
					break;
				}
			
			}
			else if (animBefore == &idleDownRight)
			{
				anim = &dashDownRight;

				if (!vCollision)
				{
					SDL_Rect copyWallcol = wallCol->rectArea;
					copyWallcol.x += pos.x;
					copyWallcol.y += pos.y;

					fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });

					SDL_Rect otherCol;
					float distance = abs(pos.y - (startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(45.0f))));

					copyWallcol.y += distance;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{
						pos.y = startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(45.0f));
					}
					else
					{
						vCollision = true;
						distance -= (copyWallcol.y + copyWallcol.h) - otherCol.y;
						pos.y += distance;
					}
				}

				if (!hCollision)
				{
					SDL_Rect copyWallcol = wallCol->rectArea;
					SDL_Rect otherCol;
					fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });
					float distance = abs(pos.x - (startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(45.0f))));
					copyWallcol.x += pos.x + distance;
					copyWallcol.y += pos.y;

					

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{
						pos.x = startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(45.0f));
					}
					else
					{
						distance -= (copyWallcol.x + copyWallcol.w) - otherCol.x;
						hCollision = true;
						pos.x += distance;
					}
				}

				if (vCollision && hCollision)
				{
					ResetDash();
					break;
				}

				
			}
			else if (animBefore == &idleDownLeft)
			{
				anim = &dashDownLeft;

				if (!vCollision)
				{
					SDL_Rect copyWallcol = wallCol->rectArea;
					copyWallcol.x += pos.x;
					copyWallcol.y += pos.y;

					fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });

					SDL_Rect otherCol;
					float distance = abs(pos.y - (startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(135.0f))));

					copyWallcol.y += distance;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{
						pos.y = startPos.y + dashDistance * bezierPoint.y * sin(DEG_2_RAD(135.0f));
					}
					else
					{
						vCollision = true;
						distance -= (copyWallcol.y + copyWallcol.h) - otherCol.y;
						pos.y += distance;
					}
				}

				if (!hCollision)
				{
					SDL_Rect copyWallcol = wallCol->rectArea;
					copyWallcol.x += pos.x;
					copyWallcol.y += pos.y;
					SDL_Rect otherCol;

					fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });
					float distance = abs(pos.x - (startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(135.0f))));
					copyWallcol.x -= distance;

					if (distance < 0)
						int a = 0;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{
						pos.x = startPos.x + dashDistance * bezierPoint.y * cos(DEG_2_RAD(135.0f));
					}
					else
					{
						distance -= (otherCol.x + otherCol.w) - copyWallcol.x;
						hCollision = true;
						pos.x -= distance;
					}
				}

				if (vCollision && hCollision)
				{
					ResetDash();
					break;
				}
			}
			else
			{
				anim = GetAnimFromAngle(angle, true);

				if (anim == &dashRight)
				{
					SDL_Rect copyWallcol = wallCol->rectArea;
					SDL_Rect otherCol;
					float distance = abs(pos.x-(startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * cos(DEG_2_RAD(angle))));
					copyWallcol.x += pos.x + distance;
					copyWallcol.y += pos.y;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{
						pos.x = startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * cos(DEG_2_RAD(angle));
						pos.y = startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * sin(DEG_2_RAD(angle));
					}
					else
					{
						distance -= (copyWallcol.x + copyWallcol.w) - otherCol.x;
						pos.x += distance;
						ResetDash();
						break;
					}
				}
				else if (anim == &dashLeft)
				{
					SDL_Rect copyWallcol = wallCol->rectArea;
					SDL_Rect otherCol;
					float distance = abs(pos.x - (startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * cos(DEG_2_RAD(angle))));
					copyWallcol.x += pos.x - distance;
					copyWallcol.y += pos.y;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{
						pos.x = startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * cos(DEG_2_RAD(angle));
						pos.y = startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * sin(DEG_2_RAD(angle));
					}
					else
					{
						distance -= (otherCol.x + otherCol.w) - copyWallcol.x;

						pos.x -= distance;
						ResetDash();
						break;
					}
				}
				else if (anim == &dashUp)
				{
					SDL_Rect copyWallcol = wallCol->rectArea;
					SDL_Rect otherCol;
					float distance = abs(pos.y-(startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * sin(DEG_2_RAD(angle))));
					copyWallcol.x += pos.x;
					copyWallcol.y += pos.y - distance;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{
						pos.x = startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * cos(DEG_2_RAD(angle));
						pos.y = startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * sin(DEG_2_RAD(angle));
					}
					else
					{
						distance -= (otherCol.y + otherCol.h) - copyWallcol.y;

						pos.y -= distance;
						ResetDash();
						break;
					}
				}
				else if (anim == &dashDown)
				{
					SDL_Rect copyWallcol = wallCol->rectArea;
					SDL_Rect otherCol;
					float distance = abs(pos.y - (startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * sin(DEG_2_RAD(angle))));
					copyWallcol.x += pos.x;
					copyWallcol.y += pos.y + distance;

					if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
					{
						pos.x = startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * cos(DEG_2_RAD(angle));
						pos.y = startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * sin(DEG_2_RAD(angle));
					}
					else
					{
						distance -= (copyWallcol.y + copyWallcol.h) - otherCol.y;

						pos.y += distance;
						ResetDash();
						break;
					}
				}
				else if (anim == &dashUpLeft)
				{
					if (!vCollision)
					{
						SDL_Rect copyWallcol = wallCol->rectArea;
						copyWallcol.x += pos.x;
						copyWallcol.y += pos.y;

						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });

						SDL_Rect otherCol;
						float distance = abs(pos.y - (startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * sin(DEG_2_RAD(angle))));

						copyWallcol.y -= distance;

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{
							pos.y = startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * sin(DEG_2_RAD(angle));
						}
						else
						{
							vCollision = true;
							distance -= (otherCol.y + otherCol.h) - copyWallcol.y;
							pos.y -= distance;
						}
					}

					if (!hCollision)
					{
						SDL_Rect copyWallcol = wallCol->rectArea;
						copyWallcol.x += pos.x;
						copyWallcol.y += pos.y;
						SDL_Rect otherCol;

						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });
						float distance = abs(pos.x - (startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * cos(DEG_2_RAD(angle))));
						copyWallcol.x -= distance;

						if (distance < 0)
							int a = 0;

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{
							pos.x = startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * cos(DEG_2_RAD(angle));
						}
						else
						{
							distance -= (otherCol.x + otherCol.w) - copyWallcol.x;
							hCollision = true;
							pos.x -= distance;
						}
					}

					if (vCollision && hCollision)
					{
						ResetDash();
						break;
					}
				}
				else if (anim == &dashUpRight)
				{
					if (!vCollision)
					{
						anim = &dashUpRight;
						SDL_Rect copyWallcol = wallCol->rectArea;
						copyWallcol.x += pos.x;
						copyWallcol.y += pos.y;

						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });

						SDL_Rect otherCol;
						float distance = abs(pos.y - (startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * sin(DEG_2_RAD(angle))));

						copyWallcol.y -= distance;

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{

							pos.y = startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * sin(DEG_2_RAD(angle));
						}
						else
						{
							vCollision = true;
							distance -= (otherCol.y + otherCol.h) - copyWallcol.y;
							pos.y -= distance;
						}
					}

					if (!hCollision)
					{
						anim = &dashUpRight;
						SDL_Rect copyWallcol = wallCol->rectArea;
						SDL_Rect otherCol;
						float distance = abs(pos.x - (startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * cos(DEG_2_RAD(angle))));
						copyWallcol.x += pos.x + distance;
						copyWallcol.y += pos.y;

						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{
							pos.x = startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * cos(DEG_2_RAD(angle));
						}
						else
						{
							distance -= (copyWallcol.x + copyWallcol.w) - otherCol.x;
							hCollision = true;
							pos.x += distance;
						}
					}

					if (vCollision && hCollision)
					{
						ResetDash();
						break;
					}
				}
				else if (anim == &dashDownRight)
				{
					if (!vCollision)
					{
						SDL_Rect copyWallcol = wallCol->rectArea;
						copyWallcol.x += pos.x;
						copyWallcol.y += pos.y;

						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });

						SDL_Rect otherCol;
						float distance = abs(pos.y - (startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * sin(DEG_2_RAD(angle))));

						copyWallcol.y += distance;

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{
							pos.y = startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * sin(DEG_2_RAD(angle));
						}
						else
						{
							vCollision = true;
							distance -= (copyWallcol.y + copyWallcol.h) - otherCol.y;
							pos.y += distance;
						}
					}

					if (!hCollision)
					{
						SDL_Rect copyWallcol = wallCol->rectArea;
						SDL_Rect otherCol;
						float distance = abs(pos.x - (startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * cos(DEG_2_RAD(angle))));
						copyWallcol.x += pos.x + distance;
						copyWallcol.y += pos.y;

						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{
							pos.x = startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * cos(DEG_2_RAD(angle));
						}
						else
						{
							distance -= (copyWallcol.x + copyWallcol.w) - otherCol.x;
							hCollision = true;
							pos.x += distance;
						}
					}

					if (vCollision && hCollision)
					{
						ResetDash();
						break;
					}
				}
				else if (anim == &dashDownLeft)
				{
					if (!vCollision)
					{
						SDL_Rect copyWallcol = wallCol->rectArea;
						copyWallcol.x += pos.x;
						copyWallcol.y += pos.y;

						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });

						SDL_Rect otherCol;
						float distance = abs(pos.y - (startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * sin(DEG_2_RAD(angle))));

						copyWallcol.y += distance;

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{
							pos.y = startPos.y + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * sin(DEG_2_RAD(angle));
						}
						else
						{
							vCollision = true;
							distance -= (copyWallcol.y + copyWallcol.h) - otherCol.y;
							pos.y += distance;
						}
					}

					if (!hCollision)
					{
						SDL_Rect copyWallcol = wallCol->rectArea;
						copyWallcol.x += pos.x;
						copyWallcol.y += pos.y;
						SDL_Rect otherCol;

						fPoint bezierPoint = CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f });
						float distance = abs(pos.x - (startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * cos(DEG_2_RAD(angle))));
						copyWallcol.x -= distance;

						if (distance < 0)
							int a = 0;

						if (!App->colliders->collideWithWalls(copyWallcol, otherCol))
						{
							pos.x = startPos.x + CalculatePosFromBezier({ 0.0f, 0.0f }, handleA, t, handleB, { 1.0f, 1.0f }).y * dashDistance * cos(DEG_2_RAD(angle));
						}
						else
						{
							distance -= (otherCol.x + otherCol.w) - copyWallcol.x;
							hCollision = true;
							pos.x -= distance;
						}
					}

					if (vCollision && hCollision)
					{
						ResetDash();
						break;
					}
				}
			}

			float x = 0.1f / dt;
			t += (x * dt);

		}
		else
		{
			if (attackWhileDash)
			{
				state = states::PL_ATTACK;
				attackWhileDash = false;

				if (anim == &dashRight)
					animBefore = &idleRight;
				else if (anim == &dashDown)
					animBefore = &idleDown;
				else if (anim == &dashUpRight)
					animBefore = &idleUpRight;
				else if (anim == &dashDownLeft)
					animBefore = &idleDownLeft;
				else if (anim == &dashDownRight)
					animBefore = &idleDownRight;
				else if (anim == &dashUpRight)
					animBefore = &idleUpRight;
				else if (anim == &dashLeft)
					animBefore = &idleLeft;
				else if (anim == &dashUpLeft)
					animBefore = &idleUpLeft;
				else if (anim == &dashUp)
					animBefore = &idleUp;
				
				
				Attack();
				DashCD = DashConfigCD;
				t = 0.0f;
				vCollision = hCollision = false;
				break;

			}
			else if (App->input->InsideDeadZone())
			{
				state = states::PL_IDLE;

				if (anim == &dashRight)
					anim = &idleRight;
				else if (anim == &dashDown)
					anim = &idleDown;
				else if (anim == &dashUpRight)
					anim = &idleUpRight;
				else if (anim == &dashDownLeft)
					anim = &idleDownLeft;
				else if (anim == &dashDownRight)
					anim = &idleDownRight;
				else if (anim == &dashUpRight)
					anim = &idleUpRight;
				else if (anim == &dashLeft)
					anim = &idleLeft;
				else if (anim == &dashUpLeft)
					anim = &idleUpLeft;
				else if (anim == &dashUp)
					anim = &idleUp;
			}
			else
			{
				state = states::PL_MOVE;
				anim = animBefore;
			}
			DashCD = DashConfigCD;
			animBefore = nullptr;
			vCollision = hCollision = false;
			t = 0.0f;
		}

		break;
	}

	case states::PL_MOVE:
	{
		if (App->input->GetAction("Dash") == KEY_DOWN && t == 0.0f && DashCD == 0.0f && move)
		{
			App->audio->PlayFx(App->audio->Thrall_Dash_FX);
			App->input->PlayJoyRumble(0.75f, 100);
			animBefore = anim;
			startPos = pos;
			state = states::PL_DASH;

			float X = App->input->GetXAxis() / MAX_JAXIS_VALUE;
			float Y = App->input->GetYAxis() / MAX_JAXIS_VALUE;

			angle = App->input->GetAngleFromAxis();

			break;
		}

		float X = App->input->GetXAxis() / MAX_JAXIS_VALUE;
		float Y = App->input->GetYAxis() / MAX_JAXIS_VALUE;

		pos.x += X * numStats.speed * dt;
		pos.y += Y * numStats.speed * dt;

		angle = App->input->GetAngleFromAxis();

		Animation* tmpAnim = GetAnimFromAngle(angle);

		if (tmpAnim != nullptr)
			anim = tmpAnim;

		if (App->input->GetXAxis() == 0 && App->input->GetYAxis() == 0)
		{
			if (anim == &up)
				anim = &idleUp;
			if (anim == &down)
				anim = &idleDown;
			else if (anim == &right)
				anim = &idleRight;
			else if (anim == &left)
				anim = &idleLeft;
			else if (anim == &upRight)
				anim = &idleUpRight;
			else if (anim == &upLeft)
				anim = &idleUpLeft;
			else if (anim == &downRight)
				anim = &idleDownRight;
			else if (anim == &downLeft)
				anim = &idleDownLeft;

			state = states::PL_IDLE;
			break;
		}

		if (App->input->GetAction("Attack") == KEY_DOWN)
		{
			animBefore = anim;
			state = states::PL_ATTACK;
			Attack();
			break;
		}

		else if (App->input->GetAction("Skill") == KEY_DOWN && numStats.energy == 100)
		{
			animBefore = anim;
			anim = &skill;
			numStats.energy = 0;
			state = states::PL_SKILL;
			UseSkill();
		}
		break;
	}

	case states::PL_ATTACK:
	{
		if (animBefore == &idleDown || animBefore == &down)
			anim = &attackDown;

		else if (animBefore == &idleUp || animBefore == &up)
			anim = &attackUp;

		else if (animBefore == &idleLeft || animBefore == &left)
			anim = &attackLeft;

		else if (animBefore == &idleRight || animBefore == &right)
			anim = &attackRight;

		else if (animBefore == &upLeft || animBefore == &idleUpLeft)
			anim = &attackUpLeft;

		else if (animBefore == &upRight || animBefore == &idleUpRight)
			anim = &attackUpRight;

		else if (animBefore == &downLeft || animBefore == &idleDownLeft)
			anim = &attackDownLeft;

		else if (animBefore == &downRight || animBefore == &idleDownRight)
			anim = &attackDownRight;

		if (anim->Finished())
		{
			anim->Reset();
			anim = animBefore;

			if (animBefore == &left || animBefore == &up || animBefore == &right || animBefore == &down || animBefore == &upRight || animBefore == &upLeft || animBefore == &downLeft || animBefore == &downRight)
				state = states::PL_MOVE;
			else
				state = states::PL_IDLE;
		}
		break;
	}

	case states::PL_SKILL:
	{
		if (anim->Finished())
		{

			anim->Reset();
			anim = animBefore;

			if (animBefore == &left || animBefore == &up || animBefore == &right || animBefore == &down || animBefore == &upRight || animBefore == &upLeft || animBefore == &downLeft || animBefore == &downRight)
				state = states::PL_MOVE;
			else
				state = states::PL_IDLE;
		}
		break;
	}

	case states::PL_DEAD:
	{
		if (anim != &deadDownRight)
		{
			anim->Reset();
			animBefore = anim;
			anim = &deadDownRight;
		}
		else if (anim->Finished())
		{
			App->items->newEvent(ModuleItems::ItemEvent::PLAYER_DIED);
			if (state == states::PL_DEAD)
			{
				deadinfloorcd += dt;
				// PlayFX, Go to the main menu.
				if (deadinfloorcd > deadinfloorConfigCD)
				{
					deadinfloorcd = 0.0f;
					App->scene->GoMainMenu();
				}
			}
			else
			{
				deadDownRight.Reset();
			}
			
		}
		break;
	}

	case states::PL_RELIVE:
	{
		if (reliveCounter == 0.0f)
			App->effects->CreateEffect({ pos.x - 10, pos.y - 100 }, timeReliving, App->effects->playerReliveAnim);

		anim = &idleDown;

		reliveCounter += dt;

		if (reliveCounter > timeReliving)
		{
			state = states::PL_IDLE;
			reliveCounter = 0.0f;
			numStats.hp = numStats.maxhp;
			App->audio->PlayMusic(App->audio->InGameBSO.data());
		}

		break;
	}

	case states::PL_WIN:
	{
		afterWinCounter += dt;
		if (afterWinCounter > 20)
		{
			App->scene->GoMainMenu();
		}
		else if (afterWinCounter > 2 && !App->scene->gratitudeON)
		{
			App->scene->CreateGratitudeScreen();
		}
		break;
	}
	}

	if (DashCD > 0.0f)
	{
		DashCD -= dt;
		if (DashCD < 0.0f)
			DashCD = 0.0f;
	}

	if (damaged)
	{
		damagedCD += dt;

		uint percent = damagedCD * 100 / damagedConfigCD;
		Uint8 alpha = 255 - (percent * 255 / 100);
		App->scene->blood->setOpacity(alpha);

		if (damagedCD > damagedConfigCD)
		{
			SDL_SetTextureColorMod(App->entities->spritesheetsEntities[THRALL_SHEET], 255, 255, 255);
			damaged = false;
			damagedCD = 0.0f;
		
			App->gui->DestroyElem(App->scene->blood);
			App->scene->blood = nullptr;
		}

		
	}
}

bool PlayerEntity::GetConcretePlayerStates(int stat)
{
	if (stat == (int)state)
		return true;
	return false;
}

void PlayerEntity::CheckIddleStates()
{
	switch (state)
	{
	case states::PL_DOWN:
		anim = &idleDown;
		state = states::PL_IDLE;
		break;
	case states::PL_DOWN_LEFT:
		anim = &idleDownLeft;
		state = states::PL_IDLE;
		break;
	case states::PL_LEFT:
		anim = &idleLeft;
		state = states::PL_IDLE;
		break;
	case states::PL_UP_LEFT:
		anim = &idleUpLeft;
		state = states::PL_IDLE;
		break;
	case states::PL_UP:
		anim = &idleUp;
		state = states::PL_IDLE;
		break;
	case states::PL_UP_RIGHT:
		anim = &idleUpRight;
		state = states::PL_IDLE;
		break;
	case states::PL_RIGHT:
		anim = &idleRight;
		state = states::PL_IDLE;
		break;
	case states::PL_DOWN_RIGHT:
		anim = &idleDownRight;
		state = states::PL_IDLE;
		break;
	case states::PL_DASH:
		anim = &idleDown;
		state = states::PL_IDLE;
		break;
	case states::PL_ATTACK:
		anim = &idleDown;
		state = states::PL_IDLE;
		break;
	default:
		anim = &idleDown;
		state = states::PL_IDLE;
		ResetDash();
		break;
	}
}

Animation* PlayerEntity::GetAnimFromAngle(float angle, bool dashOn)
{
	Animation* animToReturn = nullptr;

	if (angle >= 247.5f && angle < 292.5f) // to change
	{
		if (!dashOn)
			animToReturn = &up;
		else
			animToReturn = &dashUp;
		lastFixedAnglePlayer = FIXED_ANGLE::UP;
	}

	else if (angle >= 67.5f && angle < 112.5f)
	{
		if (!dashOn)
			animToReturn = &down;
		else
			animToReturn = &dashDown;
		lastFixedAnglePlayer = FIXED_ANGLE::DOWN;
	}

	else if (((angle >= 337.5f && angle < 360.0f) || (angle >= 0 && angle < 22.5f) && !App->input->InsideDeadZone()))
	{
		if (!dashOn)
			animToReturn = &right;
		else
			animToReturn = &dashRight;
		lastFixedAnglePlayer = FIXED_ANGLE::RIGHT;
	}

	else if (angle >= 157.5f && angle < 202.5f) // to change
	{
		if (!dashOn)
			animToReturn = &left;
		else
			animToReturn = &dashLeft;
		lastFixedAnglePlayer = FIXED_ANGLE::LEFT;
	}

	else if (angle >= 292.5f && angle < 337.5f)
	{
		if (!dashOn)
			animToReturn = &upRight;
		else
			animToReturn = &dashUpRight;
		lastFixedAnglePlayer = FIXED_ANGLE::UP_RIGHT;
	}

	else if (angle >= 202.5f && angle < 247.5f) // to change
	{
		if (!dashOn)
			animToReturn = &upLeft;
		else
			animToReturn = &dashUpLeft;
		lastFixedAnglePlayer = FIXED_ANGLE::UP_LEFT;
	}

	else if (angle >= 112.5f && angle < 157.5f)
	{
		if (!dashOn)
			animToReturn = &downLeft;
		else
			animToReturn = &dashDownLeft;
		lastFixedAnglePlayer = FIXED_ANGLE::DOWN_LEFT;
	}

	else if (angle >= 22.5f && angle < 67.5f)
	{
		if (!dashOn)
			animToReturn = &downRight;
		else
			animToReturn = &dashDownRight;
		lastFixedAnglePlayer = FIXED_ANGLE::DOWN_RIGHT;
	}
	else
	{
		if (dashOn)
			animToReturn = &dashRight;
	}

	return animToReturn;
}

FIXED_ANGLE PlayerEntity::returnFixedAngle()
{
	//if (angle >= 247.5f && angle < 292.5f)
	//	lastFixedAnglePlayer = UP;
	//else if (angle >= 67.5f && angle < 112.5f)
	//	lastFixedAnglePlayer = DOWN;
	//else if ((angle >= 337.5f && angle < 360.0f) || (angle >= 0 && angle < 22.5f))
	//	lastFixedAnglePlayer = RIGHT;
	//else if (angle >= 157.5f && angle < 202.5f)
	//	lastFixedAnglePlayer = LEFT;
	//else if (angle >= 292.5f && angle < 337.5f)
	//	lastFixedAnglePlayer = UP_RIGHT;
	//else if (angle >= 202.5f && angle < 247.5f)
	//	lastFixedAnglePlayer = UP_LEFT;
	//else if (angle >= 112.5f && angle < 157.5f)
	//	lastFixedAnglePlayer = DOWN_LEFT;
	//else if (angle >= 22.5f && angle < 67.5f)
	//	lastFixedAnglePlayer = DOWN_RIGHT;

	return lastFixedAnglePlayer;
}

bool PlayerEntity::IsPlayerMoving()
{
	return state == states::PL_MOVE;
}

void PlayerEntity::GodMode(bool state)
{
	godMode = state;
}

void PlayerEntity::Walk(bool can)
{
	this->move = can;
}

void PlayerEntity::InitCulling()
{
	if (this == App->scene->player)
	{
		SDL_Rect currRect = anim->GetCurrentRect();
		iPoint pivot = anim->GetCurrentPivot();

		App->render->fcamerax = -1 * (this->pos.x - pivot.x + currRect.w / 2 - App->render->camera.w / 2);
		App->render->fcameray = -1 * (this->pos.y - pivot.y + currRect.h / 2 - App->render->camera.h / 2);

		freeZonex = pos.x - pivot.x - 55 / 2;
		freeZoney = pos.y - pivot.y - 55 / 2;

		freeZone.x = pos.x - pivot.x - 55;
		freeZone.y = pos.y - pivot.y - 55;
		freeZone.w = 55 / 2 * 2 + 55;
		freeZone.h = 55 / 2 * 2 + 47;
	}
}

void PlayerEntity::CheckCulling()
{
	if (this == App->scene->player)
	{
		uint w, h;
		int tilesize;
		App->map->getSize(w, h);
		tilesize = App->map->getTileSize() + 2;
		SDL_Rect currentRect = anim->GetCurrentRect();
		currentRect.w = wallCol->rectArea.w;
		currentRect.h = wallCol->rectArea.h;
		//iPoint pivot = anim->GetCurrentPivot();
		iPoint pivot = { wallCol->rectArea.x, wallCol->rectArea.y };//false
		fPoint topleft = { pos.x + pivot.x, pos.y + pivot.y };

		if (freeZonex > topleft.x && -App->render->camera.x > 0)
		{
			if (App->render->fcamerax + freeZonex - topleft.x > 0)
			{
				freeZonex -= -App->render->fcamerax;
				App->render->fcamerax = 0;
			}
			else
			{
				App->render->fcamerax += freeZonex - topleft.x;
				freeZonex = topleft.x;
			}
		}


		else if (freeZonex + freeZone.w < topleft.x + currentRect.w  && -App->render->camera.x < w * (tilesize - 2) - App->render->camera.w)
		{
			App->render->fcamerax -= (topleft.x + currentRect.w) - (freeZonex + freeZone.w);
			freeZonex = (topleft.x + currentRect.w) - freeZone.w;
		}

		if (freeZoney > topleft.y && -App->render->camera.y > 0)
		{
			if (App->render->fcameray + freeZoney - topleft.y > 0)
			{
				freeZoney -= -App->render->fcameray;
				App->render->fcameray = 0;
			}
			else
			{
				App->render->fcameray += freeZoney - topleft.y;
				freeZoney = topleft.y;
			}

		}
		else if (freeZoney + freeZone.h < topleft.y + currentRect.h && -App->render->camera.y + App->render->camera.h < h * (tilesize - 2))
		{
			App->render->fcameray -= (topleft.y + currentRect.h) - (freeZoney + freeZone.h);
			freeZoney = topleft.y + currentRect.h - freeZone.h;
		}

		freeZone.x = (int)freeZonex;
		freeZone.y = (int)freeZoney;


	}
}

void PlayerEntity::CheckMapLimits()
{
	uint w, h;
	App->map->getSize(w, h);

	if (w != 0 && h != 0)
	{
		if (pos.x - this->anim->GetCurrentPivot().x < 0)
			pos.x = 0 + this->anim->GetCurrentPivot().x;

		else if (pos.x + 55 - this->anim->GetCurrentPivot().x > w * (48 - 2))
		{
			pos.x = w * (48 - 2) - 55 + this->anim->GetCurrentPivot().x;
		}

		if (pos.y - this->anim->GetCurrentPivot().y < 0)
			pos.y = 0 + this->anim->GetCurrentPivot().y;

		else if (pos.y + 47 - this->anim->GetCurrentPivot().y > h * (48 - 2))
		{
			pos.y = h * (48 - 2) - 47 + this->anim->GetCurrentPivot().y;
		}
	}
}

void PlayerEntity::SetDamage(float damage, bool setStateDamage)
{
	if (numStats.hp > 0 && damaged == false)
	{
		if ((int)numStats.hp - damage <= 0)
		{
			numStats.hp = 0;
			ResetDash();
			state = states::PL_DEAD;
			App->audio->PauseMusic(0.5);
			App->audio->PlayFx(App->audio->Thrall_Die_FX);
			App->audio->HaltFX(-1, 1000);
			if(App->scene->lvlIndex != 100)
				App->fs->deleteSavedGame();
		}
		else
		{
			GUIWindow* blood = (GUIWindow*)App->gui->CreateGUIWindow({ 0,0 }, { 0,0,0,0 }, nullptr, nullptr);
			blood->menu = false;
			GUIImage* image = (GUIImage*)App->gui->CreateGUIImage({ 0,0 }, { 0, 912, 640, 360 }, nullptr, blood);
			App->scene->blood = blood;
			
			App->audio->PlayFx(App->audio->Thrall_Hitted_FX);
			damaged = true;
			SDL_SetTextureColorMod(App->entities->spritesheetsEntities[THRALL_SHEET], 255, 100, 100);
			numStats.hp -= damage;
		}
	}
}

void PlayerEntity::Heal(float amount)
{
	if (numStats.hp + amount > numStats.maxhp)
		numStats.hp = numStats.maxhp;
	else
		numStats.hp += amount;
}

void PlayerEntity::GenerateDashParticles()
{
	fPoint anglePoint;

	if (anim == &dashUp)
	{
		dashEmitter->MoveEmitter({ ((pos.x + App->render->camera.x) / App->winScale) + 16, ((pos.y + App->render->camera.y) / App->winScale) + 15 });
		dashEmitter->ChangeEmissionAngleRange(90.0f, 90.0f);
		dashEmitter->ChangeEmitterTextureRect({ 108, 63, 32, 34 });
		anglePoint.x = 90.0f;
		anglePoint.y = 90.0f;
	}
	else if (anim == &dashUpRight)
	{
		dashEmitter->MoveEmitter({ ((pos.x + App->render->camera.x) / App->winScale) + 13, ((pos.y + App->render->camera.y) / App->winScale) + 15 });
		dashEmitter->ChangeEmissionAngleRange(45.0f, 45.0f);
		dashEmitter->ChangeEmitterTextureRect({ 0, 66, 35, 31 });
		anglePoint.x = 45.0f;
		anglePoint.y = 45.0f;
	}
	else if (anim == &dashRight)
	{
		dashEmitter->MoveEmitter({ ((pos.x + App->render->camera.x) / App->winScale) + 5, ((pos.y + App->render->camera.y) / App->winScale) + 15 });
		dashEmitter->ChangeEmissionAngleRange(0.0f, 0.0f);
		dashEmitter->ChangeEmitterTextureRect({ 95, 27, 45, 35 });
		anglePoint.x = 0.0f;
		anglePoint.y = 0.0f;
	}
	else if (anim == &dashDownRight)
	{
		dashEmitter->MoveEmitter({ ((pos.x + App->render->camera.x) / App->winScale) + 13, ((pos.y + App->render->camera.y) / App->winScale) + 15 });
		dashEmitter->ChangeEmissionAngleRange(315.0f, 315.0f);
		dashEmitter->ChangeEmitterTextureRect({ 70, 33, 25, 31 });
		anglePoint.x = 315.0f;
		anglePoint.y = 315.0f;
	}
	else if (anim == &dashDown)
	{
		dashEmitter->MoveEmitter({ ((pos.x + App->render->camera.x) / App->winScale) + 13, ((pos.y + App->render->camera.y) / App->winScale) + 15 });
		dashEmitter->ChangeEmissionAngleRange(270.0f, 270.0f);
		dashEmitter->ChangeEmitterTextureRect({ 75, 67, 32, 30 });
		anglePoint.x = 270.0f;
		anglePoint.y = 270.0f;
	}
	else if (anim == &dashDownLeft)
	{
		dashEmitter->MoveEmitter({ ((pos.x + App->render->camera.x) / App->winScale) + 13, ((pos.y + App->render->camera.y) / App->winScale) + 15 });
		dashEmitter->ChangeEmissionAngleRange(225.0f, 225.0f);
		dashEmitter->ChangeEmitterTextureRect({ 0, 33, 41, 32 });
		anglePoint.x = 225.0f;
		anglePoint.y = 225.0f;
	}
	else if (anim == &dashLeft)
	{
		dashEmitter->MoveEmitter({ ((pos.x + App->render->camera.x) / App->winScale) + 5, ((pos.y + App->render->camera.y) / App->winScale) + 15 });
		dashEmitter->ChangeEmissionAngleRange(180.0f, 180.0f);
		dashEmitter->ChangeEmitterTextureRect({ 37, 64, 37, 33 });
		anglePoint.x = 180.0f;
		anglePoint.y = 180.0f;
	}
	else if (anim == &dashUpLeft)
	{
		dashEmitter->MoveEmitter({ ((pos.x + App->render->camera.x) / App->winScale) + 13, ((pos.y + App->render->camera.y) / App->winScale) + 15 });
		dashEmitter->ChangeEmissionAngleRange(135.0f, 135.0f);
		dashEmitter->ChangeEmitterTextureRect({ 42, 35, 27, 29 });
		anglePoint.x = 135.0f;
		anglePoint.y = 135.0f;
	}

	if (dashEmitter->GetEmitterAngleRange().x == anglePoint.x && dashEmitter->GetEmitterAngleRange().y == anglePoint.y)
		dashEmitter->StartEmission(100);
}

void PlayerEntity::DrawFreeZone(bool boolean)
{
	drawFZ = boolean;
}

void PlayerEntity::PushOut(Collider* wall)
{
	bool collideByRight = false, collideByLeft = false, collideByTop = false, collideByBottom = false;
	SDL_Rect wall_r = wall->rectArea;
	SDL_Rect player_col = { wallCol->rectArea.x + (int)pos.x, wallCol->rectArea.y + (int)pos.y, wallCol->rectArea.w, wallCol->rectArea.h };

	if (wall->rectArea.x + wall->rectArea.w / 2 <= wallCol->rectArea.x + (int)pos.x)
		collideByRight = true;

	else if (wall->rectArea.x + wall->rectArea.w / 2 > wallCol->rectArea.x + (int)pos.x + wallCol->rectArea.w)
		collideByLeft = true;

	if (wall->rectArea.y + wall->rectArea.h / 2 < wallCol->rectArea.y + (int)pos.y)
		collideByBottom = true;

	else if (wall->rectArea.y + wall->rectArea.h / 2 >= wallCol->rectArea.y + (int)pos.y + wallCol->rectArea.h)
		collideByTop = true;

	//4 main direction collisions
	if (collideByRight && !collideByBottom && !collideByTop)
	{
		pos.x += (wall->rectArea.x + wall->rectArea.w - (wallCol->rectArea.x + (int)pos.x));
	}
	else if (collideByLeft && !collideByTop && !collideByBottom)
	{
		pos.x -= (wallCol->rectArea.x + wallCol->rectArea.w + pos.x) - wall->rectArea.x;
	}
	else if (collideByTop && !collideByLeft && !collideByRight)
	{
		pos.y -= (wallCol->rectArea.y + (int)pos.y + wallCol->rectArea.h) - wall->rectArea.y;
	}
	else if (collideByBottom && !collideByLeft && !collideByRight)
	{
		pos.y += wall->rectArea.y + wall->rectArea.h - (wallCol->rectArea.y + (int)pos.y);
	}

	//Combination between them (choose the closest direction)
	else if (collideByTop && collideByRight)
	{
		if ((player_col.y + player_col.h) - wall_r.y < (wall_r.x + wall_r.w - player_col.x))
		{
			pos.y -= (wallCol->rectArea.y + (int)pos.y + wallCol->rectArea.h) - wall->rectArea.y;
		}
		else
		{
			pos.x += (wall->rectArea.x + wall->rectArea.w) - ((int)pos.x + wallCol->rectArea.x);
		}
	}
	else if (collideByTop && collideByLeft)
	{
		if ((player_col.y + player_col.h) - wall_r.y < (player_col.x + player_col.w - wall_r.x))
		{
			pos.y -= (player_col.y + player_col.h) - wall_r.y;
		}
		else
		{
			pos.x -= player_col.x + player_col.w - wall_r.x;
		}
	}
	else if (collideByBottom && collideByRight)
	{
		if ((wall_r.y + wall_r.h - player_col.y) < (wall_r.x + wall_r.w - player_col.x))
		{
			pos.y += (wall_r.y + wall_r.h - player_col.y);
		}
		else
		{
			pos.x += (wall_r.x + wall_r.w - player_col.x);
		}
	}
	else if (collideByBottom && collideByLeft)
	{
		if ((wall_r.y + wall_r.h - player_col.y) < (player_col.x + player_col.w - wall_r.x))
		{
			pos.y += (wall_r.y + wall_r.h - player_col.y);
		}
		else
		{
			pos.x -= (player_col.x + player_col.w - wall_r.x);
		}
	}
}

void PlayerEntity::IncreaseEnergy(int percent)
{
	if (numStats.energy + percent < 100)
		numStats.energy += percent;

	else if (numStats.energy < 100)
	{
		numStats.energy = 100;
		App->audio->PlayFx(App->audio->Thrall_EnergyMax_FX);
	}

}

bool PlayerEntity::Draw()
{
	bool ret = true;

	if (damaged)
		ret = App->printer->PrintSprite(iPoint(pos.x, pos.y), texture, anim->GetCurrentFrame(), 0, ModulePrinter::Pivots::CUSTOM_PIVOT, anim->GetCurrentPivot(), ModulePrinter::Pivots::UPPER_LEFT, { 0,0 }, 0, { 255,100,100,255 });
	else
		ret = App->printer->PrintSprite(iPoint(pos.x, pos.y), texture, anim->GetCurrentFrame(), 0, ModulePrinter::Pivots::CUSTOM_PIVOT, anim->GetCurrentPivot());

	return ret;
}