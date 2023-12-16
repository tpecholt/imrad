#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <imgui.h>
#include "stx.h"
#include "utils.h"
#include "cpp_parser.h"

struct UIContext;

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

struct color32
{
	color32(ImU32 c = 0) : c(c) {} //0 means style default color
	color32(const ImVec4& v) : c(IM_COL32(255*v.x, 255*v.y, 255*v.z, 255*v.w)) {}
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
		return os << "0x" << std::hex << std::setw(2*4) 
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

//------------------------------------------------------------

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
	virtual std::string to_arg(std::string_view unit) const = 0;
	virtual void set_from_arg(std::string_view s) = 0;
	virtual const char* c_str() const = 0;
	virtual std::vector<std::string> used_variables() const = 0;
	virtual void rename_variable(const std::string& oldn, const std::string& newn) = 0;
	virtual void scale_dimension(float scale) = 0;
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
	
	template <class U = T>
	U eval(const UIContext& ctx) const;
	
	void set_from_arg(std::string_view s) {
		str = s;
	}
	std::string to_arg(std::string_view = "") const {
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
	void scale_dimension(float)
	{}
	const char* c_str() const { return str.c_str(); }
	std::string* access() { return &str; }
private:
	std::string str;
};

//member function name
template <class Arg = void>
struct event : property_base
{
	bool empty() const {
		return str.empty();
	}
	void set_from_arg(std::string_view s) {
		str = s;
	}
	std::string to_arg(std::string_view = "") const {
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
	void scale_dimension(float)
	{}
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
	std::string to_arg(std::string_view = "") const {
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
	void scale_dimension(float scale)
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
		if (factor == "*fs" || factor == "*dp")
		{
			std::istringstream is(std::string(s.substr(0, s.size() - 3)));
			dimension v;
			if ((is >> v) && is.eof())
				val = v;
		}
	}
	std::string to_arg(std::string_view unit) const {
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
	void scale_dimension(float scale)
	{
		val = (int)std::round(100 * val * scale) / 100.f;
	}
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
		if (factor == "*fs" || factor == "*dp")
		{
			std::istringstream is(std::string(s.substr(0, s.size() - 3)));
			dimension v;
			if ((is >> v) && is.eof())
				val = v;
		}
	}
	std::string to_arg(std::string_view unit) const {
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
	void scale_dimension(float scale)
	{
		if (!has_value())
			return;
		val = (int)std::round(100 * val * scale) / 100.f;
	}
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
	std::string to_arg(std::string_view unit) const {
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
	void scale_dimension(float scale)
	{
		if (!has_value())
			return;
		val = ImVec2{
			(int)std::round(100 * val[0] * scale) / 100.f,
			(int)std::round(100 * val[1] * scale) / 100.f
		};
	}
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
	
	void set_from_arg(std::string_view s) {
		val = cpp::parse_str_arg(s);
	}
	std::string to_arg(std::string_view = "") const {
		return cpp::to_str_arg(val);
	}
	std::vector<std::string> used_variables() const {
		return {};
	}
	void rename_variable(const std::string& oldn, const std::string& newn)
	{}
	void scale_dimension(float)
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
	std::string to_arg(std::string_view = "") const {
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
	void scale_dimension(float) {}
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
		if (!has_value()) return {};
		std::istringstream is(str);
		T val{};
		is >> std::boolalpha >> val;
		return val;
	}
	//T eval(const UIContext& ctx) const;

	void set_from_arg(std::string_view s) {
		str = s;
	}
	std::string to_arg(std::string_view = "") const {
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
	void scale_dimension(float scale) {}
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
	bindable(float val) {
		std::ostringstream os;
		os << val;
		str = os.str();
	}
	bool empty() const { return str.empty(); }
	bool zero() const {
		if (empty())
			return false;
		std::istringstream is(str);
		dimension val;
		if (!(is >> val) || val)
			return false;
		return is.eof() || is.tellg() == str.size();
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
	float eval_px(const UIContext& ctx) const;
	
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
	std::string to_arg(std::string_view unit) const {
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
	void scale_dimension(float scale)
	{
		if (has_value()) {
			float val = (int)std::round(100 * value() * scale) / 100.f;
			*this = val;
		}
	}
	const char* c_str() const { return str.c_str(); }
	std::string* access() { return &str; }

private:
	dimension value() const {
		if (!has_value()) return {};
		std::istringstream is(str);
		dimension val{};
		is >> val;
		return val;
	}

	std::string str;
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
	std::string to_arg(std::string_view = "") const
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
	void scale_dimension(float) {}
	const char* c_str() const { return str.c_str(); }
	std::string* access() { return &str; }
private:
	std::string str;
};

//{items} or prvni\0druha\0
template <>
struct bindable<std::vector<std::string>> : property_base
{
	bindable() {}
	bindable(const char* val) {
		str = val;
	}
	bindable(const std::string& val) {
		str = val;
	}
	bool empty() const { return str.empty(); }
	void set_from_arg(std::string_view s)
	{
		str = cpp::parse_str_arg(s);
		if (str.empty() && s != "\"\"")
			str = "{" + s + "}";
	}
	std::string to_arg(std::string_view = "") const
	{
		auto vars = used_variables();
		if (vars.empty())
			return cpp::to_str_arg(str);
		else
			return vars[0];
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
				size_t e = i + 1;
				for (; e < str.size(); ++e)
					if (!std::isalnum(str[e]) && str[e] != '_')
						break;
				if (e == str.size())
					break;
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
	};
	void scale_dimension(float) {}
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
	color32 eval(const UIContext& ctx) const;

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
	std::string to_arg(std::string_view = "") const {
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
	void scale_dimension(float scale) {}
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
	std::string index_name_or(const std::string& s) const {
		return index.empty() ? s : index.to_arg();
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
	std::string to_arg(std::string_view vn) const {
		if (empty())
			return "";
		std::ostringstream os;
		std::string name = index_name_or(std::string(vn));
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
	void scale_dimension(float)
	{}
	const char* c_str() const { return limit.c_str(); }
	std::string* access() { return limit.access(); }
};

