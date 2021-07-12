#ifndef PATH_H
#define PATH_H

#include <iostream>
#include <vector>

using namespace std;

class Path {

	public:
		Path(unsigned maxLength = 0);
		~Path(){}

		unsigned  getFrom() const {
			return from;
		}
		unsigned  getLength() const {
			return length;
		}
		unsigned  getTo() const {
			return to;
		}
		void addLinkToPath(unsigned next);
		void goStart(void);
		unsigned getActualStep(void);
		unsigned getNextStep(void);
		void goAhead(void);
		void goLastLink(void);
		bool isEndPath(void) const;
		void clear();

		friend std::ostream &operator<<(std::ostream &os, const Path &path);

	private:
		unsigned length;
		//from e to representam o no
		unsigned from;
		unsigned to;
		//actualPos representa o indice no vector
		unsigned actualPos;
		vector<unsigned> path;
};

Path::Path(unsigned maxLength)
{
	if (maxLength) path.reserve(maxLength);
	to = from = -1;
	actualPos = length = 0;
}

void Path::addLinkToPath(unsigned next)
{
	if (length++ == 0)
		from = next;
	path.push_back(next);
	to = next;
}

void Path::goStart(void)
{
	actualPos = 0;
}

unsigned Path::getActualStep(void)
{
	return path[actualPos];
}

unsigned Path::getNextStep(void)
{
	return path[actualPos + 1];
}

void Path::goAhead(void)
{
	actualPos++;
}

void Path::goLastLink(void)
{
    actualPos = path.size() - 2;
}

bool Path::isEndPath(void) const
{
	return (actualPos >= path.size()-1);
}

void Path::clear()
{
    length = from = to = actualPos = 0;
    path.clear();
}

ostream &operator<<(ostream &os, const Path &path)
{
    Path p = path;
    p.goStart();
    int l = p.getLength();
    if (l==0)
    {
        os << "Path: empty";
        return os;
    }
    os << "Path: ";
    for ( int i = 0 ; i < l - 1 ; i++,p.goAhead())
        os << p.getActualStep() << " -> ";
    os << p.getTo();
    return os;
}
#endif // PATH_H
