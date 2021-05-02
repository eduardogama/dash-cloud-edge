#include "path.h"

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

void Path::goLastLink(void)
{
    actualPos = path.size() - 2;
}

unsigned Path::getNextStep(void)
{
	return path[actualPos + 1];
}

void Path::goAhead(void)
{
	actualPos++;
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

std::ostream &operator<<(std::ostream &os, const Path &path)
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
        os << p.getActualStep() << "->";
    os << p.getTo();
    return os;
}
