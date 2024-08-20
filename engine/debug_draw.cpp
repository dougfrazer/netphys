#include "debug_draw.h"

#include <fstream>
#include <string>
#include <windows.h>
#include <gl\gl.h>
#include <assert.h>
#include <vector>

static constexpr int WIDTH_DATA_OFFSET = 20; // Offset to width data with BFF file
static constexpr int WIDTH_DATA_SIZE = 256;
static constexpr int MAP_DATA_OFFSET = WIDTH_DATA_OFFSET + WIDTH_DATA_SIZE; // Offset to texture image data with BFF file

enum TwoDimensions
{
	X,
	Y,
	NUM_DIMENSIONS_2D,
};

static char s_startingGlyphChar;
static char s_glyphsWidths[WIDTH_DATA_SIZE]; // precise width of a specific glyph
static int s_bitmapSize[NUM_DIMENSIONS_2D];
static int s_glyphSize[NUM_DIMENSIONS_2D]; // maximum width of each glyph
static int s_bitsPerPixel = 0;
static GLuint s_texID;

// some math we can do upfront and cache
static int s_rowSize;
static float s_rowToV;
static float s_colToU;

#ifndef GL_CLAMP_TO_EDGE 
#define GL_CLAMP_TO_EDGE 0x812F 
#endif

static int s_stringIdentifier = 1;

struct DebugDrawString
{
	int identifier;
	int x;
	int y;
	DebugDrawColor color;
	char string[256];
};
static std::vector<DebugDrawString> s_strings;

void InitTextures()
{
	std::fstream in;
	in.open("TestFont.bff", std::ios_base::binary | std::ios_base::in);

	assert(!in.fail());

	// Get Filesize
	in.seekg(0, std::ios_base::end);
	int fileSize = (int)in.tellg();
	in.seekg(0, std::ios_base::beg);

	// allocate space for file data
	uint8_t *fileData = new uint8_t[fileSize];

	assert(fileData);

	in.read((char*)fileData, fileSize);

	assert(!in.fail());

	in.close();

	assert(fileData[0] == 0xBF && fileData[1] == 0xF2);

	s_bitmapSize[X] = *(int*)(&fileData[2]);
	s_bitmapSize[Y] = *(int*)(&fileData[6]);
	s_glyphSize[X] = *(int*)(&fileData[10]);
	s_glyphSize[Y] = *(int*)(&fileData[14]);
	s_bitsPerPixel = (int)(fileData[18]);
	assert(s_bitsPerPixel == 32); // could support others but just have the one for now
	s_startingGlyphChar = fileData[19];

	s_rowSize = s_bitmapSize[X] / s_glyphSize[X];
	s_rowToV = (float)s_glyphSize[Y] / (float)s_bitmapSize[Y];
	s_colToU = (float)s_glyphSize[X] / (float)s_bitmapSize[X];

	const int bitmapPixels = s_bitmapSize[X] * s_bitmapSize[Y];
	const int textureSize = bitmapPixels * (s_bitsPerPixel / 8);
	uint8_t* bitmapTexture = new uint8_t[textureSize];
	assert(bitmapTexture);


	memcpy(s_glyphsWidths, &fileData[WIDTH_DATA_OFFSET], WIDTH_DATA_SIZE);
	memcpy(bitmapTexture, &fileData[MAP_DATA_OFFSET], textureSize);

	// Create Texture
	glGenTextures(1, &s_texID);
	glBindTexture(GL_TEXTURE_2D, s_texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// Load it up on the GPU
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s_bitmapSize[X], s_bitmapSize[Y], 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmapTexture);

	delete [] fileData;
	delete [] bitmapTexture;
}

int DebugDraw_AddString(const char* string, int x, int y, DebugDrawColor color)
{
	s_strings.emplace_back();
	auto& newString = s_strings.back();

	int newIdentifier = ++s_stringIdentifier;
	newString.identifier = newIdentifier;
	newString.x = x;
	newString.y = y;
	newString.color = color;
	strcpy_s(newString.string, 256, string);

	return newIdentifier;
}
void DebugDraw_RemoveString(int identifier)
{
	for (auto it = s_strings.begin(); it != s_strings.end(); ++it)
	{
		if (it->identifier == identifier)
		{
			s_strings.erase(it);
			return;
		}
	}
}
void DebugDraw_RemoveAll()
{
	s_strings.clear();
}
void DebugDraw_Render()
{
	// make sure we called InitTextures() first
	if (!s_texID)
	{
		InitTextures();
	}
	
	if(!s_texID)
		return;

	GLint ViewPort[4];

	glGetIntegerv(GL_VIEWPORT, ViewPort);
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, ViewPort[2], 0, ViewPort[3], -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s_texID);

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);

	for (const auto& string : s_strings)
	{
		glColor3f(string.color.r, string.color.g, string.color.b);

		int currX = string.x;

		const char* text = string.string;
		int stringLength = (int)strnlen(text, 256);

		glBegin(GL_QUADS);

		for (int charIndex = 0; charIndex != stringLength; ++charIndex)
		{
			const char c = text[charIndex];
			const int row = (c - s_startingGlyphChar) / s_rowSize;
			const int col = (c - s_startingGlyphChar) - row * s_rowSize;

			const float U = col * s_colToU;
			const float V = row * s_rowToV;
			const float U1 = U + s_colToU;
			const float V1 = V + s_rowToV;

			glTexCoord2f(U, V1);   glVertex2i(currX,                  string.y                 );
			glTexCoord2f(U1, V1);  glVertex2i(currX + s_glyphSize[X], string.y                 );
			glTexCoord2f(U1, V);   glVertex2i(currX + s_glyphSize[X], string.y + s_glyphSize[Y]);
			glTexCoord2f(U, V);    glVertex2i(currX,                  string.y + s_glyphSize[Y]);

			currX += s_glyphsWidths[c];
		}

		glEnd();
	}

	// Restore previous state
	glPopAttrib();
}

