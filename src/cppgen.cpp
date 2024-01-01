#include "cppgen.h"
#include "stx.h"
#include "utils.h"
#include <fstream>
#include <cctype>
#include <set>

const std::string CppGen::INDENT = "    ";
const std::string CppGen::FOR_VAR = "i";


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

bool CppGen::ExportUpdate(
	const std::string& fname, 
	TopWindow* node, 
	const std::map<std::string, std::string>& params, 
	std::string& err
)
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

	auto hpath = fs::path(fname).replace_extension(".h");
	if (!fs::exists(hpath) || fs::is_empty(hpath))
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
	auto origNames = ExportH(fout, fprev, m_hname, node);
	m_hname = hpath.filename().string();
	fout.close();

	auto fpath = fs::path(fname).replace_extension(".cpp");
	if (!fs::exists(fpath) || fs::is_empty(fpath))
	{
		std::ofstream fout(fpath);
		CreateCpp(fout);
	}
	fin.open(fpath);
	fprev.str("");
	fprev.clear();
	while (std::getline(fin, line))
		fprev << line << "\n";
	fin.close();
	fprev.seekg(0);
	fout.open(fpath, std::ios::trunc);
	err = ExportCpp(fout, fprev, origNames, params, node);
	return true;
}

void CppGen::CreateH(std::ostream& out)
{
	out << "// Generated with " << VER_STR << "\n"
		<< "// visit " << GITHUB_URL << "\n\n";

	out << "#pragma once\n";
	out << "#include \"imrad.h\"\n";

	out << "\nclass " << m_name << "\n{\npublic:\n";
	out << INDENT << "/// @begin interface\n";
	out << INDENT << "/// @end interface\n";

	out << "\nprivate:\n";
	out << INDENT << "/// @begin impl\n";
	out << INDENT << "/// @end impl\n";

	out << "};\n\n";

	out << "extern " << m_name << " " << m_vname << ";\n";
}

void CppGen::CreateCpp(std::ostream& out)
{
	out << "// Generated with " << VER_STR << "\n"
		<< "// visit " << GITHUB_URL << "\n\n";

	out << "#include \"" << m_hname << "\"\n";
	out << "\n";

	out << m_name << " " << m_vname << ";\n\n";
}

//follows fprev and overwrites generated members/functions only
std::array<std::string, 3> 
CppGen::ExportH(
	std::ostream& fout, 
	std::istream& fprev, 
	const std::string& origHName, 
	TopWindow* node
)
{
	int level = 0;
	bool in_class = false;
	int skip_to_level = -1;
	std::vector<std::string> line;
	std::streampos fpos = 0;
	bool ignore_section = false;
	std::string className;
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
			if (tok == "/// @interface" || tok == "/// @begin interface")
			{
				origName = className;
				ignore_section = true;
				copy_content();
				out << "\n";

				//write special members
				if (node->kind == TopWindow::ModalPopup)
				{
					out << INDENT << "void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});\n";
					out << INDENT << "void ClosePopup(ImRad::ModalResult mr = ImRad::Cancel);\n";
					out << INDENT << "void Draw();\n\n";
				}
				else if (node->kind == TopWindow::Popup)
				{
					out << INDENT << "void OpenPopup();\n";
					out << INDENT << "void ClosePopup();\n";
					out << INDENT << "void Draw();\n\n";
				}
				else if (node->kind == TopWindow::Window)
				{
					out << INDENT << "void Open();\n";
					out << INDENT << "void Close();\n";
					out << INDENT << "void Draw();\n\n";
				}
				else if (node->kind == TopWindow::MainWindow)
				{
					out << INDENT << "void Draw(GLFWwindow* window);\n\n";
				}
				else if (node->kind == TopWindow::Activity)
				{
					out << INDENT << "void Open();\n\n";
					out << INDENT << "void Draw();\n\n";
				}
				else
				{
					assert(false);
				}

				//write stuctures
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
						out << ";\n";
					}
					out << INDENT << "};\n";
				}
				//write fields
				for (const auto& var : m_fields[""])
				{
					if ((var.flags & Var::UserCode) || !(var.flags & Var::Interface))
						continue;
					if (!var.type.compare(0, 5, "void(")) {
						if (var.name == "Close" || var.name == "ClosePopup")
							continue;
						out << INDENT << "void " << var.name << "();\n";
					}
					else {
						out << INDENT << var.type << " " << var.name;
						if (var.init != "")
							out << " = " << var.init;
						out << ";\n";
					}
				}
			}
			else if (tok == "/// @impl" || tok == "/// @begin impl")
			{
				origName = className;
				ignore_section = true;
				copy_content();
				out << "\n";

				//write special fields
				if (node->kind == TopWindow::Popup || node->kind == TopWindow::ModalPopup ||
					node->kind == TopWindow::Activity)
				{
					out << INDENT << "void Init();\n\n";
				}

				if (node->kind == TopWindow::ModalPopup)
				{
					out << INDENT << "ImGuiID ID = 0;\n";
					out << INDENT << "ImRad::ModalResult modalResult;\n";
					if (node->animate) {
						out << INDENT << "ImRad::Animator animator;\n";
						out << INDENT << "ImVec2 animPos;\n";
					}
					out << INDENT << "std::function<void(ImRad::ModalResult)> callback;\n";
				}
				else if (node->kind == TopWindow::Popup)
				{
					out << INDENT << "ImGuiID ID = 0;\n";
					out << INDENT << "ImRad::ModalResult modalResult;\n";
					if (node->animate) {
						out << INDENT << "ImRad::Animator animator;\n";
						out << INDENT << "ImVec2 animPos;\n";
					}
				}
				else if (node->kind == TopWindow::Window)
				{
					out << INDENT << "bool isOpen = true;\n";
				}
				else if (node->kind == TopWindow::MainWindow || node->kind == TopWindow::Activity)
				{
				}
				else
				{
					assert(false);
				}

				//write impl fields
				out << "\n";
				for (const auto& var : m_fields[""])
				{
					if ((var.flags & Var::UserCode) || !(var.flags & Var::Impl))
						continue;
					if (!var.type.compare(0, 5, "void(")) {
						out << INDENT << "void " << var.name << "(";
						std::string arg = var.type.substr(5, var.type.size() - 6);
						if (arg.size())
							out << "const " << arg << "& args";
						out << ");\n";
					}
					else {
						out << INDENT << var.type << " " << var.name;
						if (var.init != "")
							out << " = " << var.init;
						out << ";\n";
					}
				}
			}
			else if (tok == "/// @end interface" || tok == "/// @end impl")
			{
				ignore_section = false;
				fpos = fprev.tellg();
				out << INDENT << tok;
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
				className = line[1];
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
	return { origName, origVName, origHName };
}

//follows fprev and overwrites generated members/functions only only
std::string 
CppGen::ExportCpp(
	std::ostream& fout, 
	std::istream& fprev, 
	const std::array<std::string, 3>& origNames, //name, vname, old header name
	const std::map<std::string, std::string>& params, 
	TopWindow* node
)
{
	int level = 0;
	int skip_to_level = -1;
	std::vector<std::string> line;
	std::streampos fpos = 0;
	std::set<std::string> events;
	auto animPos = node->animate ? (TopWindow::Placement)node->placement : TopWindow::None;

	UIContext ctx;
	ctx.codeGen = this;
	ctx.ind = INDENT;
	auto uit = params.find("unit");
	if (uit != params.end())
		ctx.unit = uit->second;
	std::ostringstream code;
	node->Export(code, ctx);
	std::string err;
	for (const std::string& e : ctx.errors)
		err += e + "\n";
	
	//xpos == 0 => copy until current position
	//xpos > 0 => copy until xpos
	//xpos < 0 => copy except last xpos characters
	auto copy_content = [&](int xpos = 0) {
		int pos = (int)fprev.tellg();
		int ignore_last = xpos < 0 ? -xpos : xpos > 0 ? pos - xpos : 0;
		std::string buf;
		fprev.seekg(fpos);
		buf.resize(pos - fpos - ignore_last);
		fprev.read(buf.data(), buf.size());
		fout.write(buf.data(), buf.size());
		fprev.ignore(ignore_last);
		fpos = pos;
	};

	for (cpp::token_iterator iter(fprev); iter != cpp::token_iterator(); ++iter)
	{
		std::string tok = *iter;
		if (!level) //global scope
		{
			if (origNames[2] != "" &&
				!tok.compare(0, 10, "#include \"") &&
				!tok.compare(10, origNames[2].size(), origNames[2]))
			{
				copy_content(-(int)tok.size());
				fout << "#include \"" << m_hname << "\"";
			}
			else if (!tok.compare(0, 2, "//") || !tok.compare(0, 1, "#"))
				;
			else if (tok == ";") {
				line.clear();
			}
			else if (tok == "{") {
				++level;
				auto name = IsMemFun(line);
				if (name != "")
				{
					events.insert(name);
					//rewrite generated functions
					if (name == "Draw")
					{
						skip_to_level = level - 1;
						WriteStub(fout, "Draw", node->kind, animPos, params, code.str());
					}
					else if (name == "OpenPopup" || name == "ClosePopup" ||
						name == "Open" || name == "Close") //ignore Init it contains user code
					{
						skip_to_level = level - 1;
						if (!WriteStub(fout, name, node->kind, animPos)) 
						{
							//this member is no longer needed => rollback hack
							fout.seekp((int)fout.tellp() - 5 - m_name.size());
							//fout << std::string(5 + m_name.size(), ' ');
							fout << "// void " << m_name << "::" << name << " REMOVED";
						}
					}
				}
				line.clear();
			}
			else if (tok == "}") {
				--level;
			}
			else {
				//replace old names & move cursor past consistently
				if (tok == m_name) {
					copy_content(-(int)tok.size());
					fout << m_name;
				}
				else if (tok == origNames[0]) {
					tok = m_name;
					copy_content(-(int)origNames[0].size());
					fout << m_name;
				}
				else if (tok == origNames[1]) {
					tok = m_vname;
					copy_content(-(int)origNames[1].size());
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
			}
		}
	}

	//copy remaining code
	fprev.clear(); //clear EOF flag
	fprev.seekg(0, std::ios::end);
	copy_content();

	//add missing members
	for (const char* name : { "OpenPopup", "ClosePopup", "Open", "Close", "Init", "Draw" })
	{
		if (events.count(name))
			continue;
		std::ostringstream os;
		if (WriteStub(os, name, node->kind, animPos, params, code.str())) {
			events.insert(name);
			fout << "\nvoid " << m_name << os.str() << "\n";
		}
	}

	//add missing events
	for (const auto& var : m_fields[""])
	{
		if (var.flags & Var::UserCode)
			continue;
		if (var.type.compare(0, 5, "void("))
			continue;
		if (events.count(var.name))
			continue;
		fout << "\nvoid " << m_name << "::" << var.name << "(";
		std::string arg = var.type.substr(5, var.type.size() - 6);
		if (arg.size())
			fout << "const " << arg << "& args";
		fout << ")\n{\n}\n";
	}

	return err;
}

//we always replace code of all generated functions because parameters and code depend
//on kind and other things
bool CppGen::WriteStub(
	std::ostream& fout, 
	const std::string& id, 
	TopWindow::Kind kind,
	TopWindow::Placement animPos,
	const std::map<std::string, std::string>& params,
	const std::string& code)
{
	//OpenPopup has to be called from the window which wants to open it so that's why
	//immediate call
	//CloseCurrentPopup has to be called from the popup Draw code so that's why
	//defered call
	if (id == "OpenPopup" && kind == TopWindow::ModalPopup)
	{
		fout << "::OpenPopup(std::function<void(ImRad::ModalResult)> clb)\n{\n";
		fout << INDENT << "callback = clb;\n";
		fout << INDENT << "modalResult = ImRad::None;\n";
		
		if (animPos == TopWindow::Left || animPos == TopWindow::Right)
			fout << INDENT << "animator.Start(&animPos.x, -ImGui::GetMainViewport()->Size.x / 2, 0);\n";
		else if (animPos == TopWindow::Top || animPos == TopWindow::Bottom || animPos == TopWindow::Center)
			fout << INDENT << "animator.Start(&animPos.y, -ImGui::GetMainViewport()->Size.y / 2, 0);\n";
		
		fout << INDENT << "ImGui::OpenPopup(ID);\n";
		fout << INDENT << "Init();\n";
		fout << "}";
		return true;
	}
	else if (id == "OpenPopup" && kind == TopWindow::Popup)
	{
		fout << "::OpenPopup()\n{\n";
		fout << INDENT << "modalResult = ImRad::None;\n";
		
		if (animPos == TopWindow::Left || animPos == TopWindow::Right)
			fout << INDENT << "animator.Start(&animPos.x, -ImGui::GetMainViewport()->Size.x / 2, 0);\n";
		else if (animPos == TopWindow::Top || animPos == TopWindow::Bottom || animPos == TopWindow::Center)
			fout << INDENT << "animator.Start(&animPos.y, -ImGui::GetMainViewport()->Size.y / 2, 0);\n";

		fout << INDENT << "ImGui::OpenPopup(ID);\n";
		fout << INDENT << "Init();\n";
		fout << "}";
		return true;
	}
	else if (id == "ClosePopup" && kind == TopWindow::ModalPopup)	
	{
		fout << "::ClosePopup(ImRad::ModalResult mr)\n{\n";
		fout << INDENT << "modalResult = mr;\n";
		
		if (animPos == TopWindow::Left || animPos == TopWindow::Right)
			fout << INDENT << "animator.Start(&animPos.x, animPos.x, -animator.GetWindowSize().x);\n";
		else if (animPos == TopWindow::Top || animPos == TopWindow::Bottom || animPos == TopWindow::Center)
			fout << INDENT << "animator.Start(&animPos.y, animPos.y, -animator.GetWindowSize().y);\n";

		fout << "}";
		return true;
	}
	else if (id == "ClosePopup" && kind == TopWindow::Popup)
	{
		fout << "::ClosePopup()\n{\n";
		fout << INDENT << "modalResult = ImRad::Cancel;\n";
		
		if (animPos == TopWindow::Left || animPos == TopWindow::Right)
			fout << INDENT << "animator.Start(&animPos.x, animPos.x, -animator.GetWindowSize().x);\n";
		else if (animPos == TopWindow::Top || animPos == TopWindow::Bottom || animPos == TopWindow::Center)
			fout << INDENT << "animator.Start(&animPos.y, animPos.y, -animator.GetWindowSize().y);\n";

		fout << "}";
		return true;
	}
	else if (id == "Open" && kind == TopWindow::Window)
	{
		fout << "::Open()\n{\n";
		fout << INDENT << "isOpen = true;\n";
		fout << "}";
		return true;
	}
	else if (id == "Open" && kind == TopWindow::Activity)
	{
		fout << "::Open()\n{\n";
		fout << INDENT << "auto* ioUserData = (ImRad::IOUserData*)ImGui::GetIO().UserData;\n";
		fout << INDENT << "if (ioUserData->activeActivity != \"" << m_name << "\")\n";
		fout << INDENT << "{\n";
		fout << INDENT << INDENT << "ioUserData->activeActivity = \"" << m_name << "\";\n";
		fout << INDENT << INDENT << "Init();\n";
		fout << INDENT << "}\n";
		fout << "}";
		return true;
	}
	else if (id == "Close" && kind == TopWindow::Window)
	{
		fout << "::Close()\n{\n";
		fout << INDENT << "isOpen = false;\n";
		fout << "}";
		return true;
	}
	else if (id == "Init" && 
		(kind == TopWindow::ModalPopup || kind == TopWindow::Popup || kind == TopWindow::Activity))
	{
		fout << "::Init()\n{\n";
		fout << INDENT << "// TODO: Add your code here\n";
		fout << "}";
		return true;
	}
	else if (id == "Draw")
	{
		fout << "::Draw(";
		if (kind == TopWindow::MainWindow)
			fout << "GLFWwindow* window";
		fout << ")\n{\n";

		for (const auto& p : params)
			fout << INDENT << "/// @" << p.first << " " << p.second << "\n";
		
		fout << code << "}";
		return true;
	}
	return false;
}

std::unique_ptr<TopWindow> 
CppGen::Import(
	const std::string& path, 
	std::map<std::string, std::string>& params, 
	std::string& err
)
{
	m_fields.clear();
	m_fields[""];
	m_name = m_vname = "";
	m_error = "";
	ctx_workingDir = fs::path(path).parent_path().string();
	std::unique_ptr<TopWindow> node;

	auto fpath = fs::path(path).replace_extension("h");
	m_hname = fpath.filename().string();
	std::ifstream fin(fpath.string());
	if (!fin)
		m_error += "Can't read " + fpath.string() + "\n";
	else
		node = ImportCode(fin, params);
	fin.close();

	fpath = fs::path(path).replace_extension("cpp");
	fin.open(fpath.string());
	if (!fin)
		m_error += "Can't read " + fpath.string() + "\n";
	else {
		auto node2 = ImportCode(fin, params);
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

std::unique_ptr<TopWindow> 
CppGen::ImportCode(std::istream& fin, std::map<std::string, std::string>& params)
{
	std::unique_ptr<TopWindow> node;
	cpp::token_iterator iter(fin);
	bool in_class = false; 
	bool in_interface = false;
	bool in_impl = false;
	bool in_fun = false;
	std::vector<std::string> scope; //contains struct/class/empty names for each nested block
	std::vector<std::string> line;
	while (iter != cpp::token_iterator())
	{
		std::string tok = *iter++;
		bool is_comment = !tok.compare(0, 2, "//");

		if (tok == "{") {
			if (line.size() && (line[0] == "class" || line[0] == "struct")) {
				scope.push_back(line[1]);
				if (in_class)
					m_fields[scope.back()];
			}
			else {
				scope.push_back("");
				auto nod = ParseDrawFun(line, iter, params);
				if (nod)
					node = std::move(nod);
			}
			line.clear();
		}
		else if (tok == "}") {
			scope.pop_back();
			if (scope.empty())
				in_class = false;
		}
		else if (scope.size() >= 1 && scope.back() != "" && tok == ":") { //public/private...
			line.clear();
		}
		else if (scope.size() == 1 && (tok == "/// @interface" || tok == "/// @begin interface")) {
			if (!in_class) {
				in_class = true;
				m_name = scope[0];
			}
			in_interface = true;
		}
		else if (scope.size() == 1 && (tok == "/// @impl" || tok == "/// @begin impl")) {
			if (!in_class) {
				in_class = true;
				m_name = scope[0];
			}
			in_impl = true;
		}
		else if (in_class && tok == "/// @end interface") {
			in_interface = false;
		}
		else if (in_class && tok == "/// @end impl") {
			in_impl = false;
		}
		else if (!tok.compare(0, 2, "//") || tok[0] == '#')
			;
		else if (tok == ";") {
			if (in_class) {
				ParseFieldDecl(
					scope.size()==1 ? "" : scope.back(), 
					line, 
					in_interface ? Var::Interface : in_impl ? Var::Impl : Var::UserCode
				);
			}
			else if (scope.empty() && line.size() == 3 && line[0] == "extern" && line[1] == m_name) {
				m_vname = line[2];
			}
			line.clear();
		}
		else
			line.push_back(tok); 
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
		if (name != "OpenPopup" && name != "Open" && name != "Draw" && name != "Init") { //other generated funs can be used as handlers
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
			//fix neg numbers
			if (init.size() >= 3 && init[0] == '-' && std::isdigit(init[2]))
				init.erase(1, 1);
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
		if (name != "ID" && name != "modalResult" && name != "callback" &&
			name != "isOpen" && name != "animator" && name != "animPos")
			CreateNamedVar(name, type, init, flags, sname);
	}
	return true;
}

std::string CppGen::IsMemFun(const std::vector<std::string>& line)
{
	if (line.size() >= 6 &&
		line[0] == "void" && line[1] == m_name && line[2] == "::" &&
		line[4] == "(" && line.back() == ")")
	{
		return line[3];
	}
	return "";
}

bool CppGen::IsMemDrawFun(const std::vector<std::string>& line)
{
	if (IsMemFun(line) != "Draw")
		return false;
	if (line.size() == 6)
		return true;
	if (line.size() == 9 && line[5] == "GLFWwindow" && line[6] == "*")
		return true;
	return false;
}

std::unique_ptr<TopWindow>
CppGen::ParseDrawFun(const std::vector<std::string>& line, cpp::token_iterator& iter, std::map<std::string, std::string>& params)
{
	if (!IsMemDrawFun(line))
		return {};
	
	cpp::stmt_iterator sit(iter);
	while (sit != cpp::stmt_iterator()) 
	{
		if (sit->line == "/// @begin TopWindow")
			break;
		if (!sit->line.compare(0, 5, "/// @")) {
			std::string key = sit->line.substr(5);
			size_t i1 = key.find_first_of(" \t");
			size_t i2 = i1 + 1;
			while (i2 + 1 < key.size() && (key[i2 + 1] == ' ' || key[i2 + 1] == '\t'))
				++i2;
			params[key.substr(0, i1)] = key.substr(i2);
		}
		++sit;
	}
	UIContext ctx;
	ctx.codeGen = this;
	ctx.workingDir = ctx_workingDir;
	auto node = std::make_unique<TopWindow>(ctx);
	node->Import(sit, ctx);
	iter = sit.base();
	for (const std::string& e : ctx.errors)
		m_error += e + "\n";
	return node;
}

std::string DecorateType(const std::string& type)
{
	if (type == "int2")
		return "ImRad::Int2";
	if (type == "int3")
		return "ImRad::Int3";
	if (type == "int4")
		return "ImRad::Int4";
	if (type == "float2")
		return "ImRad::Float2";
	if (type == "float3")
		return "ImRad::Float3";
	if (type == "float4")
		return "ImRad::Float4";
	if (type == "color3")
		return "ImRad::Color3";
	if (type == "color4")
		return "ImRad::Color4";
	if (type == "angle")
		return "float";
	return type;
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
	vit->second.push_back(Var(name, DecorateType(type), init, flags));
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
	vit->second.push_back(Var(name, DecorateType(type), init, flags));
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
	var->type = DecorateType(type);
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
CppGen::CheckVarExpr(const std::string& name, const std::string& type_, const std::string& scope)
{
	std::string type = DecorateType(type_);
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
		if (!cpp::is_container(var->type))
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

std::string DefaultInitFor(const std::string& stype)
{
	if (!stype.compare(0, 5, "void("))
		return "";
	if (stype.find('<') != std::string::npos) //vector etc
		return "";
	if (stype == "int" || stype == "size_t" || stype == "double" || stype == "float")
		return "0";
	if (stype == "bool")
		return "false";
	return "";
}

//accepts patterns recognized by CheckVarExpr
//corrects [xyz]
bool CppGen::CreateVarExpr(std::string& name, const std::string& type_, const std::string& init, const std::string& scope1)
{
	std::string type = DecorateType(type_);
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
			/* why replace any index expr to FOR_VAR??
			name.erase(i + b + 1, id.size() - b - 2);
			name.insert(i + b + 1, FOR_VAR);
			if (j != std::string::npos)
				j += FOR_VAR.size() - (id.size() - b - 2);*/
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
				std::string ninit = init.size() ? init : DefaultInitFor(stype); //initialize scalar variables
				if (!CreateNamedVar(id, stype, ninit, fun ? Var::Impl : Var::Interface, scope))
					return false;
				if (!stype.compare(0, 12, "std::vector<")) {
					std::string etype = stype.substr(12, stype.size() - 12 - 1);
					if (etype != "" && etype[0] >= 'A' && etype[0] <= 'Z')
						m_fields[etype];
				}
			}
			else {
				if (!CreateNamedVar(id, "std::vector<" + stype + ">", init, Var::Interface, scope))
					return false;
				if (!leaf)
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
//type==void() accepts all functions
//vector, array are searched recursively
std::vector<std::pair<std::string, std::string>> //(name, type) 
CppGen::GetVarExprs(const std::string& type_, bool recurse)
{
	std::string type = DecorateType(type_);
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
		else if (cpp::is_container(f.type))
		{
			if (isFun)
				continue;
			auto i = f.type.find('<');
			auto j = f.type.find_last_of('>');
			if (j == std::string::npos || j < i)
				continue;
			if (type == "" || type == "[]" || type == f.type)
				ret.push_back({ f.name, f.type });
			if (type == "" || type == "int")
				ret.push_back({ f.name + ".size()", "int" }); //we use int indexes for simplicity
			else if (recurse) {
				std::string stype = f.type.substr(i + 1, j - i - 1);
				if (type == "" || stype == type)
					ret.push_back({ f.name + "[]", stype });
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
							ret.push_back({ f.name + "[]." + ff.name, ff.type });
					}
				}
			}
		}
		//scalars
		else if (type == "" || f.type == type)
		{
			ret.push_back({ f.name, f.type });
		}
	}
	stx::sort(ret);
	return ret;
}

