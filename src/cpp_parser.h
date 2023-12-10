#pragma once
#include "stx.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include <array>

namespace cpp
{
	inline const std::string INVALID_TEXT = "???";
	
	inline bool is_id(std::string_view s)
	{
		if (s.empty())
			return false;
		if (s[0] < 0 || (!std::isalpha(s[0]) && s[0] != '_'))
			return false;
		for (size_t i = 1; i < s.size(); ++i)
			if (!std::isalnum(s[i]) && s[i] != '_')
				return false;
		return true;
	}

	struct token_iterator
	{
		token_iterator()
			: in(), line_mode(), eof(true)
		{}
		token_iterator(const token_iterator& it)
			: in(it.in), tok(it.tok), line_mode(it.line_mode), eof(it.eof)
		{}
		token_iterator(std::istream& is)
			: in(&is), line_mode(), eof()
		{
			++(*this);
		}
		bool get_line_mode() const {
			return line_mode;
		}
		void set_line_mode(bool m) {
			line_mode = m;
		}
		const std::string& operator* () const {
			return tok;
		}
		const std::string* operator-> () const {
			return &tok;
		}
		bool operator== (const token_iterator& it) const {
			if (eof != it.eof)
				return false;
			if (eof && it.eof)
				return true;
			return in == it.in && in->tellg() == it.in->tellg();
		}
		bool operator!= (const token_iterator& it) const {
			return !(*this == it);
		}
		token_iterator operator++ (int) {
			token_iterator it = *this;
			++(*this);
			return it;
		}
		token_iterator& operator++ () {
			if (eof)
				return *this;
			int in_comment = 0; //1 - //, 2 - /*
			bool in_string = false;
			bool in_pre = false;
			bool in_num = false;
			bool first_n = true;
			tok = "";
			//we putback breaking character so that it can be read in next run if needed
			//good for algorithms like replace_id
			while (in)
			{
				int c = in->get();
				if (c == EOF)
				{
					if (tok != "")
						break;
					eof = true;
					break;
				}
				else if (c == '\n')
				{
					if (in_comment == 2)
						tok += c;
					else if (tok != "") {
						in->putback(c);
						break;
					}
					else if (line_mode) {
						if (!first_n) { //ignore only first \n which was putback last time
							in->putback(c);
							break;
						}
						first_n = false;
					}
				}
				else if (std::isspace(c))
				{
					if (line_mode) //receive verbatim
						tok += c;
					else if (tok.empty()) //skip initial ws
						continue;
					else if (in_comment || in_string || in_pre)
						tok += c;
					else {
						in->putback(c);
						break;
					}
				}
				else
				{
					tok += c;
					if (line_mode && 
						tok.size() >= 3 && 
						std::all_of(tok.begin(), tok.end() - 3, [](char c){ return std::isspace(c); }) &&
						!tok.compare(tok.size() - 3, 3, "///"))
					{
						//hack - trim ws so our special comment will be recognized
						tok.erase(0, tok.size() - 3);
					}
					else if (in_comment == 2 && tok.size() >= 2 && !tok.compare(tok.size() - 2, 2, "*/"))
					{
						tok.resize(tok.size() - 2);
						break;
					}
					else if (in_string && c == '"')
					{
						break;
					}
					else if (!in_comment && !in_string && !in_pre && !line_mode)
					{
						if (!tok.compare(0, 2, "//")) {
							tok = "";
							in_comment = 1;
						}
						else if (!tok.compare(0, 2, "/*")) {
							tok = "";
							in_comment = 2;
						}
						else if (c == '\"')
						{
							if (tok.size() >= 2) {
								tok.resize(tok.size() - 1);
								in->putback(c);
								break;
							}
							in_string = true;
						}
						else if (c == '#')
						{
							if (tok.size() >= 2) {
								tok.resize(tok.size() - 1);
								in->putback(c);
								break;
							}
							in_pre = true;
						}
						else if (std::isdigit(c))
						{
							if (tok.size() == 1)
								in_num = true;
						}
						else if (c == '.' && in_num)
						{}
						else if ((c == '+' || c == '-') &&
							in_num &&
							tok.size() >= 2 && std::tolower(tok[tok.size() - 2]) == 'e')
						{}
						else if (c == '/')
						{
							if (in->peek() != '/' && in->peek() != '*')
								break;
						}
						else if (c == '{' || c == '}' || c == '(' || c == ')' || 
							c == '[' || c == ']' || c == '<' || c == '>' ||
							c == ';' || c == ':' || c == '.' || c == ',' ||
							c == '?' || c == '+' || c == '-' || c == '%' ||
							c == '*' || c == '^' || c == '&' || c == '|' || 
							c == '~' || c == '=' || c == '!')
						{
							if (tok.size() >= 2) { //output token before operator
								tok.resize(tok.size() - 1);
								in->putback(c);
								break;
							}
							if ((c == '<' && in->peek() == '<') ||
								(c == '<' && in->peek() == '=') ||
								(c == '>' && in->peek() == '>') ||
								(c == '>' && in->peek() == '=') ||
								(c == '=' && in->peek() == '=') ||
								(c == '!' && in->peek() == '=') ||
								(c == ':' && in->peek() == ':') ||
								(c == '-' && in->peek() == '>') ||
								(c == '&' && in->peek() == '&') ||
								(c == '|' && in->peek() == '|'))
							{
								tok += in->get();
							}
							break;
						}
					}
				}
			}
			if (in_comment)
				tok = "//" + tok;
			return *this;
		}

	private:
		std::istream* in;
		std::string tok;
		bool eof;
		bool line_mode;
	};

	enum Kind { CallExpr, IfCallBlock, IfCallThenCall, IfCallStmt, IfStmt, IfBlock, ForBlock, Comment, Other };

	struct stmt_iterator
	{
		struct data_t {
			int level = 0;
			Kind kind = Other;
			std::string line;
			std::string callee;
			std::vector<std::string> params;
			std::string cond;
			std::vector<std::string> params2;
		};

		stmt_iterator(const token_iterator& it)
			: iter(it)
		{
			++(*this);
		}
		/*stmt_iterator(const stmt_iterator& it)
			: iter(it.iter), tokens(it.tokens)
		{}*/
		stmt_iterator()
		{
			data.level = -1;
		}
		void enable_parsing(bool m) {
			iter.set_line_mode(!m);
		}
		bool operator== (const stmt_iterator& it) const {
			if (data.level < 0 && it.data.level < 0)
				return true;
			return iter == it.iter;
		}
		bool operator!= (const stmt_iterator& it) const {
			return !(*this == it);
		}
		const data_t& operator* () const {
			return data;
		}
		const data_t* operator->() const {
			return &data;
		}
		const token_iterator& base() const {
			return iter;
		}
		stmt_iterator& operator++ () {
			if (iter == token_iterator())
				return *this;
			if (tokens.size() == 1 && iter.get_line_mode())
				++iter;
			else if (tokens.size() == 1 && 
				(tokens[0].front() == '#' || !tokens[0].compare(0, 2, "//")))
				++iter;
			tokens.clear();
			int parenthesis = 0;
			int eat_level = 0;
			while (iter != token_iterator()) 
			{
				if (iter.get_line_mode()) {
					tokens.push_back(*iter);
					parse(false);
					break;
				}
				else if (*iter == ";" &&
					(tokens.empty() || tokens[0] != "for" || !parenthesis)) {
					if (!tokens.empty()) {
						parse(false);
						break;
					}
					++iter;
				}
				else if (*iter == "{" && !parenthesis) {
					if (!tokens.empty() && is_id(tokens.back())) {
						tokens.push_back(*iter);
						eat_level = ++data.level;
						++iter;
					}
					else if (!tokens.empty()) {
						parse(true);
						break;
					}
					else {
						++data.level;
						++iter;
					}
				}
				else if (*iter == "}" && !parenthesis) {
					if (eat_level) {
						--data.level;
						tokens.push_back(*iter);
						++iter;
						if (data.level < eat_level) {
							parse(false);
							break;
						}
					}
					else if (!tokens.empty()) {
						parse(false);
						break;
					}
					if (--data.level < 0) //don't read past original block
						break;
					++iter;
				}
				else if (*iter == "(") {
					tokens.push_back(*iter);
					++iter;
					++parenthesis;
				}
				else if (*iter == ")") {
					tokens.push_back(*iter);
					++iter;
					--parenthesis;
				}
				else if (iter->front() == '#' || !iter->compare(0, 2, "//")) {
					tokens.push_back(*iter);
					parse(false);
					break;
				}
				else {
					tokens.push_back(*iter);
					++iter;
				}
			}
			return *this;
		}
		void parse(bool block)
		{
			data.kind = Other;
			if (tokens.empty())
				return;
			if (!tokens[0].compare(0, 2, "//")) {
				data.kind = Comment;
				data.line = tokens[0];
			}
			else if (iter.get_line_mode()) {
				data.line = tokens[0];
			}
			else if (!parse_stmt(block)) {
				data.line = tokens[0];
				for (size_t i = 1; i < tokens.size(); ++i) {
					if ((data.line.back() == '_' || !std::ispunct(data.line.back())) &&
						(tokens[i][0] == '_' || !std::ispunct(tokens[i][0])))
						data.line += " ";
					data.line += tokens[i];
				}
			}
		}
		bool parse_stmt(bool block)
		{
			if (tokens.size() < 2)
				return false;
			if (tokens[0] == "if" || tokens[0] == "for" && tokens[1] == "(")
			{
				size_t b = 1, e;
				int level = 0;
				for (e = b + 1; e < tokens.size(); ++e)
				{
					if (tokens[e] == ")" && !level)
						break;
					if (tokens[e] == "{" || tokens[e] == "(" || tokens[e] == "[")
						++level;
					else if (tokens[e] == "}" || tokens[e] == ")" || tokens[e] == "]")
						--level;
				}
				if (e == tokens.size())
					return false;
				if (tokens[0] == "if")
				{
					std::string callee1, callee2;
					std::vector<std::string> params1, params2;
					if (parse_call(b + 1, e, callee1, params1)) {
						if (block) {
							data.kind = IfCallBlock;
							data.callee = callee1;
							data.params = params1;
						}
						else if (parse_call(e + 1, tokens.size(), callee2, params2)) {
							data.kind = IfCallThenCall;
							data.cond = callee1;
							data.params = params1;
							data.callee = callee2;
							data.params2 = params2;
						}
						else {
							data.kind = IfCallStmt;
							data.callee = callee1;
							data.params = params1;
						}
					}
					else {
						data.kind = block ? IfBlock : IfStmt;
						data.cond = stx::join(tokens.begin() + b + 1, tokens.begin() + e, "");
					}
				}
				else if (tokens[0] == "for")
				{
					if (!block)
						return false;
					data.kind = cpp::ForBlock;
					while (b != e && tokens[b] != ";")
						++b;
					if (b == e)
						return false;
					while (e != b && tokens[e] != ";")
						--e;
					data.cond = "";
					while (++b < e)
						data.cond += tokens[b];
				}
			}
			else if (parse_call(0, tokens.size(), data.callee, data.params)) {
				data.kind = CallExpr;
			}
			return false;
		}
		bool parse_call(size_t b1, size_t e1, std::string& callee, std::vector<std::string>& params)
		{
			size_t b = std::find(tokens.begin() + b1, tokens.begin() + e1, "(") - tokens.begin();
			if (b == e1)
				return false;
			size_t i = b1;
			size_t eq = std::find(tokens.begin() + b1, tokens.begin() + e1, "=") - tokens.begin();
			if (eq != e1 && eq < b) {
				//assignment expr
				i = eq + 1;
			}

			callee = stx::join(tokens.begin() + i, tokens.begin() + b, "");
			if (callee.empty())
				return false;
			if (!std::isalnum(callee.back()) && callee.back() != '_')
				return false; //not a call expr
			//params
			params.clear();
			++b;
			--e1;
			if (b != e1)
				params.push_back("");
			int level = 0;
			for (; b != e1; ++b)
			{
				if (tokens[b] == "{" || tokens[b] == "(" || tokens[b] == "[") {
					++level;
					params.back() += tokens[b];
				}
				else if (tokens[b] == "}" || tokens[b] == ")" || tokens[b] == "]") {
					--level;
					if (level >= 0)
						params.back() += tokens[b];
					else
						break; //if(call1() && call2())
				}
				else if (tokens[b] == "," && !level) {
					params.push_back("");
				}
				else
					params.back() += tokens[b];
			}

			return true;
		}

	private:
		token_iterator iter;
		std::vector<std::string> tokens;
		data_t data;
	};

	inline bool is_cstr(std::string_view s)
	{
		return s.size() >= 2 && s[0] == '"' && s.back() == '"';
	}

	inline bool is_builtin(std::string_view s)
	{
		if (!s.compare(0, 6, "const "))
			s.remove_prefix(6);
		return s == "int" || s == "short" || s == "unsigned" || s == "long" ||
			s == "unsigned int" || s == "unsigned short" || s == "unsigned long" ||
			s == "long long" || s == "unsigned long long" || s == "char" ||
			s == "unsigned char" || s == "char*" || s == "bool";
	}

	inline bool is_container(std::string_view s)
	{
		return !s.compare(0, 12, "std::vector<") ||
			!s.compare(0, 11, "std::array<") ||
			!s.compare(0, 10, "std::span<");
	}

	//replaces identifier but ignores strings, preprocessor
	inline void replace_id(std::string& code, std::string_view old, std::string news)
	{
		std::istringstream in(code);
		std::streampos pos = 0;
		std::string out;
		std::string buf;
		for (token_iterator iter(in); iter != token_iterator(); ++iter)
		{
			if (*iter != old)
				continue;
			//relies on token_iterator doing putback
			auto n = in.tellg() - pos;
			in.seekg(pos);
			buf.resize(n - old.size());
			in.read(buf.data(), buf.size());
			out += buf;
			in.ignore(old.size());
			out += news;
			pos += n;
		}
		in.clear(); //clear EOF flag
		in.seekg(0, std::ios::end);
		auto n = in.tellg() - pos;
		in.seekg(pos);
		buf.resize(n);
		in.read(buf.data(), n);
		out += buf;
		
		code = out;
	}

	//ImGui::GetStyle().Colors[alignment==0 ? ImGuiCol_X : ImGuiCol_Y] --> alignment, ImGuiCol_X, ImGuiCol_Y
	inline std::string_view find_id(const std::string& expr, size_t& i)
	{
		std::istringstream in(expr);
		in.seekg(i);
		std::string_view id;
		bool ignore_ids = false;
		for (token_iterator it(in); it != token_iterator(); ++it)
		{
			if (is_id(*it) || *it == "::") {
				if (ignore_ids)
					;
				else if (id != "") //append
					id = std::string_view(expr).substr(id.data() - expr.data(), expr.data() + in.tellg() - id.data());
				else {
					size_t pos = !in ? expr.size() : (size_t)in.tellg();
					id = std::string_view(expr).substr(pos - it->size(), it->size());
				}
			}
			else if (*it == "." || *it == "->") {
				ignore_ids = true;
				if (id != "") {
					i = (size_t)in.tellg() - it->size();
					return id;
				}
			}
			else if (id != "" && *it == "(") { //function call
				id = "";
				ignore_ids = false;
			}
			else {
				ignore_ids = false;
				if (id != "") {
					i = (size_t)in.tellg() - it->size();
					return id;
				}
			}
		}
		i = std::string::npos;
		return id;
	}

	inline std::string parse_var_args(const std::vector<std::string>& f)
	{
		std::string str = "";
		if (f.empty())
			return str;
		int state = 0;
		size_t arg = 1;
		for (size_t i = 1; i + 1 < f[0].size(); ++i)
		{
			char c = f[0][i];
			if (!state)
			{
				if (c == '%' && i + 1 < f[0].size())
				{
					c = f[0][i + 1];
					if (c == '%') {
						str += c;
						++i;
					}
					else
						state = 1;
				}
				else
					str += c;
			}
			else if (state)
			{
				if (isdigit(c) || c == '.' || c == '+' || c == '-' || c == '#')
					;
				else {
					str += "{" + f[arg++] + "}";
					state = 0;
				}
			}
		}
		return str;
	}

	inline std::string unescape(std::string_view str)
	{
		std::string tmp;
		for (size_t i = 0; i < str.size(); ++i)
		{
			switch (str[i])
			{
			default:
				tmp += str[i];
				break;
			case '\\':
				if (i + 1 == str.size())
					break;
				if (str[i + 1] == 't') {
					tmp += '\t';
					++i;
				}
				else if (str[i + 1] == 'n') {
					tmp += '\n';
					++i;
				}
				else if (str[i + 1] == '0') {
					tmp += '\0';
					++i;
				}
				else if (str[i + 1] == 'x' && i + 3 < str.size()) {
					int h = std::tolower(str[i + 2]) >= 'a' ? str[i + 2] - 'a' + 10 : str[i + 2] - '0';
					int l = std::tolower(str[i + 3]) >= 'a' ? str[i + 3] - 'a' + 10 : str[i + 3] - '0';
					tmp += (char)(h * 16 + l);
					i += 3;
				}
				break;
			}
		}
		return tmp;
	}

	inline std::string parse_str_arg(std::string_view str)
	{
		if (str == "0" || str == "NULL" || str == "nullptr")
		{
			return "";
		}
		else if (is_cstr(str))
		{
			return unescape(str.substr(1, str.size() - 2));
		}
		else if (!str.compare(0, 15, "ImRad::Format(\"") &&
				!str.compare(str.size() - 9, 9, ").c_str()")) 
		{
			auto find_curly = [](std::string_view s, size_t i) {
				--i;
				while (true) {
					i = s.find('{', i + 1);
					if (i != std::string::npos &&
						i + 1 < s.size() && s[i + 1] == '{')
						++i;
					else
						return i;
				}
				return std::string::npos;
			};
			std::istringstream is((std::string)str.substr(15 - 1, str.size() - 9 - 15 + 1));
			token_iterator it(is);
			std::string format = *it++;
			format = format.substr(1, format.size() - 2);
			std::string expr, str;
			size_t i = 0;
			int level = 0;
			while (true) {
				std::string tok = *it;
				if ((!level && tok == ",") || it == token_iterator()) {
					if (expr != "") {
						size_t i2 = find_curly(format, i);
						if (i2 == std::string::npos)
							return str;
						str += unescape(format.substr(i, i2 - i));
						str += "{" + expr;
						i = i2;
						i2 = format.find("}", i);
						if (i2 == std::string::npos)
							return str;
						str += format.substr(i + 1, i2 - i); //copy formating code
						i = i2 + 1;
						expr = "";
					}
				}
				else {
					if (tok == "(" || tok == "[") {
						expr += tok;
						++level;
					}
					else if (tok == ")" || tok == "]") {
						expr += tok;
						--level;
					}
					else if (cpp::is_cstr(tok))
						expr += unescape(tok);
					else
						expr += tok;
				}
				if (it == token_iterator())
					break;
				++it;
			}
			str += unescape(format.substr(i));
			return str;
		}
		else
			return INVALID_TEXT;
	}

	inline std::string escape(char c)
	{
		std::string str;
		switch (c)
		{
		default:
			if (c & 0x80) { //utf8
				int l = c & 0xf;
				int h = (c >> 4) & 0xf;
				l = l >= 10 ? l - 10 + 'a' : l + '0';
				h = h >= 10 ? h - 10 + 'a' : h + '0';
				str += "\\x";
				str += h;
				str += l;
			}
			else
				str += c;
			break;
		case '\0': //Combo.items use it
			str += "\\0";
			break;
		case '\t':
			str += "\\t";
			break;
		case '\n':
			str += "\\n";
			break;
		}
		return str;
	}

	inline std::string escape(std::string_view s)
	{
		std::string str;
		for (char c : s)
			str += escape(c);
		return str;
	}

	inline std::string to_str_arg(std::string_view str)
	{
		std::string fmt, args;
		for (size_t i = 0; i < str.size(); ++i)
		{
			switch (str[i])
			{
			default:
				fmt += escape(str[i]);
				break;
			case '{':
				if (i + 1 < str.size() && str[i + 1] == '{') {
					fmt += "{";
					++i;
				}
				else {
					size_t q = str.find('?', i + 1);
					size_t c = str.find(':', i + 1);
					size_t e = str.find('}', i + 1);
					if (e == std::string::npos)
						goto error;
					if (c != std::string::npos && c < e &&
						(q == std::string::npos || q > c)) //probably ?: operator misinterpreted as :fmt
					{
						args += ", " + str.substr(i + 1, c - i - 1);
						fmt += "{";
						fmt += str.substr(c, e - c + 1);
					}
					else {
						args += ", " + escape(str.substr(i + 1, e - i - 1));
						fmt += "{}";
					}
					i = e;
				}
				break;
			}
		}
error:
		if (args.empty())
			return "\"" + fmt + "\"";
		return "ImRad::Format(\"" + fmt + "\"" + args + ").c_str()";
	}

	inline std::pair<std::string, std::string> parse_size(const std::string& str)
	{
		std::pair<std::string, std::string> size;
		if (str.size() >= 2 && str[0] == '{' && str.back() == '}')
		{
			std::istringstream is(str.substr(1, str.size() - 2));
			token_iterator it(is);
			int level = 0, state = 0;
			for (; it != token_iterator(); ++it)
			{
				if (*it == "{" || *it == "(" || *it == "[")
					++level;
				if (*it == "}" || *it == ")" || *it == "]")
					--level;
				if (*it == "," && !level)
					state = 1;
				else if (!state)
					size.first += *it;
				else
					size.second += *it;
			}
		}
		return size;
	}

	inline ImVec2 parse_fsize(const std::string& str)
	{
		ImVec2 fsize{ 0, 0 };
		if (str.size() >= 2 && str[0] == '{' && str.back() == '}')
		{
			std::istringstream is(str.substr(1, str.size() - 2));
			is >> fsize.x;
			int c = is.get(); 
			while (c != EOF && c != ',') //ignore suffixes like 0.5f and *fs
				c = is.get();
			is >> fsize.y;
		}
		return fsize;
	}
}