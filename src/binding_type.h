#pragma once
#include <string>
#include <imgui.h>
#include <sstream>

//negative values allowed
struct dimension_t {};

//positive or zero dimension (so that neg values represent empty value)
struct pzdimension_t {};

struct pzdimension2_t {};

struct font_name_t {};

struct shortcut_t {};

struct color_t {};

//------------------------------------------------------------------

template <class T>
struct typeid_func_name;

template <class T>
std::string typeid_name()
{
    if constexpr (std::is_const_v<T>)
        return "const " + typeid_name<std::remove_const_t<T>>();
    else if constexpr (std::is_pointer_v<T>)
        return typeid_name<std::remove_pointer_t<T>>() + "*";
    else if constexpr (std::is_lvalue_reference_v<T>)
        return typeid_name<std::remove_reference_t<T>>() + "&";
    else if constexpr (std::is_rvalue_reference_v<T>)
        return typeid_name<std::remove_reference_t<T>>() + "&&";
    else if constexpr (std::is_function_v<T>)
        return typeid_func_name<T>::str();
    
    else if constexpr (std::is_same_v<T, void>)
        return "void";
    else if constexpr (std::is_same_v<T, std::string>)
        return "std::string";
    else if constexpr (std::is_same_v<T, color_t>)
        return "color4";
    else if constexpr (std::is_same_v<T, dimension_t>)
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
    else if constexpr (std::is_same_v<T, ImVec2> || std::is_same_v<T, pzdimension2_t>)
        return "ImVec2";
    else if constexpr (std::is_same_v<T, font_name_t>)
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
