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


struct property_base
{
    virtual std::string to_arg(std::string_view a = "", std::string_view b = "") const = 0;
    virtual bool set_from_arg(std::string_view s) = 0;
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

    bool set_from_arg(std::string_view s) {
        str = s;
        return true;
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
    bool set_from_arg(std::string_view s) {
        str = s;
        return true;
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

template <class T, bool E = std::is_enum_v<T>>
struct direct_val;

template <class T>
struct direct_val<T, false> : property_base
{
    direct_val(T v) : val(v) {}

    operator T&() { return val; }
    operator const T&() const { return val; }
    /*bool operator== (const direct_val& dv) const {
        return val == dv.val;
    }
    bool operator!= (const direct_val& dv) const {
        return val != dv.val;
    }*/
    direct_val& operator= (const T& v) {
        val = v;
        return *this;
    }
    bool set_from_arg(std::string_view str) {
        std::istringstream is;
        is.str(std::string(str));
        if constexpr (std::is_enum_v<T>) {
            int tmp;
            is >> tmp;
            val = (T)tmp;
        }
        else {
            is >> std::boolalpha >> val;
        }
        return is.good();
    }
    std::string to_arg(std::string_view = "", std::string_view = "") const {
        std::ostringstream os;
        os << std::boolalpha << val;
        return os.str();
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
};

#define add$(f) add(#f, f)
template <class T>
struct direct_val<T, true> : property_base
{
    direct_val(int v = 0) : val(v) {}

    void clear() {
        ids.clear();
    }
    direct_val& add(const char* id, int v) {
        ids.push_back({ id, v });
        return *this;
    }
    direct_val& separator() {
        ids.push_back({ "", 0 });
        return *this;
    }
    auto find_id(int fl) const {
        return stx::find_if(ids, [this,fl](const auto& id) {
            return id.first != "" && id.second == fl;
            });
    }
    const auto& get_ids() const { return ids; }
    const std::string& get_id() const {
        static std::string none;
        auto it = find_id(val);
        return it != ids.end() ? it->first : none;
    }
    void set_id(const std::string& v) {
        auto it = stx::find_if(ids, [&v](const auto& id) { return id.first == v; });
        if (it != ids.end())
            val = it->second;
    }

    operator T() const { return (T)val; }

    direct_val& operator= (int v) {
        val = v;
        return *this;
    }
    direct_val& operator|= (int v) {
        val |= v;
        return *this;
    }
    direct_val& operator&= (int v) {
        val &= v;
        return *this;
    }
    bool set_from_arg(std::string_view str) {
        bool ok = true;
        val = {};
        size_t i = 0;
        while (true) {
            std::string s;
            size_t j = str.find('|', i);
            if (j == std::string::npos)
                s = str.substr(i);
            else
                s = str.substr(i, j - i);
            auto id = stx::find_if(ids, [&](const auto& id) { return id.first == s; });
            //assert(id != ids.end());
            if (s == "0" ||
                (!s.compare(0, 5, "ImGui") && !s.compare(s.size() - 5, 5, "_None")))
                val;
            else if (id != ids.end())
                val |= id->second;
            else
                ok = false;
            if (j == std::string::npos)
                break;
            i = j + 1;
        }
        return ok;
    }
    std::string to_arg(std::string_view = "", std::string_view = "") const {
        std::string str;
        bool simpleEnum = false;
        for (const auto& id : ids) {
            if (id.first != "" && !id.second)
                simpleEnum = true;
        }
        if (simpleEnum) {
            auto it = stx::find_if(ids, [this](const auto& id) { return id.second == val; });
            str = it == ids.end() ? "0" : it->first;
        }
        else {
            for (const auto& id : ids)
                if ((val & id.second) == id.second && id.first != "")
                    str += id.first + " | ";
            if (str != "")
                str.resize(str.size() - 3);
            else
                str = "0";
        }
        return str;
    }
    std::vector<std::string> used_variables() const {
        return {};
    }
    void rename_variable(const std::string& oldn, const std::string& newn)
    {}
    int* access() { return &val; }
    const char* c_str() const { return nullptr; }

private:
    int val;
    std::vector<std::pair<std::string, int>> ids;
};

template <>
struct direct_val<dimension_t> : property_base
{
    direct_val(float v) : val(v) {}

    operator float&() { return val; }
    operator const float() const { return val; }
    bool operator== (float dv) const {
        return val == dv;
    }
    bool operator!= (float dv) const {
        return val != dv;
    }
    direct_val& operator= (float v) {
        val = v;
        return *this;
    }
    float eval_px(const UIContext& ctx) const;

    bool set_from_arg(std::string_view s) {
        std::istringstream is;
        is.str(std::string(s));
        is >> val;
        //strip unit calculation
        std::string_view factor = s.size() > 3 ? s.substr(s.size() - 3) : "";
        if (factor == "*dp")
        {
            std::istringstream is(std::string(s.substr(0, s.size() - 3)));
            float v;
            if ((is >> v) && is.eof())
                val = v;
        }
        return true;
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
    float* access() { return &val; }
    const char* c_str() const { return nullptr; }

private:
    float val;
};

template <>
struct direct_val<pzdimension_t> : property_base
{
    direct_val(float v = -1) : val(v) {}

    operator float&() { return val; }
    operator const float() const { return val; }
    bool empty() const { return val == -1; }
    void clear() { val = -1; }
    bool has_value() const { return !empty(); }
    bool operator== (float dv) const {
        return val == dv;
    }
    bool operator!= (float dv) const {
        return val != dv;
    }
    direct_val& operator= (float v) {
        val = v;
        return *this;
    }
    float eval_px(const UIContext& ctx) const;

    bool set_from_arg(std::string_view s) {
        std::istringstream is;
        is.str(std::string(s));
        is >> val;
        //strip unit calculation
        std::string_view factor = s.size() > 3 ? s.substr(s.size() - 3) : "";
        if (factor == "*dp")
        {
            std::istringstream is(std::string(s.substr(0, s.size() - 3)));
            float v;
            if ((is >> v) && is.eof())
                val = v;
        }
        return true;
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
    float* access() { return &val; }
    const char* c_str() const { return nullptr; }

private:
    float val;
};

template <>
struct direct_val<pzdimension2_t> : property_base
{
    direct_val(ImVec2 dim = { -1, -1 }) : val(dim) {}

    operator ImVec2&() { return val; }
    operator const ImVec2() const { return val; }
    float& operator[] (int i) { return val[i]; }
    float operator[] (int i) const { return val[i]; }

    bool operator== (ImVec2 dv) const {
        return val[0] == dv[0] && val[1] == dv[1];
    }
    bool operator!= (ImVec2 dv) const {
        return val[0] != dv[0] && val[1] != dv[1];
    }
    direct_val& operator= (ImVec2 v) {
        val = v;
        return *this;
    }
    void clear() { val[0] = val[1] = -1; }
    bool empty() const { return val[0] == -1 && val[1] == -1; }
    bool has_value() const { return !empty(); }
    ImVec2 eval_px(const UIContext& ctx) const;

    bool set_from_arg(std::string_view s) {
        val = cpp::parse_fsize(std::string(s));
        return true;
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
    ImVec2* access() { return &val; }
    const char* c_str() const { return nullptr; }

private:
    ImVec2 val;
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

    bool set_from_arg(std::string_view s) {
        val = cpp::parse_str_arg(s);
        return val != cpp::INVALID_TEXT;
    }
    std::string to_arg(std::string_view = "", std::string_view = "") const {
        return "\"" + cpp::escape(val) + "\"";
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
struct direct_val<shortcut_t> : property_base
{
    direct_val(const std::string& v) : sh(v) {}
    direct_val(const char* v) : sh(v) {}

    int flags() const { return flags_; }
    void set_flags(int f) { flags_ = f; }
    //const std::string& value() const { return val; }

    bool set_from_arg(std::string_view s) {
        sh = ParseShortcut(s);
        flags_ = 0;
        if (s.find("ImGuiInputFlags_RouteGlobal") != std::string::npos)
            flags_ |= ImGuiInputFlags_RouteGlobal;
        if (s.find("ImGuiInputFlags_Repat") != std::string::npos)
            flags_ |= ImGuiInputFlags_Repeat;
        return true;
    }
    std::string to_arg(std::string_view = "", std::string_view = "") const {
        std::ostringstream os;
        os << CodeShortcut(sh);
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
    std::string* access() { return &sh; }
    const char* c_str() const { return sh.c_str(); }
    bool empty() const { return sh.empty(); }
private:
    std::string sh;
    int flags_;
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
    bool operator== (const bindable& b) const {
        return str == b.str;
    }
    bool operator!= (const bindable& b) const {
        return str != b.str;
    }
    bool empty() const { return str.empty(); }
    bool has_value() const {
        return cpp::is_literal(str);
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
    bool has_single_variable() const {
        return cpp::is_id(str);
    }

    bool set_from_arg(std::string_view s) {
        str = s;
        //try to remove trailing f
        if (std::is_same_v<T, float> && str.size() >= 3 && !str.compare(str.size() - 2, 2, ".f")) {
            str.resize(str.size() - 2);
            if (!has_value())
                str += ".f";
        }
        else if (std::is_same_v<T, float> && str.size() && str.back() == 'f') {
            str.pop_back();
            if (!has_value())
                str.push_back('f');
        }
        return true;
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
struct bindable<dimension_t> : property_base
{
    bindable() {
    }
    bindable(float val)
    {
        std::ostringstream os;
        os << val;
        str = os.str();
    }
    bool operator== (const bindable& b) const {
        return str == b.str && grow == b.grow;
    }
    bool operator!= (const bindable& b) const {
        return str != b.str || grow != b.grow;
    }
    bool empty() const {
        return str.empty();
    }
    bool zero() const {
        if (empty())
            return false;
        std::istringstream is(str);
        float val;
        if (!(is >> val) || val)
            return false;
        return is.eof() || is.tellg() == str.size();
    }
    bool stretched() const {
        return grow;
    }
    bool has_value() const {
        return cpp::is_literal(str);
    }
    float value() const {
        if (!has_value())
            return {};
        std::istringstream is(str);
        float val{};
        is >> val;
        return val;
    }
    float eval_px(int axis, const UIContext& ctx) const;

    bool set_from_arg(std::string_view s) {
        str = s;
        stretch(false);
        //strip unit calculation
        std::string_view factor = s.size() > 3 ? s.substr(s.size() - 3) : "";
        if (factor == "*fs" || factor == "*dp")
        {
            std::istringstream is(std::string(s.substr(0, s.size() - 3)));
            float val;
            if ((is >> val) && is.eof())
                str = s.substr(0, s.size() - 3);
        }
        else if (s.size() && s.back() == 'x')
        {
            std::istringstream is(std::string(s.substr(0, s.size() - 1)));
            float val;
            if ((is >> val) && is.eof())
                str.pop_back();
            stretch(true);
        }
        return true;
    }
    std::string to_arg(std::string_view unit, std::string_view stretchCode = "") const {
        if (stretched())
        {
            if (stretchCode != "")
                return std::string(stretchCode);
            return str + "x";
        }
        if (unit != "" && has_value() &&
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
    bool operator== (const bindable& b) const {
        return str == b.str;
    }
    bool operator!= (const bindable& b) const {
        return str != b.str;
    }
    bool empty() const { return str.empty(); }
    const std::string& value() const { return str; }
    bool set_from_arg(std::string_view s)
    {
        str = cpp::parse_str_arg(s);
        return str != cpp::INVALID_TEXT;
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
                        str.replace(id.data() - s.data() + i, id.size(), newn);
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
    bool has_single_variable() const
    {
        return str.size() > 2 && str[0] == '{' &&
            str.find('{', 1) == std::string::npos &&
            str.back() == '}';
    }
    std::string to_arg(std::string_view = "", std::string_view = "") const
    {
        return cpp::to_str_arg(str, true);
    }
};

template <>
struct bindable<font_name_t> : property_base
{
    bindable() {
    }
    bindable(std::string_view fn) {
        set_font_name(fn);
    }
    bool operator== (const bindable& b) const {
        return str == b.str;
    }
    bool operator!= (const bindable& b) const {
        return str != b.str;
    }
    bool empty() const { return str.empty(); }
    bool has_value() const {
        return !str.compare(0, 22, "ImRad::GetFontByName(\"");
    }
    std::string eval(const UIContext& ctx) const;

    void set_font_name(std::string_view fn) {
        str = "ImRad::GetFontByName(\"" + std::string(fn) + "\")";
    }
    bool set_from_arg(std::string_view s) {
        str = s;
        return true;
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
struct bindable<color_t> : property_base
{
    bindable() {
    }
    bindable(ImU32 c) {
        std::ostringstream os;
        if (c)
            os << "0x" << std::hex << std::setw(2 * 4) << std::setfill('0') << c;
        str = os.str();
    }
    bool operator== (const bindable& b) const {
        return str == b.str;
    }
    bool operator!= (const bindable& b) const {
        return str != b.str;
    }
    bool empty() const { return str.empty(); }
    /*bool has_value() const {
        if (empty())
            return false;
        std::istringstream is(str);
        color_t val;
        if (!(is >> val))
            return false;
        if (is.eof())
            return true;
        return is.tellg() == str.size();
    }*/
    ImU32 eval(int col, const UIContext& ctx) const;

    bool has_style_color() const {
        if (!str.compare(0, 34, "ImGui::GetStyle().Colors[ImGuiCol_") && str.back() == ']') //compatibility
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
    bool set_from_arg(std::string_view s) {
        str = s;
        return true;
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
    std::string container_expr() {
        auto vars = limit.used_variables();
        if (vars.empty())
            return "";
        const std::string& var = vars[0];
        const std::string& str = *limit.access();
        size_t i = str.find(var);
        if ((i && str[i - 1] == '*') || !str.compare(i + var.size(), 2, "->"))
            return "(*" + var + ")";
        return var;
    }

    bool set_from_arg(std::string_view code) {
        if (code.compare(0, 4, "for("))
            return false;
        bool local = !code.compare(4, 3, "int") || !code.compare(4, 6, "size_t");
        auto i = code.find(";");
        if (i == std::string::npos)
            return false;
        code.remove_prefix(i + 1);

        i = code.find(";");
        if (i == std::string::npos)
            return false;
        code.remove_suffix(code.size() - i);

        i = code.find("<");
        if (local)
            *index.access() = "";
        else
            *index.access() = code.substr(0, i);

        limit.set_from_arg(code.substr(i + 1));
        return true;
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

