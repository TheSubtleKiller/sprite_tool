
#include <map>
#include <vector>
#include <memory>
#include <string>

#include "spritesheet.hpp"

// forward delcaration
class CCompoundSprite;

struct SActorInstance
{
	bool m_bShow = true;

	std::shared_ptr<CCompoundSprite> m_pCompound;
	uint32_t m_uActorId = 0;

	std::vector<SActorInstance> m_vectorActors;
};

//========================================
class CSpriteTool
{
public:
	int Run();

	void SetMouseScroll(double _dX, double _dY)
	{
		m_dMouseScrollX = _dX;
		m_dMouseScrollY = _dY;
	}

protected:

	bool OpenJSONFile(std::string const &_sPath);

	std::vector<SActorInstance> BuildActorInstances(std::shared_ptr<CCompoundSprite> & _pRootCompound);

	void LoadSpriteSheets(std::string const &_sParentFolder, std::vector<std::string> const& _vectorTextures, std::map<std::string, CSpriteSheet> &_mapSpriteSheets);
	void LoadTextures(std::string const& _sParentFolder, std::vector<std::string> const &_vectorTextures, std::map<std::string, uint32_t> &_mapTextureNameId);

	std::map<std::string, std::shared_ptr<CCompoundSprite>> m_mapCompounds;
	std::map<std::string, CSpriteSheet> m_mapSpriteSheets;

	std::map<std::string, uint32_t> m_mapTextureNameId;

	std::vector<SActorInstance> m_vectorActorInstances;

	double m_dMouseScrollX = 0.0;
	double m_dMouseScrollY = 0.0;

	float m_fViewPortScale = 1.0f;

	float m_fTime = 0.0f;

	bool m_bAnimate = true;
	float m_fAnimationSpeedMult = 1.0f;

	std::string m_sOpenFile;
};
//========================================
