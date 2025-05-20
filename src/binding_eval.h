#pragma once
#include "binding_property.h"
#include "cppgen.h"

template <class T>
T bindable<T>::eval(const UIContext& ctx) const {
    if (empty())
        return {};
    if (has_value())
        return value();
    const auto *var = ctx.codeGen->GetVar(str);
    if (var) {
        T val;
        std::istringstream is(var->init);
        if (is >> std::boolalpha >> val)
            return val;
    }
    return {};
}

template <class T>
T field_ref<T>::eval(const UIContext &ctx) const
{
    if (empty())
        return {};
    const auto *var = ctx.codeGen->GetVar(str);
    if (!var)
        return {};
    T val;
    std::istringstream is(var->init);
    if (is >> std::boolalpha >> val)
        return val;
    return {};
}
