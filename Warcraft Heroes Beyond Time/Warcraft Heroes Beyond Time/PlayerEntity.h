#ifndef __PLAYERENTITY_H__
#define __PLAYERENTITY_H__

#include "DynamicEntity.h"

class PlayerEntity : public DynamicEntity 
{

protected:

	PLAYER_TYPE type = PLAYER_TYPE::NON_PLAYER;
	Animation idleDown, idleUp, idleLeft, idleRight, idleUpRight, idleUpLeft, idleDownRight, idleDownLeft;
	Animation up, down, left, right, upLeft, upRight, downLeft, downRight;
	Animation dashRight, dashLeft, dashUp, dashDown, dashUpRight, dashUpLeft, dashDownRight, dashDownLeft;
	Animation* animBeforeDash = nullptr;
	Animation animDashUp[8];
	float speed = 250.0f;
	bool move = true;

	enum class states
	{
		PL_NON_STATE,
		PL_IDLE,
		PL_UP,
		PL_DOWN,
		PL_LEFT,
		PL_RIGHT,
		PL_UP_RIGHT,
		PL_UP_LEFT,
		PL_DOWN_RIGHT,
		PL_DOWN_LEFT,

		PL_DASH

	} state;

	states last_state = states::PL_NON_STATE;

public:
	PlayerEntity(fPoint coor, PLAYER_TYPE type, SDL_Texture* texture);

	void Walk(bool);

	virtual bool Start();
	virtual bool Update(float dt);
	void PlayerStates(float dt);
	void KeyboardStates(float dt);
	void JoyconStates(float dt);
	virtual bool Finish();

	//This functions calculates player postion given a Bezier Curve

	fPoint CalculatePosFromBezier(fPoint startPos, fPoint handleA, float t, fPoint handleB, fPoint endPos);

	// Dash variables

	fPoint handleA = { 0.6f, 0.0f };
	fPoint handleB = { 0.4f, 1.0f };


	fPoint endPos = { 0.0f, 0.0f };
	fPoint startPos = { 0.0f, 0.0f };
	
	bool dashEnabled = false;
	float t = 0.0f;

};

#endif