#include "utils.h"
#include "node.h"
#if WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#endif

void ShellExec(const std::string& path)
{
#ifdef WIN32
    ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
    system(("xdg-open " + path).c_str());
#endif
}

//----------------------------------------------------

child_iterator::iter::iter()
    : children(), idx()
{}

child_iterator::iter::iter(children_type& ch, bool freePos)
    : children(&ch), freePos(freePos), idx()
{
    while (!end() && !valid())
        ++idx;
}

child_iterator::iter&
child_iterator::iter::operator++ () {
    if (end())
        return *this;
    ++idx;
    while (!end() && !valid())
        ++idx;
    return *this;
}

child_iterator::iter
child_iterator::iter::operator++ (int) {
    iter it(*this);
    ++(*this);
    return it;
}

bool child_iterator::iter::operator== (const iter& it) const
{
    if (end() != it.end())
        return false;
    if (!end())
        return idx == it.idx;
    return true;
}

bool child_iterator::iter::operator!= (const iter& it) const
{
    return !(*this == it);
}

child_iterator::children_type::value_type&
child_iterator::iter::operator* ()
{
    static children_type::value_type dummy;
    if (end())
        return dummy;
    return children->at(idx);
}

const child_iterator::children_type::value_type&
child_iterator::iter::operator* () const
{
    static children_type::value_type dummy;
    if (end())
        return dummy;
    return children->at(idx);
}

bool child_iterator::iter::end() const
{
    return !children || idx >= children->size();
}

bool child_iterator::iter::valid() const
{
    if (end())
        return false;
    bool fp = children->at(idx)->hasPos;
    return freePos == fp;
}

child_iterator::child_iterator(children_type& children, bool freePos)
    : children(children), freePos(freePos)
{}

child_iterator::iter
child_iterator::begin() const
{
    return iter(children, freePos);
}

child_iterator::iter
child_iterator::end() const
{
    return iter();
}

child_iterator::operator bool() const
{
    return begin() != end();
}
