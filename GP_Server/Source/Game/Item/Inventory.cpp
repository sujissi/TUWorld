#include "pch.h"
#include "Inventory.h"

bool Inventory::LoadItem(const std::shared_ptr<Item>& item)
{
	return AddItem(item, true);
}

bool Inventory::AddItem(const std::shared_ptr<Item>& item, bool dbSaved)
{
	if (!item) return false;

	uint32 itemId = item->GetItemID();

	if (_items.contains(itemId))
	{
		LOG_W("AddItem failed - duplicate itemID: {}", itemId);
		return false;
	}

	auto inventoryItem = std::make_shared<InventoryItem>();
	inventoryItem->item = std::make_shared<Item>(*item);
	inventoryItem->saved = dbSaved;

	_items[itemId] = inventoryItem;
	if(inventoryItem->item->GetItemTypeID() == Type::EQuestItem::KEY)
		_bHasKey = true;

	return true;
}

bool Inventory::RemoveItem(uint32 itemId)
{
	auto it = _items.find(itemId);
	if (it == _items.end())
	{
		LOG_W("RemoveItem failed - itemId {} not found", itemId);
		return false;
	}

	_items.erase(it);
	return true;
}

std::shared_ptr<Item> Inventory::FindItem(uint32 itemId)
{
	auto it = _items.find(itemId);
	return (it != _items.end()) ? it->second->item : nullptr;
}

std::shared_ptr<InventoryItem> Inventory::GetInventoryItemPtr(uint32 itemId)
{
	auto it = _items.find(itemId);
	return (it != _items.end()) ? it->second : nullptr;
}


std::vector<uint8> Inventory::GetUnsavedItemTypes() const
{
	std::vector<uint8> result;
	for (const auto& [id, invItem] : _items)
	{
		if (!invItem->saved.load())
			result.push_back(invItem->item->GetItemTypeID());
	}
	return result;
}

