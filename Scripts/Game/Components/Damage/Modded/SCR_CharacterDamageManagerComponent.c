modded class SCR_CharacterDamageManagerComponent : SCR_ExtendedDamageManagerComponent
{
	protected bool m_bCheckedFaction = false;
	protected bool m_bIsOccupyingFaction = false;
	protected IEntity m_eLastInstigator;
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		GetOnDamage().Insert(WhenDamaged);
		GetOnDamageStateChanged().Insert(WhenDamageStateChanged);
	}
	
	void WhenDamaged(BaseDamageContext damageContext)
	{		
		if(damageContext.instigator)
		{	
			IEntity entity = damageContext.instigator.GetInstigatorEntity();
			if(entity) 
			{
				OVT_PlayerWantedComponent wanted = OVT_PlayerWantedComponent.Cast(entity.FindComponent(OVT_PlayerWantedComponent));
				
				if(wanted)
				{
					// Check if player was disguised - if so, blow their cover
					if(wanted.IsDisguisedAsOccupying())
					{
						wanted.SetBaseWantedLevel(2, "WantedDisguiseBlown");
						wanted.BlowDisguise();
					}
					else
					{
						// Not disguised, normal wanted level increase
						wanted.SetBaseWantedLevel(2);
					}
				}
			}
		}
	}
	
	

	void WhenDamageStateChanged(EDamageState state)
	{		
		IEntity instigator = GetInstigator().GetInstigatorEntity();				
		if(state == EDamageState.DESTROYED){
			// Fire universal character killed event for all characters regardless of faction
			OVT_OverthrowGameMode gameMode = OVT_OverthrowGameMode.Cast(GetGame().GetGameMode());
			if(gameMode)
			{
				gameMode.GetOnCharacterKilled().Invoke(GetOwner(), instigator);
			}
			
			if(IsOccupyingFaction())
			{
				OVT_Global.GetOccupyingFaction().OnAIKilled(GetOwner(), instigator);	
				
				//Check immediate surrounds for a vehicle (hoping for a better way soon pls BI)
				GetGame().GetWorld().QueryEntitiesBySphere(GetOwner().GetOrigin(), 5, CheckVehicleSetWanted, FilterVehicleEntities, EQueryEntitiesFlags.ALL);		
			}
			if(instigator)
			{			
				OVT_PlayerWantedComponent wanted = OVT_PlayerWantedComponent.Cast(instigator.FindComponent(OVT_PlayerWantedComponent));
				
				if(wanted)
				{
					// Check if player was disguised - if so, blow their cover
					if(wanted.IsDisguisedAsOccupying())
					{
						wanted.SetBaseWantedLevel(3, "WantedDisguiseBlown");
						wanted.BlowDisguise();
					}
					else
					{
						// Not disguised, normal wanted level increase
						wanted.SetBaseWantedLevel(3);
					}
				}
			}
		}		
	}
	
	protected bool IsOccupyingFaction()
	{
		if(!m_bCheckedFaction)
		{
			OVT_OverthrowConfigComponent config = OVT_Global.GetConfig();
			FactionAffiliationComponent aff = FactionAffiliationComponent.Cast(GetOwner().FindComponent(FactionAffiliationComponent));
			Faction fac = aff.GetAffiliatedFaction();
			if(fac && fac.GetFactionKey() == config.m_sOccupyingFaction)
			{
				m_bIsOccupyingFaction = true;
			}
			m_bCheckedFaction = true;
		}
		return m_bIsOccupyingFaction;
	}
	
	protected bool FilterVehicleEntities(IEntity entity)
	{
		if(entity.ClassName() == "Vehicle") return true;
		return false;
	}
	
	protected bool CheckVehicleSetWanted(IEntity entity)
	{
		SCR_BaseCompartmentManagerComponent mgr = SCR_BaseCompartmentManagerComponent.Cast(entity.FindComponent(SCR_BaseCompartmentManagerComponent));
		
		autoptr array<IEntity> occupants = new array<IEntity>;
		
		mgr.GetOccupants(occupants);
				
		foreach(IEntity occupant : occupants)
		{	
			OVT_PlayerWantedComponent wanted = OVT_PlayerWantedComponent.Cast(occupant.FindComponent(OVT_PlayerWantedComponent));
		
			if(wanted)
			{
				// Check if player was disguised - if so, blow their cover
				if(wanted.IsDisguisedAsOccupying())
				{
					wanted.SetBaseWantedLevel(3, "WantedDisguiseBlown");
					wanted.BlowDisguise();
				}
				else
				{
					// Not disguised, normal wanted level increase
					wanted.SetBaseWantedLevel(3);
				}
			}
		}
		
		return true;
	}
}