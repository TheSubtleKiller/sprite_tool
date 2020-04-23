
#include "spritesheet.hpp"

#include "tiny_xml/ticpp.h"

//========================================
CSpriteSheet::CSpriteSheet()
{

}

CSpriteSheet::~CSpriteSheet()
{

}
//========================================

//========================================
void CSpriteSheet::ParseCell(ticpp::Element* _pElemCell)
{
	try
	{
		SSpriteCell _Cell;

		_Cell.m_sName = _pElemCell->GetAttribute<std::string>("name");

		_pElemCell->GetAttribute("x", &_Cell.x);
		_pElemCell->GetAttribute("y", &_Cell.y);
		_pElemCell->GetAttribute("w", &_Cell.w);
		_pElemCell->GetAttribute("h", &_Cell.h);

		_pElemCell->GetAttribute("ax", &_Cell.ax, false);
		_pElemCell->GetAttribute("ay", &_Cell.ay, false);
		_pElemCell->GetAttribute("aw", &_Cell.aw, false);
		_pElemCell->GetAttribute("ah", &_Cell.ah, false);

		m_mapSpriteData[_Cell.m_sName] = _Cell;
	}
	catch (ticpp::Exception& error)
	{
		fprintf(stderr, "%s\n", error.m_details.c_str());
		assert(false);
	}
}

void CSpriteSheet::ParseAnimation(ticpp::Element* _pElemAnim)
{
	std::string _sAnimName;
	_pElemAnim->GetAttribute("name", &_sAnimName);

	// Loop through all cells of the animation
	ticpp::Element* _pAnimCell = _pElemAnim->FirstChildElement("Cell", false);
	while (_pAnimCell)
	{
		ParseCell(_pAnimCell);

		_pAnimCell = _pAnimCell->NextSiblingElement("Cell", false);
	}
}

void CSpriteSheet::ParseXML(std::string const& _sXML)
{
	try
	{
		ticpp::Document _Document = ticpp::Document();

		_Document.Parse(_sXML, true, TIXML_ENCODING_UTF8);

		// Get root element
		ticpp::Element *_pElemSpriteInfo = _Document.FirstChildElement("SpriteInformation");

		ticpp::Element* _pElemFrameInfo = _pElemSpriteInfo->FirstChildElement("FrameInformation");
		if (_pElemFrameInfo)
		{
			// Get texture info
			_pElemFrameInfo->GetAttribute("name", &m_sTexName);
			_pElemFrameInfo->GetAttribute("type", &m_sTexType);
			_pElemFrameInfo->GetAttribute("texw", &m_uTexWidth);
			_pElemFrameInfo->GetAttribute("texh", &m_uTexHeight);

			// Find the sprite cell infos
			ticpp::Element* _pChildElem = _pElemFrameInfo->FirstChildElement(false);
			while (_pChildElem)
			{
				if (_pChildElem->Value() == "Cell")
				{
					ParseCell(_pChildElem);
				}
				else if (_pChildElem->Value() == "Animation")
				{
					ParseAnimation(_pChildElem);
				}

				_pChildElem = _pChildElem->NextSiblingElement(false);
			}
		}
	}
	catch (ticpp::Exception& error)
	{
		fprintf(stderr, "%s\n", error.m_details.c_str());
		assert(false);
	}
}
//========================================