#include <vector>
#include <cstdio>
#include <iostream>
#include <cstring>
#include "directoryrange.h"
#include "spritefile.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

#define UNUSED(x) (void)x

bool isValidDirectory(const std::string & string)
{
	if(string.empty())
		return false;

	DIR * dp = opendir(string.c_str());
	if(!dp)
	{
		std::cerr << "unable to open directory: " << string << std::endl;
		std::cerr << "\t" << strerror(errno) << std::endl;
		return false;
	}
	closedir(dp);

	return true;
}

int main(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	float scale = 1;
	std::string path;
	std::string outDir;
	std::string extension;

	bool printedSizeTable = false;

#if 1
	for(;;)
	{
		std::cin.clear();
		std::cout << "enter scaling factor > ";
		std::cin >> scale;

		if(0 < scale && scale <= 2)
			break;

		std::cout << "enter a floating point number between (0,2]." << std::endl;

		if(!printedSizeTable)
		{
			printedSizeTable = true;
			std::cout << "Creatures Adventures are about 2x the size of Creature 3 sprites\n" << std::endl;

			std::cout << "life stage size table: \n"
			"       0      1      2      3      4      5\n"
			"0  1.000  1.067  1.200  1.400  1.600  1.600\n"
			"1  0.938  1.000  1.125  1.313  1.500  1.500\n"
			"2  0.833  0.889  1.000  1.167  1.333  1.333\n"
			"3  0.714  0.762  0.857  1.000  1.143  1.143\n"
			"4  0.625  0.667  0.750  0.875  1.000  1.000\n"
			"5  0.625  0.667  0.750  0.875  1.000  1.000\n" << std::endl;
		}
	}

	do {
		std::cin.clear();
		std::cout << "enter an input directory > ";
		std::cin >> path;
	} while(!isValidDirectory(path));

	do {
		std::cin.clear();
		std::cout << "enter an output directory > ";
		std::cin >> outDir;

		if(path == outDir)
		{
			std::cout << "the input directory must be different from the output directory" << std::endl;
		}
	} while(!isValidDirectory(outDir));

	do {
		std::cin.clear();
		std::cout << "output c16 or s16? ";
		std::cin >> extension;
	} while(extension != "s16" && extension != "c16");
#endif


	if(outDir[outDir.size()-1] != SLASH_CHAR)
	{
		outDir.push_back(SLASH_CHAR);
	}

	for(FileTypesRange<DirectoryRange> directory(path, {"s16", "c16"}); !directory.empty(); directory.popFront())
	{
		SpriteFile * sprite = SpriteFile::create(directory.syspath());

		if(!sprite)
		{
			std::cerr << "failed to open sprite" << std::endl;
			continue;
		}

		sprite->scaleFrames(scale);
		sprite->mirrorFrames();

		sprite->writeFile(outDir + directory.filename() + "." + extension);

		delete sprite;
	}

	return 0;
}
