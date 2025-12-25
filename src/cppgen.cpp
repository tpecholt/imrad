#include "cppgen.h"
#include "cpp_parser.h"
#include "stx.h"
#include "utils.h"
#include <fstream>
#include <cctype>
#include <set>

const std::string GENERATED_WITH = "Generated with ";

const std::string SPEC_FUN[] = {
    "Open", "Close", "OpenPopup", "ClosePopup", "ResetLayout", "Init", "Draw",
};

const std::string_view CppGen::INDENT = "    ";
const std::string_view CppGen::FOR_VAR_NAME = "i";
const std::string_view CppGen::CUR_ITEM_VAR_NAME = "_item";
const std::string_view CppGen::HBOX_NAME = "hb";
const std::string_view CppGen::VBOX_NAME = "vb";

bool IsFunType(std::string_view type, std::string& ret, std::string& arg)
{
    size_t i = type.find('(');
    if (i == std::string::npos || type.back() != ')')
        return false;
    ret = type.substr(0, i);
    arg = type.substr(i + 1, type.size() - i - 2);
    return true;
}

bool IsFunType(std::string_view type)
{
    std::string ret, arg;
    return IsFunType(type, ret, arg);
}

std::string DefaultInitFor(const std::string& stype)
{
    /*if (!valueType.compare(0, 5, "void("))
        return "";*/
    if (stype.size() && stype.back() == '*')
        return "nullptr";
    if (stype == "int" || stype == "size_t" || stype == "double" || stype == "float")
        return "0";
    if (stype == "bool")
        return "false";
    return "";
}

//ImRad 0.9 -> 9000
//ImRad 0.9.1 -> 9010
int ParseVersion(const std::string& str)
{
    std::string s = str.substr(str.find_first_of("0123456789"));
    stx::replace(s, '.', ' ');
    std::istringstream is(s);
    int num;
    int ver = 0;
    int parts = 0;
    while (is >> num) {
        if (num < 10)
            num *= 10;
        ver = ver * 100 + num;
        ++parts;
    }
    for (; parts < 3; ++parts)
        ver *= 100;
    return ver;
}

//----------------------------------------------------------------

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
        else if ((c & 0xc0) == 0xc0) //ignore utf8 subsequent bytes
            continue;
        else {
            char cc = c >= 0 ? c : 'x';
            if (upper) {
                cc = std::toupper(cc);
                upper = false;
            }
            name += cc;
        }
    }
    m_name = (char)std::toupper(name[0]) + name.substr(1);
    m_vname = (char)std::tolower(name[0]) + name.substr(1);
}

std::string CppGen::AltFName(const std::string& path) const
{
    fs::path p = u8path(path);
    if (!u8string(p.extension()).compare(0, 2, ".h"))
        return u8string(p.replace_extension("cpp"));
    else
        return u8string(p.replace_extension("h"));
    return "";
}

int CppGen::ReadGenVersion(const std::string& fname) const
{
    std::ifstream fin(u8path(fname));
    std::string line;
    if (!fin || !std::getline(fin, line))
        return 0;
    if (line.compare(0, 3, "// "))
        return 0;
    if (line.compare(3, GENERATED_WITH.size(), GENERATED_WITH))
        return 0;
    size_t n = VER_STR.find(' ');
    if (n == std::string::npos)
        n = VER_STR.size();
    if (line.compare(3 + GENERATED_WITH.size(), n, VER_STR, 0, n))
        return 0;
    return ParseVersion(line.substr(3 + GENERATED_WITH.size()));
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

    //export node before ExportH
    //TopWindow::Export generates some variables on the fly
    UIContext ctx;
    ctx.codeGen = this;
    ctx.ind = INDENT;
    auto uit = params.find("unit");
    if (uit != params.end())
        ctx.unit = uit->second;
    std::ostringstream code;
    node->Export(code, ctx);
    for (const std::string& e : ctx.errors)
        err += e + "\n";

    //export .h
    m_error = "";
    auto hpath = u8path(fname).replace_extension(".h");
    if (!fs::exists(hpath) || fs::is_empty(hpath))
    {
        std::ofstream fout(hpath);
        if (!fout) {
            err = "can't write to '" + hpath.string() + "'";
            return false;
        }
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
    m_hname = u8string(hpath.filename());
    fout.close();
    err += m_error;

    //export .cpp
    m_error = "";
    auto fpath = u8path(fname).replace_extension(".cpp");
    if (!fs::exists(fpath) || fs::is_empty(fpath))
    {
        std::ofstream fout(fpath);
        if (!fout) {
            err = "can't write to '" + hpath.string() + "'";
            return false;
        }
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
    ExportCpp(fout, fprev, origNames, params, node, code.str());
    err += m_error;
    return true;
}

void CppGen::CreateH(std::ostream& out)
{
    out << "// " << GENERATED_WITH << VER_STR << "\n"
        << "// visit " << GITHUB_URL << "\n\n";

    out << "#pragma once\n";
    out << "#include <imrad.h>\n";

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
    bool preamble = true;
    bool first_include = true;
    int skip_to_level = -1;
    std::vector<std::string> line;
    std::streampos fpos = 0;
    std::string ignore_section = "";
    std::string className;
    std::string origName, origVName;
    std::stringstream out;
    bool hasLayout = GetLayoutVars().size();

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
        out.write(buf.data(), buf.size());
        fprev.ignore(ignore_last);
        fpos = pos;
    };

    for (cpp::token_iterator iter(fprev); iter != cpp::token_iterator(); ++iter)
    {
        std::string tok = *iter;
        if (in_class && level == 1) //class scope
        {
            if (tok == "/// @interface" || tok == "/// @begin interface")
            {
                origName = className;
                ignore_section = "interface";
                copy_content(-(int)tok.size());
                out << "/// @begin interface\n";

                //write special members
                if (node->kind == TopWindow::ModalPopup)
                {
                    out << INDENT << "void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});\n";
                    out << INDENT << "void ClosePopup(ImRad::ModalResult mr = ImRad::Cancel);\n";
                    out << INDENT << "void Draw();\n";
                }
                else if (node->kind == TopWindow::Popup)
                {
                    out << INDENT << "void OpenPopup();\n";
                    out << INDENT << "void ClosePopup();\n";
                    out << INDENT << "void Draw();\n";
                }
                else if (node->kind == TopWindow::Window)
                {
                    out << INDENT << "void Open();\n";
                    out << INDENT << "void Close();\n";
                    out << INDENT << "void Draw();\n";
                }
                else if (node->kind == TopWindow::MainWindow)
                {
                    out << INDENT << "void Draw(GLFWwindow* window);\n";
                }
                else if (node->kind == TopWindow::Activity)
                {
                    out << INDENT << "void Open();\n";
                    out << INDENT << "void Draw();\n";
                }
                else
                {
                    assert(false);
                }
                out << "\n";

                //write stuctures
                bool found = false;
                for (const auto& scope : m_fields)
                {
                    if (scope.first == "")
                        continue;
                    bool userCode = stx::count_if(scope.second, [](const auto& var) { return var.flags & Var::UserCode; });
                    if (userCode)
                        continue;
                    found = true;
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
                if (found)
                    out << "\n";

                //write events
                found = false;
                std::string ret, arg;
                for (const auto& var : m_fields[""])
                {
                    if ((var.flags & Var::UserCode) || !(var.flags & Var::Interface))
                        continue;
                    if (!IsFunType(var.type, ret, arg))
                        continue;
                    if (stx::count(SPEC_FUN, var.name))
                        continue;
                    found = true;
                    out << INDENT << ret << " " << var.name << "(";
                    if (arg != "")
                        out << arg << " args";
                    out << ");\n";
                }
                if (found)
                    out << "\n";

                //write fields
                for (const auto& var : m_fields[""])
                {
                    if ((var.flags & Var::UserCode) || !(var.flags & Var::Interface))
                        continue;
                    if (IsFunType(var.type))
                        continue;
                    out << INDENT << var.type << " " << var.name;
                    if (var.init != "")
                        out << " = " << var.init;
                    out << ";\n";
                }
            }
            else if (tok == "/// @impl" || tok == "/// @begin impl")
            {
                origName = className;
                ignore_section = "impl";
                copy_content(-(int)tok.size());
                out << "/// @begin impl\n";

                //write special members
                bool found = false;
                if (hasLayout)
                {
                    found = true;
                    out << INDENT << "void ResetLayout();\n";
                }
                if (node->kind == TopWindow::Popup || node->kind == TopWindow::ModalPopup ||
                    node->kind == TopWindow::Activity)
                {
                    found = true;
                    out << INDENT << "void Init();\n";
                }

                if (found)
                    out << "\n";

                //write events
                found = false;
                std::string ret, arg;
                for (const auto& var : m_fields[""])
                {
                    if ((var.flags & Var::UserCode) || !(var.flags & Var::Impl))
                        continue;
                    if (IsFunType(var.type, ret, arg) &&
                        !stx::count(SPEC_FUN, var.name))
                    {
                        found = true;
                        out << INDENT << ret << " " << var.name << "(";
                        if (arg != "")
                            out << arg << " args";
                        out << ");\n";
                    }
                }
                if (found)
                    out << "\n";

                //special fields
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

                //other fields
                for (const auto& var : m_fields[""])
                {
                    if ((var.flags & Var::UserCode) || !(var.flags & Var::Impl))
                        continue;
                    if (!IsFunType(var.type))
                    {
                        out << INDENT << var.type << " " << var.name;
                        if (var.init != "")
                            out << " = " << var.init;
                        out << ";\n";
                    }
                }
            }
            else if (tok == "/// @end interface" || tok == "/// @end impl")
            {
                ignore_section = "";
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
                    if (ignore_section != "") {
                        fpos = fprev.tellg();
                        out << INDENT << "/// @end " << ignore_section;
                        out << "\n" << tok;
                        ignore_section = "";
                    }
                }
                line.clear();
            }
            if (tok == "public" || tok == "private" || tok == "protected")
            {
                if (ignore_section != "") {
                    fpos = fprev.tellg();
                    out << INDENT << "/// @end " << ignore_section;
                    out << "\n\n" << tok;
                    ignore_section = "";
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
        else if (!tok.compare(0, 2, "//")) {
            if (preamble && tok.find(GENERATED_WITH) != std::string::npos) {
                if (tok.find(VER_STR) == std::string::npos)
                    m_error += "\"" + m_hname + "\" was upgraded to " + VER_STR + "\n";
                copy_content(-(int)tok.size());
                out << "// " << GENERATED_WITH << VER_STR;
            }
        }
        else if (!tok.compare(0, 1, "#")) {
            preamble = false;
            if (!tok.compare(0, 8, "#include"))
            {
                if (node->kind == TopWindow::MainWindow &&
                    first_include &&
                    tok.find("glfw") == std::string::npos)
                {
                    //add GLFW include
                    copy_content(-(int)tok.size());
                    out << "#include <GLFW/glfw3.h>\n";
                    out << tok << "\n";
                }
                else if (node->kind != TopWindow::MainWindow &&
                    first_include &&
                    tok.find("glfw") != std::string::npos)
                {
                    //remove GLFW include
                    copy_content(-(int)tok.size());
                }
                first_include = false;
            }
        }
        else if (tok == ";") {
            if (line.size() == 3 && line[0] == "extern" && line[1] == origName) {
                origVName = line[2];
            }
            line.clear();
        }
        else {
            preamble = false;
            line.push_back(tok);
        }
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

//follows fprev and overwrites generated members/functions only
void
CppGen::ExportCpp(
    std::ostream& fout,
    std::istream& fprev,
    const std::array<std::string, 3>& origNames, //name, vname, old header name
    const std::map<std::string, std::string>& params,
    TopWindow* node,
    const std::string& code
)
{
    int level = 0;
    int skip_to_level = -1;
    int comment_to_level = -1;
    bool preamble = true;
    std::vector<std::string> line;
    std::streampos fpos = 0;
    std::set<std::string> funs;
    auto animPos = node->animate ? (TopWindow::Placement)node->placement : TopWindow::None;

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
            else if (!tok.compare(0, 2, "//")) {
                if (preamble && tok.find(GENERATED_WITH) != std::string::npos) {
                    if (tok.find(VER_STR) == std::string::npos)
                        m_error += "\"" + u8string(u8path(m_hname).replace_extension(".cpp")) + "\" was upgraded to " + VER_STR + "\n";
                    copy_content(-(int)tok.size());
                    fout << "// " << GENERATED_WITH << VER_STR;
                }
            }
            else if (!tok.compare(0, 1, "#")) {
                preamble = false;
            }
            else if (tok == ";") {
                line.clear();
            }
            else if (tok == "{") {
                ++level;
                auto name = IsMemFun(line);
                //rewrite generated functions
                if (name == "Init")
                {
                    std::ostringstream tmp;
                    if (WriteStub(tmp, name, node->kind, animPos)) {
                        funs.insert(name);
                        //copy following code
                    }
                    else {
                        //remove member which is no longer needed
                        //Init contains user code so we don't want to delete it completely
                        fout.seekp((int)fout.tellp() - 5 - m_name.size());
                        comment_to_level = level - 1;
                        fpos = fprev.tellg();
                        fout << "/* void " << m_name << "::" << name << "() REMOVED\n{";
                    }
                }
                else if (stx::count(SPEC_FUN, name))
                {
                    if (WriteStub(fout, name, node->kind, animPos, params, code)) {
                        funs.insert(name);
                        skip_to_level = level - 1;
                    }
                    else {
                        //remove member which is no longer needed
                        fout.seekp((int)fout.tellp() - 5 - m_name.size());
                        skip_to_level = level - 1;
                        fout << "// void " << m_name << "::" << name << " REMOVED";
                    }
                }
                else if (name != "")
                {
                    funs.insert(name);
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
                preamble = false;
                line.push_back(tok);
            }
        } //if (!level)
        else if (tok == "{") {
            ++level;
        }
        else if (tok == "}") {
            --level;
            if (skip_to_level == level) {
                skip_to_level = -1;
                fpos = fprev.tellg();
            }
            if (comment_to_level == level) {
                comment_to_level = -1;
                copy_content();
                fout << "*/";
            }
        }
    }

    //copy remaining code
    fprev.clear(); //clear EOF flag
    fprev.seekg(0, std::ios::end);
    copy_content();

    //add missing members
    for (const auto& name : SPEC_FUN)
    {
        if (funs.count(name))
            continue;
        std::ostringstream os;
        if (WriteStub(os, name, node->kind, animPos, params, code)) {
            funs.insert(name);
            fout << "\nvoid " << m_name << os.str() << "\n";
        }
    }

    //add missing events
    std::string ret, arg;
    for (const auto& var : m_fields[""])
    {
        if (var.flags & Var::UserCode)
            continue;
        if (!IsFunType(var.type, ret, arg))
            continue;
        if (funs.count(var.name) || stx::count(SPEC_FUN, var.name))
            continue;
        fout << "\n" << ret << " " << m_name << "::" << var.name << "(";
        if (arg != "")
            fout << arg << " args";
        fout << ")\n{\n";
        if (ret != "void")
        {
            fout << INDENT << "return ";
            std::string init = DefaultInitFor(ret);
            fout << (init != "" ? init : ret + "()");
            fout << ";\n";
        }
        fout << "}\n";
    }
}

std::vector<std::string> CppGen::GetLayoutVars()
{
    std::vector<std::string> vars;
    const auto& allVars = m_fields[""];
    for (const Var& var : allVars)
    {
        if (var.type == "ImRad::HBox" || var.type == "ImRad::VBox")
            vars.push_back(var.name);
    }
    return vars;
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
    assert(stx::count(SPEC_FUN, id));
    //OpenPopup has to be called from the window which wants to open it so that's why
    //immediate call
    //CloseCurrentPopup has to be called from the popup Draw code so that's why
    //defered call
    if (id == "OpenPopup" &&
        (kind == TopWindow::ModalPopup || kind == TopWindow::Popup))
    {
        if (kind == TopWindow::ModalPopup) {
            fout << "::OpenPopup(std::function<void(ImRad::ModalResult)> clb)\n{\n";
            fout << INDENT << "callback = clb;\n";
            fout << INDENT << "modalResult = ImRad::None;\n";
        }
        else {
            fout << "::OpenPopup()\n{\n";
            fout << INDENT << "modalResult = ImRad::None;\n";
        }

        if (animPos != TopWindow::Placement::None)
        {
            if (animPos == TopWindow::Left || animPos == TopWindow::Right)
                fout << INDENT << "animator.StartPersistent(&animPos.x, -ImGui::GetMainViewport()->Size.x / 2.f, 0.f, ImRad::Animator::DurOpenPopup);\n";
            else if (animPos == TopWindow::Top || animPos == TopWindow::Bottom || animPos == TopWindow::Center)
                fout << INDENT << "animator.StartPersistent(&animPos.y, -ImGui::GetMainViewport()->Size.y / 2.f, 0.f, ImRad::Animator::DurOpenPopup);\n";
            if (kind == TopWindow::ModalPopup)
                fout << INDENT << "animator.StartPersistent(&ImRad::GetUserData().dimBgRatio, 0.f, 1.f, ImRad::Animator::DurOpenPopup);\n";
        }
        else if (kind == TopWindow::ModalPopup)
        {
            fout << INDENT << "ImRad::GetUserData().dimBgRatio = 1.f;\n";
        }

        fout << INDENT << "IM_ASSERT(ID && \"Call Draw at least once to get ID assigned\");\n";
        fout << INDENT << "ImGui::OpenPopup(ID);\n";
        fout << INDENT << "Init();\n";
        fout << "}";
        return true;
    }
    else if (id == "ClosePopup" &&
        (kind == TopWindow::ModalPopup || kind == TopWindow::Popup))
    {
        if (kind == TopWindow::ModalPopup) {
            fout << "::ClosePopup(ImRad::ModalResult mr)\n{\n";
            fout << INDENT << "modalResult = mr;\n";
        }
        else {
            fout << "::ClosePopup()\n{\n";
            fout << INDENT << "modalResult = ImRad::Cancel;\n";
        }

        if (animPos != TopWindow::Placement::None)
        {
            if (animPos == TopWindow::Left || animPos == TopWindow::Right)
                fout << INDENT << "animator.StartOnce(&animPos.x, animPos.x, -animator.GetWindowSize().x, ImRad::Animator::DurClosePopup);\n";
            else if (animPos == TopWindow::Top || animPos == TopWindow::Bottom || animPos == TopWindow::Center)
                fout << INDENT << "animator.StartOnce(&animPos.y, animPos.x, -animator.GetWindowSize().y, ImRad::Animator::DurClosePopup);\n";
            if (kind == TopWindow::ModalPopup)
                fout << INDENT << "animator.StartOnce(&ImRad::GetUserData().dimBgRatio, ImRad::GetUserData().dimBgRatio, 0.f, ImRad::Animator::DurClosePopup);\n";
        }
        else if (kind == TopWindow::ModalPopup)
        {
            fout << INDENT << "ImRad::GetUserData().dimBgRatio = 0.f;\n";
        }

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
        fout << INDENT << "if (ImRad::GetUserData().activeActivity != \"" << m_name << "\")\n";
        fout << INDENT << "{\n";
        fout << INDENT << INDENT << "ImRad::GetUserData().activeActivity = \"" << m_name << "\";\n";
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
    else if (id == "ResetLayout" && GetLayoutVars().size())
    {
        auto vars = GetLayoutVars();
        fout << "::ResetLayout()\n{\n";
        fout << INDENT << "// ImGui::GetCurrentWindow()->HiddenFramesCannotSkipItems = 2;\n";
        for (const std::string& var : vars)
            fout << INDENT << var << ".Reset();\n";
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
    ctx_workingDir = u8string(u8path(path).parent_path());
    ctx_importVersion = 0;
    std::unique_ptr<TopWindow> node;

    auto fpath = u8path(path).replace_extension("h");
    m_hname = u8string(fpath.filename());
    std::ifstream fin(fpath);
    if (!fin)
        m_error += "Can't read " + u8string(fpath) + "\n";
    else
        node = ImportCode(fin, m_hname, params);
    fin.close();

    fpath = u8path(path).replace_extension("cpp");
    fin.open(fpath);
    if (!fin)
        m_error += "Can't read \"" + u8string(fpath) + "\"\n";
    else {
        auto node2 = ImportCode(fin, u8string(fpath.filename()), params);
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
CppGen::ImportCode(std::istream& fin, const std::string& fname, std::map<std::string, std::string>& params)
{
    std::unique_ptr<TopWindow> node;
    cpp::token_iterator iter(fin);
    bool in_class = false;
    bool in_interface = false;
    bool in_impl = false;
    bool in_fun = false;
    bool preamble = true;
    std::vector<std::string> scope; //contains struct/class/empty names for each nested block
    std::vector<std::string> line;
    while (iter != cpp::token_iterator())
    {
        std::string tok = *iter;

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
            if (scope.empty()) {
                m_error += "Parsing stopped: found non-matching '}'\n";
                break;
            }
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
        else if (!tok.compare(0, 2, "//"))
        {
            size_t i;
            if (preamble && (i = tok.find(GENERATED_WITH)) != std::string::npos) {
                std::string ver = tok.substr(i + GENERATED_WITH.size());
                ctx_importVersion = ParseVersion(ver);
                if (ver != VER_STR)
                    m_error += "\"" + fname + "\" was saved in different version ["
                        + ver + "]. Full compatibility is not guaranteed.\n";
            }
        }
        else if (tok[0] == '#')
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
        else {
            preamble = false;
            line.push_back(tok);
        }

        ++iter;
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
    if (line[0] == "enum" || line[0] == "template")
        return false;

    //parse event declaration
    if (line.size() >= 4 &&
        cpp::is_id(line[0]) &&
        cpp::is_id(line[1]) && line[2] == "(" && line.back() == ")")
    {
        std::string name = line[1];
        //currently we allow generated Close/Popup in event handler list
        if (stx::count(SPEC_FUN, name) && name != "Close" && name != "ClosePopup")
            return false;

        //primitive arguments parser
        //doesn't handle function pointers or template types argument expressions
        std::string argTypes;
        bool ignore = false;
        bool typeValid = false;
        bool gotId = false;
        int level = 0;
        for (size_t i = 3; i < line.size() - 1; ++i)
        {
            if (line[i] == "," && !level) {
                ignore = false;
                gotId = false;
                argTypes += ",";
                continue;
            }
            if (ignore)
                continue;
            if (line[i] == "=") { //skip default value
                ignore = true;
                continue;
            }
            if (cpp::is_id(line[i]) && typeValid) { //skip argument name
                ignore = true;
                continue;
            }
            argTypes += line[i];
            if (line[i] == "const")
                argTypes += " ";
            if (cpp::is_id(line[i]))
                gotId = true;
            if (line[i] == "<")
                ++level;
            if (line[i] == ">")
                --level;
            typeValid = gotId && !level &&
                (cpp::is_id(line[i]) ||
                line[i] == ">" || line[i] == "&" || line[i] == "*");
        }
        std::string type = line[0] + "(" + argTypes + ")";

        CreateNamedVar(name, type, "", flags, sname);
    }
    //parse variable
    else
    {
        //skip other functions
        //todo: parse std::function<void(xxx)> koko;
        if (stx::count(line, "(") || stx::count(line, ")"))
            return false;
        std::string type;
        size_t beg = 0;
        while (beg < line.size())
        {
            int level = 0;
            size_t end;
            for (end = beg; end < line.size(); ++end)
            {
                const std::string& s = line[end];
                if (s == "<" || s == "(" || s == "[" || s == "{")
                    ++level;
                else if (s == ">" || s == ")" || s == "]" || s == "}")
                    --level;
                else if (!level && s == ",") //multiple variables
                    break;
            }
            auto it = std::find(line.begin() + beg, line.begin() + end, "=");
            std::string name, init;
            size_t name_idx = 0;
            if (it != line.begin() + end)
            {
                name_idx = it - line.begin() - 1;
                if (name_idx + 2 >= end) //= without initializer?
                    return false;
                init = *++it;
                for (++it; it != line.begin() + end; ++it) {
                    bool ws = true;
                    if (*it == "::" ||
                        (init.size() >= 2 && init.back() == ':' && init[init.size() - 2] == ':'))
                        ws = false;
                    if (init.back() == '-' && std::isdigit((*it)[0]))
                        ws = false;
                    if (ws)
                        init += ' ';
                    init += *it;
                }
            }
            else {
                name_idx = end - 1;
            }
            if (name_idx < 1) //no name or type
                return false;
            name = line[name_idx];
            if (!beg)
            {
                for (size_t i = 0; i < name_idx; ++i)
                {
                    bool op = !stx::count_if(line[i], [](char c) {
                        return std::isalnum(c) || c == '_';
                        });
                    if (op) {
                        if (type.size() && type.back() == ' ')
                            type.pop_back();
                        type += line[i];
                    }
                    else
                        type += line[i] + " ";
                }
                if (type.back() == ' ')
                    type.pop_back();
            }
            if (name != "ID" && name != "modalResult" && name != "callback" &&
                name != "isOpen" && name != "animator" && name != "animPos")
            {
                CreateNamedVar(name, type, init, flags, sname);
            }
            beg = end + 1;
        }
    }
    return true;
}

std::string CppGen::IsMemFun(const std::vector<std::string>& line)
{
    if (line.size() >= 6 &&
        /*line[0] == "void" &&*/ line[1] == m_name && line[2] == "::" &&
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

    auto pos1 = iter.stream().tellg();
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
    iter.stream().seekg(pos1); //reparse to capture potential userCodeBefore
    iter = cpp::token_iterator(iter.stream(), true);
    sit = cpp::stmt_iterator(iter);
    UIContext ctx;
    ctx.codeGen = this;
    ctx.workingDir = ctx_workingDir;
    ctx.importVersion = ctx_importVersion;
    auto node = std::make_unique<TopWindow>(ctx);
    node->Import(sit, ctx);
    iter = sit.base();
    for (const std::string& e : ctx.errors)
        m_error += e + "\n";
    return node;
}

std::string CppType(const std::string& type)
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
    vit->second.push_back(Var(name, CppType(type), init, flags));
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
    vit->second.push_back(Var(name, CppType(type), init, flags));
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

void CppGen::RemovePrefixedVars(std::string_view prefix, std::string_view scope)
{
    auto vit = m_fields.find(std::string(scope));
    if (vit == m_fields.end())
        return;
    stx::erase_if(vit->second, [&](const auto& var)
    {
        if (var.flags & Var::UserCode)
            return false;
        if (var.name.compare(0, prefix.size(), prefix))
            return false;
        for (size_t i = prefix.size(); i < var.name.size(); ++i)
            if (!std::isdigit(var.name[i]))
                return false;
        return true;
    });
}

bool CppGen::ChangeVar(const std::string& name, const std::string& type, const std::string& init, int flags, const std::string& scope)
{
    auto* var = FindVar(name, scope);
    if (!var)
        return false;
    var->type = CppType(type);
    var->init = init;
    if (flags != -1)
        var->flags = flags;
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

bool CppGen::RenameStruct(const std::string& oldn, const std::string& newn)
{
    if (newn == "" || newn == oldn || m_fields.count(newn))
        return false;
    auto it = m_fields.find(oldn);
    if (it == m_fields.end())
        return false;
    m_fields[newn] = std::move(it->second);
    m_fields.erase(oldn);
    return true;
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
//    id
//    id[*]
//    id[*].id
CppGen::VarExprResult
CppGen::CheckVarExpr(const std::string& name, const std::string& type_, const std::string& scope)
{
    std::string type = CppType(type_);
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
        std::string stype;
        if (!cpp::is_std_container(var->type, stype))
            return ConflictError;
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

//accepts name patterns recognized by CheckVarExpr
bool CppGen::CreateVarExpr(std::string& name, const std::string& type_, const std::string& init, int flags, const std::string& scope1)
{
    std::string type = CppType(type_);
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
            std::string stype;
            bool isCont = cpp::is_std_container(var->type, stype);
            if (!leaf)
            {
                if (!isCont)
                    return false;
                scope = stype;
            }
            else
            {
                if (!array)
                    return var->type == type;
                if (!isCont)
                    return false;
                return stype == type;
            }
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
                bool fun = IsFunType(stype);
                std::string ninit = init.size() ? init : DefaultInitFor(stype); //initialize scalar variables
                if (!CreateNamedVar(id, stype, ninit, flags, scope))
                    return false;
                /* better not to impicitly define new struct
                std::string elemType;
                if (!cpp::is_std_container(valueType, elemType)) {
                    if (elemType != "" && elemType[0] >= 'A' && elemType[0] <= 'Z')
                        m_fields[elemType];
                }*/
            }
            else {
                if (!CreateNamedVar(id, "std::vector<" + stype + ">", init, flags, scope))
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

std::vector<std::pair<std::string, std::string>>
CppGen::MatchType(const std::string& name, std::string_view type, std::string_view match, bool reference, const std::string& curArray)
{
    std::vector<std::pair<std::string, std::string>> ret;
    if (!type.compare(0, 6, "const ")) {
        if (reference)
            return ret;
        type.remove_prefix(6);
    }
    if (type.size() && type.back() == '&')
        type.remove_suffix(1);

    if ((match == "" && !IsFunType(type)) || type == match)
        ret.push_back({ name, std::string(type) });

    std::string valueType, firstType, secondType;
    if (cpp::is_ptr(type, valueType))
    {
        auto more = MatchType("*" + name, valueType, match, reference, curArray);
        ret.insert(ret.end(), more.begin(), more.end());
    }
    else if (cpp::is_std_container(type, valueType))
    {
        if (match == "[]")
            ret.push_back({ name, std::string(type) });
        if (!reference && (match == "" || match == "int")) {
            std::string pre = name[0]=='*' ? name.substr(1) + "->" : name + ".";
            ret.push_back({ pre + "size()", "int" });
        }
        size_t i = name.find(curArray);
        if (i != std::string::npos &&
            i + curArray.size() == name.size() &&
            std::count(name.begin(), name.begin() + i, '*') == i)
        {
            auto more = MatchType(CUR_ITEM_SYMBOL, valueType, match, reference, "");
            ret.insert(ret.end(), more.begin(), more.end());
        }
    }
    else if (cpp::is_std_pair(type, firstType, secondType))
    {
        std::string pre = name[0]=='*' ? name.substr(1) + "->" : name + ".";
        auto more = MatchType(pre + "first", firstType, match, reference, "");
        ret.insert(ret.end(), more.begin(), more.end());
        more = MatchType(pre + "second", secondType, match, reference, "");
        ret.insert(ret.end(), more.begin(), more.end());
    }
    else
    {
        auto it = m_fields.find(std::string(type));
        if (it == m_fields.end())
            return ret;
        std::string pre = name[0]=='*' ? name.substr(1) + "->" : name + ".";
        for (const auto& f : it->second)
        {
            auto more = MatchType(pre + f.name, f.type, match, reference, "");
            ret.insert(ret.end(), more.begin(), more.end());
        }
    }

    return ret;
}

//type=="" accepts all variables
//type=="int" suggests container.size() as well
//type=="[]" accepts all arrays
//type==void() accepts all functions of this type
//reference only accepts lvalues
//curArray accepts its array elements as well
std::vector<std::pair<std::string, std::string>> //(name, type)
CppGen::GetVarExprs(const std::string& type_, bool reference, const std::string& curArray)
{
    assert(type_.find("const ") == std::string::npos && type_.find("&") == std::string::npos);
    std::string type = CppType(type_);
    std::vector<std::pair<std::string, std::string>> ret;
    for (const auto& f : m_fields[""])
    {
        auto match = MatchType(f.name, f.type, type, reference, curArray);
        ret.insert(ret.end(), match.begin(), match.end());
    }
    stx::sort(ret);
    return ret;
}

