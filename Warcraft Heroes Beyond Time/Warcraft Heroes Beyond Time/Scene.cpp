#include "Application.h"
#include "Scene.h"
#include "ModuleEntitySystem.h"
#include "ModuleGUI.h"
#include "ModuleInput.h"
#include "Console.h"
#include "ModuleAudio.h"
#include "ModuleMapGenerator.h"
#include "ModuleRender.h"
#include "ModuleColliders.h"
#include "ChestEntity.h"
#include "PortalEntity.h"
#include "Pathfinding.h"
#include "PlayerEntity.h"
#include "ModulePrinter.h"
#include "Item.h"
#include "WCItem.h"
#include "ModuleTextures.h"
#include "ModuleItems.h"
#include "FileSystem.h"
#include "BossEntity.h"
#include "ModuleEffects.h"
#include "ModuleProjectiles.h"
#include "Guldan.h"
#include "ModuleTransitions.h"
#include "IntroVideo.h"
#include "ModuleVideo.h"
#include "ParticleSystem.h"

#include "Brofiler\Brofiler.h"
#include "Label.h"
#include "InputBox.h"
#include "Button.h"
#include "GUIWindow.h"
#include "Slider.h"
#include "GUIImage.h"
#include "ItemContainer.h"



class ConsoleMap : public ConsoleOrder
{
	std::string orderName() { return "map"; }
	void Exec(std::string parametre, int parametreNumeric) {
		if (parametre == "printwalkables")
			if (parametreNumeric == 1)
				App->path->printWalkables = true;
			else if (parametreNumeric == 0)
				App->path->printWalkables = false;
	}
};


Scene::Scene()
{
	name = "scene";
}

Scene::~Scene() {}

bool Scene::Awake(pugi::xml_node& sceneNode)
{
	App->audio->PlayMusic(App->audio->MainMenuBSO.data(), 0);

	return true;
}

bool Scene::Start()
{
	gratitudeON = false;
	restart = false;
	App->gui->Activate();

	App->psystem->AddEmiter({ 200, 200 }, EMITTER_TYPE_FIRE);

	currentPercentAudio = App->audio->MusicVolumePercent;

	SetScene(next_scene);

	switch (actual_scene)
	{
		case Stages::INTRO_VIDEO:
		{
			App->introVideo->Activate();

			break;
		}
		case Stages::MAIN_MENU:
		{
			CreateMainMenuScreen();
			lvlIndex = 0;

			break;
		}
		case Stages::SETTINGS:
		{
			CreateSettingsScreen();

			break;
		}
		case Stages::INGAME:
		{
			BROFILER_CATEGORY("InGame Generation", Profiler::Color::Chocolate);

			int result = App->map->UseYourPowerToGenerateMeThisNewMap(lvlIndex);

			if (result == -1)
				return false;
			else if (result == 0)
			{
				App->items->Activate();
				App->colliders->Activate();
				App->entities->Activate();
				App->console->Activate();
				App->map->Activate();
				App->printer->Activate();
				App->projectiles->Activate();

				App->audio->PlayMusic(App->audio->GuldanBSO.data(), 1);

				portal = (PortalEntity*)App->entities->AddStaticEntity({ 15 * 46,17 * 46, }, PORTAL);
				portal->locked = true;
				player = App->entities->AddPlayer({ 15 * 46 + 10,16 * 46, }, THRALL, playerStats);
				player_HP_Bar = App->gui->CreateHPBar(player, { 10,5 });
				guldan = (Guldan*)App->entities->AddBoss(GULDAN_BASE, BossType::GULDAN);
			}
			else
			{
				App->effects->Activate();
				App->colliders->Activate();
				App->entities->Activate();
				App->console->Activate();
				App->map->Activate();
				App->printer->Activate();
				App->projectiles->Activate();

				player = App->entities->AddPlayer({ (float)App->map->begginingNode->pos.x * 46, (float)App->map->begginingNode->pos.y * 46 }, THRALL, playerStats);
				player_HP_Bar = App->gui->CreateHPBar(player, { 10,5 });

				App->path->LoadPathMap();

				std::list<SDL_Rect>::iterator it = App->map->archers.begin();
				std::advance(it, lvlIndex);

				int numberArchers = 0;
				do
				{
					int randomNumber = rand() % (100 - 1 + 1) + 1;
					if (randomNumber <= (*it).y)
					{
						iPoint enemyPos = App->map->GetRandomValidPoint();
						App->entities->AddEnemy({ (float)enemyPos.x * 46 ,(float)enemyPos.y * 46 }, ENEMY_TYPE::ARCHER_TIER_1);
						numberArchers++;
						if (numberArchers >= (*it).x)
							continue;
					}

					randomNumber = rand() % (100 - 1 + 1) + 1;
					if (randomNumber <= (*it).w)
					{
						iPoint enemyPos = App->map->GetRandomValidPoint();
						App->entities->AddEnemy({ (float)enemyPos.x * 46 ,(float)enemyPos.y * 46 }, ENEMY_TYPE::ARCHER_TIER_2);
						numberArchers++;
						if (numberArchers >= (*it).x)
							continue;
					}

					randomNumber = rand() % (100 - 1 + 1) + 1;
					if (randomNumber <= (*it).h)
					{
						iPoint enemyPos = App->map->GetRandomValidPoint();
						App->entities->AddEnemy({ (float)enemyPos.x * 46 ,(float)enemyPos.y * 46 }, ENEMY_TYPE::ARCHER_TIER_3);
						numberArchers++;
						if (numberArchers >= (*it).x)
							continue;
					}
				}
				while (numberArchers < (*it).x);

				App->items->Activate();
				if (!App->items->isPoolEmpty())
					lvlChest = App->entities->AddChest({ (float)App->map->chestNode->pos.x * 46 + 5, (float)App->map->chestNode->pos.y * 46 }, MID_CHEST);
				else
					lvlChest = nullptr;
			}

			break;
		}
	}

	return true;
}

bool Scene::PreUpdate()
{
	return true;
}

bool Scene::Update(float dt)
{
	bool ret = true;

	if (App->introVideo->isVideoFinished && actual_scene == Stages::INTRO_VIDEO)
	{
		restart = true;
		next_scene = Stages::MAIN_MENU;
	}

	if (actual_scene == Stages::INGAME && lvlIndex < App->map->numberOfLevels && portal == nullptr && App->entities->enemiescount == 0)
	{
		GeneratePortal();
	}

	//TESTING SAVES
	if (App->input->GetKey(SDL_SCANCODE_1) == KEY_DOWN && !App->console->isWritting())
	{
		App->Save();
	}

	//TESTING LOAD
	if (App->input->GetKey(SDL_SCANCODE_2) == KEY_DOWN && !App->console->isWritting())
	{
		App->Load();
	}

	//GENERATE A NEW MAP
	if (App->input->GetKey(SDL_SCANCODE_G) == KEY_DOWN && actual_scene == Stages::INGAME && !App->console->isWritting())
	{
		restart = true;
	}

	if (App->input->GetKey(SDL_SCANCODE_Q) == KEY_DOWN && actual_scene == Stages::INGAME && !App->console->isWritting())
	{
		GoNextLevel();
	}

	if (App->input->GetKey(SDL_SCANCODE_F1) == KeyState::KEY_DOWN && actual_scene == Stages::INGAME && !App->console->isWritting())
	{
		lvlIndex = 100;
		restart = true;
	}

	if (actual_scene == Stages::MAIN_MENU && App->input->GetKey(SDL_SCANCODE_H) == KEY_DOWN && !App->console->isWritting()) // DELETE THIS AFTER VERTICAL
	{
		App->audio->PlayMusic(App->audio->InGameBSO.data(), 1);
		next_scene = Stages::INGAME;
		restart = true;
	}

	//PAUSE GAME
	if (actual_scene == Stages::INGAME)
	{
		if (App->input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN ||
			App->input->GetPadButtonDown(SDL_CONTROLLER_BUTTON_START) == KEY_DOWN)
		{
			if (!paused)
			{
				App->audio->PauseFX();
				paused = true;
				player->Walk(false);
				currentPercentAudio = App->audio->MusicVolumePercent;
				uint tmpAudio = (uint)(currentPercentAudio * 0.3f);
				if (tmpAudio == 0)
					tmpAudio = 1;
				App->audio->setMusicVolume(tmpAudio);
				CreatePauseMenu();

			}
			else if(!ItemSelection)
			{
				App->audio->ResumeFX();
				paused = false;
				player->Walk(true);
				// Decreasing audio when pause game
				App->audio->setMusicVolume(currentPercentAudio);
				App->gui->DestroyElem(PauseMenu);
			}
		}
	}

	return ret;
}

bool Scene::PostUpdate()
{
	bool ret = true;
	if (actual_scene == Stages::MAIN_MENU || actual_scene == Stages::SETTINGS)
	{
		SDL_Rect back = { 0,0,640,360 };
		//App->render->DrawQuad(back, 0, 205, 193, 255, true, false);
		App->render->DrawQuad(back, 64, 66, 159, 255, true, false);
	}

	if (App->path->printWalkables == true)
		App->path->PrintWalkableTiles();

	BROFILER_CATEGORY("SceneRestart", Profiler::Color::Chocolate);
	if (restart)
	{
		restart = false;

		if (next_scene == Stages::INGAME && actual_scene != Stages::MAIN_MENU)
		{
			App->transitions->StartTransition(this, this, 2.0f, fades::circular_fade);
		}
		else if ((actual_scene == Stages::MAIN_MENU && next_scene == Stages::INGAME) ||
				(actual_scene == Stages::INGAME && next_scene == Stages::MAIN_MENU))
		{
			App->transitions->StartTransition(this, this, 2.0f, fades::slider_fade);
		}

		if ((actual_scene == Stages::MAIN_MENU && next_scene == Stages::SETTINGS) ||
			(actual_scene == Stages::SETTINGS && next_scene == Stages::MAIN_MENU))
		{
			actual_scene = next_scene;
			this->DeActivate();
			this->Activate();
		}

		if (actual_scene == Stages::INTRO_VIDEO && next_scene == Stages::MAIN_MENU)
		{
			App->video->CloseAVI();
	
			App->introVideo->DeActivate();
			App->video->DeActivate();
			this->Start();
		}
	}

	return ret;
}

bool Scene::CleanUp()
{
	if (player)
	{
		playerStats = player->numStats;
		uint quantityToHeal = (playerStats.maxhp - playerStats.hp) * playerStats.hpRecover / 100;
		playerStats.hp = playerStats.hp + quantityToHeal > playerStats.maxhp ? playerStats.maxhp : playerStats.hp + quantityToHeal;
	}
		

	App->gui->DeActivate();
	App->map->DeActivate();
	App->entities->DeActivate();
	App->console->DeActivate();
	App->path->ClearMap();
	App->colliders->DeActivate();
	App->effects->DeActivate();
	App->projectiles->DeActivate();

	if (next_scene == Stages::MAIN_MENU)
	{
		App->items->DeActivate();
	}



	player = nullptr;
	lvlChest = nullptr;
	portal = nullptr;
	PauseMenu = nullptr;

	return true;
}

//-----------------------------------
bool Scene::OnUIEvent(GUIElem* UIelem, UIEvents _event)
{
	bool ret = true;
	switch (UIelem->type)
	{
		case GUIElem::GUIElemType::BUTTON:
		{
			Button* button = (Button*)UIelem;
			switch (_event)
			{
				case UIEvents::MOUSE_ENTER:
				{
					App->audio->HaltFX(App->audio->ButtonHovered);
					App->audio->PlayFx(App->audio->ButtonHovered);
					button->atlasRect = Button1MouseHover;
					break;
				}
				case UIEvents::MOUSE_RIGHT_UP:
				{
					button->atlasRect = Button1MouseHover;
					break;
				}
				case UIEvents::MOUSE_LEFT_CLICK:
				{
					App->audio->PlayFx(App->audio->ButtonClicked);
					button->atlasRect = Button1Pressed;
					button->MoveChilds({ 0.0f, 1.0f });
					break;
				}
				case UIEvents::MOUSE_LEAVE:
				case UIEvents::NO_EVENT:
				{
					button->atlasRect = Button1;
					break;
				}
				case UIEvents::MOUSE_LEFT_UP:
				{
					button->atlasRect = Button1MouseHover;
					button->MoveChilds({ 0.0f, -1.0f });
					switch (button->btype)
					{
						case BType::PLAY:
							if (!App->transitions->IsFading())
							{
								playerStats = EntitySystem::PlayerStats();
								App->audio->PlayMusic(App->audio->InGameBSO.data(), 1);
								next_scene = Stages::INGAME;
								restart = true;
							}
							break;
						case BType::EXIT_GAME:
							if (!App->transitions->IsFading())
							{
								return false;
							}
							break;
						case BType::SETTINGS:
							if (!App->transitions->IsFading())
							{
								next_scene = Stages::SETTINGS;
								restart = true;
							}
							break;
						case BType::GO_MMENU:
							if (!App->transitions->IsFading())
							{
								if (actual_scene == Stages::INGAME)
								{
									App->audio->PlayMusic(App->audio->MainMenuBSO.data(), 0);
									App->audio->setMusicVolume(currentPercentAudio);
									App->audio->HaltFX();
								}
								next_scene = Stages::MAIN_MENU;
								lvlIndex = 0;
								paused = false;
								restart = true;
							}
							break;
						case BType::RESUME:
							if (!App->transitions->IsFading())
							{
								paused = false;
								player->Walk(true);
								App->audio->ResumeFX();
								App->gui->DestroyElem(PauseMenu);
								App->audio->setMusicVolume(currentPercentAudio);
							}
							break;
					}
					break;
				}
			}
			break;
		}

	}
	return ret;
}

void Scene::CreateMainMenuScreen()
{
	GUIWindow* window = (GUIWindow*)App->gui->CreateGUIWindow({ 0,0 }, { 0,0,0,0 }, nullptr, nullptr);

	//LOGO
	GUIImage* logo = (GUIImage*)App->gui->CreateGUIImage({ 100,25 }, { 624, 21, 448, 129 }, nullptr);

	//PLAY BUTTON
	Button* button = (Button*)App->gui->CreateButton({ 241.0f , 165}, BType::PLAY, this, window);

	LabelInfo defLabel;
	defLabel.color = White;
	defLabel.fontName = "LifeCraft80";
	defLabel.text = "Start";
	App->gui->CreateLabel({ 53,11 }, defLabel, button, this);

	//SETTINGS BUTTON
	Button* button2 = (Button*)App->gui->CreateButton({ 241.0f , 215 }, BType::SETTINGS, this, window);
	LabelInfo defLabel2;
	defLabel2.color = White;
	defLabel2.fontName = "LifeCraft80";
	defLabel2.text = "Settings";
	App->gui->CreateLabel({ 42,10 }, defLabel2, button2, this);

	//EXIT GAME BUTTON
	Button* button3 = (Button*)App->gui->CreateButton({ 241.0f , 265 }, BType::EXIT_GAME, this, window);
	LabelInfo defLabel3;
	defLabel3.color = White;
	defLabel3.fontName = "LifeCraft80";
	defLabel3.text = "Quit";
	App->gui->CreateLabel({ 60,10 }, defLabel3, button3, this);

	//VERSION LABEL
	LabelInfo versionLabel;
	versionLabel.color = White;
	versionLabel.fontName = "Arial30";
	versionLabel.text = App->gui->getVersion();
	App->gui->CreateLabel({ 10,340 }, versionLabel);

}

void Scene::CreateSettingsScreen()
{
	GUIWindow* window = (GUIWindow*)App->gui->CreateGUIWindow({ 0,0 }, { 0,0,0,0 }, nullptr, nullptr);

	//MUSIC VOLUME SLIDER
	SliderInfo sinfo;
	sinfo.type = Slider::SliderType::MUSIC_VOLUME;
	Slider* slider = (Slider*)App->gui->CreateSlider({ 183, 95 }, sinfo, this, window);

	LabelInfo defLabel3;
	defLabel3.color = White;
	defLabel3.fontName = "Arial80";
	std::string temp = (char*)std::to_string(App->audio->MusicVolumePercent).data();
	defLabel3.text = (char*)temp.data();
	App->gui->CreateLabel({ 265,-4 }, defLabel3, slider, this);

	LabelInfo defLabel;
	defLabel.color = Black;
	defLabel.fontName = "LifeCraft90";
	defLabel.text = "Music Volume";
	App->gui->CreateLabel({ 0,-35 }, defLabel, slider, this);

	//FX VOLUME SLIDER
	SliderInfo sinfo2;
	sinfo2.type = Slider::SliderType::FX_VOLUME;
	Slider* slider2 = (Slider*)App->gui->CreateSlider({ 183, 190 }, sinfo2, this, window);

	LabelInfo defLabel4;
	defLabel4.color = White;
	defLabel4.fontName = "Arial80";
	std::string temp2 = (char*)std::to_string(App->audio->FXVolumePercent).data();
	defLabel4.text = (char*)temp2.data();
	App->gui->CreateLabel({ 265,-4 }, defLabel4, slider2, this);

	LabelInfo defLabel5;
	defLabel5.color = Black;
	defLabel5.fontName = "LifeCraft90";
	defLabel5.text = "FX Volume";
	App->gui->CreateLabel({ 0,-35 }, defLabel5, slider2, this);

	//BACK BUTTON
	Button* button3 = (Button*)App->gui->CreateButton({ 640 / 2 - 158 / 2, 250.0f }, BType::GO_MMENU, this, window);

	LabelInfo defLabel2;
	defLabel2.color = White;
	defLabel2.fontName = "LifeCraft80";
	defLabel2.text = "Back";
	App->gui->CreateLabel({ 56,11 }, defLabel2, button3, this);
}

void Scene::CreatePauseMenu()
{
	fPoint localPos = { 640 / 2 - 249 / 2, 360 / 2 - 286 / 2 };
	PauseMenu = (GUIWindow*)App->gui->CreateGUIWindow(localPos, WoodWindow, this);
	PauseMenu->blackBackground = true;

	Button* Resume = (Button*)App->gui->CreateButton({ 249 / 2 - 158 / 2, 40 }, BType::RESUME, this, PauseMenu);

	LabelInfo defLabel1;
	defLabel1.color = White;
	defLabel1.fontName = "LifeCraft80";
	defLabel1.text = "Resume";
	App->gui->CreateLabel({ 46,10 }, defLabel1, Resume, this);

	Button* MainMenu = (Button*)App->gui->CreateButton({ 249 / 2 - 158 / 2, 120 }, BType::GO_MMENU, this, PauseMenu);

	LabelInfo defLabel2;
	defLabel2.color = White;
	defLabel2.fontName = "LifeCraft46";
	defLabel2.text = "Return to the Main Menu";
	App->gui->CreateLabel({ 18,15 }, defLabel2, MainMenu, this);

	Button* SaveAndExit = (Button*)App->gui->CreateButton({ 249 / 2 - 158 / 2, 200 }, BType::EXIT_GAME, this, PauseMenu);

	LabelInfo defLabel3;
	defLabel3.color = White;
	defLabel3.fontName = "LifeCraft80";
	defLabel3.text = "Save and Exit";
	App->gui->CreateLabel({ 23,10 }, defLabel3, SaveAndExit, this);
}

void Scene::AddCommands()
{
	ConsoleOrder* order = new ConsoleMap();
	App->console->AddConsoleOrderToList(order);
}

void Scene::GeneratePortal()
{
	if (portal == nullptr && App->entities->spritesheetsEntities.size() > 0)
	{
		iPoint position = App->map->GetRandomValidPointProxyForThisPos(5, 2, { (int)player->pos.x, (int)player->pos.y });
		portal = (PortalEntity*)App->entities->AddStaticEntity({ (float)position.x * 46, (float)position.y * 46 }, PORTAL);
	}
}

void Scene::GoMainMenu()
{
	if (actual_scene == Stages::INGAME)
		App->audio->PauseFX(App->audio->GuldanFireSecondPhase); // I DON'T KNOW IF THIS IS GOOD PLS REVISE
		App->audio->PlayMusic(App->audio->MainMenuBSO.data(), 0.5f);
	next_scene = Stages::MAIN_MENU;
	restart = true;
	lvlIndex = 0;
}

void Scene::CreateGratitudeScreen()
{
	static bool alreadyCreated = false;

	if(!alreadyCreated)
	{
		GUIWindow* window = (GUIWindow*)App->gui->CreateGUIWindow({ 0,0 }, { 0,0,0,0 }, nullptr, nullptr);
		window->blackBackground = true;
		gratitudeON = true;
		LabelInfo gratitude;
		gratitude.color = White;
		gratitude.fontName = "LifeCraft90";
		gratitude.multilabelWidth = 1500;
		gratitude.text = "                         Victory! \n       Thanks for playing the game. \n       Your support means a lot ^^ \n       More at: @SoftCactus_Team";
		App->gui->CreateLabel({ 150, 130 }, gratitude, window, nullptr);
	}
	alreadyCreated = true;
}

void Scene::CreateItemSelectionScreen(Item* item1, Item* item2, Item* item3)
{
	paused = true;
	ItemSelection = (GUIWindow*)App->gui->CreateGUIWindow({ 0,0 }, { 0,0,0,0 }, nullptr, nullptr);
	ItemSelection->blackBackground = true;
	ItemSelection->vertical = false;

	App->gui->CreateItemContainer({ 30+85,50+121 }, item1, ItemSelection, this);
	App->gui->CreateItemContainer({ 230+85,50+121 }, item2, ItemSelection, this);
	App->gui->CreateItemContainer({ 430+85,50+121 }, item3, ItemSelection, this);
	//App->gui->CreateItemContainer({})
}

void Scene::GoNextLevel()
{
	lvlIndex++;
	restart = true;
}
