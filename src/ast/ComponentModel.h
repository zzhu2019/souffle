/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ComponentModel.h
 *
 ***********************************************************************/

#pragma once

#include "ast/Analysis.h"
#include "ast/Component.h"
#include "ast/Relation.h"
#include "ast/Transformer.h"
#include "ast/TranslationUnit.h"

#include <memory>
#include <string>
#include <vector>

namespace souffle::ast {

/**
 * Class that encapsulates std::map of types binding that comes from .init c = Comp<MyType>
 * Type binding in this example would be T->MyType if the component code is .comp Comp<T> ...
 */
class TypeBinding {
    /**
     * Key value pair. Keys are names that should be forwarded to value,
     * which is the actual name. Example T->MyImplementation.
     */
    std::map<TypeIdentifier, TypeIdentifier> binding;

public:
    /**
     * Returns binding for given name or empty string if such binding does not exist.
     */
    const TypeIdentifier& find(const TypeIdentifier& name) const {
        const static TypeIdentifier unknown;
        auto pos = binding.find(name);
        if (pos == binding.end()) {
            return unknown;
        }
        return pos->second;
    }

    TypeBinding extend(const std::vector<TypeIdentifier>& formalParams,
            const std::vector<TypeIdentifier>& actualParams) const {
        TypeBinding result;
        if (formalParams.size() != actualParams.size()) {
            return *this;  // invalid init => will trigger a semantic error
        }

        for (std::size_t i = 0; i < formalParams.size(); i++) {
            auto pos = binding.find(actualParams[i]);
            if (pos != binding.end()) {
                result.binding[formalParams[i]] = pos->second;
            } else {
                result.binding[formalParams[i]] = actualParams[i];
            }
        }

        return result;
    }
};

class ComponentLookup : public Analysis {
private:
    std::set<const Component*> globalScopeComponents;  // components defined outside of any components
    std::map<const Component*, std::set<const Component*>>
            nestedComponents;  // components defined inside a component
    std::map<const Component*, const Component*>
            enclosingComponent;  // component definition enclosing a component definition
public:
    static constexpr const char* name = "component-lookup";

    void run(const TranslationUnit& translationUnit) override;

    /**
     * Performs a lookup operation for a component with the given name within the addressed scope.
     *
     * @param scope the component scope to lookup in (null for global scope)
     * @param name the name of the component to be looking for
     * @return a pointer to the obtained component or null if there is no such component.
     */
    const Component* getComponent(
            const Component* scope, const std::string& name, const TypeBinding& activeBinding) const;
};

class ComponentInstantiationTransformer : public Transformer {
private:
    bool transform(TranslationUnit& translationUnit) override;

public:
    std::string getName() const override {
        return "ComponentInstantiationTransformer";
    }
};

}  // end of namespace souffle::ast
