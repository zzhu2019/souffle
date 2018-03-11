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

#include "Transformer.h"

namespace souffle {

class AstComponent;
class AstComponentScope;
class AstComponentType;
class AstComponentInit;
class AstProgram;
class AstSrcLocation;
class AstTranslationUnit;
class ComponentLookup;
class ErrorReport;
class TypeBinding;

class AstComponentChecker : public AstTransformer {
private:
    bool transform(AstTranslationUnit& translationUnit) override;

    static const AstComponent* checkComponentNameReference(ErrorReport& report,
            const AstComponent* enclosingComponent, const ComponentLookup& componentLookup,
            const std::string& name, const AstSrcLocation& loc, const TypeBinding& binding);
    static void checkComponentReference(ErrorReport& report, const AstComponent* enclosingComponent,
            const ComponentLookup& componentLookup, const AstComponentType& type, const AstSrcLocation& loc,
            const TypeBinding& binding);
    static void checkComponentInit(ErrorReport& report, const AstComponent* enclosingComponent,
            const ComponentLookup& componentLookup, const AstComponentInit& init, const TypeBinding& binding);
    static void checkComponent(ErrorReport& report, const AstComponent* enclosingComponent,
            const ComponentLookup& componentLookup, const AstComponent& component,
            const TypeBinding& binding);
    static void checkComponents(
            ErrorReport& report, const AstProgram& program, const ComponentLookup& componentLookup);
    static void checkComponentNamespaces(ErrorReport& report, const AstProgram& program);

public:
    ~AstComponentChecker() override = default;

    std::string getName() const override {
        return "AstComponentChecker";
    }
};
}
