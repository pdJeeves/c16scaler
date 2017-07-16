#include "frame.h"
#include <cstring>

Frame::Frame() :
	width(0),
	height(0)
{
}

void Frame::to565()
{
	for(uint32_t i = 0; i < pixels.size(); ++i)
	{
		pixels[i] = ((pixels[i] & 0xFFE0) << 1) | (pixels[i] & 0x001F);
	}
}

bool Frame::isBlank() const
{
	for(uint32_t i = 0; i < pixels.size(); ++i)
	{
		if(pixels[i])
			return false;
	}

	return true;
}

bool Frame::operator==(const Frame & frame) const
{
	if(frame.width  != width
	|| frame.height != height)
		return false;

	return 0 == memcmp(&pixels[0], &frame.pixels[0], pixels.size()*2);
}


Frame Frame::getMirror() const
{
	Frame r;
	r.width = width;
	r.height = height;
	r.pixels.resize(width*height);

	for(uint16_t y = 0; y < height; ++y)
	{
		for(uint16_t x = 0; x < width; ++x)
		{
			r.pixels[y*height + ((width-1) - x)] = pixels[y*height + x];
		}
	}

	return r;
}

uint16_t Frame::getColor(float min_x, float min_y, float max_x, float max_y) const
{
	float red		  = 0;
	float green		  = 0;
	float blue		  = 0;
	float denominator = 0;

	if((int)min_y == (int)max_y
	&& (int)min_x == (int)max_x)
	{
		return pixels[((int)min_y)*height + (int)min_x];
	}

	for(int y = min_y; y < max_y; ++y)
	{
		for(int x = min_x; x < max_x; ++x)
		{
			float weight = 1;
			uint16_t px = pixels[y*height + x];

			if(px == 0)
				continue;

			if(x < min_x)
				weight *= 1 - (min_x - x);
			else if(x+1 > max_x)
				weight *= (max_x - x);

			if(x < min_y)
				weight *= 1 - (min_y - y);
			else if(x+1 > max_y)
				weight *= (max_y - y);


			red   += ((px & 0xF800) >> 8)*weight;
			green += ((px & 0x07E0) >> 3)*weight;
			blue  += ((px & 0x001F) << 3)*weight;
			denominator += weight;
		}
	}

	if(!denominator)
		return 0;

	red   /= denominator;
	green /= denominator;
	blue  /= denominator;

	return (((int)red   & 0xF8) << 8)
		|  (((int)green & 0xFC) << 3)
		|  (((int)blue  & 0xF8) >> 3);
}

Frame Frame::getScaled(float scale) const
{
	Frame r;
	r.width	 = width *scale + .5;
	r.height = height*scale + .5;
	r.pixels.resize(r.width*r.height, 0);

	for(uint16_t y = 0; y < r.height; ++y)
	{
		const float min_y =  y   *width / (double)r.height;
		const float max_y = (y+1)*width / (double)r.height;

		for(uint16_t x = 0; x < r.width; ++x)
		{
			const float min_x =  x   *width / (double)r.width;
			const float max_x = (x+1)*width / (double)r.width;

			r.pixels[y*r.width+x] = getColor(min_x, min_y, max_x, max_y);
		}
	}


	return r;
}

void Frame::readS16(FILE * file, bool is565)
{
	uint32_t offset;
	fread(&offset, 4, 1, file);
	fread(&width, 2, 1, file);
	fread(&height, 2, 1, file);

	fpos_t position;
	fgetpos(file, &position);
	fseek(file, offset, SEEK_SET);

	pixels.resize(width*height);
	fread(&pixels[0], 2, pixels.size(), file);

	fsetpos(file, &position);

	if(!is565)
		to565();
}

void Frame::readC16(FILE * file, bool is565)
{
	offset.resize(1);
	fread(&offset[0], 4, 1, file);
	fread(&width, 2, 1, file);
	fread(&height, 2, 1, file);
	offset.resize(height);
	fread(&offset[1], 4, offset.size()-1, file);

	pixels.resize(width*height, 0);

	fpos_t position;
	fgetpos(file, &position);

	for(uint16_t y = 0; y < height; ++y)
	{
		fseek(file, offset[y], SEEK_SET);

		for(uint16_t x = 0; x < width; )
		{
			uint16_t length;
			fread(&length, 2, 1, file);

			if(length == 0)
				break;

			if(length & 0x01)
				fread(&pixels[y*width+x], 1, length-1, file);

			x += (length >> 1);
		}
	}

	fsetpos(file, &position);

	if(!is565)
		to565();

	offset.clear();
}

void Frame::writeHeader(FILE * file)
{
	if(offset.empty())
		return;

	fwrite(&(offset[0]), 4, 1, file);
	fwrite(&(width),	 2, 1, file);
	fwrite(&(height),	 2, 1, file);

	if(offset.size() != 1)
		fwrite(&(offset[1]), 4, offset.size()-1, file);

	offset.clear();
}

void Frame::writeS16(FILE * file)
{
	offset.push_back(ftell(file));
	fwrite(&pixels[0], 2, pixels.size(), file);
}

void Frame::writeC16(FILE * file)
{
	offset.clear();

	uint16_t end_tag = 0;

	for(uint16_t y = 0; y < height; ++y)
	{
		offset.push_back(ftell(file));

		for(int x = 0; x < width; )
		{
			uint16_t length = 0;

			bool transparent = !pixels[y*width + x];
			for(;x+length < width && transparent == !pixels[y*width + x+length]; ++length) ;

			if(transparent)
			{
				x += length;
				length <<= 1;
				fwrite(&length, 2, 1, file);
			}
			else
			{
				length = (length << 1) | 1;
				fwrite(&length, 2, 1, file);
				fwrite(&pixels[y*width + x], 1, length-1, file);
				x += (length >> 1);
			}
		}

		fwrite(&end_tag, 2, 1, file);
	}

	fwrite(&end_tag, 2, 1, file);

}
