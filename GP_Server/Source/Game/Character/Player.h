#pragma once
#include "Character.h"
#include "Inventory.h"
#include "WorldItem.h"

class Player : public Character
{
public:
	Player(int32 id) :Character(id) { Init(_channelId); }
	void Init(EWorldChannel channelId) override;
	void LoadFromDB(const DBLoginResult& dbRes);
	void SaveStatsToDB();
	void SetCharacterType(Type::EPlayer type);
	void OnEnterGame();

	void OnDamaged(float damage) override;
	void UpdateViewList(std::shared_ptr<Character> other) override;
	void AddMonsterToViewList(std::shared_ptr<Character> monster);
	void AddPlayerToViewList(std::shared_ptr<Character> player);
	void RemoveMonsterFromViewList(std::shared_ptr<Character> monster);
	void RemovePlayerFromViewList(std::shared_ptr<Character> player);

	bool AddInventoryItem(std::shared_ptr<Item> item);
	bool LoadInventoryItem(std::shared_ptr<Item> item);
	bool BuyItem(std::shared_ptr<Item> item, uint32 price, uint16 quantity);
	bool SellItem(uint32 itemId);

	bool TakeWorldItem(const std::shared_ptr<WorldItem> item);
	bool Attack(std::shared_ptr<Character> target);

	void UseSkill(ESkillGroup groupId);
	void EndSkill();
	void ExecuteSkillEffect(const FSkillTableData& skill);
	void ResetSkillEffect();
	void LearnSkill(ESkillGroup groupId);
	void UpgradeSkill(ESkillGroup groupId);
	void UnlockSkillsOnLevelUp();

	void OnEquipWeapon();
	void OnUnequipWeapon();
	void UseItem(uint32 itemId);
	uint8 EquipItem(uint32 itemId);
	uint8 UnequipItem(uint32 itemId);

	void CheckAndUpdateQuestProgress(EQuestCategory type);

	bool GiveQuestReward(QuestType quest);
	bool SetCurrentQuest(QuestType quest);
	bool RejectTutorialQuest();
	bool IsQuestInProgress(QuestType quest) const;
	bool StartQuest(QuestType newQuest);
	bool CompleteCurrentQuest();

	QuestType GetCurrentQuest() { return _curQuest.QuestType; }
	const QuestData* GetCurrentQuestData() { return _curQuestData; }

	void AddItemStats(const ItemStats& stats);
	void RemoveItemStats(const ItemStats& stats);

	void AddExp(float amount);
	void LevelUp();
	void ApplyLevelStats(uint32 level);

	void AddGold(int amount) { _gold += amount; }
	bool SpendGold(int amount)
	{
		if (_gold < amount) return false;
		_gold -= amount;
		return true;
	}
	int GetGold() const { return _gold; }
	float GetAttackDamage() override
	{
		return _info.GetAttackDamage(RandomUtils::GetRandomFloat(0.0f, 1.0f));
	}
	void ChangeState(ECharacterStateType newState) override
	{
		if (!_info.HasState(newState))
		{
			_info.AddState(newState);
		}
	}
	bool RemoveState(ECharacterStateType oldState)
	{
		if (_info.HasState(oldState))
		{
			_info.RemoveState(oldState);
			return true;
		}
		return false;
	}
	FStatData& GetStats() const { return _stats; }
	bool IsInTutorialQuest() { return _bTutQuest; }
	bool HasKey() const { return _inventory.HasKey(); }
	std::vector<uint8> GetUnsavedItemTypes() const { return _inventory.GetUnsavedItemTypes(); }
	std::shared_ptr<InventoryItem> GetInventoryItemPtr(uint32 itemId) { return _inventory.GetInventoryItemPtr(itemId); }
private:
	Type::EPlayer _playerType;
	Inventory _inventory;
	FStatData& _stats = _info.Stats;
	uint32& _gold = _info.Gold;
	QuestStatus& _curQuest = _info.CurrentQuest;
	const QuestData* _curQuestData = nullptr;
	bool _bTutQuest = false;

	float prevDmg = 0;
	float prevAtkR = 0;
	float prevFov = 0;
};
