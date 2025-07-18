class OVT_UndeployFOBAction : ScriptedUserAction
{	
	
	//---------------------------------------------------------
 	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity) 
 	{
		OVT_Global.GetServer().UndeployFOB(pOwnerEntity.GetParent());
 	}
		
	override bool GetActionNameScript(out string outName)
	{
		return false;
	}	
			
	override bool CanBeShownScript(IEntity user) {
		// Check server config to see if FOB undeployment is restricted to officers only
		if(OVT_Global.GetConfig().m_ConfigFile.mobileFOBOfficersOnly)
		{
			if(!OVT_Global.GetPlayers().LocalPlayerIsOfficer()) return false;
		}
		return true;
	}
	
	override bool HasLocalEffectOnlyScript() { return true; };
}