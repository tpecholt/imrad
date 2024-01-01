#pragma once
#include <string>
#include <vector>
#include <map>
#include "node.h"
#include "cpp_parser.h"

//------------------------------------------------------
class CppGen
{
public:
	static const std::string INDENT;
	static const std::string FOR_VAR;

	CppGen();
	bool ExportUpdate(const std::string& fname, TopWindow* node, const std::map<std::string, std::string>& params, std::string& err);
	auto Import(const std::string& path, std::map<std::string, std::string>& params, std::string& err) -> std::unique_ptr<TopWindow>;
	static auto AltFName(const std::string& path) -> std::string;

	struct Var
	{
		std::string type;
		std::string name;
		std::string init;
		enum { UserCode = 0x1, Interface = 0x2, Impl = 0x4 };
		int flags = 0;
		Var(const std::string& n, const std::string& t, const std::string& i, int f)
			: name(n), type(t), init(i), flags(f) {}
	};

	const std::string& GetName() const { return m_name; }
	const std::string& GetVName() const { return m_vname; }
	//void SetName(const std::string& name) { m_name = name; }
	//void SetVName(const std::string& name) { m_vname = name; }
	void SetNamesFromId(const std::string& fname);

	std::string CreateVar(const std::string& type, const std::string& init, int flags, const std::string& scope = "");
	bool CreateNamedVar(const std::string& name, const std::string& type, const std::string& init, int flags, const std::string& scope = "");
	bool RenameVar(const std::string& oldn, const std::string& newn, const std::string& scope = "");
	bool RemoveVar(const std::string& name, const std::string& scope = "");
	bool ChangeVar(const std::string& name, const std::string& type, const std::string& init, const std::string& scope = "");
	const Var* GetVar(const std::string& name, const std::string& scope = "") const;
	const std::vector<Var>& GetVars(const std::string& scope = "");
	std::vector<std::string> GetStructTypes();
	enum VarExprResult { SyntaxError, ConflictError, Existing, New, New_ImplicitStruct };
	VarExprResult CheckVarExpr(const std::string& name, const std::string& type, const std::string& scope = "");
	bool CreateVarExpr(std::string& name, const std::string& type, const std::string& init, const std::string& scope = "");
	auto GetVarExprs(const std::string& type, bool recurse = false) ->std::vector<std::pair<std::string, std::string>>;

private:
	Var* FindVar(const std::string& name, const std::string& scope);

	void CreateH(std::ostream& out);
	void CreateCpp(std::ostream& out);
	auto ExportH(std::ostream& out, std::istream& prev, const std::string& origHName, TopWindow* node) -> std::array<std::string, 3>;
	auto ExportCpp(std::ostream& out, std::istream& prev, const std::array<std::string, 3>& origNames, const std::map<std::string, std::string>& params, TopWindow* node) -> std::string;
	bool WriteStub(std::ostream& fout,	const std::string& id, TopWindow::Kind kind, TopWindow::Placement animPos, const std::map<std::string, std::string>& params = {},	const std::string& code = {});
	auto ImportCode(std::istream& in, std::map<std::string, std::string>& params) -> std::unique_ptr<TopWindow>;

	bool ParseFieldDecl(const std::string& stype, const std::vector<std::string>& line, int flags);
	auto IsMemFun(const std::vector<std::string>& line)->std::string;
	bool IsMemDrawFun(const std::vector<std::string>& line);
	auto ParseDrawFun(const std::vector<std::string>& line, cpp::token_iterator& iter, std::map<std::string, std::string>& params) -> std::unique_ptr<TopWindow>;

	std::map<std::string, std::vector<Var>> m_fields;
	std::string m_name, m_vname, m_hname;
	std::string ctx_workingDir;
	std::string m_error;
};