//! Data structure for storing AI recruit information
class OVT_RecruitData : Managed
{
	//! Unique identifier for this recruit
	string m_sRecruitId;
	
	//! Display name of the recruit
	string m_sName;
	
	//! Persistent ID of the player who owns this recruit
	string m_sOwnerPersistentId;
	
	//! Number of enemies killed by this recruit
	int m_iKills = 0;
	
	//! Experience points accumulated
	int m_iXP = 0;
	
	//! Current level (cached for performance)
	int m_iLevel = 1;
	
	//! Skills and their levels
	ref map<string, int> m_mSkills = new map<string, int>;
	
	//! Whether the recruit is currently in training
	bool m_bIsTraining = false;
	
	//! Game time when training will be complete
	float m_fTrainingCompleteTime = 0;
	
	//! Last known position of the recruit
	vector m_vLastKnownPosition = "0 0 0";
		
	//! Whether the recruit entity is currently spawned in the world
	bool m_bIsOnline = false;
	
	//! ID of the town where this recruit was hired from
	int m_iTownId = -1;
	
	//------------------------------------------------------------------------------------------------
	//! Calculate level from XP (same formula as player)
	int GetLevel()
	{
		return Math.Floor(1 + (0.1 * Math.Sqrt(m_iXP)));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get XP required for next level
	int GetNextLevelXP()
	{
		int level = GetLevel();
		return Math.Pow(level / 0.1, 2);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get XP required for a specific level
	int GetLevelXP(int level)
	{
		return Math.Pow(level / 0.1, 2);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get progress to next level as percentage (0-1)
	float GetLevelProgress()
	{
		int levelFromXP = GetLevelXP(GetLevel() - 1);
		int levelToXP = GetNextLevelXP();
		int total = levelToXP - levelFromXP;
		int current = m_iXP - levelFromXP;
		
		if (total <= 0)
			return 0;
			
		return current / total;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Add experience points and update level
	void AddXP(int xp)
	{
		m_iXP += xp;
		m_iLevel = GetLevel();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if recruit has a specific skill
	bool HasSkill(string skillName)
	{
		return m_mSkills.Contains(skillName);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get skill level (0 if not learned)
	int GetSkillLevel(string skillName)
	{
		if (!HasSkill(skillName))
			return 0;
			
		return m_mSkills[skillName];
	}
	
	//------------------------------------------------------------------------------------------------
	//! Add or increase skill level
	void AddSkill(string skillName, int level = 1)
	{
		if (HasSkill(skillName))
			m_mSkills[skillName] = m_mSkills[skillName] + level;
		else
			m_mSkills[skillName] = level;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Set the recruit's display name
	void SetName(string name)
	{
		m_sName = name;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get the recruit's display name
	string GetName()
	{
		return m_sName;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get the recruit's hometown name
	string GetHometown()
	{
		OVT_TownManagerComponent townManager = OVT_Global.GetTowns();
		if (townManager && m_iTownId != -1)
		{
			return townManager.GetTownName(m_iTownId);
		}
		return "Unknown";
	}
	
	//------------------------------------------------------------------------------------------------
	//! Static method to get recruit data from entity
	static OVT_RecruitData GetRecruitDataFromEntity(IEntity entity)
	{
		if (!entity)
			return null;
			
		OVT_RecruitManagerComponent recruitManager = OVT_Global.GetRecruits();
		if (!recruitManager)
			return null;
			
		return recruitManager.GetRecruitFromEntity(entity);
	}
}