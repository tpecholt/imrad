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

template <class T>
struct typeid_func_name;

template <class R>
struct typeid_func_name<R()>
{
    static std::string str()
    {
        return typeid_name<R>() + "()";
    }
};

template <class R, class A>
struct typeid_func_name<R(A)>
{
    static std::string str()
    {
        return typeid_name<R>() + "(" + typeid_name<A>() + ")";
    }
};

template <class T>
std::string typeid_name()
{
    if constexpr (std::is_pointer_v<T>)
        return typeid_name<std::remove_pointer_t<T>>() + "*";
    else if constexpr (std::is_lvalue_reference_v<T>)
        return typeid_name<std::remove_reference<T>>() + "&";
    else if constexpr (std::is_rvalue_reference_v<T>)
        return typeid_name<std::remove_reference<T>>() + "&&";
    else if constexpr (std::is_function_v<T>)
        return typeid_func_name<T>::str();
    
    else if constexpr (std::is_same_v<T, void>)
        return "void";
    else if constexpr (std::is_same_v<T, std::string>)
        return "std::string";
    else if constexpr (std::is_same_v<T, color32>)
        return "color4";
    else if constexpr (std::is_same_v<T, dimension>)
        return "float";
    else if constexpr (std::is_same_v<T, int>)
        return "int";
    else if constexpr (std::is_same_v<T, float>)
        return "float";
    else if constexpr (std::is_same_v<T, double>)
        return "double";
    else if constexpr (std::is_same_v<T, size_t>)
        return "size_t";
    else if constexpr (std::is_same_v<T, bool>)
        return "bool";
    else if constexpr (std::is_same_v<T, ImVec2> || std::is_same_v<T, pzdimension2>)
        return "ImVec2";
    else if constexpr (std::is_same_v<T, font_name>)
        return "ImFont*";
    else if constexpr (std::is_same_v<T, std::vector<std::string>>)
        return "std::vector<std::string>";
    else
    {
        std::string str = typeid(T).name();
        if (str.size() >= 14 && !str.compare(str.size() - 14, 14, " __cdecl(void)"))
            str = str.substr(0, str.size() - 14) + "()";
        auto i = str.find(' ');
        if (i != std::string::npos)
            str.erase(0, i + 1);
        auto it = stx::find_if(str, [](char c) { return isalpha(c); });
        if (it != str.end())
            str.erase(0, it - str.begin());
        return str;
    }
}



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