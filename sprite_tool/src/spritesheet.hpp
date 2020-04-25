
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
public:
	struct SSpriteCell
	{
		std::string m_sName;
		uint32_t x = 0, y = 0;
		uint32_t w = 0, h = 0;
		uint32_t ax = 0, ay = 0;
		uint32_t aw = 0, ah = 0;

		float m_fMinX = 0.0f, m_fMinY = 0.0f;
		float m_fMaxX = 0.0f, m_fMaxY = 0.0f;

		void CalculateNormalisedValues(uint32_t const _uTexW, uint32_t const _uTexH)
		{
			m_fMinX = static_cast<float>(x) / static_cast<float>(_uTexW);
			m_fMinY = static_cast<float>(y) / static_cast<float>(_uTexH);
			m_fMaxX = static_cast<float>(x+w) / static_cast<float>(_uTexW);
			m_fMaxY = static_cast<float>(y+h) / static_cast<float>(_uTexH);
		}
	};

	CSpriteSheet();
	~CSpriteSheet();

	void ParseXML(std::string const &_sXML);

	std::map<std::string, SSpriteCell> const& GetSpriteData() const { return m_mapSpriteData; }

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