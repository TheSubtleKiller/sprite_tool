
#include "spritesheet.hpp"
#include "compound_sprite.hpp"

#include "GL/glew.h"

#include <array>

//========================================
namespace gl_render_helper
{
	void DrawSprite(CSpriteSheet::SSpriteCell const& _SpriteCell, 
					CCompoundSprite::SActorState const& _ActorState,
					uint32_t _uShaderProgram,
					uint32_t _uTexture)
	{
		struct SPosition
		{
			float xyz[3];
		};
		struct SColour
		{
			uint8_t rgba[4];
		};
		struct STexCoord
		{
			float uv[2];
		};

		SPosition _arrayPos[6] = 
		{
			{-1, -1, 0.0f},
			{1, -1, 0.0f},
			{1, 1, 0.0f},
			{1, 1, 0.0f},
			{-1, 1, 0.0f},
			{-1, -1, 0.0f},
		};

		SColour _arrayCol[6] =
		{
			{255, 255, 255, 255},
			{255, 255, 255, 255},
			{255, 255, 255, 255},
			{255, 255, 255, 255},
			{255, 255, 255, 255},
			{255, 255, 255, 255},
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