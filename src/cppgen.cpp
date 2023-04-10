#include "cppgen.h"
#include "stx.h"
#include "utils.h"
#include <fstream>
#include <cctype>
#include <set>

const std::string CppGen::INDENT = "    ";


CppGen::CppGen()
	: m_name("Untitled"), m_vname("untitled")
{
	m_fields[""];
}

void CppGen::SetNamesFromId(const std::string& fname)
{
	std::string name;
	bool upper = false;
	for (char c : fname) {
		if (c == '_' || c == ' ')
			upper = true;
		else if (upper) {
			name += std::toupper(c);
			upper = false;
		}
		else
			name += c;
	}
	m_name = (char)std::toupper(name[0]) + name.substr(1);
	m_vname = (char)std::tolower(name[0]) + name.substr(1);
}

std::string CppGen::AltFName(const std::string& path)
{
	fs::path p(path);
	if (p.extension() == ".h")
		return p.replace_extension("cpp").string();
	if (p.extension() == ".cpp")
		return p.replace_extension("h").string();
	return "";
}

bool CppGen::Export(const std::string& fname, bool trunc, UINode* node, std::string& err)
{
	//enforce uppercase for class name
	/*auto ext = name.find('.');
	m_name = name.substr(0, ext);
	if (m_name.empty() || m_name[0] < 'A' || m_name[0] > 'Z') {
		err = "class name doesn't start with big letter";
		return false;
	}
	m_vname = (char)std::tolower(m_name[0]) + m_name.substr(1);
	*/

	UIContext ctx;
	ctx.codeGen = this;
	std::ostringstream code;
	node->Export(code, ctx);
	
	auto hpath = fs::path(fname).replace_extension(".h");
	if (trunc || !fs::exists(hpath) || fs::is_empty(hpath))
	{
		std::ofstream fout(hpath);
		CreateH(fout);
	}
	std::ifstream fin(hpath);
	std::string line;
	std::stringstream fprev;
	while (std::getline(fin, line))
		fprev << line << "\n";
	fin.close();
	fprev.seekg(0);
	std::ofstream fout(hpath, std::ios::trunc);
	auto origNames = ExportH(fout, fprev, ctx.modalPopup);
	fout.close();

	auto fpath = fs::path(fname).replace_extension(".cpp");
	if (trunc || !fs::exists(fpath) || fs::is_empty(fpath))
	{
		std::ofstream fout(fpath);
		CreateCpp(fout, hpath.filename().string());
	}
	fin.open(fpath);
	fprev.str("");
	fprev.clear();
	while (std::getline(fin, line))
		fprev << line << "\n";
	fin.close();
	fprev.seekg(0);
	fout.open(fpath, std::ios::trunc);
	ExportCpp(fout, fprev, origNames, ctx.modalPopup, code.str());

	err = "";
	for (const std::string& e : ctx.errors)
		err += e + "\n";
	return true;
}

void CppGen::CreateH(std::ostream& out)
{
	out << "// Generated with " << VER_STR << "\n"
		<< "// visit " << GITHUB_STR << "\n\n";

	out << "#pragma once\n";
	out << "#include \"imrad.h\"\n";

	out << "\nclass " << m_name << "\n{\npublic:\n";
	out << INDENT << "/// @interface\n";

	out << "\nprivate:\n";
	out << INDENT << "/// @impl\n";

	out << "};\n\n";

	out << "extern " << m_name << " " << m_vname << ";\n";
}

void CppGen::CreateCpp(std::ostream& out, const std::string& hName)
{
	out << "// Generated with " << VER_STR << "\n"
		<< "// visit " << GITHUB_STR << "\n\n";

	out << "#include \"" << hName << "\"\n";
	out << "\n";

	out << m_name << " " << m_vname << ";\n\n";
}

//follows fprev and overwrites generated members/functions only
std::array<std::string, 2> 
CppGen::ExportH(std::ostream& fout, std::istream& fprev, bool modalPopup)
{
	int level = 0;
	bool in_class = false;
	int skip_to_level = -1;
	std::vector<std::string> line;
	std::streampos fpos = 0;
	bool ignore_section = false;
	std::string origName, origVName;
	std::stringstream out;

	auto copy_content = [&] {
		std::string buf;
		auto n = fprev.tellg() - fpos;
		fprev.seekg(fpos);
		buf.resize(n);
		fprev.read(buf.data(), n);
		out.write(buf.data(), n);
		fpos += n;
	};
	
	for (cpp::token_iterator iter(fprev); iter != cpp::token_iterator(); ++iter)
	{
		std::string tok = *iter;
		if (in_class && level == 1) //class scope
		{
			if (tok == "/// @interface")
			{
				ignore_section = true;
				copy_content();
				out << "\n";

				//write special members
				if (modalPopup)
				{
					out << INDENT << "void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});\n";
					out << INDENT << "void ClosePopup();\n";
				}
				out << INDENT << "void Draw();\n\n";

				//write fields
				for (const auto& scope : m_fields)
				{
					if (scope.first == "")
						continue;
					bool userCode = stx::count_if(scope.second, [](const auto& var) { return var.flags & Var::UserCode; });
					if (userCode)
						continue;
					out << INDENT << "struct " << scope.first << " {\n";
					for (const auto& var : scope.second)
					{
						out << INDENT << INDENT << var.type << " " << var.name;
						if (var.init != "")
							out << " = " << var.init;
						out << ";\n\n";
					}
					out << INDENT << "};\n";
				}
				for (const auto& var : m_fields[""])
				{
					if ((var.flags & Var::UserCode) ||
						!(var.flags & Var::Interface))
						continue;
					if (!var.type.compare(0, 5, "void("))
						out << INDENT << "void " << var.name << "();\n";
					else {
						out << INDENT << var.type << " " << var.name;
						if (var.init != "")
							out << " = " << var.init;
						out << ";\n";
					}
				}
			}
			else if (tok == "/// @impl")
			{
				ignore_section = true;
				copy_content();
				out << "\n";

				//write events
				for (const auto& var : m_fields[""])
				{
					if ((var.flags & Var::UserCode) ||
						!(var.flags & Var::Impl))
						continue;
					if (!var.type.compare(0, 5, "void("))
						out << INDENT << "void " << var.name << "();\n";
					else {
						out << INDENT << var.type << " " << var.name;
						if (var.init != "")
							out << " = " << var.init;
						out << ";\n";
					}
				}
				//write special fields
				if (modalPopup)
				{
					out << "\n";
					out << INDENT << "bool requestOpen = false;\n";
					out << INDENT << "bool requestClose = false;\n";
					out << INDENT << "std::function<void(ImRad::ModalResult)> callback;\n\n";
				}
			}
			else if (!tok.compare(0, 1, "#") || !tok.compare(0, 2, "//"))
				;
			else if (tok == ";") {
				line.clear();
			}
			else if (tok == "{") {
				++level;
				line.clear();
			}
			else if (tok == "}") {
				--level;
				if (in_class && !level) {
					in_class = false;
					if (ignore_section) {
						fpos = fprev.tellg();
						out << tok;
					}
				}
				line.clear();
			}
			if (tok == "public" || tok == "private" || tok == "protected")
			{
				if (ignore_section) {
					ignore_section = false;
					fpos = fprev.tellg();
					out << "\n" << tok;
				}
			}
			else if (tok == ":")
				;
			else
				line.push_back(tok);
		}
		else if (tok == "{") {
			++level;
			if (line[0] == "class" && level == 1) {
				in_class = true;
				origName = line[1];
			}
			line.clear();
		}
		else if (tok == "}") {
			--level;
			if (skip_to_level == level) {
				skip_to_level = -1;
				fpos = fprev.tellg();
				out << tok;
			}
			line.clear();
		}
		else if (!tok.compare(0, 1, "#") || !tok.compare(0, 2, "//"))
		;
		else if (tok == ";") {
			if (line.size() == 3 && line[0] == "extern" && line[1] == origName) {
				origVName = line[2];
			}
			line.clear();
		}
		else
			line.push_back(tok);
	}
	
	//copy remaining code
	fprev.clear(); //clear EOF flag
	fprev.seekg(0, std::ios::end);
	copy_content();

	//replace class name
	std::string code = out.str();
	cpp::replace_id(code, origName, m_name);
	cpp::replace_id(code, origVName, m_vname);

	//flush
	fout << code;
	return { origName, origVName };
}

//follows fprev and overwrites generated members/functions only only
void CppGen::ExportCpp(std::ostream& fout, std::istream& fprev, const std::array<std::string, 2>& origNames, bool modalPopup, const std::string& code)
{
	int level = 0;
	int skip_to_level = -1;
	std::vector<std::string> line;
	std::streampos fpos = 0;
	std::set<std::string> events;
	
	auto copy_content = [&](size_t ignore_last = 0) {
		std::string buf;
		auto n = fprev.tellg() - fpos;
		fprev.seekg(fpos);
		buf.resize(n - ignore_last);
		fprev.read(buf.data(), buf.size());
		fout.write(buf.data(), buf.size());
		fprev.ignore(ignore_last);
		fpos += n;
	};

	for (cpp::token_iterator iter(fprev); iter != cpp::token_iterator(); ++iter)
	{
		std::string tok = *iter;
		if (!level) //global scope
		{
			if (!tok.compare(0, 1, "#") || !tok.compare(0, 2, "//"))
				;
			else if (tok == ";") {
				line.clear();
			}
			else if (tok == "{") {
				++level;
				if (auto name = IsMemFun(line, origNames[0]); name != "")
				{
					events.insert(name);
					if (IsMemFun0(line, origNames[0]) == "Draw")
					{
						skip_to_level = level - 1;
						copy_content();
						//write draw code
						fout << "\n" << code;
					}
				}
				line.clear();
			}
			else if (tok == "}") {
				--level;
			}
			else {
				//replace old names
				if (tok == origNames[0]) { 
					copy_content(origNames[0].size());
					fout << m_name;
				}
				else if (tok == origNames[1]) {
					copy_content(origNames[1].size());
					fout << m_vname;
				}
				line.push_back(tok);
			}
		}
		else if (tok == "{") {
			++level;
		}
		else if (tok == "}") {
			--level;
			if (skip_to_level == level) {
				skip_to_level = -1;
				fpos = fprev.tellg();
				fout << tok;
			}
		}
	}

	//copy remaining code
	fprev.clear(); //clear EOF flag
	fprev.seekg(0, std::ios::end);
	copy_content();

	//add missing members
	if (modalPopup && !events.count("OpenPopup"))
	{
		fout << "\nvoid " << m_name << "::OpenPopup(std::function<void(ImRad::ModalResult)> clb)\n{\n";
		fout << INDENT << "callback = clb;\n";
		fout << INDENT << "requestOpen = true;\n";
		fout << INDENT << "requestClose = false;\n\n";
		fout << INDENT << "//*** Add your init code here\n";
		fout << "}\n";
	}

	if (modalPopup && !events.count("ClosePopup"))
	{
		fout << "\nvoid " << m_name << "::ClosePopup()\n{\n";
		fout << INDENT << "requestClose = true;\n";
		fout << "}\n";
	}

	if (!events.count("Draw"))
	{
		fout << "\nvoid " << m_name << "::Draw()\n{\n";
		fout << code << "}\n";
	}

	//add missing events
	for (const auto& var : m_fields[""])
	{
		if (var.flags & Var::UserCode)
			continue;
		if (var.type.compare(0, 5, "void("))
			continue;
		if (!events.count(var.name))
			fout << "\nvoid " << m_name << "::" << var.name << "()\n{\n}\n";
	}
}

std::unique_ptr<UINode> 
CppGen::Import(const std::string& path, std::string& err)
{
	m_fields.clear();
	m_fields[""];
	m_name = m_vname = "";
	m_error = "";
	ctx_fname = path;
	std::unique_ptr<UINode> node;

	auto fpath = fs::path(path).replace_extension("h");
	std::ifstream fin(fpath.string());
	if (!fin)
		m_error += "Can't read " + fpath.string() + "\n";
	else
		node = ImportCode(fin);
	fin.close();

	fpath = fs::path(path).replace_extension("cpp");
	fin.open(fpath.string());
	if (!fin)
		m_error += "Can't read " + fpath.string() + "\n";
	else {
		auto node2 = ImportCode(fin);
		if (!node)
			node = std::move(node2);
	}
	
	if (m_name == "")
		m_error += "No window class found!\n";
	if (m_vname == "")
		m_error += "No window class variable found!\n";
	if (!node)
		m_error += "No Draw() code found!\n";
	/*if (!found_fields)
		m_error += "No fields section found!\n";
	if (!found_events)
		m_error += "No events section found!\n";*/
	err = m_error;
	return node;
}

std::unique_ptr<UINode> 
CppGen::ImportCode(std::istream& fin)
{
	std::unique_ptr<UINode> node;
	cpp::token_iterator iter(fin);
	int level = 0;
	bool in_class = false;
	bool in_interface = false;
	bool in_impl = false;
	bool found_interface = false;
	bool found_impl = false;
	std::string sname;
	std::vector<std::string> line;
	while (iter != cpp::token_iterator())
	{
		std::string tok = *iter++;
		bool is_comment = !tok.compare(0, 2, "//");
		if (in_class && (tok == "public" || tok == "private" || tok == "protected"))
		{
			if (level == 1)
				in_interface = in_impl = false;
		}
		else if (in_class && level == 1 && tok == ":")
			;
		else if (!level || in_class) //global or class scope
		{
			if (in_class && tok == "/// @interface") {
				in_interface = true;
				found_interface = true;
			}
			else if (in_class && tok == "/// @impl") {
				in_impl = true;
				found_impl = true;
			}
			else if (!tok.compare(0, 1, "#") || !tok.compare(0, 2, "//"))
				;
			else if (tok == ";") {
				if (in_class) {
					ParseFieldDecl(sname, line, in_interface ? Var::Interface : in_impl ? Var::Impl : Var::UserCode);
				}
				else if (!level && line.size() == 3 && line[0] == "extern" && line[1] == m_name)
					m_vname = line[2];
				line.clear();
			}
			else if (tok == "{") {
				++level;
				if (line[0] == "class" || line[0] == "struct") {
					if (level == 1 && line[0] == "class") {
						m_name = line[1];
						in_class = true;
						sname = "";
					}
					else {
						sname = line[1];
						m_fields[sname];
					}
				}
				else {
					auto nod = ParseFunDef(line, iter);
					if (nod)
						node = std::move(nod);
				}
				line.clear();
			}
			else if (tok == "}") {
				--level;
				if (in_class && !level)
					in_class = false;
				else
					sname = "";
			}
			else
				line.push_back(tok); 
		}
		else if (tok == "{") {
			++level;
		}
		else if (tok == "}") {
			--level;
		}
	}

	return node;
}

bool CppGen::ParseFieldDecl(const std::string& sname, const std::vector<std::string>& line, int flags)
{
	if (line.empty())
		return false;
	if (line[0] == "struct" || line[0] == "class") //forward declaration
		return false;
	if (line[0] == "using" || line[0] == "typedef" || line[0] == "namespace")
		return false;
	if (line[0] == "enum")
		return false;
	
	if (line[0] == "void" && line.back() == ")" && line[line.size() - 2] == "(")
	{
		std::string name = stx::join(line.begin() + 1, line.end() - 2, "");
		if (name != "ClosePopup" && name != "Draw") {
			CreateNamedVar(name, "void()", "", flags, sname);
		}
	}
	else
	{
		//todo: parse std::function<void(xxx)> koko;
		if (stx::count(line, "(") || stx::count(line, ")"))
			return false;
		std::string type, name, init;
		size_t name_idx;
		auto it = stx::find(line, "=");
		if (it != line.end()) {
			name_idx = it - line.begin() - 1;
			if (it + 1 == line.end()) //= without initializer
				return false;
			for (++it; it != line.end(); ++it)
				init += *it + " ";
			init.pop_back();
		}
		else {
			name_idx = (int)line.size() - 1;
		}
		if (name_idx < 1) //no name or type
			return false;
		name = line[name_idx];
		for (size_t i = 0; i < name_idx; ++i) {
			if (line[i] == "::" || line[i] == "<" || line[i] == ">") {
				if (type != "")
					type.pop_back();
				type += line[i];
			}
			else
				type += line[i] + " ";
		}
		if (type.back() == ' ')
			type.pop_back();
		if (name != "requestOpen" && name != "requestClose" && name != "callback")
			CreateNamedVar(name, type, init, flags, sname);
	}
	return true;
}

std::string CppGen::IsMemFun(const std::vector<std::string>& line, const std::string& cname)
{
	if (line.size() >= 6 &&
		line[0] == "void" && line[1] == cname && line[2] == "::" &&
		line[4] == "(" && line.back() == ")")
	{
		return line[3];
	}
	return "";
}

std::string CppGen::IsMemFun0(const std::vector<std::string>& line, const std::string& cname)
{
	if (line.size() == 6)
		return IsMemFun(line, cname);
	return "";
}

std::unique_ptr<UINode>
CppGen::ParseFunDef(const std::vector<std::string>& line, cpp::token_iterator& iter)
{
	if (IsMemFun0(line, m_name) == "Draw")
	{
		cpp::stmt_iterator sit(iter);
		while (sit != cpp::stmt_iterator()) 
		{
			if (sit->line == "/// @begin TopWindow")
				break;
			++sit;
		}
		UIContext ctx;
		ctx.codeGen = this;
		ctx.fname = ctx_fname;
		auto node = std::make_unique<TopWindow>(ctx);
		node->Import(sit, ctx);
		iter = sit.base();
		for (const std::string& e : ctx.errors)
			m_error += e + "\n";
		return node;
	}
	return {};
}

std::string CppGen::CreateVar(const std::string& type, const std::string& init, int flags, const std::string& scope)
{
	auto vit = m_fields.find(scope);
	if (vit == m_fields.end())
		return "";
	if (type.empty())
		return "";
	//generate new var
	int max = 0;
	for (const auto& var : vit->second)
	{
		if (!var.name.compare(0, 5, "value"))
		{
			try {
				int val = std::stoi(var.name.substr(5));
				if (val > max)
					max = val;
			}
			catch (std::exception&) {}
		}
	}
	std::string name = "value" + std::to_string(++max);
	vit->second.push_back(Var(name, type, init, flags));
	return name;
}

bool CppGen::CreateNamedVar(const std::string& name, const std::string& type, const std::string& init, int flags, const std::string& scope)
{
	auto vit = m_fields.find(scope);
	if (vit == m_fields.end())
		return false;
	if (type.empty())
		return false;
	if (FindVar(name, scope))
		return false;
	vit->second.push_back(Var(name, type, init, flags));
	return true;
}

bool CppGen::RenameVar(const std::string& oldn, const std::string& newn, const std::string& scope)
{
	if (FindVar(newn, scope))
		return false;
	auto* var = FindVar(oldn, scope);
	if (!var)
		return false;
	var->name = newn;
	return true;
}

bool CppGen::RemoveVar(const std::string& name, const std::string& scope)
{
	auto vit = m_fields.find(scope);
	if (vit == m_fields.end())
		return false;
	auto it = stx::find_if(vit->second, [&](const auto& var) { return var.name == name; });
	if (it == vit->second.end())
		return false;
	vit->second.erase(it);
	return true;
}

bool CppGen::ChangeVar(const std::string& name, const std::string& type, const std::string& init, const std::string& scope)
{
	auto* var = FindVar(name, scope);
	if (!var)
		return false;
	var->type = type;
	var->init = init;
	return true;
}

const CppGen::Var* CppGen::GetVar(const std::string& name, const std::string& scope) const
{
	auto vit = m_fields.find(scope);
	if (vit == m_fields.end())
		return {};
	auto it = stx::find_if(vit->second, [&](const auto& var) { return var.name == name; });
	if (it == vit->second.end())
		return {};
	else
		return &*it;
}

CppGen::Var* CppGen::FindVar(const std::string& name, const std::string& scope)
{
	return const_cast<Var*>(GetVar(name, scope));
}

const std::vector<CppGen::Var>& 
CppGen::GetVars(const std::string& scope)
{
	static std::vector<CppGen::Var> dummy;
	auto vit = m_fields.find(scope);
	if (vit == m_fields.end())
		return dummy;
	return vit->second;
}

std::vector<std::string> CppGen::GetStructTypes()
{
	std::vector<std::string> names;
	for (const auto& scope : m_fields)
		if (scope.first != "")
			names.push_back(scope.first);
	return names;
}

// name as:
//	id
//	id[*]
//	id[*].id
CppGen::VarExprResult 
CppGen::CheckVarExpr(const std::string& name, const std::string& type, const std::string& scope)
{
	auto i = name.find('[');
	if (i != std::string::npos)
	{
		std::string id1 = name.substr(0, i);
		if (!cpp::is_id(id1))
			return SyntaxError;
		auto j = name.find(']', i + 1);
		if (j == std::string::npos)
			return SyntaxError;
		std::string id2;
		if (j + 1 < name.size()) 
		{
			if (name[j + 1] != '.')
				return SyntaxError;
			id2 = name.substr(j + 2);
			if (!cpp::is_id(id2))
				return SyntaxError;
		}
		const auto* var = FindVar(id1, scope);
		if (!var) {
			return id2.empty() ? New : New_ImplicitStruct;
		}
		i = var->type.find_first_of('<');
		j = var->type.find_last_of('>');
		if (i == std::string::npos || j == std::string::npos || i > j)
			return ConflictError;
		if (var->type.compare(0, i, "std::vector"))
			return ConflictError;
		std::string stype = var->type.substr(i + 1, j - i - 1);
		if (id2 == "")
		{
			if (stype != type)
				return ConflictError;
		}
		else
		{
			var = FindVar(id2, stype); //id1 exists => id2 has to exist
			if (!var)
				return New;
			else if (var->type != type) 
				return ConflictError;
		}
		return Existing;
	}
	else
	{
		if (!cpp::is_id(name))
			return SyntaxError;
		auto* var = FindVar(name, scope);
		if (!var)
			return New;
		if (var->type != type)
			return ConflictError;
		else
			return Existing;
	}
}

//accepts patterns recognized by CheckVarExpr
//corrects [xyz]
bool CppGen::CreateVarExpr(std::string& name, const std::string& type, const std::string& scope1)
{
	auto SingularUpperForm = [](const std::string& id) {
		std::string sing;
		sing += std::toupper(id[0]);
		sing += id.substr(1);
		if (sing.back() == 's')
			sing.resize(sing.size() - 1);
		if (sing == id)
			sing += "Data";
		return sing;
	};
	size_t i = 0;
	std::string scope = scope1;
	while (i < name.size())
	{
		size_t j = name.find('.', i);
		bool leaf = j == std::string::npos;
		std::string id = leaf ? name.substr(i) : name.substr(i, j - i);
		bool array = id.back() == ']';
		if (array) {
			size_t b = id.find('[');
			if (b == std::string::npos)
				return false;
			name.erase(i + b + 1, id.size() - b - 2);
			name.insert(i + b + 1, FOR_VAR);
			if (j != std::string::npos)
				j += FOR_VAR.size() - (id.size() - b - 2);
			id.resize(b);
		}
		const Var* var = FindVar(id, scope);
		if (var)
		{
			if (leaf && !array)
				return var->type == type;
			auto b = var->type.find('<');
			auto e = var->type.find_last_of('>');
			if (b == std::string::npos || e == std::string::npos || b > e)
				return false;
			scope = var->type.substr(b + 1, e - b - 1);
			if (leaf)
				return scope == type;
		}
		else
		{
			std::string stype = leaf ? type : SingularUpperForm(id);
			if (stype == "struct") {
				if (FindVar(id, scope))
					return false;
				m_fields[id];
			}
			else if (!array) {
				bool fun = !stype.compare(0, 5, "void(");
				if (!CreateNamedVar(id, stype, "", fun ? Var::Impl : Var::Interface, scope))
					return false;
				if (!stype.compare(0, 12, "std::vector<")) {
					std::string etype = stype.substr(12, stype.size() - 12 - 1);
					if (etype != "" && etype[0] >= 'A' && etype[0] <= 'Z')
						m_fields[etype];
				}
			}
			else {
				if (!CreateNamedVar(id, "std::vector<" + stype + ">", "", Var::Interface, scope))
					return false;
				m_fields[stype];
			}
			scope = stype;
		}
		if (j == std::string::npos)
			break;
		i = j + 1;
	}
	return true;
}

//type=="" accepts all scalar variables
//type=="[]" accepts all arrays
//type==size_t accepts all size_t, int, vec.size()
//type==void() accepts all functions
//vector, array are searched recursively
std::vector<std::pair<std::string, std::string>> //(name, type) 
CppGen::GetVarExprs(const std::string& type)
{
	std::vector<std::pair<std::string, std::string>> ret;
	bool isFun = !type.compare(0, 5, "void(");
	for (const auto& f : m_fields[""])
	{
		//events
		if (!f.type.compare(0, 5, "void("))
		{
			if (f.type == type)
				ret.push_back({ f.name, f.type });
		}
		//arrays
		else if (!f.type.compare(0, 12, "std::vector<") ||
				!f.type.compare(0, 11, "std::array<"))
		{
			if (isFun)
				continue;
			auto i = f.type.find('<');
			auto j = f.type.find_last_of('>');
			if (j == std::string::npos || j < i)
				continue;
			if (type == "" || type == "[]" || type == f.type)
				ret.push_back({ f.name, f.type });
			if (type == "" || type == "size_t")
				ret.push_back({ f.name + ".size()", "size_t" });
			else {
				std::string stype = f.type.substr(i + 1, j - i - 1);
				if (type == "" || stype == type)
					ret.push_back({ f.name + "[" + FOR_VAR + "]", stype });
				/*todo: else if (!stype.compare(0, 10, "std::pair<"))
				{
				}*/
				else
				{
					auto scope = m_fields.find(stype);
					if (scope == m_fields.end())
						continue;
					for (const auto& ff : scope->second)
					{
						if (type == "" || ff.type == type)
							ret.push_back({ f.name + "[" + FOR_VAR + "]." + ff.name, ff.type });
					}
				}
			}
		}
		//scalars
		else if (type == "" || f.type == type ||
			(type == "size_t" && f.type == "int"))
		{
			ret.push_back({ f.name, f.type });
		}
	}
	return ret;
}

