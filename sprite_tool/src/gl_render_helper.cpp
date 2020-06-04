
#include "spritesheet.hpp"
#include "compound_sprite.hpp"

#define GLEW_STATIC
#include "GL/glew.h"

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective

#include <array>

//======================================== 
namespace gl_render_helper
{
	void DrawSprite(glm::mat4 &_matModelView,
					CSpriteSheet::SSpriteCell const& _SpriteCell, 
					CCompoundSprite::SActorState const& _ActorState,
					uint32_t _uShaderProgram,
					uint32_t _uTexture)
	{
		if (_ActorState.m_bShown == false)
		{
			return;
		}

		struct SPosition
		{
			float xyz[3];
		};
		struct SColour
		{
			uint32_t colour;
		};
		struct STexCoord
		{
			float uv[2];
		};

		float _fHalfW = static_cast<float>(_SpriteCell.w) * 0.5f;
		float _fHalfH = static_cast<float>(_SpriteCell.h) * 0.5f;

		float _fMinX = 0.0f;
		float _fMinY = 0.0f;
		float _fMaxX = static_cast<float>(_SpriteCell.w);
		float _fMaxY = static_cast<float>(_SpriteCell.h);

		switch (static_cast<CCompoundSprite::Alignment>(_ActorState.m_uAlignmentX))
		{
			case CCompoundSprite::Alignment::Centre:
			{
				_fMinX = -_fHalfW;
				_fMaxX = _fHalfW;
				break;
			}
			case CCompoundSprite::Alignment::Left:
			{
				break;
			}
			case CCompoundSprite::Alignment::Right:
			{
				_fMinX = -static_cast<float>(_SpriteCell.w);
				_fMaxX = 0.0f;
				break;
			}
			default:
				break;
		}

		switch (static_cast<CCompoundSprite::Alignment>(_ActorState.m_uAlignmentY))
		{
			case CCompoundSprite::Alignment::Centre:
			{
				_fMinY = -_fHalfH;
				_fMaxY = _fHalfH;
				break;
			}
			case CCompoundSprite::Alignment::Top:
			{
				break;
			}
			case CCompoundSprite::Alignment::Bottom:
			{
				_fMinY = -static_cast<float>(_SpriteCell.h);
				_fMaxY = 0.0f;
				break;
			}
			default:
				break;
		}

		float _fScaleX = _ActorState.m_fScaleX * _SpriteCell.m_fTextureScale;
		float _fScaleY = _ActorState.m_fScaleY * _SpriteCell.m_fTextureScale;
		if (_ActorState.m_uFlip & static_cast<uint32_t>(CCompoundSprite::Flip::FlipX))
		{
			_fScaleX *= -1;
		}
		if (_ActorState.m_uFlip & static_cast<uint32_t>(CCompoundSprite::Flip::FlipY))
		{
			_fScaleY *= -1;
		}

		_fMinX *= _fScaleX;
		_fMinY *= _fScaleY;
		_fMaxX *= _fScaleX;
		_fMaxY *= _fScaleY;

		_fMinX += _ActorState.m_fPosX;
		_fMinY += _ActorState.m_fPosY;
		_fMaxX += _ActorState.m_fPosX;
		_fMaxY += _ActorState.m_fPosY;

		glm::vec4 _vec4Min = _matModelView * glm::vec4(_fMinX, _fMinY, 0.0f, 1.0f);
		glm::vec4 _vec4Max = _matModelView * glm::vec4(_fMaxX, _fMaxY, 0.0f, 1.0f);

		SPosition _arrayPos[6] =
		{
			{_vec4Min.x, _vec4Min.y, 0.0f},
			{_vec4Max.x, _vec4Min.y, 0.0f},
			{_vec4Max.x, _vec4Max.y, 0.0f},
			{_vec4Max.x, _vec4Max.y, 0.0f},
			{_vec4Min.x, _vec4Max.y, 0.0f},
			{_vec4Min.x, _vec4Min.y, 0.0f},
		};

		SColour _arrayCol[6] =
		{
			_ActorState.m_uColour,
			_ActorState.m_uColour,
			_ActorState.m_uColour,
			_ActorState.m_uColour,
			_ActorState.m_uColour,
			_ActorState.m_uColour,
		};

		STexCoord _arrayUV[6] =
		{
			{_SpriteCell.m_fMinX, _SpriteCell.m_fMinY},
			{_SpriteCell.m_fMaxX, _SpriteCell.m_fMinY},
			{_SpriteCell.m_fMaxX, _SpriteCell.m_fMaxY},
			{_SpriteCell.m_fMaxX, _SpriteCell.m_fMaxY},
			{_SpriteCell.m_fMinX, _SpriteCell.m_fMaxY},
			{_SpriteCell.m_fMinX, _SpriteCell.m_fMinY},
		};

		GLint vpos_location = glGetAttribLocation(_uShaderProgram, "vPos");
		GLint vcol_location = glGetAttribLocation(_uShaderProgram, "vCol");
		GLint uv_location = glGetAttribLocation(_uShaderProgram, "uv");

		GLuint buffer_pos, buffer_col, buffer_uv;

		//---------- pos
		glGenBuffers(1, &buffer_pos);
		glBindBuffer(GL_ARRAY_BUFFER, buffer_pos);
		glBufferData(GL_ARRAY_BUFFER, sizeof(SPosition) * 6, _arrayPos, GL_STATIC_DRAW);

		glEnableVertexAttribArray(vpos_location);
		glVertexAttribPointer(vpos_location, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		//---------- colour
		glGenBuffers(1, &buffer_col);
		glBindBuffer(GL_ARRAY_BUFFER, buffer_col);
		glBufferData(GL_ARRAY_BUFFER, sizeof(SColour) * 6, _arrayCol, GL_STATIC_DRAW);

		glEnableVertexAttribArray(vcol_location);
		glVertexAttribPointer(vcol_location, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);

		//---------- uv
		glGenBuffers(1, &buffer_uv);
		glBindBuffer(GL_ARRAY_BUFFER, buffer_uv);
		glBufferData(GL_ARRAY_BUFFER, sizeof(STexCoord) * 6, _arrayUV, GL_STATIC_DRAW);

		glEnableVertexAttribArray(uv_location);
		glVertexAttribPointer(uv_location, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		//---------- bind texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _uTexture);

		//---------- draw time
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDeleteBuffers(1, &buffer_pos);
		glDeleteBuffers(1, &buffer_col);
		glDeleteBuffers(1, &buffer_uv);
	}
};
//========================================