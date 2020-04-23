
#include <map>
#include <string>

// Forward declarations
namespace ticpp
{
	class Element;
};

//========================================
class CSpriteSheet
{
	struct SSpriteCell
	{
		std::string m_sName;
		uint32_t x = 0, y = 0;
		uint32_t w = 0, h = 0;
		uint32_t ax = 0, ay = 0;
		uint32_t aw = 0, ah = 0;
	};

public:
	CSpriteSheet();
	~CSpriteSheet();

	void ParseXML(std::string const &_sXML);

protected:
	void ParseCell(ticpp::Element* _pElemCell);
	void ParseAnimation(ticpp::Element* _pElemAnim);

	std::map<std::string, SSpriteCell> m_mapSpriteData;

	std::string m_sTexName;
	std::string m_sTexType;

	uint32_t m_uTexWidth;
	uint32_t m_uTexHeight;
};
//========================================