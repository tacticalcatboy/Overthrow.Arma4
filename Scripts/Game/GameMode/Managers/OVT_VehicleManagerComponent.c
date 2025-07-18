class OVT_VehicleManagerComponentClass: OVT_RplOwnerManagerComponentClass
{
};

//------------------------------------------------------------------------------------------------
//! Callback class for vehicle upgrade transfers
class OVT_VehicleUpgradeCallback : OVT_StorageProgressCallback
{
	protected OVT_VehicleManagerComponent m_Component;
	
	void OVT_VehicleUpgradeCallback(OVT_VehicleManagerComponent component)
	{
		m_Component = component;
	}
	
	override void OnProgressUpdate(float progress, int currentItem, int totalItems, string operation)
	{
		// No progress tracking needed for vehicle upgrades
	}
	
	override void OnComplete(int itemsTransferred, int itemsSkipped)
	{
		if (m_Component)
			m_Component.OnUpgradeTransferComplete(itemsTransferred, itemsSkipped);
	}
	
	override void OnError(string errorMessage)
	{
		if (m_Component)
			m_Component.OnUpgradeTransferError(errorMessage);
	}
};

class OVT_VehicleManagerComponent: OVT_RplOwnerManagerComponent
{	

	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Players starting cars", params: "et", category: "Vehicles")]
	ref array<ResourceName> m_pStartingCarPrefabs;
	
	[Attribute()]
	ref SCR_EntityCatalogMultiList m_CivilianVehicleEntityCatalog;
		
	ref array<EntityID> m_aAllVehicleShops;	
	
	ref array<ref EntityID> m_aVehicles;
	
	OVT_RealEstateManagerComponent m_RealEstate;
		
	static OVT_VehicleManagerComponent s_Instance;	
	
	protected ref array<EntityID> m_aParkingSearch;
	
	// Vehicle upgrade tracking
	protected IEntity m_pUpgradeOldVehicle;
	protected ref OVT_VehicleUpgradeCallback m_UpgradeCallback;
	
	static OVT_VehicleManagerComponent GetInstance()
	{
		if (!s_Instance)
		{
			BaseGameMode pGameMode = GetGame().GetGameMode();
			if (pGameMode)
				s_Instance = OVT_VehicleManagerComponent.Cast(pGameMode.FindComponent(OVT_VehicleManagerComponent));
		}

		return s_Instance;
	}
	
	void OVT_VehicleManagerComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{		
		m_aAllVehicleShops = new array<EntityID>;		
		m_aVehicles = new array<ref EntityID>;
	}
	
	void Init(IEntity owner)
	{			
		m_RealEstate = OVT_Global.GetRealEstate();
	}
	
	void SpawnStartingCar(IEntity home, string playerId)
	{		
		vector mat[4];
		
		int i = s_AIRandomGenerator.RandInt(0, m_pStartingCarPrefabs.Count()-1);
		ResourceName prefab = m_pStartingCarPrefabs[i];
		
		//Find us a parking spot
		IEntity veh;
		
		if(GetParkingSpot(home, mat, OVT_ParkingType.PARKING_CAR, true))
		{
			
			veh = SpawnVehicleMatrix(prefab, mat, playerId);
			
		}else if(FindNearestKerbParking(home.GetOrigin(), 20, mat))
		{
			Print("Unable to find OVT_ParkingComponent in starting house prefab. Trying to spawn car next to a kerb.");
			veh = SpawnVehicleMatrix(prefab, mat, playerId);
			
		}else{
			Print("Failure to spawn player's starting car. Add OVT_ParkingComponent to all starting house prefabs in config");			
		}
		
		if(veh)
		{
			OVT_PlayerOwnerComponent playerowner = EPF_Component<OVT_PlayerOwnerComponent>.Find(veh);
			if(playerowner) playerowner.SetLocked(true);
		}
	}
	
	bool GetParkingSpot(IEntity building, out vector outMat[4], OVT_ParkingType type = OVT_ParkingType.PARKING_CAR, bool skipObstructionCheck = false)
	{
		OVT_ParkingComponent parking = OVT_ParkingComponent.Cast(building.FindComponent(OVT_ParkingComponent));
		if(!parking) return false;
		return parking.GetParkingSpot(outMat, type, skipObstructionCheck);
	}
	
	bool GetNearestParkingSpot(vector pos, out vector outMat[4], OVT_ParkingType type = OVT_ParkingType.PARKING_CAR)
	{
		m_aParkingSearch = new array<EntityID>();
		GetGame().GetWorld().QueryEntitiesBySphere(pos, 15, null, FilterParkingAddToArray, EQueryEntitiesFlags.ALL);
		
		if(m_aParkingSearch.Count() == 0) return false;
		
		return GetParkingSpot(GetGame().GetWorld().FindEntityByID(m_aParkingSearch[0]), outMat, type);
	}
	
	bool FindNearestKerbParking(vector pos, float range, out vector outMat[4])
	{
		m_aParkingSearch.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(pos, range, null, FilterKerbAddToArray, EQueryEntitiesFlags.STATIC);
		
		if(m_aParkingSearch.Count() == 0) return false;
		
		float nearestDistance = range;
		IEntity nearest;
		
		foreach(EntityID id : m_aParkingSearch)
		{
			IEntity kerb = GetGame().GetWorld().FindEntityByID(id);
			float distance = vector.Distance(kerb.GetOrigin(), pos);
			if(distance < nearestDistance)
			{
				nearest = kerb;
				nearestDistance = distance;
			}
		}
		
		if(!nearest) return false;
		
		vector mat[4];
		
		nearest.GetTransform(mat);
			
		mat[3] = mat[3] + (mat[2] * 3);
		
		vector p = mat[3];
	
		vector angles = Math3D.MatrixToAngles(mat);
		angles[0] = angles[0] - 90;
		Math3D.AnglesToMatrix(angles, outMat);
		outMat[3] = p;
		
		return true;
		
	}
	
	bool FilterKerbAddToArray(IEntity entity)
	{
		if(entity.ClassName() == "StaticModelEntity"){
			VObject mesh = entity.GetVObject();
			
			if(mesh){
				string res = mesh.GetResourceName();
				if(res.IndexOf("Pavement_") > -1) m_aParkingSearch.Insert(entity.GetID());
				if(res.IndexOf("Kerb_") > -1) m_aParkingSearch.Insert(entity.GetID());				
			}
		}
		return false;
	}
	
	bool FilterParkingAddToArray(IEntity entity)
	{
		if(entity.FindComponent(OVT_ParkingComponent)){
			m_aParkingSearch.Insert(entity.GetID());
		}
		return false;
	}
	
	IEntity SpawnVehicle(ResourceName prefab, vector pos,  int ownerId = 0, float rotation = 0)
	{
		
	}
	
	IEntity SpawnVehicleNearestParking(ResourceName prefab, vector pos,  string ownerId = "")
	{
		OVT_EconomyManagerComponent economy = OVT_Global.GetEconomy();
		OVT_ParkingType parkingType = OVT_ParkingType.PARKING_CAR;
		
		int id = economy.GetInventoryId(prefab);
		if(id > -1)
		{
			parkingType = economy.GetParkingType(id);
		}
		
		vector mat[4];		
		if(!GetNearestParkingSpot(pos, mat, parkingType))
		{
			if(!FindNearestKerbParking(pos, 30, mat))
			{				
				return null;
			}
		}
		return SpawnVehicleMatrix(prefab, mat, ownerId);
	}
	
	IEntity SpawnVehicleBehind(ResourceName prefab, IEntity entity, string ownerId="")
	{
		vector mat[4];
			
		entity.GetTransform(mat);
		mat[3] = mat[3] + (mat[2] * -5);
		vector pos = mat[3];
			
		vector angles = Math3D.MatrixToAngles(mat);
		angles[0] = angles[0] - 90;
		Math3D.AnglesToMatrix(angles, mat);
		mat[3] = pos;
		
		return SpawnVehicleMatrix(prefab, mat, ownerId);
	}
	
	IEntity SpawnVehicleMatrix(ResourceName prefab, vector mat[4], string ownerId = "")
	{		
		IEntity ent = OVT_Global.SpawnEntityPrefabMatrix(prefab, mat);
		if(!ent)
		{
			Print("Failure to spawn vehicle");
			return null;
		}
				
		if(ownerId != "") 
		{
			SetOwnerPersistentId(ownerId, ent);
			OVT_PlayerOwnerComponent playerowner = EPF_Component<OVT_PlayerOwnerComponent>.Find(ent);
			if(playerowner)
			{
				playerowner.SetPlayerOwner(ownerId);
			}
		}
		
		m_aVehicles.Insert(ent.GetID());
		
		return ent;
	}
	
	void UpgradeVehicle(RplId vehicle, int id)
	{
		OVT_EconomyManagerComponent economy = OVT_Global.GetEconomy();
		
		ResourceName res = economy.GetResource(id);
		
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(vehicle));
		if(!rpl) return;
		IEntity entity = rpl.GetEntity();
		
		string ownerId = GetOwnerID(entity);
		
		vector mat[4];
		entity.GetTransform(mat);
		
		IEntity newveh = SpawnVehicleMatrix(res, mat, ownerId);
		RplComponent newrpl = RplComponent.Cast(newveh.FindComponent(RplComponent));
		
		m_aVehicles.RemoveItem(entity.GetID());
		
		// Store old vehicle for deletion after transfer completes
		m_pUpgradeOldVehicle = entity;
		
		// Transfer storage from old vehicle to new vehicle using inventory manager
		OVT_StorageOperationConfig config = new OVT_StorageOperationConfig(
			false,      // skipWeaponsOnGround
			false,      // deleteEmptyContainers
			50,         // itemsPerBatch
			100,        // batchDelayMs (normal delay to prevent crashes)
			-1,         // searchRadius (not needed for direct transfer)
			1           // maxBatchesPerFrame (normal batching)
		);
		
		OVT_Global.GetInventory().TransferStorageByRplId(rpl.Id(), newrpl.Id(), config, GetUpgradeCallback());		
	}
	
	void RepairVehicle(RplId vehicle)
	{					
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(vehicle));
		if(!rpl) return;
		IEntity entity = rpl.GetEntity();
		
		SCR_VehicleDamageManagerComponent dmg = SCR_VehicleDamageManagerComponent.Cast(entity.FindComponent(SCR_VehicleDamageManagerComponent));
		if(dmg)
		{
			dmg.FullHeal();
			dmg.SetHealthScaled(dmg.GetMaxHealth());
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get or create the upgrade callback instance
	protected OVT_VehicleUpgradeCallback GetUpgradeCallback()
	{
		if (!m_UpgradeCallback)
			m_UpgradeCallback = new OVT_VehicleUpgradeCallback(this);
		return m_UpgradeCallback;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Called when vehicle upgrade transfer completes successfully
	void OnUpgradeTransferComplete(int itemsTransferred, int itemsSkipped)
	{		
		if (m_pUpgradeOldVehicle)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_pUpgradeOldVehicle);
			m_pUpgradeOldVehicle = null;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Called when vehicle upgrade transfer fails
	void OnUpgradeTransferError(string errorMessage)
	{
		Print(string.Format("Vehicle upgrade transfer failed: %1", errorMessage), LogLevel.ERROR);
		
		// Still delete old vehicle to prevent it being stuck
		if (m_pUpgradeOldVehicle)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_pUpgradeOldVehicle);
			m_pUpgradeOldVehicle = null;
		}
	}
	
}