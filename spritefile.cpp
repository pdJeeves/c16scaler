#include "spritefile.h"
#include <cctype>
#include <iostream>
#include <cstring>

#ifdef _WIN32
#define SlashStr "\\"
#define SlashChar '\\'
#else
#define SlashStr "/"
#define SlashChar '/'
#endif

SpriteFile * SpriteFile::create(const std::string & path)
{
	if(path.size() < 5)
	{
		std::cerr << "given input path '" << path << "', which is too short to be a sprite file" << std::endl;
		return 0L;
	}

	size_t dotPos   = path.find_last_of('.');
	size_t slashPos = path.find_last_of(SlashChar);

	if(slashPos == std::string::npos)
		slashPos = 0;
	else
		++slashPos;


	std::string extension = path.substr(dotPos);
	bool isC16 = (tolower(extension[1]) == 'c');

	FILE * file = fopen(path.c_str(), "rb");
	if(!file)
	{
		std::cerr << "error opening file: " << path << std::endl;
		std::cerr << "\t" <<  strerror(errno) << std::endl;
		return 0L;
	}

	uint32_t header;
	fread(&header, 4, 1, file);
	bool is565 = header & 0x01;

	if(isC16 == !(header & 0x02)
	|| (header & 0xFFFFFFFC))
	{
		std::cerr << "input file '" << path << "''s header does not match a c16 or s16 file." << std::endl;
		fclose(file);
		return 0L;
	}

	int bodyPart = 0;

	if(path.size() >= 8
	&& 'a' <= tolower(path[path.size()-5]) && tolower(path[path.size()-5]) <= 'z'
	&& '0' <= tolower(path[path.size()-6]) && tolower(path[path.size()-6]) <= '9'
	&& '0' <= tolower(path[path.size()-7]) && tolower(path[path.size()-7]) <= '9'
	&& 'a' <= tolower(path[path.size()-8]) && tolower(path[path.size()-8]) <= 'z')
	{
		bodyPart = tolower(path[path.size()-8]);
	}

	SpriteFile * r = new SpriteFile(path.substr(slashPos, dotPos), file, bodyPart, is565, isC16);
	fclose(file);
	return r;
}

void SpriteFile::writeFile(const std::string & path)
{
	FILE * file = fopen(path.c_str(), "wb");
	if(!file)
	{
		std::cerr << "unable to open: " << path << std::endl;
		std::cerr << "\n" << strerror(errno) << std::endl;
		return;
	}

	if(tolower(path[path.size()-3]) == 'c')
		saveC16(file);
	else
		saveS16(file);

	fclose(file);
}

void SpriteFile::mirrorFrames()
{
	if(m_bodyPart == 0)
		return;

	int N = m_isC16? 16 : 10;
	for(uint32_t i = 0; i < m_frames.size(); i += N)
	{
		for(int j = 4; j < 8; ++j)
		{
			if(m_frames[i+j].isBlank())
			{
				m_frames[i+j] = m_frames[i+j-4].getMirror();
			}
		}
	}
}

void SpriteFile::scaleFrames(float scale)
{
	if(scale == 1)
		return;

	for(uint32_t i = 0; i < m_frames.size(); ++i)
	{
		m_frames[i] = m_frames[i].getScaled(scale);
	}
}


SpriteFile::SpriteFile(const std::string & name, FILE * file, int bodyPart, bool is565, bool isC16) :
	m_name(name),
	m_isC16(isC16),
	m_bodyPart(bodyPart)
{
	uint16_t frames;
	fread(&frames, 2, 1, file);

	m_frames.resize(frames);

	for(uint32_t i = 0; i < m_frames.size(); ++i)
	{
		if(isC16)
			m_frames[i].readC16(file, is565);
		else
			m_frames[i].readS16(file, is565);
	}

}

void SpriteFile::saveC16(FILE * file)
{
	uint32_t header = 3;
	uint16_t length = m_frames.size();

	int header_size = 0;

	if(!m_isC16 && m_bodyPart)
	{
		length = length*16/10;

		for(int i = m_frames.size()-1; i >= 0; --i)
		{
			int size = 4 + m_frames[i].height*4;
			header_size += (i % 10) < 8? size : size*4;

			if((i % 10) >= 8)
			{
				m_frames.insert(m_frames.begin()+i, m_frames[i]);
				m_frames.insert(m_frames.begin()+i, m_frames[i]);
				m_frames.insert(m_frames.begin()+i, m_frames[i]);
			}
		}
	}
	else
	{
		for(uint32_t i = 0; i < m_frames.size(); ++i)
		{
			header_size += 4 + m_frames[i].height*4;
		}
	}

	fwrite(&header, 4, 1, file);
	fwrite(&length, 2, 1, file);
	fseek(file, header_size+6, SEEK_SET);

	for(uint16_t i = 0; i < m_frames.size(); ++i)
	{
		m_frames[i].writeC16(file);
	}

	fseek(file, 6, SEEK_SET);

	for(uint32_t i = 0; i < m_frames.size(); ++i)
	{
		m_frames[i].writeHeader(file);
	}

	m_isC16 = true;
}

void SpriteFile::saveS16(FILE * file)
{
	uint32_t header = 1;
	uint16_t length = m_frames.size();

	if(m_isC16 && m_bodyPart == 0)
	{
		length = length*10/16;
	}

	int header_size = length*8;

	fwrite(&header, 4, 1, file);
	fwrite(&length, 2, 1, file);
	fseek(file, header_size+6, SEEK_SET);

	if(m_isC16 && m_bodyPart)
	{
		for(uint16_t i = 0; i < m_frames.size(); ++i)
		{
			switch(i % 16)
			{
			case 0: case 1: case 2: case 3:
			case 4: case 5: case 6: case 7:
			case 9: case 13:
				m_frames[i].writeS16(file);
				break;
			default:
				break;
			}
		}
	}
	else
	{
		for(uint32_t i = 0; i < m_frames.size(); ++i)
			m_frames[i].writeS16(file);
	}

	fseek(file, 6, SEEK_SET);

	for(uint32_t i = 0; i < m_frames.size(); ++i)
	{
		m_frames[i].writeHeader(file);
	}
}

