#include "p2Defs.h"

enum ItemFunctions
{
	GetItem,
	UpdateLogic,
	ByeByeItem
};

enum class ItemType
{
	no_item_type = -1,
	active_item_type,
	passive_item_type
};

class Item
{
protected:
	char* name = nullptr;
	uint rarity = 0u;

public:

	Item();
	Item(char* name, ItemType type, uint rarity);
	~Item();

	bool GetItem();
	bool UpdateLogic();
	bool ByeByeItem();

};