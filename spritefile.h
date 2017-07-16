#ifndef SPRITEFILE_H
#define SPRITEFILE_H
#include <frame.h>
#include <string>

class SpriteFile
{
	std::string m_name;
	bool m_isC16;
	int  m_bodyPart;

	std::vector<Frame > m_frames;

	SpriteFile(const std::string & name, FILE * file, int bodyPart, bool is565, bool isC16);
	void saveC16(FILE * file);
	void saveS16(FILE * file);

public:
	static SpriteFile * create(const std::string & path);

	void writeFile(const std::string & path);

	void mirrorFrames();
	void scaleFrames(float scale);
};

#endif // SPRITEFILE_H
