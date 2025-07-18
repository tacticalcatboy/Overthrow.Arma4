class OVT_Global : Managed
{	
	static OVT_PlayerCommsComponent GetServer()
	{		
		if(Replication.IsServer())
		{
			return OVT_PlayerCommsComponent.Cast(GetOverthrow().FindComponent(OVT_PlayerCommsComponent));
		}		
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		return OVT_PlayerCommsComponent.Cast(player.FindComponent(OVT_PlayerCommsComponent));
	}

	static OVT_UIManagerComponent GetUI()
	{	
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		return OVT_UIManagerComponent.Cast(player.FindComponent(OVT_UIManagerComponent));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get the local player's overthrow controller entity
	//! \return Controller entity or null if not found/on server
	static OVT_OverthrowController GetController()
	{		
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		if (!player) return null;
		
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(player);
		return GetPlayers().GetController(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Convenience method to get container transfer component
	//! \return Container transfer component or null
	static OVT_ContainerTransferComponent GetContainerTransfer()
	{
		OVT_OverthrowController controller = GetController();
		if (!controller) return null;
		
		return OVT_ContainerTransferComponent.Cast(controller.FindComponent(OVT_ContainerTransferComponent));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Convenience method for battlefield looting
	//! \param[in] vehicle Target vehicle to loot into
	//! \param[in] searchRadius Search radius for lootable items
	static void LootBattlefield(IEntity vehicle, float searchRadius = 25.0)
	{
		OVT_ContainerTransferComponent transfer = GetContainerTransfer();
		if (transfer && transfer.IsAvailable())
		{
			transfer.LootBattlefield(vehicle, searchRadius);
		}
	}
	
	static OVT_OverthrowGameMode GetOverthrow()
	{
		return OVT_OverthrowGameMode.Cast(GetGame().GetGameMode());
	}
	
	static OVT_OverthrowConfigComponent GetConfig()
	{
		return OVT_OverthrowConfigComponent.GetInstance();
	}
	
	static OVT_DifficultySettings GetDifficulty()
	{
		return GetConfig().m_Difficulty;
	}
	
	static OVT_EconomyManagerComponent GetEconomy()
	{
		return OVT_EconomyManagerComponent.GetInstance();
	}
	
	static OVT_PlayerManagerComponent GetPlayers()
	{
		return OVT_PlayerManagerComponent.GetInstance();
	}
	
	static OVT_RealEstateManagerComponent GetRealEstate()
	{
		return OVT_RealEstateManagerComponent.GetInstance();
	}
	
	static OVT_VehicleManagerComponent GetVehicles()
	{
		return OVT_VehicleManagerComponent.GetInstance();
	}
	
	static OVT_TownManagerComponent GetTowns()
	{
		return OVT_TownManagerComponent.GetInstance();
	}
	
	static OVT_OccupyingFactionManager GetOccupyingFaction()
	{
		return OVT_OccupyingFactionManager.GetInstance();
	}
	
	static OVT_ResistanceFactionManager GetResistanceFaction()
	{
		return OVT_ResistanceFactionManager.GetInstance();
	}
	
	static OVT_JobManagerComponent GetJobs()
	{
		return OVT_JobManagerComponent.GetInstance();
	}
	
	static OVT_NotificationManagerComponent GetNotify()
	{
		return OVT_NotificationManagerComponent.GetInstance();
	}
	
	static OVT_OverthrowFactionManager GetFactions()
	{
		return OVT_OverthrowFactionManager.Cast(GetGame().GetFactionManager());
	}
	
	static OVT_SkillManagerComponent GetSkills()
	{
		return OVT_SkillManagerComponent.GetInstance();
	}
	
	static OVT_InventoryManagerComponent GetInventory()
	{
		return OVT_InventoryManagerComponent.GetInstance();
	}
	
	static OVT_DeploymentManagerComponent GetDeploymentManager()
	{
		return OVT_DeploymentManagerComponent.GetInstance();
	}
	
	static OVT_RecruitManagerComponent GetRecruits()
	{
		return OVT_RecruitManagerComponent.GetInstance();
	}
	
	static OVT_LoadoutManagerComponent GetLoadouts()
	{
		return OVT_LoadoutManagerComponent.GetInstance();
	}
	
	static bool PlayerInRange(vector pos, int range)
	{		
		array<int> players = new array<int>;
		PlayerManager mgr = GetGame().GetPlayerManager();
		int numplayers = mgr.GetPlayers(players);
		
		if(numplayers > 0)
		{
			IEntity player;
			DamageManagerComponent dmg;
			foreach(int playerID : players)
			{
				player = mgr.GetPlayerControlledEntity(playerID);				
				if(!player) continue;
				dmg = DamageManagerComponent.Cast(player.FindComponent(DamageManagerComponent));
				if(dmg && dmg.IsDestroyed())
				{
					//Is dead, ignore
					continue;
				}
				float distance = vector.Distance(player.GetOrigin(), pos);
				if(distance < range)
				{
					return true;
				}
			}
		}
		
		return false;
	}
	
	static int NearestPlayer(vector pos)
	{		
		array<int> players = new array<int>;
		PlayerManager mgr = GetGame().GetPlayerManager();
		int numplayers = mgr.GetPlayers(players);
		
		float nearestDist = 9999999;
		int nearest = -1;
		
		if(numplayers > 0)
		{
			IEntity player;
			foreach(int playerID : players)
			{
				player = mgr.GetPlayerControlledEntity(playerID);
				if(!player) continue;
				float distance = vector.Distance(player.GetOrigin(), pos);
				if(distance < nearestDist)
				{
					nearestDist = distance;
					nearest = playerID;
				}
			}
		}
		
		return nearest;
	}
	
	static vector FindSafeSpawnPosition(vector pos, vector mins = "-0.5 0 -0.5", vector maxs = "0.5 2 0.5")
	{
		//a crude and brute-force way to find a spawn position, try to improve this later
		vector foundpos = pos;
		int i = 0;
		
		BaseWorld world = GetGame().GetWorld();
		float ground = world.GetSurfaceY(pos[0],pos[2]);
		
		vector checkpos;
		TraceBox trace;
		while(i < 30)
		{
			i++;
			
			//Get a random vector in a 2m radius sphere centered on pos and above the ground
			checkpos = s_AIRandomGenerator.GenerateRandomPointInRadius(0,2,pos,false);
			checkpos[1] = pos[1] + s_AIRandomGenerator.RandFloatXY(0, 2);
						
			//check if a box on that position collides with anything
			trace = new TraceBox;
			trace.Flags = TraceFlags.ENTS;
			trace.Start = checkpos;
			trace.Mins = mins;
			trace.Maxs = maxs;
			
			float result = world.TracePosition(trace, null);
				
			if (result < 0)
			{
				//collision, try again
				continue;
			}else{
				//no collision, this pos is safe
				foundpos = checkpos;
				break;
			}
		}
		
		return foundpos;
	}
	
	static void TransferToWarehouse(RplId from)
	{
		OVT_RealEstateManagerComponent realEstate = GetRealEstate();
		IEntity fromEntity = RplComponent.Cast(Replication.FindItem(from)).GetEntity();
		
		if(!fromEntity) return;
		
		InventoryStorageManagerComponent fromStorage = InventoryStorageManagerComponent.Cast(fromEntity.FindComponent(InventoryStorageManagerComponent));		
				
		if(!fromStorage) return;
		
		OVT_WarehouseData warehouse = realEstate.GetNearestWarehouse(fromEntity.GetOrigin(), 50);
		if(!warehouse) return;
						
		array<IEntity> items = new array<IEntity>;
		fromStorage.GetItems(items);
		if(items.Count() == 0) return;
		
		map<ResourceName,int> collated = new map<ResourceName,int>;
				
		foreach(IEntity item : items)
		{
			if(!item) continue;			
			ResourceName res = EPF_Utils.GetPrefabName(item);
			if(fromStorage.TryDeleteItem(item))
			{			
				if(!collated.Contains(res)) collated[res] = 0;
				collated[res] = collated[res] + 1;
			}
		}
		
		for(int i=0; i<collated.Count(); i++)
		{
			ResourceName res = collated.GetKey(i);
			realEstate.AddToWarehouse(warehouse, res, collated[res]);
		}
		
		// Play sound if one is defined
		SimpleSoundComponent simpleSoundComp = SimpleSoundComponent.Cast(fromEntity.FindComponent(SimpleSoundComponent));
		if (simpleSoundComp)
		{
			vector mat[4];
			fromEntity.GetWorldTransform(mat);
			
			simpleSoundComp.SetTransformation(mat);
			simpleSoundComp.PlayStr("LOAD_VEHICLE");
		}	
	}
	
	static void TakeFromWarehouseToVehicle(int warehouseId, string id, int qty, RplId from)
	{
		IEntity fromEntity = RplComponent.Cast(Replication.FindItem(from)).GetEntity();		
		if(!fromEntity) return;
		
		InventoryStorageManagerComponent fromStorage = InventoryStorageManagerComponent.Cast(fromEntity.FindComponent(InventoryStorageManagerComponent));				
		if(!fromStorage) return;
		
		OVT_RealEstateManagerComponent re = OVT_Global.GetRealEstate();
		
		OVT_WarehouseData warehouse = re.m_aWarehouses[warehouseId];
		
		if(warehouse.inventory[id] < qty) qty = warehouse.inventory[id];
		int actual = 0;
		
		for(int i = 0; i < qty; i++)
		{
			if(fromStorage.TrySpawnPrefabToStorage(id))
			{
				actual++;
			}
		}
		
		re.TakeFromWarehouse(warehouse, id, actual);
	}
	
	static IEntity SpawnEntityPrefab(ResourceName prefab, vector origin, vector orientation = "0 0 0", bool global = true)
	{
		EntitySpawnParams spawnParams();

		spawnParams.TransformMode = ETransformMode.WORLD;

		Math3D.AnglesToMatrix(orientation, spawnParams.Transform);
		spawnParams.Transform[3] = origin;

		if (!global) return GetGame().SpawnEntityPrefabLocal(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);

		return GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);
	}
	
	static IEntity SpawnEntityPrefabMatrix(ResourceName prefab, vector mat[4], bool global = true)
	{
		EntitySpawnParams spawnParams();

		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform = mat;

		if (!global) return GetGame().SpawnEntityPrefabLocal(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);

		return GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);
	}
	
	//! Spawn a character entity directly without creating a group
	static SCR_ChimeraCharacter SpawnCharacterEntity(ResourceName prefab, vector origin, vector orientation = "0 0 0")
	{
		EntitySpawnParams spawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		
		Math3D.AnglesToMatrix(orientation, spawnParams.Transform);
		spawnParams.Transform[3] = origin;
		
		// Load the prefab resource
		Resource resource = Resource.Load(prefab);
		if (!resource)
		{
			Print("[Overthrow] Error: Could not load prefab resource: " + prefab);
			return null;
		}
		
		// Spawn the entity directly
		IEntity spawnedEntity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
		if (!spawnedEntity)
		{
			Print("[Overthrow] Error: Failed to spawn entity from prefab: " + prefab);
			return null;
		}
		
		// Cast to character and return
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(spawnedEntity);
		if (!character)
		{
			Print("[Overthrow] Error: Spawned entity is not a SCR_ChimeraCharacter: " + spawnedEntity);
			// Clean up the spawned entity since it's not what we expected
			SCR_EntityHelper.DeleteEntityAndChildren(spawnedEntity);
			return null;
		}
		
		return character;
	}
	
	static bool IsOceanAtPosition(vector checkpos)
	{		
		World world = GetGame().GetWorld();
		return 1 > world.GetSurfaceY(checkpos[0],checkpos[2]);
		return world.GetOceanBaseHeight() > world.GetSurfaceY(checkpos[0],checkpos[2]);
	}
	
	static vector GetRandomNonOceanPositionNear(vector pos, float range)
	{
		int i = 0;
		vector checkpos;
		while(i < 30)
		{
			i++;			
			
			checkpos = s_AIRandomGenerator.GenerateRandomPointInRadius(0,range,pos,false);
			
			if(!OVT_Global.IsOceanAtPosition(checkpos))
			{	
				checkpos[1] = GetGame().GetWorld().GetSurfaceY(checkpos[0],checkpos[2]) + 1;
				return checkpos;
			}
		}
		
		return pos;
	}
	
	static bool GetNearbyBodiesAndWeapons(vector pos, int range, out array<IEntity> entities)
	{
		m_Bodies = new array<IEntity>;
		GetGame().GetWorld().QueryEntitiesBySphere(pos, range, FilterDeadBodiesAndWeapons);
		entities.InsertAll(m_Bodies);
		
		return true;
	}
	
	protected static ref array<IEntity> m_Bodies;
	
	protected static bool FilterDeadBodiesAndWeapons(IEntity ent)
	{		
		DamageManagerComponent dmg = EPF_Component<DamageManagerComponent>.Find(ent);
		if(dmg && dmg.IsDestroyed())
		{
			m_Bodies.Insert(ent);
			return true;
		}
		
		WeaponComponent weapon = EPF_Component<WeaponComponent>.Find(ent);		
		if(weapon) m_Bodies.Insert(ent);
				
		return true;
	}
	
	static SCR_EditableVehicleUIInfo GetVehicleUIInfo(ResourceName res)
	{
		Resource holder = BaseContainerTools.LoadContainer(res);
		if (holder)
		{
			IEntitySource ent = holder.GetResource().ToEntitySource();
			for(int t=0; t<ent.GetComponentCount(); t++)
			{
				IEntityComponentSource comp = ent.GetComponent(t);
				if(comp.GetClassName() == "SCR_EditableVehicleComponent")
				{
					SCR_EditableVehicleUIInfo info;
					comp.Get("m_UIInfo",info);
					return info;
				}
			}
		}
		return null;
	}
	
	static SCR_EditableEntityUIInfo GetEditableUIInfo(ResourceName res)
	{
		Resource holder = BaseContainerTools.LoadContainer(res);
		if (holder)
		{
			IEntitySource ent = holder.GetResource().ToEntitySource();
			for(int t=0; t<ent.GetComponentCount(); t++)
			{
				IEntityComponentSource comp = ent.GetComponent(t);
				if(comp.GetClassName() == "SCR_EditableVehicleComponent")
				{
					SCR_EditableEntityUIInfo info;
					comp.Get("m_UIInfo",info);
					return info;
				}
			}
		}
		return null;
	}
	
	static UIInfo GetItemUIInfo(ResourceName prefab)
	{
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(Resource.Load(prefab));
		if (entitySource)
		{
		    for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
		    {
		        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
		        if(componentSource.GetClassName().ToType().IsInherited(InventoryItemComponent))
		        {
		            BaseContainer attributesContainer = componentSource.GetObject("Attributes");
		            if (attributesContainer)
		            {
		                BaseContainer itemDisplayNameContainer = attributesContainer.GetObject("ItemDisplayName");
		                if (itemDisplayNameContainer)
		                {
		                    UIInfo resultInfo = UIInfo.Cast(BaseContainerTools.CreateInstanceFromContainer(itemDisplayNameContainer));
		                    return resultInfo;
		                }
		            }
		        }
		    }
		}
		return null;
	}
	
	//! call RandomizeCivilianClothes for whole group
	static void RandomizeCivilianGroupClothes(SCR_AIGroup aigroup)
	{
		array<AIAgent> civs = new array<AIAgent>;
		aigroup.GetAgents(civs);
		foreach(AIAgent agent : civs)
		{
			RandomizeCivilianClothes(agent);
		}
	}
	
	//! Randomize clothes for civilian. Accounts for config properties like SkipChance and PlayerOnly.
	static void RandomizeCivilianClothes(AIAgent agent)
	{
		IEntity civ = agent.GetControlledEntity();
		ApplyCivilianLoadout(civ);
	}
	
	//! Apply civilian loadout to any character entity
	static void ApplyCivilianLoadout(IEntity character)
	{
		InventoryStorageManagerComponent storageManager = EPF_Component<InventoryStorageManagerComponent>.Find(character);
		if (!storageManager) 
			return;
		foreach (OVT_LoadoutSlot loadoutItem : OVT_Global.GetConfig().m_CivilianLoadout.m_aSlots)
		{
			if (loadoutItem.m_bPlayerOnly) continue;
			
			if (loadoutItem.m_fSkipChance > 0)
			{
				float rnd = s_AIRandomGenerator.RandFloat01();
				if(rnd <= loadoutItem.m_fSkipChance) continue; 
			}
			
			IEntity slotEntity = OVT_Global.SpawnDefaultCharacterItem(storageManager, loadoutItem);
			if (!slotEntity) continue;
			
			array<BaseInventoryStorageComponent> storages = new array<BaseInventoryStorageComponent>;
			storageManager.GetStorages(storages, EStoragePurpose.PURPOSE_LOADOUT_PROXY);
			
			BaseInventoryStorageComponent loadoutStorage;
			int suitableSlotId = -1;
			if (!storages.IsEmpty()) {
				loadoutStorage = storages[0];
				InventoryStorageSlot suitableSlot = loadoutStorage.FindSuitableSlotForItem(slotEntity);
				if (suitableSlot) {
					suitableSlotId = suitableSlot.GetID();
				}
			}
			
			if (!loadoutStorage || suitableSlotId == -1 || !storageManager.TryReplaceItem(slotEntity, loadoutStorage, suitableSlotId))
			{
				SCR_EntityHelper.DeleteEntityAndChildren(slotEntity);
			}
		}
	}
	
	static IEntity SpawnDefaultCharacterItem(InventoryStorageManagerComponent storageManager, OVT_LoadoutSlot loadoutItem)
	{
		int selection = s_AIRandomGenerator.RandInt(0, loadoutItem.m_aChoices.Count() - 1);
		ResourceName prefab = loadoutItem.m_aChoices[selection];
		
		EntitySpawnParams spawnParams();
		spawnParams.Transform[3] = storageManager.GetOwner().GetOrigin();
		
		IEntity slotEntity = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);
		if (!slotEntity) return null;
		
		return slotEntity;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Find the nearest road position to a given location
	//! @param center Starting position to search from
	//! @param searchRadius Maximum distance to search for roads
	//! @return Position on nearest road, or original position if no road found
	static vector FindNearestRoad(vector center)
	{
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
			return center;
			
		RoadNetworkManager roadManager = aiWorld.GetRoadNetworkManager();
		if (!roadManager)
			return center;
			
		BaseRoad foundRoad;
		float distance;
		int result = roadManager.GetClosestRoad(center, foundRoad, distance, false);
				
		if (result >= 0 && foundRoad && foundRoad.GetWidth() > 0)
		{
			// Try to get a reachable waypoint on the road
			vector roadPos;
			if (roadManager.GetReachableWaypointInRoad(center, center, 500, roadPos))
				return roadPos;
		}
		
		// If no road found within range, return original position
		return center;
	}
}