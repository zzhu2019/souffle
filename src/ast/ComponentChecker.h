/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ComponentChecker.h
 *
 * Defines the component semantic checker pass.
 *
 ***********************************************************************/

#pragma once

#include "ast/Transformer.h"

namespace souffle {
class ErrorReport;
} 

namespace souffle::ast {

class Component;
class ComponentInit;
class ComponentLookup;
class ComponentScope;
class ComponentType;
class Program;
class SrcLocation;
class TranslationUnit;
class TypeBinding;

class ComponentChecker : public Transformer {
private:
    bool transform(TranslationUnit& translationUnit) override;

    static const Component* checkComponentNameReference(ErrorReport& report,
            const Component* enclosingComponent, const ComponentLookup& componentLookup,
            const std::string& name, const SrcLocation& loc, const TypeBinding& binding);
    static void checkComponentReference(ErrorReport& report, const Component* enclosingComponent,
            const ComponentLookup& componentLookup, const ComponentType& type, const SrcLocation& loc,
            const TypeBinding& binding);
    static void checkComponentInit(ErrorReport& report, const Component* enclosingComponent,
            const ComponentLookup& componentLookup, const ComponentInit& init, const TypeBinding& binding);
    static void checkComponent(ErrorReport& report, const Component* enclosingComponent,
            const ComponentLookup& componentLookup, const Component& component, const TypeBinding& binding);
    static void checkComponents(
            ErrorReport& report, const Program& program, const ComponentLookup& componentLookup);
    static void checkComponentNamespaces(ErrorReport& report, const Program& program);

public:
    ~ComponentChecker() override = default;

    std::string getName() const override {
        return "ComponentChecker";
    }
};
}  // namespace souffle::ast
