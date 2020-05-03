
#include "compound_sprite.hpp"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <iostream>
#include <string>

//========================================
CCompoundSprite::SActorState ParseActorState(rapidjson::Value const &_root)
{
    CCompoundSprite::SActorState _ActorState;

    using namespace rapidjson;

    Value const& _valAlignment = _root["Alignment"];
    if (_valAlignment.IsArray() && _valAlignment.Size() == 2)
    {
        _ActorState.m_uAlignmentX = _valAlignment[0].GetUint();
        _ActorState.m_uAlignmentY = _valAlignment[1].GetUint();
    }

    _ActorState.m_fAlpha = _root["Alpha"].GetFloat();
    _ActorState.m_fAngle = _root["Angle"].GetFloat();

    _ActorState.m_uColour = _root["Colour"].GetUint();

    _ActorState.m_uFlip = _root["Flip"].GetUint();

    Value const& _valPos = _root["Position"];
    if (_valPos.IsArray() && _valPos.Size() == 2)
    {
        _ActorState.m_fPosX = _valPos[0].GetFloat();
        _ActorState.m_fPosY = _valPos[1].GetFloat();
    }

    Value const& _valScale = _root["Scale"];
    if (_valScale.IsArray() && _valScale.Size() == 2)
    {
        _ActorState.m_fScaleX = _valScale[0].GetFloat();
        _ActorState.m_fScaleY = _valScale[1].GetFloat();
    }

    _ActorState.m_bShown = _root["Shown"].GetBool();

    return _ActorState;
}

void CCompoundSprite::ParseJSON(std::string const& _sJSON)
{
    assert(_sJSON.empty() == false);

    using namespace rapidjson;

    Document _doc;
    _doc.Parse(_sJSON.c_str());

    // Read alignment
    Value& _valAlignment = _doc["Alignment"];
    if (_valAlignment.IsArray() && _valAlignment.Size() == 2)
    {
        m_uAlignmentX = _valAlignment[0].GetUint();
        m_uAlignmentY = _valAlignment[1].GetUint();
    }

    // Read Point
    Value& _valPoint = _doc["Point"];
    if (_valPoint.IsArray() && _valPoint.Size() == 2)
    {
        m_fPointX = _valPoint[0].GetFloat();
        m_fPointY = _valPoint[1].GetFloat();
    }

    //---------- Read stage options
    Value& _valStageOptions = _doc["stageOptions"];
    if (_valStageOptions.IsObject())
    {
        m_fStageLength = _valStageOptions["StageLength"].GetFloat();
        m_iVersion = _valStageOptions["Version"].GetInt();

        Value& _valSpriteInfo = _valStageOptions["SpriteInfo"];

        if (_valSpriteInfo.IsArray())
        {
            for (Value::ConstValueIterator _it = _valSpriteInfo.Begin(); _it != _valSpriteInfo.End(); ++_it)
            {
                std::string _sSprite = (*_it)["SpriteInfo"].GetString();
                std::string _sTexture = (*_it)["Texture"].GetString();

                m_mapTextureSprites[_sTexture].insert(_sSprite);

                //std::cout << "Sprite: " << _sSprite << ", Texture: " << _sTexture << std::endl;
            }
        }
    }

    //---------- Read actors
    Value& _valActors = _doc["actors"];
    if (_valActors.IsArray())
    {
        for (Value::ConstValueIterator _it = _valActors.Begin(); _it != _valActors.End(); ++_it)
        {
            Value const& _valActor = (*_it);

            SActor _Actor;

            _Actor.m_State = ParseActorState(_valActor);

            _Actor.m_sSprite = _valActor["sprite"].GetString();
            _Actor.m_uType = _valActor["type"].GetUint();
            _Actor.m_uID = _valActor["uid"].GetUint();

            m_mapActors[_Actor.m_uID] = _Actor;
        }
    }

    //---------- Read timelines
    Value& _valTimelines = _doc["timelines"];
    if (_valTimelines.IsArray())
    {
        for (Value::ConstValueIterator _itTimeline = _valTimelines.Begin(); _itTimeline != _valTimelines.End(); ++_itTimeline)
        {
            Value const& _valTimeline = (*_itTimeline);

            uint32_t _uActorId = _valTimeline["spriteuid"].GetUint();

            std::vector<STimelineFrame> _vectorFrames;

            Value const& _valStage = _valTimeline["stage"];
            if (_valStage.IsArray())
            {
                for (Value::ConstValueIterator _itFrame = _valStage.Begin(); _itFrame != _valStage.End(); ++_itFrame)
                {
                    Value const& _valFrame = (*_itFrame);

                    STimelineFrame _Frame;

                    _Frame.m_State = ParseActorState(_valFrame);

                    _Frame.m_fTime = _valFrame["Time"].GetFloat();

                    _vectorFrames.emplace_back(_Frame);
                }
            }

            m_mapTimelineStates[_uActorId] = _vectorFrames;
        }
    }

    return;
}
//========================================