class OVT_BuildContext : OVT_UIContext
{
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Layout to show when building", params: "layout")]
	ResourceName m_BuildLayout;	
	
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Layout to show when removing", params: "layout")]
	ResourceName m_RemovalLayout;
	
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Icon to show on remove card", params: "edds")]
	ResourceName m_RemoveIcon;
	
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Build Camera Prefab", params: "et")]
	ResourceName m_BuildCameraPrefab;
	
	SCR_ManualCamera m_Camera;
	CameraBase m_PlayerCamera;
	
	Widget m_BuildWidget;
	Widget m_RemovalWidget;
		
	protected SCR_PrefabPreviewEntity m_eBuildingEntity;
	protected ResourceName m_pBuildingPrefab;
	protected OVT_Buildable m_Buildable;
	
	protected vector[] m_vCurrentTransform[4];
		
	protected OVT_RealEstateManagerComponent m_RealEstate;
	protected OVT_OccupyingFactionManager m_OccupyingFaction;
	protected OVT_ResistanceFactionManager m_Resistance;
	protected OVT_TownManagerComponent m_Towns;
	protected CameraManager m_CameraManager;
	protected ref OVT_ItemLimitChecker m_ItemLimitChecker;
	
	const int MAX_FOB_BUILD_DIS = 100;
	const int MAX_CAMP_BUILD_DIS = 50;
	
	bool m_bBuilding = false;
	bool m_bRemovalMode = false;
	protected IEntity m_eHighlightedEntity = null;
	int m_iPrefabIndex = 0;
	int m_iPageNum = 0;
	int m_iNumPages = 0;
		
	override void PostInit()
	{
		m_RealEstate = OVT_Global.GetRealEstate();
		m_OccupyingFaction = OVT_Global.GetOccupyingFaction();
		m_Resistance = OVT_Global.GetResistanceFaction();
		m_Towns = OVT_Global.GetTowns();		
		m_CameraManager = GetGame().GetCameraManager();
		m_ItemLimitChecker = new OVT_ItemLimitChecker();
	}
	
	override void OnFrame(float timeSlice)
	{		
		if (m_bBuilding && m_Camera)	
		{
			TryForceCamera();
			
			m_InputManager.ActivateContext("OverthrowBuildContext");
			
			if(m_eBuildingEntity)
			{
				vector normal = vector.Zero;				
				m_eBuildingEntity.SetOrigin(GetBuildPosition(normal));
				m_eBuildingEntity.GetTransform(m_vCurrentTransform);				
				m_eBuildingEntity.Update();			
			}
		}
		
		if (m_bRemovalMode)
		{
			if (m_Camera)
			{
				TryForceCamera();
			}
			m_InputManager.ActivateContext("OverthrowBuildContext");
			HighlightRemovableItems();
		}
	}
	
	protected bool TryForceCamera()
	{
		if (!m_Camera || !m_CameraManager)
			return false;
		
		if (m_Camera != m_CameraManager.CurrentCamera())
		{
			array<CameraBase> cameras = {};
			m_CameraManager.GetCamerasList(cameras);
			if (cameras.Contains(m_Camera))
				m_CameraManager.SetCamera(m_Camera);
		}
		
		return true;
	}
	
	override void OnShow()
	{
		m_iPageNum = 0;

		// Set up the previous button
		Widget prevButton = m_wRoot.FindAnyWidget("PrevButton");
		SCR_InputButtonComponent btn = SCR_InputButtonComponent.Cast(prevButton.FindHandler(SCR_InputButtonComponent));
		btn.m_OnActivated.Insert(PreviousPage);

		// Set up the next button
		Widget nextButton = m_wRoot.FindAnyWidget("NextButton");
		btn = SCR_InputButtonComponent.Cast(nextButton.FindHandler(SCR_InputButtonComponent));
		btn.m_OnActivated.Insert(NextPage);

		// Set up the close button
		Widget closeButton = m_wRoot.FindAnyWidget("CloseButton");
		btn = SCR_InputButtonComponent.Cast(closeButton.FindHandler(SCR_InputButtonComponent));
		btn.m_OnActivated.Insert(CloseLayout);


		Refresh();
	}
	
	void PreviousPage()
	{
		if(!m_wRoot) return;
		m_iPageNum--;
		if(m_iPageNum < 0) m_iPageNum = 0;
		
		Refresh();
	}
	
	void NextPage()
	{
		if(!m_wRoot) return;
		m_iPageNum++;
		if(m_iPageNum > m_iNumPages-1) m_iPageNum = m_iNumPages-1;
		
		Refresh();
	}
	
	void Refresh()
	{
		if(!m_wRoot) return;
		
		TextWidget pages = TextWidget.Cast(m_wRoot.FindAnyWidget("Pages"));
		Widget root = m_wRoot.FindAnyWidget("m_BrowserGrid");
		
		int done = 0;
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		
		// Show Remove card as first item
		Widget removeCard = root.FindWidget("BuildMenu_Card" + done);
		OVT_BuildMenuCardComponent removeCardComponent = OVT_BuildMenuCardComponent.Cast(removeCard.FindHandler(OVT_BuildMenuCardComponent));
		if(removeCardComponent)
		{
			removeCardComponent.InitRemoveCard(this, m_RemoveIcon);
		}
		removeCard.SetOpacity(1);
		done++;
		
		// Calculate pages based on remaining items
		int totalBuildables = m_Resistance.m_BuildablesConfig.m_aBuildables.Count();
		m_iNumPages = Math.Ceil(totalBuildables / 14); // 14 items per page (leaving room for remove card)
		if(m_iPageNum >= m_iNumPages) m_iPageNum = 0;
		string pageNumText = (m_iPageNum + 1).ToString();
		
		pages.SetText(pageNumText + "/" + m_iNumPages);
		
		// Show buildables for current page (14 items instead of 15)
		for(int i = m_iPageNum * 14; i < (m_iPageNum + 1) * 14 && i < m_Resistance.m_BuildablesConfig.m_aBuildables.Count(); i++)
		{
			OVT_Buildable buildable = m_Resistance.m_BuildablesConfig.m_aBuildables[i];
			Widget w = root.FindWidget("BuildMenu_Card" + done);
			OVT_BuildMenuCardComponent card = OVT_BuildMenuCardComponent.Cast(w.FindHandler(OVT_BuildMenuCardComponent));
			
			// Check if buildable can be built at current location
			string reason;
			bool canBuild = CanBuild(buildable, player.GetOrigin(), reason);
			
			card.Init(buildable, this, canBuild, reason);
			w.SetOpacity(1);
			
			done++;
		}
		
		// Hide unused cards
		for(int i=done; i < 15; i++)
		{
			Widget w = root.FindWidget("BuildMenu_Card" + i);
			w.SetOpacity(0);
		}
	}
	
	override void RegisterInputs()
	{
		super.RegisterInputs();
		if(!m_InputManager) return;
		
		m_InputManager.AddActionListener("MenuSelect", EActionTrigger.DOWN, DoBuild);
		m_InputManager.AddActionListener("OverthrowRotateLeft", EActionTrigger.PRESSED, RotateLeft);
		m_InputManager.AddActionListener("OverthrowRotateRight", EActionTrigger.PRESSED, RotateRight);
		m_InputManager.AddActionListener("OverthrowNextItem", EActionTrigger.DOWN, NextItem);
		m_InputManager.AddActionListener("OverthrowPrevItem", EActionTrigger.DOWN, PrevItem);
		m_InputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, Cancel);
		
		m_InputManager.AddActionListener("CharacterRight", EActionTrigger.VALUE, MoveRight);
		m_InputManager.AddActionListener("CharacterForward", EActionTrigger.VALUE, MoveForward);
		m_InputManager.AddActionListener("Inventory_InspectZoom", EActionTrigger.VALUE, Zoom);
	}
	
	override void UnregisterInputs()
	{
		super.UnregisterInputs();
		if(!m_InputManager) return;
		
		m_InputManager.RemoveActionListener("MenuSelect", EActionTrigger.DOWN, DoBuild);
		m_InputManager.RemoveActionListener("OverthrowRotateLeft", EActionTrigger.PRESSED, RotateLeft);
		m_InputManager.RemoveActionListener("OverthrowRotateRight", EActionTrigger.PRESSED, RotateRight);
		m_InputManager.RemoveActionListener("OverthrowNextItem", EActionTrigger.DOWN, NextItem);
		m_InputManager.RemoveActionListener("OverthrowPrevItem", EActionTrigger.DOWN, PrevItem);
		m_InputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, Cancel);
		
		m_InputManager.RemoveActionListener("CharacterRight", EActionTrigger.VALUE, MoveRight);
		m_InputManager.RemoveActionListener("CharacterForward", EActionTrigger.VALUE, MoveForward);
		m_InputManager.RemoveActionListener("Inventory_InspectZoom", EActionTrigger.VALUE, Zoom);
	}
	
	void Cancel(float value = 1, EActionTrigger reason = EActionTrigger.DOWN)
	{
		if(m_bBuilding)
		{
			m_bBuilding = false;
			RemoveGhost();
			RevertCamera();
			if(m_BuildWidget)
				m_BuildWidget.RemoveFromHierarchy();
			return;
		}
		
		if(m_bRemovalMode)
		{
			m_bRemovalMode = false;
			ClearHighlights();
			RevertCamera();
			if(m_RemovalWidget)
				m_RemovalWidget.RemoveFromHierarchy();
			return;
		}
	}
	

	bool CanBuild(OVT_Buildable buildable, vector pos, out string reason)
	{
		reason = "#OVT-CannotBuildHere";
		
		// Check item limits using the build-specific location detection
		string tempReason;
		string locationId;
		EOVTBaseType baseType;
		int itemCount = m_ItemLimitChecker.CountItemsAtLocationForBuild(pos, locationId, baseType);
		
		if(itemCount > 0)
		{
			int limit = 0;
			if(baseType == EOVTBaseType.NONE)
				limit = OVT_Global.GetConfig().GetHouseItemLimit();
			else if(baseType == EOVTBaseType.CAMP)
				limit = OVT_Global.GetConfig().GetCampItemLimit();
			else if(baseType == EOVTBaseType.FOB || baseType == EOVTBaseType.BASE)
				limit = OVT_Global.GetConfig().GetFOBItemLimit();
			
			// If limit is 0 or negative, allow unlimited items
			if(limit > 0 && itemCount >= limit)
			{
				reason = "#OVT-ItemLimitReached";
				return false;
			}
		}
		
		float dist;
		
		if(buildable.m_bBuildAtBase)
		{			
			OVT_BaseData base = m_OccupyingFaction.GetNearestBase(pos);			
			dist = vector.Distance(base.location,pos);
			if(dist < OVT_Global.GetConfig().m_Difficulty.baseRange && !base.IsOccupyingFaction())
			{
				return true;
			}
		}
		
		if(buildable.m_bBuildInTown)
		{	
			OVT_TownData town = m_Towns.GetNearestTown(pos);
			if(town.size == 1) {
				reason = "#OVT-CannotBuildVillage";
				return false;
			}
			dist = vector.Distance(town.location,pos);
			int range = m_Towns.m_iCityRange;
			if(town.size < 3) range = m_Towns.m_iTownRange;
			if(dist < range)
			{
				return true;
			}
		}
		
		if(buildable.m_bBuildInVillage)
		{	
			OVT_TownData town = m_Towns.GetNearestTown(pos);
			if(town.size > 1) {
				reason = "#OVT-CannotBuildTown";
				return false;
			}
			dist = vector.Distance(town.location,pos);
			int range = m_Towns.m_iVillageRange;			
			if(dist < range)
			{
				return true;
			}
		}
		
		if(buildable.m_bBuildAtCamp)
		{	
			vector fob = m_Resistance.GetNearestCamp(pos);		
			dist = vector.Distance(fob, pos);
			if(dist < MAX_CAMP_BUILD_DIS) return true;	
		}
		
		if(buildable.m_bBuildAtFOB)
		{
			vector fob = m_Resistance.GetNearestFOB(pos);		
			dist = vector.Distance(fob, pos);
			if(dist < MAX_FOB_BUILD_DIS) return true;
		}
							
		return false;
	}
	
	void StartBuild(OVT_Buildable buildable)
	{
		if(m_bIsActive) CloseLayout();
				
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		
		m_Buildable = buildable;
		
		string reason;
		if(!CanBuild(m_Buildable, player.GetOrigin(), reason))
		{
			ShowHint(reason);
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.ERROR);
			return;
		}
		
		if(!m_Economy.PlayerHasMoney(m_sPlayerID, OVT_Global.GetConfig().GetBuildableCost(buildable)))
		{
			ShowHint("#OVT-CannotAfford");
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.ERROR);
			return;
		}
		
		WorkspaceWidget workspace = GetGame().GetWorkspace(); 
		m_BuildWidget = workspace.CreateWidgets(m_BuildLayout);
		
		m_bBuilding = true;		
		m_iPrefabIndex = 0;
		
		SpawnGhost();
		CreateCamera();
		MoveCamera();
	}
	
	protected void CreateCamera()
	{
		if(m_Camera) return;
		
		CameraManager cameraMgr = GetGame().GetCameraManager();
		BaseWorld world = GetGame().GetWorld();
		
		m_PlayerCamera = cameraMgr.CurrentCamera();
		
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		
		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;		
		params.Transform[3] = player.GetOrigin() + "0 15 0";
						
		m_Camera = SCR_ManualCamera.Cast(GetGame().SpawnEntityPrefab(Resource.Load(m_BuildCameraPrefab), GetGame().GetWorld(), params));
		m_Camera.SetName("BuildCam");
	}
	
	protected void MoveCamera()
	{
		if(!m_Camera) return;
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		m_Camera.SetAngles("-45 0 0");
		m_Camera.SetOrigin(player.GetOrigin() + "0 15 0");
	}
	
	protected void RevertCamera()
	{
		if (m_Camera)
		{
			m_Camera.Terminate();
			m_Camera.SwitchToPreviousCamera();
			delete m_Camera;
		}
	}
	
	void MoveRight(float value = 1, EActionTrigger reason = EActionTrigger.DOWN)
	{
		if(!m_bBuilding && !m_bRemovalMode) return;
		
		vector move = "0 0 0";
		move[0] = value * 0.07;
		vector pos = m_Camera.GetOrigin() + move;
		
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		vector groundPos = Vector(pos[0],GetGame().GetWorld().GetSurfaceY(pos[0],pos[2]),pos[2]);
		float dist = vector.Distance(player.GetOrigin(), groundPos);
		if(dist > 50) return;
		
		m_Camera.SetOrigin(pos);
	}	
	
	void MoveForward(float value = 1, EActionTrigger reason = EActionTrigger.DOWN)
	{
		if(!m_bBuilding && !m_bRemovalMode) return;
		vector move = "0 0 0";
		move[2] = value * 0.07;
		vector pos = m_Camera.GetOrigin() + move;
		
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		vector groundPos = Vector(pos[0],GetGame().GetWorld().GetSurfaceY(pos[0],pos[2]),pos[2]);
		float dist = vector.Distance(player.GetOrigin(), groundPos);
		if(dist > 50) return;		
		
		m_Camera.SetOrigin(pos);
	}	
	
	void Zoom(float value = 1, EActionTrigger reason = EActionTrigger.DOWN)
	{
		if(!m_bBuilding && !m_bRemovalMode) return;
		vector move = "0 0 0";
		move[1] = value * -0.005;
		
		vector lookpos = m_vCurrentTransform[3];
		
		vector pos = m_Camera.GetOrigin();
		pos = pos + move;
		
		float ground = GetGame().GetWorld().GetSurfaceY(pos[0],pos[2]);
		if(pos[1] < ground + 10)
		{
			pos[1] = ground + 10;
		}
		
		if(pos[1] > ground + 25)
		{
			pos[1] = ground + 25;
		}
		
		m_Camera.SetOrigin(pos);		
	}	
	
	protected void SpawnGhost()
	{		
		vector normal = vector.Zero;
		vector pos = GetBuildPosition(normal);
				
		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = pos;
		m_pBuildingPrefab = m_Buildable.m_aPrefabs[m_iPrefabIndex];
		//m_eBuildingEntity = GetGame().SpawnEntityPrefabLocal(Resource.Load(m_pBuildingPrefab), null, params);
		m_eBuildingEntity = SCR_PrefabPreviewEntity.Cast(SCR_PrefabPreviewEntity.SpawnPreviewFromPrefab(Resource.Load(m_pBuildingPrefab), "SCR_PrefabPreviewEntity", null, params, "{58F07022C12D0CF5}Assets/Editor/PlacingPreview/Preview.emat"));
		
		if(m_vCurrentTransform)
		{
			m_eBuildingEntity.SetPreviewTransform(m_vCurrentTransform, EEditorTransformVertical.TERRAIN);
		}
	}
	
	protected void RemoveGhost()
	{
		SCR_EntityHelper.DeleteEntityAndChildren(m_eBuildingEntity);
		m_eBuildingEntity = null;
	}
	
	void DoBuild(float value = 1, EActionTrigger reason = EActionTrigger.DOWN)
	{
		if(m_bRemovalMode)
		{
			DoRemove();
			return;
		}
		
		if(!m_bBuilding) return;
			
		int cost = OVT_Global.GetConfig().GetBuildableCost(m_Buildable);
		vector mat[4];
		
		if(m_eBuildingEntity)
		{			
			m_eBuildingEntity.GetTransform(mat);
			RemoveGhost();
			string error;
			if(!CanBuild(m_Buildable, mat[3], error))
			{
				ShowHint(error);
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.ERROR);
				
				// Close layout if item limit was reached during build attempt
				if(error == "#OVT-ItemLimitReached")
				{
					CloseLayout();
				}
				return;
			}
			
			if(!m_Economy.PlayerHasMoney(m_sPlayerID, cost))
			{
				ShowHint("#OVT-CannotAfford");
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.ERROR);
				return;
			}
			
			vector angles = Math3D.MatrixToAngles(mat);
			int buildableIndex = m_Resistance.m_BuildablesConfig.m_aBuildables.Find(m_Buildable);
			int prefabIndex = m_Buildable.m_aPrefabs.Find(m_pBuildingPrefab);
			OVT_Global.GetServer().BuildItem(buildableIndex, prefabIndex, mat[3], angles, m_iPlayerID);
						
			m_Economy.TakePlayerMoney(m_iPlayerID, OVT_Global.GetConfig().GetBuildableCost(m_Buildable));
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.CLICK);
		}
		
		Cancel();
	}
	
	void NextItem(float value = 1, EActionTrigger reason = EActionTrigger.DOWN)
	{
		if(!m_bBuilding) return;
		int newIndex = m_iPrefabIndex + 1;
		if(newIndex > m_Buildable.m_aPrefabs.Count() - 1)
		{
			newIndex = 0;
		}		
		
		if(newIndex != m_iPrefabIndex)
		{
			RemoveGhost();
			m_iPrefabIndex = newIndex;
			SpawnGhost();
		}
	}
	
	void PrevItem(float value = 1, EActionTrigger reason = EActionTrigger.DOWN)
	{
		if(!m_bBuilding) return;
		int newIndex = m_iPrefabIndex - 1;
		if(newIndex < 0)
		{
			newIndex = m_Buildable.m_aPrefabs.Count() - 1;
		}
		
		if(newIndex != m_iPrefabIndex)
		{
			RemoveGhost();
			m_iPrefabIndex = newIndex;
			SpawnGhost();
		}
	}
	
	void RotateLeft(float value = 1, EActionTrigger reason = EActionTrigger.DOWN)
	{
		if(m_eBuildingEntity)
		{
			vector angles = m_eBuildingEntity.GetYawPitchRoll();			
			angles[0] = angles[0] - 1;				
			m_eBuildingEntity.SetYawPitchRoll(angles);
		}
	}
	
	void RotateRight(float value = 1, EActionTrigger reason = EActionTrigger.DOWN)
	{
		if(m_eBuildingEntity)
		{
			vector angles = m_eBuildingEntity.GetYawPitchRoll();			
			angles[0] = angles[0] + 1;			
			m_eBuildingEntity.SetYawPitchRoll(angles);
		}
	}
	
	vector GetBuildPosition(out vector normal)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		BaseWorld world = GetGame().GetWorld();
		
		float screenW, screenH;
		workspace.GetScreenSize(screenW, screenH);
		vector cameraDir;
		vector cameraPos = workspace.ProjScreenToWorldNative(screenW / 2, screenH / 2, cameraDir, world, -1);
		
		//--- Find object/ground intersection, or use maximum distance when none is found
		float traceDis = GetTraceDis(cameraPos, cameraDir * 500, normal);
		if (traceDis == 1)
			traceDis = 500;
		else
			traceDis *= 500;
		
		vector endPos = cameraPos + cameraDir * traceDis;
		
		return endPos;
	}
	
	protected float GetTraceDis(vector pos, vector dir, out vector hitNormal)
	{
		BaseWorld world = GetGame().GetWorld();
		autoptr TraceParam trace = new TraceParam();
		trace.Start = pos;
		trace.End = trace.Start + dir;
		trace.Flags = TraceFlags.WORLD;
		trace.Exclude = m_eBuildingEntity;
		
		float dis = world.TraceMove(trace, SCR_Global.FilterCallback_IgnoreCharacters);
		hitNormal = trace.TraceNorm;
		
		return dis;
	}
	
	//! Start removal mode
	void StartRemovalMode()
	{
		if(m_bIsActive) CloseLayout();
		
		m_bRemovalMode = true;
		m_bBuilding = false;
		
		if(m_eBuildingEntity)
		{
			RemoveGhost();
		}
		
		CreateCamera();
		MoveCamera();
		
		if (!m_RemovalWidget && m_RemovalLayout != "")
		{
			WorkspaceWidget workspace = GetGame().GetWorkspace();
			m_RemovalWidget = workspace.CreateWidgets(m_RemovalLayout);
		}
		
		ShowHint("#OVT-RemovalModeActive");
	}
	
	//! Highlight the item the player is looking at (if removable)
	void HighlightRemovableItems()
	{
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		if(!player) return;
		
		// Raycast to find what the player is looking at
		vector cameraPos, cameraDir;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		BaseWorld world = GetGame().GetWorld();
		
		float screenW, screenH;
		workspace.GetScreenSize(screenW, screenH);
		cameraPos = workspace.ProjScreenToWorldNative(screenW / 2, screenH / 2, cameraDir, world, -1);
		
		// Trace to find what the player is looking at
		autoptr TraceParam trace = new TraceParam();
		trace.Start = cameraPos;
		trace.End = cameraPos + cameraDir * 50; // 50m range
		trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		
		float traceDis = world.TraceMove(trace, null);
		if(traceDis < 1)
		{
			IEntity hitEntity = trace.TraceEnt;
			if(hitEntity)
			{
				OVT_BuildableComponent buildableComp = OVT_BuildableComponent.Cast(hitEntity.FindComponent(OVT_BuildableComponent));
				if(buildableComp && CanRemoveItem(buildableComp))
				{
					// Only highlight if it's a different entity
					if(hitEntity != m_eHighlightedEntity)
					{
						ClearHighlights();
						m_eHighlightedEntity = hitEntity;
						// Apply red highlighting using the CannotBuild material for consistency
						SCR_Global.SetMaterial(hitEntity, "{14A9DCEA57D1C381}Assets/Conflict/CannotBuild.emat");
					}
					return;
				}
			}
		}
		
		// No valid target found, clear highlights
		ClearHighlights();
	}
	
	//! Check if player can remove an item
	bool CanRemoveItem(OVT_BuildableComponent buildableComp)
	{
		// Officers can remove any item
		OVT_PlayerData player = OVT_Global.GetPlayers().GetPlayer(m_sPlayerID);
		if(player && player.isOfficer)
			return true;
		
		// Players can remove their own items
		if(buildableComp.GetOwnerPersistentId() == m_sPlayerID)
			return true;
		
		return false;
	}
	
	//! Remove the item the player is looking at
	void DoRemove()
	{
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		if(!player) return;
		
		// Raycast to find the item the player is looking at
		vector cameraPos, cameraDir;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		BaseWorld world = GetGame().GetWorld();
		
		float screenW, screenH;
		workspace.GetScreenSize(screenW, screenH);
		cameraPos = workspace.ProjScreenToWorldNative(screenW / 2, screenH / 2, cameraDir, world, -1);
		
		// Trace to find what the player is looking at
		autoptr TraceParam trace = new TraceParam();
		trace.Start = cameraPos;
		trace.End = cameraPos + cameraDir * 50; // 50m range
		trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		
		float traceDis = world.TraceMove(trace, null);
		if(traceDis < 1)
		{
			IEntity hitEntity = trace.TraceEnt;
			if(hitEntity)
			{
				OVT_BuildableComponent buildableComp = OVT_BuildableComponent.Cast(hitEntity.FindComponent(OVT_BuildableComponent));
				if(buildableComp && CanRemoveItem(buildableComp))
				{
					// Send removal request to server
					OVT_Global.GetServer().RemovePlacedItem(hitEntity.GetID(), m_iPlayerID);
					ShowHint("#OVT-ItemRemoved");
					SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.CLICK);
				}
				else
				{
					ShowHint("#OVT-CannotRemoveItem");
					SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.ERROR);
				}
			}
		}
	}
	
	//! Clear highlighting from all items
	void ClearHighlights()
	{
		if(m_eHighlightedEntity)
		{
			// Reset material to empty string to restore original
			SCR_Global.SetMaterial(m_eHighlightedEntity, "");
			m_eHighlightedEntity = null;
		}
	}
}