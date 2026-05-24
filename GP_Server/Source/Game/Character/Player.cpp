#include "pch.h"
#include "Player.h"
#include "SessionManager.h"
#include "GameWorldManager.h"

void Player::Init(EWorldChannel channelId)
{
	Character::Init(channelId);
	SetCharacterType(Type::EPlayer::WARRIOR);
#ifndef DB_MODE
	_info.SetName(L"플레이어");
	SetCharacterType(Type::EPlayer::WARRIOR);

	_info.Stats.Level = 1;
	_info.Stats.Speed = 200.f;
	_info.CollisionRadius = playerCollision;
	_info.State = ECharacterStateType::STATE_IDLE;
	ApplyLevelStats(_info.Stats.Level);
	SetCurrentQuest(QuestType::CH1_TALK_TO_STUDENT_A);
	OnUnequipWeapon();
#endif
}

void Player::LoadFromDB(const DBLoginResult& dbRes)
{
	SetInfo(dbRes.info);
	for (auto& [itemID, itemTypeID] : dbRes.items)
	{
		LoadInventoryItem(std::make_shared<Item>(itemID, itemTypeID));
	}
}

void Player::SaveStatsToDB()
{
#ifdef DB_MODE
	auto session = SessionManager::GetInst().GetSession(_id);
	if (!session) return;
	uint32 dbId = session->GetDBID();
	FInfoData snapshot = _info;
	DBWorker::GetInst().Push([dbId, snapshot = std::move(snapshot)]() {
		DBManager::GetInst().UpdatePlayerStats(dbId, snapshot);
	});
#endif
}

void Player::SetCharacterType(Type::EPlayer type)
{
	_playerType = type;
	_info.CharacterType = static_cast<uint8>(_playerType);
	OnUnequipWeapon();
}

void Player::OnEnterGame()
{
	for (const auto& [id, invItem] : _inventory.GetItems())
	{
		if (invItem && invItem->item)
		{
			auto pkt = ItemPkt::AddInventoryPacket(invItem->item->GetItemID(), invItem->item->GetItemTypeID());
			SessionManager::GetInst().SendPacket(_id, &pkt);
		}
	}
	auto quest = _curQuest.QuestType;
	if (quest != QuestType::NONE)
	{
		SetCurrentQuest(GetInfo().CurrentQuest.QuestType);
	}
}

void Player::OnDamaged(float damage)
{
	Character::OnDamaged(damage);
	auto pkt = InfoPacket(EPacketType::S_DAMAGED_PLAYER, GetInfo());
	SessionManager::GetInst().SendPacket(_id, &pkt);
	auto world = _world;
	if (IsDead())
	{
		ChangeState(ECharacterStateType::STATE_DIE);
		auto playerID = _id;
		TimerQueue::AddTimer([playerID, world] { world->PlayerDead(playerID);}, 10, false);
		TimerQueue::AddTimer([playerID, world] { world->RespawnPlayer(playerID, ZoneType::TUK);}, 3000, false);
	}
}

void Player::UpdateViewList(std::shared_ptr<Character> other)
{
	if (!other || !other->IsValid())
	{
		LOG_D("Invalid character!");
		return;
	}
	if (GetZone() != other->GetZone())
	{
		LOG_W("Not Same Zone");
		return;
	}
	if (IsInViewDistance(other->GetInfo().Pos, VIEW_DIST))
	{
		if (other->IsMonster())
		{
			AddMonsterToViewList(other);
		}
		else
		{
			AddPlayerToViewList(other);
		}
	}
	else
	{
		if (other->IsMonster())
		{
			RemoveMonsterFromViewList(other);
		}
		else
		{
			RemovePlayerFromViewList(other);
		}
	}
}

void Player::AddMonsterToViewList(std::shared_ptr<Character> monster)
{
	auto monsterId = monster->GetInfo().ID;
	if (!AddToViewList(monsterId)) return;
	auto addPkt = InfoPacket(EPacketType::S_ADD_MONSTER, monster->GetInfo());
	SessionManager::GetInst().SendPacket(_id, &addPkt);
	monster->AddToViewList(_id);
}

void Player::RemoveMonsterFromViewList(std::shared_ptr<Character> monster)
{
	auto monsterId = monster->GetInfo().ID;
	if (!RemoveFromViewList(monsterId)) return;
	auto removePkt = IDPacket(EPacketType::S_REMOVE_MONSTER, monsterId);
	SessionManager::GetInst().SendPacket(_id, &removePkt);
	monster->RemoveFromViewList(_id);
}

void Player::AddPlayerToViewList(std::shared_ptr<Character> otherPlayer)
{
	auto otherPlayerId = otherPlayer->GetInfo().ID;
	if (!AddToViewList(otherPlayerId)) return;
	auto addPkt = InfoPacket(EPacketType::S_ADD_PLAYER, otherPlayer->GetInfo());
	SessionManager::GetInst().SendPacket(_id, &addPkt);

	if (otherPlayer->AddToViewList(_id))
	{
		auto addPkt = InfoPacket(EPacketType::S_ADD_PLAYER, _info);
		SessionManager::GetInst().SendPacket(otherPlayer->GetInfo().ID, &addPkt);
	}
}

void Player::RemovePlayerFromViewList(std::shared_ptr<Character> player)
{
	auto otherPlayerId = player->GetInfo().ID;
	if (!RemoveFromViewList(otherPlayerId)) return;
	auto removePkt = IDPacket(EPacketType::S_REMOVE_PLAYER, otherPlayerId);
	SessionManager::GetInst().SendPacket(_id, &removePkt);

	if (player->RemoveFromViewList(_id))
	{
		auto removePkt = IDPacket(EPacketType::S_REMOVE_PLAYER, _id);
		SessionManager::GetInst().SendPacket(player->GetInfo().ID, &removePkt);
	}
}

bool Player::AddInventoryItem(std::shared_ptr<Item> item)
{
	return _inventory.AddItem(item);
}

bool Player::LoadInventoryItem(std::shared_ptr<Item> item)
{
	return _inventory.LoadItem(item);
}

bool Player::BuyItem(std::shared_ptr<Item> item, uint32 price, uint16 quantity)
{
	uint32 totalPrice = price * quantity;

	if (!SpendGold(totalPrice))
	{
		LOG_D("Not enough gold");
		return false;
	}
	auto type = item->GetItemCategory();
	if (GetCurrentQuest() == QuestType::TUT_BUY_ITEM && type == EItemCategory::Weapon)
	{
		CheckAndUpdateQuestProgress(EQuestCategory::ITEM);
	}

	return AddInventoryItem(item);
}

bool Player::SellItem(uint32 itemId)
{
	bool bSuccess = false;
	ResultCode resultCode = ResultCode::SUCCESS;
	uint32 resellPrice = 0;

	auto item = _inventory.FindItem(itemId);
	if (!item)
	{
		LOG_W("Invalid item");
		resultCode = ResultCode::ITEM_NOT_FOUND;
	}
	else
	{
		auto itemType = item->GetItemTypeID();
		auto itemData = ItemTable::GetInst().GetItemByTypeId(itemType);

		if (!itemData)
		{
			LOG_W("Invalid item data");
			resultCode = ResultCode::ITEM_NOT_FOUND;
		}
		else if (!_inventory.RemoveItem(itemId))
		{
			LOG_W("Failed remove item");
			resultCode = ResultCode::ITEM_NOT_FOUND;
		}
		else
		{
			auto pkt = ItemPkt::RemoveInventoryPacket(itemId);
			SessionManager::GetInst().SendPacket(_id, &pkt);
			resellPrice = itemData->ResellPrice;
			AddGold(resellPrice);
			bSuccess = true;
		}
	}
	uint32 curgold = GetGold();
	auto respkt = SellItemResultPacket(bSuccess, resultCode, curgold);
	SessionManager::GetInst().SendPacket(_id, &respkt);
	return bSuccess;
}

bool Player::TakeWorldItem(const std::shared_ptr<WorldItem> item)
{
	float detectDist = 500.f;
	if (!IsCollision(item->GetPos(), detectDist))
		return false;
	if (GetCurrentQuest() == QuestType::CH1_FIND_KEY_ITEM)
	{
		auto type = item->GetItemCategory();
		if (type == EItemCategory::Quest)
		{
			CheckAndUpdateQuestProgress(EQuestCategory::ITEM);
		}
	}
	return  AddInventoryItem(item);
}

bool Player::Attack(std::shared_ptr<Character> target)
{
	auto monster = std::dynamic_pointer_cast<Monster>(target);
	if (!monster) return false;
	if (!IsInAttackRange(monster->GetInfo()))return false;

#if TEST
	float atkDamage = GetAttackDamage() * TEST_ATK_WEIGHT;
#else
	float atkDamage = GetAttackDamage();
#endif
	if (atkDamage > 0.0f)
	{
		monster->OnDamaged(atkDamage);
	}
	if (monster->IsDead())
	{
		ChangeState(ECharacterStateType::STATE_DIE);
	}
	auto pkt = MonsterDamagePacket(monster->GetInfo(), atkDamage);
	SessionManager::GetInst().SendPacket(_id, &pkt);
	SessionManager::GetInst().BroadcastToViewList(&pkt, _viewList);
	return true;
}

void Player::UseSkill(ESkillGroup groupId)
{
	auto curSkill = _info.GetSkillData(groupId);
	if (curSkill == nullptr || !curSkill->IsValid())
	{
		LOG_W("Invaild!");
		return;
	}

	uint32 level = curSkill->SkillLevel;
	const FSkillTableData* skilltable = PlayerSkillTable::GetInst().GetSkill(static_cast<uint32>(groupId), level);
	if (!skilltable)
	{
		LOG_W("Invaild!");
		return;
	}
	LOG_I("Use Skill - {}", static_cast<uint8>(groupId));
	if (groupId == ESkillGroup::HitHard || groupId == ESkillGroup::Throwing)
		_info.AddState(STATE_SKILL_Q);
	else if (groupId == ESkillGroup::Clash || groupId == ESkillGroup::FThrowing)
		_info.AddState(STATE_SKILL_E);
	else if (groupId == ESkillGroup::Whirlwind || groupId == ESkillGroup::Anger)
		_info.AddState(STATE_SKILL_R);

	ExecuteSkillEffect(*skilltable);
}

void Player::EndSkill()
{
	ResetSkillEffect();

	auto targetState = ECharacterStateType::STATE_NONE;
	if (GetInfo().HasState(ECharacterStateType::STATE_SKILL_Q))
		targetState = ECharacterStateType::STATE_SKILL_Q;
	else if (GetInfo().HasState(ECharacterStateType::STATE_SKILL_E))
		targetState = ECharacterStateType::STATE_SKILL_E;
	else if (GetInfo().HasState(ECharacterStateType::STATE_SKILL_R))
		targetState = ECharacterStateType::STATE_SKILL_R;

	GetInfo().RemoveState(targetState);
}

void Player::ExecuteSkillEffect(const FSkillTableData& skill)
{
	if (skill.Type0 == ESkillType::Atk)
	{
		prevDmg = _stats.Damage;
		prevAtkR = _info.AttackRadius;
		prevFov = _info.fovAngle;

		float percent = skill.Value0;
		_stats.Damage += static_cast<int>(_stats.Damage * (percent / 100.0f));


		switch (skill.Type1)
		{
		case ESkillType::Dash:
			_info.SetLocation(_info.Pos + _info.GetFrontVector() * skill.Value1);
			break;
		case ESkillType::RangeAtk:
			_info.AttackRadius = 100.f * skill.Value1;
			_info.fovAngle = 360;
			break;
		case ESkillType::SectorAtk:
			_info.fovAngle += skill.Value1;
			break;
		default:
			break;
		}
	}
	else if (skill.Type0 == ESkillType::BuffTime)
	{
		switch (skill.Type1)
		{
		case ESkillType::AtkSpd:// 공격속도 n% 증가 (클라에서 처리)
			break;
		}

	}

}

void Player::ResetSkillEffect()
{
	_stats.Damage = prevDmg;
	_info.AttackRadius = prevAtkR;
	_info.fovAngle = prevFov;
}

void Player::LearnSkill(ESkillGroup groupId)
{
	FSkillState& skills = _info.Skills;

	if (!skills.Q.IsValid())
	{
		skills.Q.SetSkill(groupId, 1);
		return;
	}
	if (!skills.E.IsValid())
	{
		skills.E.SetSkill(groupId, 1);
		return;
	}
	if (!skills.R.IsValid())
	{
		skills.R.SetSkill(groupId, 1);
		return;
	}
}

void Player::UpgradeSkill(ESkillGroup groupId)
{
	auto* skill = _info.GetSkillData(groupId);
	if (!skill)
	{
		LOG_W("Trying to upgrade unlearned skill");
		return;
	}

	skill->LevelUp();
}

void Player::UnlockSkillsOnLevelUp()
{
	if (_stats.Level < 2)
		return;

	int32 order = _stats.Level - 2;

	int32 baseGroup = 0;
	switch (_playerType)
	{
	case Type::EPlayer::WARRIOR:
		baseGroup = (int32)ESkillGroup::HitHard;
		break;
	case Type::EPlayer::GUNNER:
		baseGroup = (int32)ESkillGroup::Throwing;
		break;
	default:
		LOG_W("Unknown Player Type");
		return;
	}

	int32 skillGroup = baseGroup + (order % 3);
	int32 skillLevel = (order / 3) + 1;

	const FSkillTableData* skill = PlayerSkillTable::GetInst().GetSkill(skillGroup, skillLevel);
	if (skill == nullptr)
	{
		LOG_W("Invaild!");
		return;
	}
	ESkillGroup groupId = static_cast<ESkillGroup>(skill->SkillGroup);

	auto* curSkill = _info.GetSkillData(groupId);
	if (!curSkill->IsValid())
	{
		LearnSkill(groupId);
		auto pkt = SkillUnlockPacket(groupId);
		SessionManager::GetInst().SendPacket(_id, &pkt);
		LOG_D("Learned New Skill [{}] Level [{}]", static_cast<uint32>(groupId), skillLevel);
	}
	else
	{
		UpgradeSkill(groupId);
		auto pkt = UpgradeSkillPacket(groupId);
		SessionManager::GetInst().SendPacket(_id, &pkt);
		LOG_D("Upgraded Skill [{}] to Level [{}]", static_cast<uint32>(groupId), curSkill->SkillLevel);
	}
}


void Player::OnEquipWeapon()
{
	if (_playerType == Type::EPlayer::WARRIOR)
	{
		_info.AttackRadius = DfWarriorAtkRadius;
		_info.fovAngle = DFWarriorfovAngle;
	}
	else
	{
		_info.AttackRadius = DfGunnerAtkRadius;
		_info.fovAngle = DFGunnerfovAngle;
	}
}

void Player::OnUnequipWeapon()
{
	_info.AttackRadius = DfAtkRadius;
	_info.fovAngle = DFfovAngle;
}

void Player::UseItem(uint32 itemId)
{
	auto targetItem = _inventory.FindItem(itemId);
	if (!targetItem) return;

	const ItemStats& stats = targetItem->GetStats();
	const EAbilityType type = targetItem->GetAbilityType();
	const float value = targetItem->GetAbilityValue();

	switch (type)
	{
	case EAbilityType::Recove:
		_stats.Hp = std::min(_stats.Hp + value, _stats.MaxHp);
		break;

	case EAbilityType::AtcBuff:
		_stats.Damage += value;
		break;

	case EAbilityType::Gold:
		AddGold(value);
		break;

	default:
		break;
	}
	auto pkt = ItemPkt::ItemUsedPacket(itemId, _info);
	SessionManager::GetInst().SendPacket(_id, &pkt);
	_inventory.RemoveItem(itemId);

	if (GetCurrentQuest() == QuestType::TUT_USE_ITEM && type == EAbilityType::Recove)
	{
		CheckAndUpdateQuestProgress(EQuestCategory::ITEM);
	}

}

uint8 Player::EquipItem(uint32 itemId)
{
	auto targetItem = _inventory.FindItem(itemId);
	if (!targetItem) return 0;
	auto type = targetItem->GetItemCategory();
	if (type == EItemCategory::Weapon)
	{
		OnEquipWeapon();
	}

	const ItemStats& itemStats = targetItem->GetStats();
	AddItemStats(itemStats);

	uint8 itemType = targetItem->GetItemTypeID();
	_info.EquipItemByType(itemType);


	if (GetCurrentQuest() == QuestType::TUT_EQUIP_ITEM && type == EItemCategory::Weapon)
	{
		CheckAndUpdateQuestProgress(EQuestCategory::ITEM);
	}

	return itemType;
}

uint8 Player::UnequipItem(uint32 itemId)
{
	auto targetItem = _inventory.FindItem(itemId);
	if (!targetItem) return 0;

	const ItemStats& itemStats = targetItem->GetStats();
	RemoveItemStats(itemStats);

	uint8 itemType = targetItem->GetItemTypeID();
	_info.UnequipItemByType(itemType);

	auto type = targetItem->GetItemCategory();
	if (type == EItemCategory::Weapon)
	{
		OnUnequipWeapon();
	}
	return itemType;
}

void Player::CheckAndUpdateQuestProgress(EQuestCategory type)
{
	auto questData = GetCurrentQuestData();
	if (!questData) return;
	QuestType quest = GetCurrentQuest();
	if (!IsQuestInProgress(quest))
		return;

	if (type != questData->Catagory)
	{
		return;
	}

	bool res = false;
	switch (type)
	{
	case EQuestCategory::INTERACT:
		res = true;
		if (quest == QuestType::TUT_COMPLETE)
		{
			_bTutQuest = false;
		}
		break;
	case EQuestCategory::MOVE:
	{
		res = true;
		break;
	}
	case EQuestCategory::KILL:
	{
		if (quest == QuestType::CH1_BUNKER_CLEANUP
			|| quest == QuestType::TUT_KILL_ONE_MON
			|| quest == QuestType::CH2_KILL_DESKMON
			|| quest == QuestType::CH3_KILL_DRILL
			|| quest == QuestType::CH4_KILL_TINO)
		{
			res = true;
		}
		else if (quest == QuestType::CH2_CLEAR_E_BUILDING || quest == QuestType::CH3_CLEAR_SERVER_ROOM)
		{
			int32 mcnt = _world->GetMonsterCnt(GetZone());
			if (mcnt == 1)
			{
				res = true;
			}
		}

		break;
	}
	case EQuestCategory::ITEM:

		res = true;
		break;
	}

	if (res)
		GiveQuestReward(quest);
}


bool Player::GiveQuestReward(QuestType quest)
{
	const QuestData* questData = _curQuestData;
	if (!_curQuestData)
	{
		LOG_W("Invalid quest datatable");
		return false;
	}

	uint32 exp = 0, gold = 0;
	bool res = CompleteCurrentQuest();
	if (!res) return false;
	exp = questData->ExpReward;
	gold = questData->GoldReward;
	AddExp(exp);
	AddGold(gold);
	auto infopkt = InfoPacket(EPacketType::S_PLAYER_STATUS_UPDATE, GetInfo());
	SessionManager::GetInst().SendPacket(_id, &infopkt);

	if (questData->NextQuestID != QuestType::NONE)
	{
		SetCurrentQuest(questData->NextQuestID);
	}

	auto pkt = QuestRewardPacket(quest, res, exp, gold);
	SessionManager::GetInst().SendPacket(_id, &pkt);
	auto itemid = questData->RewarditemType;

	if (questData->RewarditemType != -1)
	{
		FVector forward = _info.GetFrontVector();
		float spawnDistance = 100.f;
		FVector itemPos = GetPos() + forward * spawnDistance;
		auto droppedItem = WorldItem(itemid, itemPos);
		_world->SpawnWorldItem(droppedItem, GetZone());
	}

	return true;
}

bool Player::SetCurrentQuest(QuestType quest)
{
	const QuestData* questData = QuestTable::GetInst().GetQuest(quest);
	if (!questData)
	{
		LOG_W("Invalid quest ID");
		return false;
	}

	if (_curQuestData != nullptr)
	{
		if (GetCurrentQuest() != QuestType::TUT_START && _curQuestData->NextQuestID != quest)
		{
			LOG_D("is not next quest");
			return false;
		}
	}
	StartQuest(quest);
	_curQuestData = questData;
	auto qpkt = QuestStartPacket(questData->QuestID);
	SessionManager::GetInst().SendPacket(_id, &qpkt);
	auto infopkt = InfoPacket(EPacketType::S_PLAYER_STATUS_UPDATE, GetInfo());
	SessionManager::GetInst().SendPacket(_id, &infopkt);

	if (questData->Catagory == EQuestCategory::KILL)
	{
		_world->QuestSpawn(_id, quest);
	}
	_bTutQuest = (static_cast<uint8>(quest) >= static_cast<uint8>(QuestType::TUT_START));

	return true;
}

bool Player::RejectTutorialQuest()
{
	QuestType newQuest = QuestType::CH1_TALK_TO_STUDENT_A;
	const QuestData* questData = QuestTable::GetInst().GetQuest(newQuest);
	if (!questData)
	{
		LOG_W("Invalid quest ID");
		return false;
	}

	_curQuest.QuestType = newQuest;
	_curQuest.Status = EQuestStatus::InProgress;
	_curQuestData = questData;

	auto qpkt = QuestStartPacket(questData->QuestID);
	SessionManager::GetInst().SendPacket(_id, &qpkt);
	auto infopkt = InfoPacket(EPacketType::S_PLAYER_STATUS_UPDATE, GetInfo());
	SessionManager::GetInst().SendPacket(_id, &infopkt);

	_bTutQuest = false;

	return true;
}

bool Player::IsQuestInProgress(QuestType quest) const
{
	return _curQuest.QuestType == quest && _curQuest.Status == EQuestStatus::InProgress;
}

bool Player::StartQuest(QuestType newQuest)
{
	if (_curQuest.Status == EQuestStatus::InProgress)
		return false;

	_curQuest.QuestType = newQuest;
	_curQuest.Status = EQuestStatus::InProgress;
	return true;
}

bool Player::CompleteCurrentQuest()
{
	if (_curQuest.QuestType != QuestType::NONE && _curQuest.Status == EQuestStatus::InProgress)
	{
		_curQuest.Status = EQuestStatus::Completed;
		return true;
	}
	return false;
}

void Player::AddItemStats(const ItemStats& stats)
{
	LOG_I(" [AddItemStats] Before Add - Damage: {}, MaxHp: {}, Hp: {}, CrtRate: {}, Dodge: {}, Speed: {}",
		_stats.Damage, _stats.MaxHp, _stats.Hp, _stats.CrtRate, _stats.Dodge, _stats.Speed);

	_stats.Damage += stats.damage;
	_stats.MaxHp += stats.hp;
	_stats.Hp += stats.hp;

	_stats.CrtRate = std::clamp(_stats.CrtRate + stats.critRate, 0.0f, 1.0f);
	_stats.Dodge = std::clamp(_stats.Dodge + stats.dodgeRate, 0.0f, 1.0f);
	_stats.Speed = std::max(0.0f, _stats.Speed + stats.moveSpeed);

	LOG_I(" [AddItemStats] After Add - Damage: {}, MaxHp: {}, Hp: {}, CrtRate: {}, Dodge: {}, Speed: {}",
		_stats.Damage, _stats.MaxHp, _stats.Hp, _stats.CrtRate, _stats.Dodge, _stats.Speed);
}

void Player::RemoveItemStats(const ItemStats& stats)
{
	LOG_I(" [RemoveItemStats] Before Remove - Damage: {}, MaxHp: {}, Hp: {}, CrtRate: {}, Dodge: {}, Speed: {}",
		_stats.Damage, _stats.MaxHp, _stats.Hp, _stats.CrtRate, _stats.Dodge, _stats.Speed);

	_stats.Damage = std::max(0.0f, _stats.Damage - stats.damage);
	_stats.Hp = std::max(0.0f, _stats.Hp - stats.hp);
	_stats.MaxHp = std::max(1.0f, _stats.MaxHp - stats.hp);

	_stats.CrtRate = std::clamp(_stats.CrtRate - stats.critRate, 0.0f, 1.0f);
	_stats.Dodge = std::clamp(_stats.Dodge - stats.dodgeRate, 0.0f, 1.0f);
	_stats.Speed = std::max(0.0f, _stats.Speed - stats.moveSpeed);

	LOG_I(" [RemoveItemStats] After Remove - Damage: {}, MaxHp: {}, Hp: {}, CrtRate: {}, Dodge: {}, Speed: {}",
		_stats.Damage, _stats.MaxHp, _stats.Hp, _stats.CrtRate, _stats.Dodge, _stats.Speed);
}

void Player::AddExp(float amount)
{
	_stats.Exp += amount;

	while (_stats.Exp >= _stats.MaxExp)
	{
		_stats.Exp -= _stats.MaxExp;
		LevelUp();
	}
}

void Player::LevelUp()
{
	_stats.Level++;
	ApplyLevelStats(_stats.Level);
	auto pkt = PlayerLevelUpPacket(_info);
	SessionManager::GetInst().SendPacket(_id, &pkt);

	UnlockSkillsOnLevelUp();
	SaveStatsToDB();
}

void Player::ApplyLevelStats(uint32 level)
{
	const FStatData* newStats = PlayerLevelTable::GetInst().GetStatByLevel(level);
	if (!newStats)
	{
		LOG_W("Invaild");
		return;
	}

	//for test

	_stats.Hp = _stats.MaxHp;
	_stats.Damage = newStats->Damage;
	_stats.CrtRate = newStats->CrtRate;
	_stats.CrtValue = newStats->CrtValue;
	_stats.Dodge = newStats->Dodge;
	_stats.MaxExp = newStats->MaxExp;
	_stats.MaxHp = newStats->MaxHp;
}
