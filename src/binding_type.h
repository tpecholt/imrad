#pragma once
#include <string>
#include <imgui.h>
#include <sstream>

//negative values allowed
struct dimension
{
    float value;
    dimension(float v = 0) : value(v) {}
    dimension& operator= (float v) { value = v; return *this; }
    operator float& () { return value; }
    operator const float& () const { return value; }
};

//positive or zero dimension (so that neg values represent empty value)
struct pzdimension
{
    float value;
    pzdimension(float v = 0) : value(v) {}
    pzdimension& operator= (float v) { value = v; return *this; }
    operator float& () { return value; }
    operator const float& () const { return value; }
};

struct pzdimension2
{
    ImVec2 value;
    pzdimension2(ImVec2 v = {}) : value(v) {}
    pzdimension2& operator= (ImVec2 v) { value = v; return *this; }
    operator ImVec2& () { return value; }
    operator const ImVec2& () const { return value; }
    float& operator[] (int i) { return value[i]; }
    const float operator[] (int i) const { return value[i]; } //no & as in ImVec2
    bool operator== (ImVec2 v) const { return value[0] == v[0] && value[1] == v[1]; }
    bool operator!= (ImVec2 v) const { return !(*this == v); }
};

struct font_name
{
    std::string value;
    font_name(const std::string& v = {}) : value(v) {}
    font_name& operator= (const std::string& v) { value = v; return *this; }
    operator std::string& () { return value; }
    operator const std::string& () const { return value; }
    operator std::string_view() const { return value; }
    /*friend std::ostream& operator<< (std::ostream& os, const font_name& fn)
    {
        return os << "\"" << fn.value << "\"";
    }
    friend std::istream& operator>> (std::istream& is, font_name& fn)
    {
        if (is.get() != '"') {
            is.setstate(std::ios::failbit);
            return is;
        }
        std::getline(is, fn.value);
        if (value.empty() || fn.value.back() != '"') {
            is.setstate(std::ios::failbit);
            return is;
        }
        value.pop_back();
        return is;
    }*/
};

struct shortcut_
{
    std::string value;
    shortcut_(const std::string& v = {}) : value(v) {}
    operator const std::string& () { return value; }
};

struct color32
{
    color32(ImU32 c = 0) : c(c) {} //0 means style default color
    color32(const ImVec4& v) : c(IM_COL32(255 * v.x, 255 * v.y, 255 * v.z, 255 * v.w)) {}
    color32& operator= (ImU32 cc) { c = cc; return *this; }
    operator ImU32 () const { return c; }
    operator ImU32& () { return c; }
    bool operator== (const color32& a) const { return c == a.c; }
    bool operator!= (const color32& a) const { return c != a.c; }
    std::string string() const {
        std::ostringstream os;
        os << *this;
        return os.str();
    }
    friend std::ostream& operator<< (std::ostream& os, color32 clr)
    {
        if (!clr)
            return os;
        return os << "0x" << std::hex << std::setw(2 * 4)
            << std::setfill('0') << (ImU32)clr;
        /*os.fill('0');
        return os << "#" << std::hex
            << std::setw(2) << ((clr >> IM_COL32_R_SHIFT) & 0xff)
            << std::setw(2) << ((clr >> IM_COL32_G_SHIFT) & 0xff)
            << std::setw(2) << ((clr >> IM_COL32_B_SHIFT) & 0xff)
            << std::setw(2) << ((clr >> IM_COL32_A_SHIFT) & 0xff);*/
    }
    friend std::istream& operator>> (std::istream& is, color32& c)
    {
        if (is.peek() == EOF) {
            c = color32();
            return is;
        }
        if (is.get() != '0') {
            //c = color32();
            is.setstate(std::ios::failbit);
            return is;
        }
        if (is.get() != 'x') {
            //c = color32();
            is.setstate(std::ios::failbit);
            return is;
        }
        ImU32 cc = 0;
        for (int i = 0; i < 8; ++i)
        {
            int v = is.get();
            if (v >= 'a' && v <= 'f')
                v = v - 'a' + 10;
            else if (v >= 'A' && v <= 'F')
                v = v - 'A' + 10;
            else
                v -= '0';
            cc = (cc << 4) | v;
        }
        c = cc;
        return is;
    }


private:
    ImU32 c;
};

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
