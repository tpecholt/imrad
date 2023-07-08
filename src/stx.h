#pragma once
#include <algorithm>
#include <string_view>

namespace stx {

	template <class C, class T>
	decltype(auto) find(C& c, const T& val)
	{
		return std::find(std::begin(c), std::end(c), val);
	}
	
	template <class C, class T>
	decltype(auto) rfind(C& c, const T& val)
	{
		if (c.begin() == c.end())
			return c.end();
		for (auto it = --c.end(); it != c.begin(); --it)
			if (*it == val)
				return it;
		if (*c.begin() == val)
			return c.begin();
		return c.end();
	}
	
	template <class C, class F>
	decltype(auto) find_if(C& c, F&& val)
	{
		return std::find_if(std::begin(c), std::end(c), std::forward<F>(val));
	}

	template <class C, class T>
	decltype(auto) count(const C& c, const T& val)
	{
		return std::count(std::begin(c), std::end(c), val);
	}
	
	template <class C, class F>
	decltype(auto) count_if(const C& c, F&& val)
	{
		return std::count_if(std::begin(c), std::end(c), std::forward<F>(val));
	}

	template <class C, class D>
	decltype(auto) equal(const C& c, const D& d)
	{
		return std::equal(std::begin(c), std::end(c), std::begin(d), std::end(d));
	}

	template <class C, class T>
	decltype(auto) fill(C& c, const T& val)
	{
		return std::fill(std::begin(c), std::end(c), val);
	}

	template <class C, class T>
	decltype(auto) replace(C& c, const T& oldv, const T& newv)
	{
		return std::replace(std::begin(c), std::end(c), oldv, newv);
	}

	template <class C>
	decltype(auto) unique(C& c)
	{
		return std::unique(std::begin(c), std::begin(c));
	}

	template <class C, class F>
	decltype(auto) erase_if(C& c, F&& fun)
	{
		c.erase(std::remove_if(std::begin(c), std::end(c), std::forward<F>(fun)), c.end());
	}

	template <class C, class I>
	decltype(auto) set_intersection(const C& c1, const C& c2, I dst)
	{
		return std::set_intersection(std::begin(c1), std::end(c1), std::begin(c2), std::end(c2), dst);
	}

	template <class C>
	decltype(auto) sort(C& c)
	{
		return std::sort(std::begin(c), std::end(c));
	}

	template <class C, class F>
	decltype(auto) for_each(C& c, F&& f)
	{
		return std::for_each(std::begin(c), std::end(c), std::forward<F>(f));
	}

	template <class C>
	int ssize(const C& c)
	{
		return (int)std::size(c);
	}

	template <class I>
	std::string join(I beg, I end, std::string_view sep)
	{
		std::string s;
		for (; beg != end; ++beg) {
			s += *beg;
			s += sep;
		}
		if (!s.empty())
			s.resize(s.size() - sep.size());
		return s;
	}
	
	template <class C>
	std::string join(const C& c, std::string_view sep)
	{
		return join(std::begin(c), std::end(c), sep);
	}
}