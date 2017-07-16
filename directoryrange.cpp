#include "directoryrange.h"
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

DirectoryRange::DirectoryRange(const std::string & m_path, const std::string & file) :
	_path(m_path + "/" + file)
{
	if(_path[_path.size()-1] == '\\' || _path[_path.size()-1] == '/')
	{
		_path.resize(_path.size()-1);
	}

	_path += "/" + file;

	auto str = _path + SLASH + file;
    dp = opendir(str.c_str());
    ep = 0L;

	if(dp == 0L)
	{
		std::cerr << "unable to open directory: " << _path << std::endl;
		std::cerr << "\t" << strerror(errno) << std::endl;
	}

	if(!empty())
	{
		popFront();
	}
}

DirectoryRange::DirectoryRange(const std::string & m_path) :
	_path(m_path)
{
	if(_path[_path.size()-1] == '\\' || _path[_path.size()-1] == '/')
	{
		_path.resize(_path.size()-1);
	}

    dp = opendir(_path.c_str());
    ep = 0L;

	if(dp == 0L)
	{
		std::cerr << "unable to open directory: " << _path << std::endl;
		std::cerr << "\t" << strerror(errno) << std::endl;
	}

	if(!empty())
	{
		popFront();
	}
}

DirectoryRange::~DirectoryRange()
{
    if(dp)
    {
        closedir(dp);
    }
}

bool DirectoryRange::empty() const
{
    return dp == 0L;
}

bool DirectoryRange::isDirectory() const
{
#if defined(__linux__) || (defined(__APPLE__) && defined(__MACH__))
	return ep->d_type == DT_DIR;
#else
	auto path = syspath();
	DIR * dp = opendir(path.c_str());
	if(dp) closedir(dp);
	return true;
#endif
}

bool DirectoryRange::isFile() const
{
#if defined(__linux__) || (defined(__APPLE__) && defined(__MACH__))
	return ep->d_type == DT_REG;
#else
	auto path = syspath();
	FILE * fp = fopen(path.c_str(), "r");
	if(fp) fclose(fp);
	return fp;
#endif
}


bool DirectoryRange::isExtension(const std::string & ext) const
{
	auto str = extension();

	if(str.size() != ext.size())
	{
		return false;
	}

	for(uint32_t i = 0; i < str.size(); ++i)
	{
		if(tolower(str[i]) != tolower(ext[i]))
		{
			return false;
		}
	}

	return true;
}



void DirectoryRange::popFront()
{
begin:
    ep = readdir(dp);

    if(ep == 0L)
    {
        closedir(dp);
        dp = 0L;
    }
	else if(ep->d_name[0] == '.'
	&& (ep->d_name[1] == '\0' || (ep->d_name[1] == '.' && ep->d_name[2] == '\0')))
	{
		goto begin;
	}
}


const std::string & DirectoryRange::directory() const
{
    return _path;
}

std::string DirectoryRange::fullname() const
{
    return std::string(ep->d_name);
}

std::string DirectoryRange::path() const
{
    return directory() + "/" + fullname();
}

std::string DirectoryRange::syspath() const
{
    return directory() + SLASH + fullname();
}

std::string DirectoryRange::filename() const
{
	auto str = fullname();
	for(int i = str.size() -1; i > 0; --i)
	{
		if(str[i] == '.')
		{
			str.resize(i);
			break;
		}
	}

	return str;
}

std::string DirectoryRange::extension() const
{
	auto str = fullname();

	for(int i = str.size() - 1; i > 0; --i)
	{
		if(str[i] == '.')
		{
			return std::string(str, i+1);
		}
	}

	return std::string();
}

RecursiveDirectoryRange::RecursiveDirectoryRange(const std::string & dir, const std::string & file) :
	DirectoryRange(dir, file),
	child(0L)
{
}

RecursiveDirectoryRange::RecursiveDirectoryRange(const std::string & _path) :
	DirectoryRange(_path),
	child(0L)
{
}

RecursiveDirectoryRange::~RecursiveDirectoryRange()
{
    delete child;
}

void RecursiveDirectoryRange::popFront()
{
start:
    if(child)
    {
        child->RecursiveDirectoryRange::popFront();

        if(child->empty())
        {
            delete child;
            child = 0L;
        }
        else
        {
            return;
        }
    }

    super::popFront();

    if(empty())
    {
        return;
    }

    if(isDirectory())
    {
        child = new RecursiveDirectoryRange(RecursiveDirectoryRange::directory(), RecursiveDirectoryRange::fullname());
        goto start;
    }
}

