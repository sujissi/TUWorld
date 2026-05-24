#pragma once
#include "Item.h"

struct InventoryItem
{
    std::shared_ptr<Item> item;
    std::atomic<bool> saved{false};
};
class Inventory
{
public:
    bool LoadItem(const std::shared_ptr<Item>& item);
    bool AddItem(const std::shared_ptr<Item>& item, bool dbSaved = false);
    bool RemoveItem(uint32 itemId);
    std::shared_ptr<Item> FindItem(uint32 itemId);
    std::shared_ptr<InventoryItem> GetInventoryItemPtr(uint32 itemId);
    std::vector<uint8> GetUnsavedItemTypes() const;
    const std::unordered_map<int, std::shared_ptr<InventoryItem>>& GetItems() const { return _items; }
    bool HasKey() const { return _bHasKey; }
private:
    std::unordered_map<int, std::shared_ptr<InventoryItem>> _items;
    bool _bHasKey = false;
};
