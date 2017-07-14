#include <QCoreApplication>
#include <QImage>
#include <QImageWriter>
#include <vector>
#include <cstdio>
#include <iostream>
#include <cstring>

struct image_header
{
	uint16_t width;
	uint16_t height;
	std::vector<uint32_t> offset;
};

QImage readC16frame(FILE * file, bool is555)
{
	uint16_t color;
	uint8_t red;
	uint8_t green;
	uint8_t blue;

	uint32_t first_offset;
	uint16_t width;
	uint16_t height;


	fread(&first_offset, 4, 1, file);
	fread(&width, 2, 1, file);
	fread(&height, 2, 1, file);

	std::vector<uint32_t> offsets;
	offsets.resize(height);
	offsets[0] = first_offset;
	fread(&offsets[1], 4, height-1, file);

	fpos_t position;
	fgetpos(file, &position);

	QImage image(width, height, QImage::Format_ARGB8565_Premultiplied);
	image.fill(0);

	for(int y = 0; y < height; ++y)
	{
		fseek(file, offsets[y], SEEK_SET);

		for(int x = 0; x < width; )
		{
			uint16_t length;
			fread(&length, 2, 1, file);

			if(length == 0)
				break;
//colored
			if((length & 0x01) == 0)
				x += length >> 1;
			else
			{
				length = x + (length >> 1);

				for(; x < length; ++x)
				{
					fread(&color, 2, 1, file);

					if(is555)
					{
						red = (color & 0x7C00) >> 7;
						green = (color & 0x03E0) >> 2;
						blue = (color & 0x001F) << 3;
					}
					else
					{
						red = (color & 0xF800) >> 8;
						green = (color & 0x07E0) >> 3;
						blue = (color & 0x001F) << 3;
					}

					image.setPixel(x, y, qRgb(red, green, blue));
				}
			}
		}
	}

	fsetpos(file, &position);
	return image;
}

image_header writeC16frame(const QImage & frame, FILE * file)
{
	uint16_t end_tag = 0;
	image_header r;
	r.width = frame.width();
	r.height = frame.height();

	for(int y = 0; y < frame.height(); ++y)
	{
		r.offset.push_back(ftell(file));

		for(int x = 0; x < frame.width(); )
		{
			if(!frame.pixel(x, y))
			{
				uint16_t length = 0;
				for(;x < frame.width() && !frame.pixel(x, y); ++x, ++length) ;
				length = length << 1;
				fwrite(&length, 2, 1, file);
			}
			else
			{
				uint16_t length = 0;
				for(;x+length < frame.width() && frame.pixel(x+length, y); ++length) ;
				length = length << 1;
				length |= 1;
				fwrite(&length, 2, 1, file);

				for(;x < frame.width() && frame.pixel(x, y); ++x)
				{
					QRgb color = frame.pixel(x,y);

					uint8_t red = qRed(color);
					uint8_t green = qGreen(color);
					uint8_t blue = qBlue(color);

					uint16_t result = ((red & 0xF8) << 8) | ((green & 0xFC) << 3)  | (blue >> 3);
					fwrite(&result, 2, 1, file);
				}
			}
		}

		fwrite(&end_tag, 2, 1, file);
	}

	fwrite(&end_tag, 2, 1, file);

	return r;
}

std::vector<QImage> loadC16File(FILE * file)
{
	std::vector<QImage> r;

	uint32_t header;
	uint16_t frames;

	fread(&header, 4, 1, file);
	fread(&frames, 2, 1, file);

	for(int i = 0; i < frames; ++i)
	{
		r.push_back(readC16frame(file, (header & 0x01) == 0));
	}

	return r;
}

void saveC16File(const std::vector<QImage> & frames, FILE * file)
{
	uint32_t header = 3;
	uint16_t length = frames.size();

	fwrite(&header, 4, 1, file);
	fwrite(&length, 2, 1, file);

	int header_size = 0;

	for(int i = 0; i < length; ++i)
	{
		header_size += 4 + frames[i].height()*4;
	}

	fseek(file, header_size+6, SEEK_SET);

	std::vector<image_header> headers;

	for(int i = 0; i < length; ++i)
	{
		headers.push_back(writeC16frame(frames[i], file));
	}

	fseek(file, 6, SEEK_SET);

	for(int i = 0; i < length; ++i)
	{
		fwrite(&(headers[i].offset[0]), 4, 1, file);
		fwrite(&(headers[i].width), 2, 1, file);
		fwrite(&(headers[i].height), 2, 1, file);
		fwrite(&(headers[i].offset[1]), 4, headers[i].offset.size()-1, file);
	}
}

bool isBlank(const QImage & image)
{
	for(int y = 0; y < image.height(); ++y)
	{
		for(int x = 0; x < image.width(); ++x)
		{
			if(image.pixel(x, y))
				return false;
		}
	}

	return true;
}

void scaleFrames(std::vector<QImage> & frames, float factor)
{
	if(factor == 1) return;

	for(uint16_t i = 0; i < frames.size(); ++i)
	{
		frames[i] = frames[i].scaled(
			frames[i].width() * factor,
			frames[i].height()*factor,
			Qt::KeepAspectRatio,
			Qt::SmoothTransformation);
	}
}

void mirrorFrames(std::vector<QImage> & frames)
{
	for(uint16_t i = 0; (i+8) <= (int)frames.size(); i += 16)
	{
		for(int j = 4; j < 8; ++j)
		{
			if(isBlank(frames[i+j]))
				frames[i+j] = frames[(i+j) - 4].mirrored(true, false);
		}
	}
}


int main(int argc, char *argv[])
{
	float scale = .5;
	if(argc < 4)	return 0;

	scale = atof(argv[1]);

	FILE * test_file = fopen(argv[2], "rb");
	if(!test_file)
	{
		std::cerr << argv[2] << ": " << strerror(errno) << std::endl;
		return 0;
	}
	std::vector<QImage> test = loadC16File(test_file);
	fclose(test_file);

	test_file = fopen(argv[3], "wb");
	if(!test_file)
	{
		std::cerr << argv[3] << ": " << strerror(errno) << std::endl;
		return 0;
	}

	scaleFrames(test, scale);
	mirrorFrames(test);

	saveC16File(test, test_file);
	fclose(test_file);
	return 0;
}
