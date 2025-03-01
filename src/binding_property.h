#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <imgui.h>
#include "uicontext.h"
#include "stx.h"
#include "utils.h"
#include "cpp_parser.h"
#include "binding_type.h"


template <class T>
struct two_step
{
public:
    T* access() { return &newVal; }
    const T& value() { return val; }
    const T& new_value() { return newVal; }
    void commit(const T& v) { newVal = v; commit(); }
    void commit() { val = newVal; }
    void revert() { newVal = val; }
private:
    T val{};
    T newVal{};
}; 

//------------------------------------------------------------

struct property_base
{
    virtual std::string to_arg(std::string_view a = "", std::string_view b = "") const = 0;
    virtual void set_from_arg(std::string_view s) = 0;
    virtual const char* c_str() const = 0;
    virtual std::vector<std::string> used_variables() const = 0;
    virtual void rename_variable(const std::string& oldn, const std::string& newn) = 0;
};

//member variable expression like id, id.member, id[0], id.size()
template <class T = void>
struct field_ref : property_base
{
    using type = T;

    bool empty() const {
        return str.empty();
    }
    const std::string& value() const {
        return str;
    }

    T eval(const UIContext& ctx) const;

    void set_from_arg(std::string_view s) {
        str = s;
    }
    std::string to_arg(std::string_view = "", std::string_view = "") const {
        return str;
    }
    std::vector<std::string> used_variables() const {
        if (empty())
            return {};
        auto i = str.find_first_of(".[");
        return { str.substr(0, i) };
    };
    void rename_variable(const std::string& oldn, const std::string& newn)
    {
        auto i = str.find_first_of(".[");
        if (str.substr(0, i) == oldn)
            str.replace(0, i, newn);
    }
    const char* c_str() const { return str.c_str(); }
    std::string* access() { return &str; }
private:
    std::string str;
};

//member function name
template <class FuncSig = void()>
struct event : property_base
{
    bool empty() const {
        return str.empty();
    }
    void set_from_arg(std::string_view s) {
        str = s;
    }
    std::string to_arg(std::string_view = "", std::string_view = "") const {
        return str;
    }
    std::vector<std::string> used_variables() const {
        if (empty())
            return {};
        return { str };
    };
    void rename_variable(const std::string& oldn, const std::string& newn)
    {
        if (str == oldn)
            str = newn;
    }
    const char* c_str() const { return str.c_str(); }
    std::string* access() { return &str; }
private:
    std::string str;
};

//value only
template <class T>
struct direct_val : property_base 
{
    direct_val(T v) : val(v) {}
    
    direct_val& add(const char* id, T v) {
        ids.push_back({ id, v });
        return *this;
    }
    const auto& get_ids() const { return ids; }
    const std::string& get_id() const {
        static std::string none;
        auto it = stx::find_if(ids, [this](const auto& id) { return id.second == val; });
        return it != ids.end() ? it->first : none;
    }

    operator T&() { return val; }
    operator const T&() const { return val; }
    bool operator== (const T& dv) const {
        return val == dv;
    }
    bool operator!= (const T& dv) const {
        return val != dv;
    }
    direct_val& operator= (const T& v) {
        val = v;
        return *this;
    }
    void set_from_arg(std::string_view s) {
        if (ids.size()) {
            auto it = stx::find_if(ids, [&](const auto& id) { return id.first == s; });
            val = it != ids.end() ? it->second : T();
        }
        else {
            std::istringstream is;
            is.str(std::string(s));
            if constexpr (std::is_enum_v<T>) {
                int tmp;
                is >> tmp;
                val = (T)tmp;
            }
            else {
                is >> std::boolalpha >> val;
            }
        }
    }
    std::string to_arg(std::string_view = "", std::string_view = "") const {
        if (ids.size()) {
            auto it = stx::find_if(ids, [this](const auto& id) { return id.second == val; });
            return it != ids.end() ? it->first : "";
        }
        else {
            std::ostringstream os;
            os << std::boolalpha << val;
            return os.str();
        }
    }
    std::vector<std::string> used_variables() const {
        return {};
    }
    void rename_variable(const std::string& oldn, const std::string& newn)
    {}
    T* access() { return &val; }
    const char* c_str() const { return nullptr; }

private:
    T val;
    std::vector<std::pair<std::string, T>> ids;
};

template <>
struct direct_val<dimension> : property_base
{
    direct_val(dimension dim) : direct_val((float)dim) {}
    direct_val(float v) : val(v) {}

    operator float&() { return val; }
    operator const float() const { return val; }
    bool operator== (dimension dv) const {
        return val == dv;
    }
    bool operator!= (dimension dv) const {
        return val != dv;
    }
    direct_val& operator= (dimension v) {
        val = v;
        return *this;
    }
    direct_val& operator= (float f) {
        val = f;
        return *this;
    }
    float eval_px(const UIContext& ctx) const;

    void set_from_arg(std::string_view s) {
        std::istringstream is;
        is.str(std::string(s));
        is >> val;
        //strip unit calculation
        std::string_view factor = s.size() > 3 ? s.substr(s.size() - 3) : "";
        if (factor == "*dp")
        {
            std::istringstream is(std::string(s.substr(0, s.size() - 3)));
            dimension v;
            if ((is >> v) && is.eof())
                val = v;
        }
    }
    std::string to_arg(std::string_view unit, std::string_view = "") const {
        std::ostringstream os;
        os << val;
        if (unit != "")
            os << "*" << unit;
        return os.str();
    }
    std::vector<std::string> used_variables() const {
        return {};
    }
    void rename_variable(const std::string& oldn, const std::string& newn)
    {}
    float* access() { return &val.value; }
    const char* c_str() const { return nullptr; }

private:
    dimension val;
};

template <>
struct direct_val<pzdimension> : property_base
{
    direct_val(pzdimension dim) : direct_val((float)dim) {}
    direct_val(float v = -1) : val(v) {}

    operator float&() { return val; }
    operator const float() const { return val; }
    bool empty() const { return val == -1; }
    bool has_value() const { return !empty(); }
    bool operator== (dimension dv) const {
        return val == dv;
    }
    bool operator!= (pzdimension dv) const {
        return val != dv;
    }
    direct_val& operator= (pzdimension v) {
        val = v;
        return *this;
    }
    direct_val& operator= (float f) {
        val = f;
        return *this;
    }
    float eval_px(const UIContext& ctx) const;

    void set_from_arg(std::string_view s) {
        std::istringstream is;
        is.str(std::string(s));
        is >> val;
        //strip unit calculation
        std::string_view factor = s.size() > 3 ? s.substr(s.size() - 3) : "";
        if (factor == "*dp")
        {
            std::istringstream is(std::string(s.substr(0, s.size() - 3)));
            dimension v;
            if ((is >> v) && is.eof())
                val = v;
        }
    }
    std::string to_arg(std::string_view unit, std::string_view = "") const {
        std::ostringstream os;
        os << val;
        if (unit != "")
            os << "*" << unit;
        return os.str();
    }
    std::vector<std::string> used_variables() const {
        return {};
    }
    void rename_variable(const std::string& oldn, const std::string& newn)
    {}
    float* access() { return &val.value; }
    const char* c_str() const { return nullptr; }

private:
    pzdimension val;
};

template <>
struct direct_val<pzdimension2> : property_base
{
    direct_val(pzdimension2 dim = ImVec2{-1, -1}) : val(dim) {}
    
    operator ImVec2&() { return val; }
    operator const ImVec2() const { return val; }
    float& operator[] (int i) { return val[i]; }
    float operator[] (int i) const { return val[i]; }

    bool operator== (pzdimension2 dv) const {
        return val == dv;
    }
    bool operator!= (pzdimension2 dv) const {
        return val != dv;
    }
    direct_val& operator= (pzdimension2 v) {
        val = v;
        return *this;
    }
    bool empty() const { return val[0] == -1 && val[1] == -1; }
    bool has_value() const { return !empty(); }
    ImVec2 eval_px(const UIContext& ctx) const;

    void set_from_arg(std::string_view s) {
        val = cpp::parse_fsize(std::string(s));
    }
    std::string to_arg(std::string_view unit, std::string_view = "") const {
        bool hasv = has_value();
        std::ostringstream os;
        os << "{ " << val[0];
        if (unit != "" && hasv && val[0])
            os << "*" << unit;
        os << ", " << val[1];
        if (unit != "" && hasv && val[1])
            os << "*" << unit;
        os << " }";
        return os.str();
    }
    std::vector<std::string> used_variables() const {
        return {};
    }
    void rename_variable(const std::string& oldn, const std::string& newn)
    {}
    ImVec2* access() { return &val.value; }
    const char* c_str() const { return nullptr; }

private:
    pzdimension2 val;
};

template <>
struct direct_val<std::string> : property_base
{
    direct_val(const std::string& v) : val(v) {}
    direct_val(const char* v) : val(v) {}

    operator std::string&() { return val; }
    operator const std::string&() const { return val; }
    //operator std::string_view() const { return val; }
    const std::string& value() const { return val; }
    
    void set_from_arg(std::string_view s) {
        val = cpp::parse_str_arg(s);
    }
    std::string to_arg(std::string_view = "", std::string_view = "") const {
        return cpp::to_str_arg(val);
    }
    std::vector<std::string> used_variables() const {
        return {};
    }
    void rename_variable(const std::string& oldn, const std::string& newn)
    {}
    std::string* access() { return &val; }
    const char* c_str() const { return val.c_str(); }
    bool empty() const { return val.empty(); }
    bool operator== (const std::string& dv) const {
        return val == dv;
    }
    bool operator!= (const std::string& dv) const {
        return val != dv;
    }
private:
    std::string val;
};

template <>
struct direct_val<shortcut_> : property_base
{
    direct_val(const char* v, int f = 0) 
        : sh(v), flags_(f)
    {}
    direct_val(const std::string& v, int f = 0)
        : sh(v), flags_(f)
    {}

    int flags() const { return flags_; }
    void set_flags(int f) { flags_ = f; }
    //const std::string& value() const { return val; }

    void set_from_arg(std::string_view s) {
        sh = ParseShortcut(s);
        if (s.find("ImGuiInputFlags_RouteGlobal") != std::string::npos)
            flags_ |= ImGuiInputFlags_RouteGlobal;
        if (s.find("ImGuiInputFlags_Repat") != std::string::npos)
            flags_ |= ImGuiInputFlags_Repeat;
    }
    std::string to_arg(std::string_view = "", std::string_view = "") const {
        std::ostringstream os;
        os << CodeShortcut(sh.value);
        std::vector<std::string> fl;
        if (flags_ & ImGuiInputFlags_RouteGlobal)
            fl.push_back("ImGuiInputFlags_RouteGlobal");
        if (flags_ & ImGuiInputFlags_Repeat)
            fl.push_back("ImGuiInputFlags_Repeat");
        if (fl.size())
            os << ", " << stx::join(fl, " | ");
        return os.str();
    }
    std::vector<std::string> used_variables() const {
        return {};
    }
    void rename_variable(const std::string& oldn, const std::string& newn)
    {}
    std::string* access() { return &sh.value; }
    const char* c_str() const { return sh.value.c_str(); }
    bool empty() const { return sh.value.empty(); }
private:
    shortcut_ sh;
    int flags_;
};

#define add$(f) add(#f, f)
struct flags_helper : property_base
{
public:
    flags_helper(int f) : f(f)
    {}
    flags_helper& operator= (int ff) {
        f = ff;
        return *this;
    }
    flags_helper& operator|= (int ff) {
        f |= ff;
        return *this;
    }
    flags_helper& operator&= (int ff) {
        f &= ff;
        return *this;
    }
    int operator& (int ff) {
        return f & ff;
    }
    flags_helper& prefix(const char* p) {
        pre = p;
        return *this;
    }
    flags_helper& add(const char* id, int v) {
        if (!pre.compare(0, pre.size(), id, 0, pre.size()))
            ids.push_back({ id + pre.size(), v });
        else
            ids.push_back({ id, v });
        return *this;
    }
    flags_helper& separator() {
        ids.push_back({ "", 0 });
        return *this;
    }
    operator int() const { return f; }
    int* access() { return &f; }
    std::string to_arg(std::string_view = "", std::string_view = "") const {
        std::string str;
        for (const auto& id : ids)
            if ((f & id.second) && id.first != "")
                str += pre + id.first + " | ";
        if (str != "")
            str.resize(str.size() - 3);
        else
            str = pre + "None";
        return str;
    }
    void set_from_arg(std::string_view str) {
        f = 0;
        size_t i = 0;
        while (true) {
            std::string s;
            size_t j = str.find('|', i);
            if (j == std::string::npos)
                s = str.substr(i);
            else
                s = str.substr(i, j - i);
            if (!str.compare(0, pre.size(), pre))
                s.erase(0, pre.size());
            auto id = stx::find_if(ids, [&](const auto& id) { return id.first == s; });
            //assert(id != ids.end());
            if (id != ids.end())
                f |= id->second;
            if (j == std::string::npos)
                break;
            i = j + 1;
        }
    }
    const auto& get_ids() const { return ids; }
    std::string get_name(int fl, bool prefixed = true) const {
        for (const auto& id : ids)
            if (id.second == fl && id.first != "") 
                return prefixed ? pre + id.first : id.first;
        return "";
    }
    std::vector<std::string> used_variables() const { return {}; }
    void rename_variable(const std::string& oldn, const std::string& newn) {}
    const char* c_str() const { return nullptr; }

private:
    int f;
    std::string pre;
    std::vector<std::pair<std::string, int>> ids;
};

//value or binding expression
//2*w
template <class T>
struct bindable : property_base
{
    bindable() {
    }
    bindable(T val) {
        std::ostringstream os;
        os << std::boolalpha << val;
        str = os.str();
    }
    bool empty() const { return str.empty(); }
    bool has_value() const {
        if (empty()) 
            return false;
        std::istringstream is(str);
        T val;
        if (!(is >> std::boolalpha >> val))
            return false;
        if (is.eof())
            return true;
        return is.tellg() == str.size();
    }
    T value() const {
        if (!has_value())
            return {};
        std::istringstream is(str);
        T val{};
        is >> std::boolalpha >> val;
        return val;
    }
    T eval(const UIContext& ctx) const {
        return value();
    }

    void set_from_arg(std::string_view s) {
        str = s;
        if (std::is_same_v<T, float> && str.size() && str.back() == 'f') { 
            //try to remove trailing f
            str.pop_back();
            if (!has_value())
                str.push_back('f');
        }
    }
    std::string to_arg(std::string_view = "", std::string_view = "") const {
        if (std::is_same_v<T, float> && has_value()) {
            std::string tmp = str;
            if (str.find('.') == std::string::npos)
                tmp += ".f";
            else
                tmp += "f";
            return tmp;
        }
        return str;
    }
    std::vector<std::string> used_variables() const {
        if (empty() || has_value())
            return {};
        std::vector<std::string> vars;
        size_t i = 0;
        while (true) {
            auto id = cpp::find_id(str, i);
            if (id == "")
                break;
            vars.push_back(std::string(id));
        }
        return vars;
    };
    void rename_variable(const std::string& oldn, const std::string& newn)
    {
        if (empty() || has_value())
            return;
        std::vector<std::string> vars;
        size_t i = 0;
        while (true) {
            auto id = cpp::find_id(str, i);
            if (id == "")
                break;
            if (id == oldn)
                str.replace(id.data() - str.data(), id.size(), newn);
        }
    }
    const char* c_str() const { return str.c_str(); }
    std::string* access() { return &str; }
private:
    std::string str;
};

template <>
struct bindable<dimension> : property_base
{
    bindable() {
    }
    bindable(dimension val)
        : bindable((float)val)
    {}
    bindable(float val) 
    {
        std::ostringstream os;
        os << val;
        str = os.str();
    }
    bool empty() const { 
        return str.empty(); 
    }
    bool zero() const {
        if (empty())
            return false;
        std::istringstream is(str);
        dimension val;
        if (!(is >> val) || val)
            return false;
        return is.eof() || is.tellg() == str.size();
    }
    bool stretched() const {
        return grow;
    }
    bool has_value() const {
        if (empty())
            return false;
        std::istringstream is(str);
        dimension val;
        if (!(is >> val))
            return false;
        return is.eof() || is.tellg() == str.size();
    }
    dimension value() const {
        if (!has_value()) 
            return {};
        std::istringstream is(str);
        dimension val{};
        is >> val;
        return val;
    }
    float eval_px(int axis, const UIContext& ctx) const;
    
    void set_from_arg(std::string_view s) {
        str = s;
        //strip unit calculation
        std::string_view factor = s.size() > 3 ? s.substr(s.size() - 3) : "";
        if (factor == "*fs" || factor == "*dp") 
        {
            std::istringstream is(std::string(s.substr(0, s.size() - 3)));
            dimension val;
            if ((is >> val) && is.eof())
                str = s.substr(0, s.size() - 3);
        }
    }
    std::string to_arg(std::string_view unit, std::string_view stretchCode = "todo") const {
        if (stretched())
        {
            return std::string(stretchCode);
        }
        if (unit != "" && !stretched() && has_value() &&
            str != "0" && str != "-1") //don't suffix special values
        {
            return str + "*" + unit;
        }
        return str;
    }
    std::vector<std::string> used_variables() const {
        if (empty() || has_value())
            return {};
        std::vector<std::string> vars;
        size_t i = 0;
        while (true) {
            auto id = cpp::find_id(str, i);
            if (id == "")
                break;
            vars.push_back(std::string(id));
        }
        return vars;
    };
    void rename_variable(const std::string& oldn, const std::string& newn)
    {
        if (empty() || has_value())
            return;
        std::vector<std::string> vars;
        size_t i = 0;
        while (true) {
            auto id = cpp::find_id(str, i);
            if (id == "")
                break;
            if (id == oldn)
                str.replace(id.data() - str.data(), id.size(), newn);
        }
    }
    const char* c_str() const { return str.c_str(); }
    std::string* access() { return &str; }
    
    void stretch(bool s) { 
        grow = s; 
        std::ostringstream os;
        if (grow) {
            auto val = value();
            os << std::fixed << std::setprecision(1) << val;
            str = os.str();
        }
        else if (has_value()) {
            auto val = value();
            os << std::defaultfloat << val;
            str = os.str();
        }
    }

private:
    std::string str;
    bool grow = false;
};

//Hi {names[i]} you are {ages[i].exact():2} years old
template <>
struct bindable<std::string> : property_base
{
    bindable() {}
    bindable(const char* val) {
        str = val;
    }
    bindable(const std::string& val) {
        str = val;
    }
    bool empty() const { return str.empty(); }
    const std::string& value() const { return str; }
    void set_from_arg(std::string_view s)
    {
        str = cpp::parse_str_arg(s);
    }
    std::string to_arg(std::string_view = "", std::string_view = "") const
    {
        return cpp::to_str_arg(str);
    }
    std::vector<std::string> used_variables() const {
        std::vector<std::string> vars;
        size_t i = 0;
        while (true) {
            size_t j = str.find('{', i);
            if (j == std::string::npos)
                break;
            if (j + 1 < str.size() && str[j + 1] == '{')
                i = j + 2;
            else {
                i = j + 1;
                size_t e;
                for (e = i + 1; e < str.size(); ++e)
                    if (str[e] == ':' || str[e] == '}')
                        break;
                if (e >= str.size())
                    break;
                if (e > i) {
                    auto s = str.substr(i, e - i);
                    size_t k = 0;
                    while (true) {
                        auto id = cpp::find_id(s, k);
                        if (id == "")
                            break;
                        vars.push_back(std::string(id));
                    }
                }
            }
        }
        return vars;
    };
    void rename_variable(const std::string& oldn, const std::string& newn) {
        std::vector<std::string> vars;
        size_t i = 0;
        while (true) {
            size_t j = str.find('{', i);
            if (j == std::string::npos)
                break;
            if (j + 1 < str.size() && str[j + 1] == '{')
                i = j + 2;
            else {
                i = j + 1;
                size_t e = std::min(str.find(':', i), str.find('}', i));
                if (e == std::string::npos)
                    break;
                auto s = str.substr(i, e - i);
                size_t k = 0;
                while (true) {
                    auto id = cpp::find_id(s, k);
                    if (id == "")
                        break;
                    if (id == oldn)
                        str.replace(id.data() - str.data(), id.size(), newn);
                }
            }
        }
    }
    const char* c_str() const { return str.c_str(); }
    std::string* access() { return &str; }
protected:
    std::string str;
};

//{items} or prvni\0druha\0
template <>
struct bindable<std::vector<std::string>> : bindable<std::string>
{
    bindable() {}
    std::string to_arg(std::string_view = "", std::string_view = "") const
    {
        return cpp::to_str_arg(str, true);
    }
};

template <>
struct bindable<font_name> : property_base
{
    bindable() {
    }
    bindable(const font_name& fn) {
        set_font_name(fn);
    }
    bool empty() const { return str.empty(); }
    bool has_value() const {
        return !str.compare(0, 22, "ImRad::GetFontByName(\"");
    }
    std::string eval(const UIContext& ctx) const;

    void set_font_name(std::string_view fn) {
        str = "ImRad::GetFontByName(\"" + std::string(fn) + "\")";
    }
    void set_from_arg(std::string_view s) {
        str = s;
    }
    std::string to_arg(std::string_view = "", std::string_view = "") const {
        return str;
    }
    std::vector<std::string> used_variables() const {
        if (empty())
            return {};
        std::vector<std::string> vars;
        size_t i = 0;
        while (true) {
            auto id = cpp::find_id(str, i);
            if (id == "")
                break;
            vars.push_back(std::string(id));
        }
        return vars;
    };
    void rename_variable(const std::string& oldn, const std::string& newn)
    {
        if (empty())
            return;
        std::vector<std::string> vars;
        size_t i = 0;
        while (true) {
            auto id = cpp::find_id(str, i);
            if (id == "")
                break;
            if (id == oldn)
                str.replace(id.data() - str.data(), id.size(), newn);
        }
    }
    const char* c_str() const { return str.c_str(); }
    std::string* access() { return &str; }
private:
    std::string str;
};

template <>
struct bindable<color32> : property_base
{
    bindable() {
    }
    bindable(color32 c) {
        std::ostringstream os;
        os << c;
        str = os.str();
    }
    bool empty() const { return str.empty(); }
    /*bool has_value() const {
        if (empty())
            return false;
        std::istringstream is(str);
        color32 val;
        if (!(is >> val))
            return false;
        if (is.eof())
            return true;
        return is.tellg() == str.size();
    }*/
    color32 eval(int col, const UIContext& ctx) const;

    bool has_style_color() const {
        if (!str.compare(0, 34, "ImGui::GetStyle().Colors[ImGuiCol_") && str.back() == ']')
            return true;
        if (!str.compare(0, 34, "ImGui::GetStyleColorVec4(ImGuiCol_") && str.back() == ')')
            return true;
        return false;
    }
    int style_color() const {
        if (!has_style_color())
            return -1;
        std::string code = str.substr(34, str.size() - 34 - 1);
        for (int i = 0; i < ImGuiCol_COUNT; ++i)
            if (code == ImGui::GetStyleColorName(i))
                return i;
        return -1;
    }
    void set_style_color(int i) {
        std::ostringstream os;
        os << "ImGui::GetStyleColorVec4(ImGuiCol_" << ImGui::GetStyleColorName(i) << ")";
        str = os.str();
    }
    void set_from_arg(std::string_view s) {
        str = s;
    }
    std::string to_arg(std::string_view = "", std::string_view = "") const {
        return str;
    }
    std::vector<std::string> used_variables() const {
        if (empty())// || has_value())
            return {};
        std::vector<std::string> vars;
        size_t i = 0;
        while (true) {
            auto id = cpp::find_id(str, i);
            if (id == "")
                break;
            vars.push_back(std::string(id));
        }
        return vars;
    };
    void rename_variable(const std::string& oldn, const std::string& newn)
    {
        if (empty()) // || has_value())
            return;
        std::vector<std::string> vars;
        size_t i = 0;
        while (true) {
            auto id = cpp::find_id(str, i);
            if (id == "")
                break;
            if (id == oldn)
                str.replace(id.data() - str.data(), id.size(), newn);
        }
    }
    const char* c_str() const { return str.c_str(); }
    std::string* access() { return &str; }
private:
    std::string str;
};

struct data_loop : property_base
{
    bindable<int> limit;
    field_ref<int> index; //int indexes are easier than size_t

    bool empty() const {
        return limit.empty();
    }
    std::string index_name_or(std::string_view s) const {
        return index.empty() ? std::string(s) : index.to_arg();
    }

    void set_from_arg(std::string_view code) {
        if (code.compare(0, 4, "for("))
            return;
        bool local = !code.compare(4, 3, "int") || !code.compare(4, 6, "size_t");
        auto i = code.find(";");
        if (i == std::string::npos)
            return;
        code.remove_prefix(i + 1);

        i = code.find(";");
        if (i == std::string::npos)
            return;
        code.remove_suffix(code.size() - i);

        i = code.find("<");
        if (local)
            *index.access() = "";
        else
            *index.access() = code.substr(0, i);

        limit.set_from_arg(code.substr(i + 1));
    }
    std::string to_arg(std::string_view forVarName, std::string_view = "") const {
        if (empty())
            return "";
        std::ostringstream os;
        std::string name = index_name_or(std::string(forVarName));
        os << "for (";
        if (index.empty())
            os << "int ";
        os << name << " = 0; " << name << " < " << limit.to_arg()
            << "; ++" << name << ")";
        return os.str();
    }
    std::vector<std::string> used_variables() const {
        auto vars = index.used_variables();
        auto vars2 = limit.used_variables();
        vars.insert(vars.end(), vars2.begin(), vars2.end());
        return vars;
    };
    void rename_variable(const std::string& oldn, const std::string& newn)
    {
        limit.rename_variable(oldn, newn);
        index.rename_variable(oldn, newn);
    }
    const char* c_str() const { return limit.c_str(); }
    std::string* access() { return limit.access(); }
};

