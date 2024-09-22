#pragma once

#include <cmath>
#include <imgui.h>
#include <iostream>
#include <vector>
// c17 standard now
#include <filesystem>
namespace fs = std::filesystem;

inline const std::string VER_STR = "ImRAD 0.8";
inline const std::string GITHUB_URL = "https://github.com/tpecholt/imrad";


inline ImVec2 operator+ (const ImVec2& a, const ImVec2& b)
{
    return ImVec2(a.x + b.x, a.y + b.y);
}

inline ImVec2 operator- (const ImVec2& a, const ImVec2& b)
{
    return ImVec2(a.x - b.x, a.y - b.y);
}

inline ImVec2 operator/ (const ImVec2& a, float f)
{
    return { a.x / f, a.y / f };
}

inline ImVec2& operator+= (ImVec2& a, const ImVec2& b)
{
    a = a + b;
    return a;
}

inline float Norm(const ImVec2& a)
{
    return std::sqrt(a.x * a.x + a.y*a.y);
}

inline std::string operator+ (const std::string& s, std::string_view t)
{
    std::string ss = s;
    ss += t;
    return ss;
}

inline std::string operator+ (std::string_view t, const std::string& s)
{
    std::string ss = s;
    ss.insert(0, t, t.size());
    return ss;
}

inline std::ostream& operator<< (std::ostream& os, std::string_view t)
{
    os.write(t.data(), t.size());
    return os;
}

void ShellExec(const std::string& path);

//----------------------------------------------------------------------

struct Widget;

//to iterate children matching a filter e.g. free-positioned or not
struct child_iterator
{
    using children_type = std::vector<std::unique_ptr<Widget>>;

    struct iter
    {
        iter();
        iter(children_type& ch, bool freePos);
        iter& operator++ ();
        iter operator++ (int);
        bool operator== (const iter& it) const;
        bool operator!= (const iter& it) const;
        children_type::value_type& operator* ();
        const children_type::value_type& operator* () const;

    private:
        bool end() const;
        bool valid() const;

        size_t idx;
        children_type* children;
        bool freePos;
    };

    child_iterator(children_type& children, bool freePos);
    iter begin() const;
    iter end() const;
    explicit operator bool() const;

private:
    children_type& children;
    bool freePos;
};