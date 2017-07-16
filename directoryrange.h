#ifndef _DIRECTORY_RANGE_H_
#define _DIRECTORY_RANGE_H_
#include <cstdint>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>
#include <dirent.h>

#ifdef _WIN32
#define SLASH "\\"
#define SLASH_CHAR '\\'
#else
#define SLASH "/"
#define SLASH_CHAR '/'
#endif


class DirectoryRange
{
private:
    DIR * dp;
    dirent *ep;
	std::string _path;

public:
	DirectoryRange(const std::string & _path, const std::string & file);
    DirectoryRange(const std::string & _path);
    virtual ~DirectoryRange();

    bool empty() const;
	bool isDirectory() const;
	bool isFile() const;
	bool isExtension(const std::string & ext) const;
    virtual void popFront();

	virtual const std::string & directory() const;
    virtual std::string fullname() const;

	virtual int depth() const { return 0; }

	std::string path() const;
	std::string syspath() const;
	std::string filename() const;
	std::string extension() const;
};

class RecursiveDirectoryRange : public DirectoryRange
{
typedef DirectoryRange super;
    RecursiveDirectoryRange * child;

public:
	RecursiveDirectoryRange(const std::string & _path, const std::string & file);
    RecursiveDirectoryRange(const std::string & _path);
    ~RecursiveDirectoryRange();

    void popFront();


	int depth() const
	{
		if(child)
		{
			return child->RecursiveDirectoryRange::depth() + 1;
		}

		return 0;
	}



#define recursive(function)\
function() const\
{\
	return child? child->RecursiveDirectoryRange::function() : super::function();\
}

	const std::string & recursive(directory)
	std::string recursive(fullname)

#undef recursive
};

template<class T>
class FileRange : public T
{
typedef T super;

public:
    FileRange(const std::string & path) :
	super(path) {}

    void popFront()
    {
		for(super::popFront(); !super::empty() && !super::isFile(); super::popFront()) ;
    }
};

template<class T>
class FileTypeRange : public T
{
typedef T super;
	const std::string ext;

	void pop()
    {
        for(; !super::empty(); super::popFront())
        {
			if(!super::isFile())
			{
				continue;
			}

			if(super::isExtension(ext))
			{
				break;
			}
        }
    }

public:
	FileTypeRange(const std::string & dir, const std::string & file, const std::string & ext) :
		super(dir, file),
		ext(ext)
    {
		pop();
    }

    FileTypeRange(const std::string & path, const std::string & ext) :
		super(path),
		ext(ext)
    {
		pop();
    }


    void popFront()
    {
        super::popFront();
		pop();
    }
};

template<class T>
class FileTypesRange : public T
{
typedef T super;
	const std::vector<std::string> extensions;

	void pop()
    {
        for(; !super::empty(); super::popFront())
        {
			if(!super::isFile())
			{
				continue;
			}

			for(auto i = extensions.begin(); i != extensions.end(); ++i)
			{
				if(super::isExtension(*i))
				{
					return;
				}
			}
        }

		return;
    }

public:
	FileTypesRange(std::string path, std::initializer_list<std::string> args) :
		super(path),
		extensions(args)
    {
		pop();
    }


    void popFront()
    {
        super::popFront();
		pop();
    }
};

#endif // _DIRECTORY_RANGE_H_
