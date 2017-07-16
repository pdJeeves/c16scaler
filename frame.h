#ifndef FRAME_H
#define FRAME_H
#include <vector>
#include <cstdint>
#include <cstdio>

class Frame
{
	void to565();

	uint16_t getColor(float min_x, float min_y, float max_x, float max_y) const;

public:
	uint16_t width;
	uint16_t height;
	std::vector<uint32_t> offset;
	std::vector<uint16_t> pixels;

	Frame();

	bool operator==(const Frame & frame) const;

	bool isBlank() const;
	Frame getMirror() const;
	Frame getScaled(float scale) const;

	void readC16(FILE *, bool is565);
	void readS16(FILE *, bool is565);
	void writeC16(FILE *);
	void writeS16(FILE *file);

	void writeHeader(FILE *file);
};

#endif // FRAME_H
