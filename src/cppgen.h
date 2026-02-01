#pragma once
#include <string>
#include <vector>
#include <map>
#include "node_window.h"

//------------------------------------------------------
class CppGen
{
public:
    static const std::string_view INDENT;
    static const std::string_view FOR_VAR_NAME;
    static const std::string_view CUR_ITEM_VAR_NAME;
    static const std::string_view HBOX_NAME;
    static const std::string_view VBOX_NAME;

    CppGen();

    struct Config
    {
        std::string name;
        TopWindow* node;
        std::map<std::string, std::string> params;
    };

    bool ExportUpdate(const std::string& fname, const std::vector<Config>& configs, std::string& err);
    auto Import(const std::string& path, std::string& err) -> std::vector<Config>;
    int ReadGenVersion(const std::string& fname) const;

    const std::string& GetName() const { return m_name; }
    const std::string& GetVName() const { return m_vname; }
    //void SetName(const std::string& name) { m_name = name; }
    //void SetVName(const std::string& name) { m_vname = name; }
    void SetNamesFromId(const std::string& fname);
    auto AltFName(const std::string& path) const -> std::string;

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

    std::string CreateVar(const std::string& type, const std::string& init, int flags, const std::string& scope = "");
    bool CreateNamedVar(const std::string& name, const std::string& type, const std::string& init, int flags, const std::string& scope = "");
    bool RenameVar(const std::string& oldn, const std::string& newn, const std::string& scope = "");
    bool RemoveVar(const std::string& name, const std::string& scope = "");
    void RemovePrefixedVars(std::string_view prefix, std::string_view scope = "");
    bool ChangeVar(const std::string& name, const std::string& type, const std::string& init, int flags = -1, const std::string& scope = "");
    const Var* GetVar(const std::string& name, const std::string& scope = "") const;
    bool RenameStruct(const std::string& oldn, const std::string& newn);
    const std::vector<Var>& GetVars(const std::string& scope = "");
    std::vector<std::string> GetLayoutVars();
    std::vector<std::string> GetStructTypes();
    enum VarExprResult { SyntaxError, ConflictError, Existing, New, New_ImplicitStruct };
    VarExprResult CheckVarExpr(const std::string& name, const std::string& type, const std::string& scope = "");
    bool CreateVarExpr(std::string& name, const std::string& type, const std::string& init, int flags, const std::string& scope = "");
    auto GetVarExprs(const std::string& type, bool reference, const std::string& curArray = "") ->std::vector<std::pair<std::string, std::string>>;

private:
    Var* FindVar(const std::string& name, const std::string& scope);
    auto MatchType(const std::string& name, std::string_view type, std::string_view match, bool reference, const std::string& curArray) -> std::vector<std::pair<std::string, std::string>>;

    void CreateH(std::ostream& out);
    void CreateCpp(std::ostream& out);
    auto ExportH(std::ostream& out, std::istream& prev, const std::string& origHName, const std::vector<Config>& configs) -> std::array<std::string, 3>;
    void ExportCpp(std::ostream& out, std::istream& prev, const std::array<std::string, 3>& origNames, const std::vector<Config>& configs, const std::vector<std::string>& drawCode);
    bool WriteStub(std::ostream& fout, const std::string& id, const std::vector<Config>& configs);
    void WriteDrawFun(std::ostream& fout, const std::string& id, const std::vector<Config>& configs, const std::string& code);
    void WriteForEachConfig(std::ostream& out, const std::vector<Config>& configs, std::function<std::string(const Config&)> fun);
    auto ImportCode(std::istream& in, const std::string& fname) -> std::vector<Config>;
    auto GetDrawFunName(const std::string& cfgName, size_t size) -> std::string;

    bool ParseFieldDecl(const std::string& stype, const std::vector<std::string>& line, int flags);
    auto IsMemFun(const std::vector<std::string>& line)->std::string;
    auto ParseDrawFun(const std::vector<std::string>& line, cpp::token_iterator& iter) -> std::optional<Config>;

    std::map<std::string, std::vector<Var>> m_fields;
    TopWindow::Kind m_kind;
    bool m_animate;
    std::string m_name, m_vname, m_hname;
    std::string ctx_workingDir;
    int ctx_importVersion;
    std::string m_error;
};