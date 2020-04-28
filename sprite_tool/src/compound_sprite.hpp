
#pragma once

#include <map>
#include <set>
#include <vector>
#include <string>

//========================================
class CCompoundSprite
{
public:

	struct SActorState
	{
		uint32_t m_uAlignmentX = 0;
		uint32_t m_uAlignmentY = 0;

		float m_fAlpha = 0.0f;
		float m_fAngle = 0.0f;

		union {
			uint32_t m_uColour = 0xFFFFFFFF;
			uint8_t m_RGBA[4];
		};

		uint32_t m_uFlip = 0;

		float m_fPosX = 0.0f;
		float m_fPosY = 0.0f;

		float m_fScaleX = 0.0f;
		float m_fScaleY = 0.0f;

		bool m_bShown = true;
	};

	struct SActor
	{
		SActorState m_State;

		std::string m_sSprite;

		uint32_t m_uType = 1;	// 1 : sprite, 2 : compound-sprite

		uint32_t m_uID = 0;
	};

	struct STimelineFrame
	{
		SActorState m_State;

		float m_fTime = 0.0f;
	};

	void ParseJSON(std::string const& _sJSON);

	std::map<uint32_t, SActor> const& GetActors() const { return m_mapActors; }
	std::map<std::string, std::set<std::string>> const& GetTextureSprites() const { return m_mapTextureSprites; }
	std::map<uint32_t, std::vector<STimelineFrame>> const& GetTimelines() const { return m_mapTimelineStates; }

protected:

	uint32_t m_uAlignmentX = 0;
	uint32_t m_uAlignmentY = 0;

	float m_fPointX = 0.0f;
	float m_fPointY = 0.0f;

	std::map<uint32_t, SActor> m_mapActors;
	std::map<std::string, std::set<std::string>> m_mapTextureSprites;
	std::map<uint32_t, std::vector<STimelineFrame>> m_mapTimelineStates;

	float m_fStageLength = 0.0f;
	int32_t m_iVersion = 0;
};
//========================================