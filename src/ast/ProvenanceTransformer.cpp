/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2017, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ProvenanceTransformer.cpp
 *
 * Implements ast::Transformer for adding provenance information via two extra columns
 *
 ***********************************************************************/

#include "ast/Transforms.h"

namespace souffle::ast {

/**
 * Helper functions
 */
const std::string identifierToString(const ast::RelationIdentifier& name) {
    std::stringstream ss;
    ss << name;
    return ss.str();
}

inline ast::RelationIdentifier makeRelationName(
        const ast::RelationIdentifier& orig, const std::string& type, int num = -1) {
    ast::RelationIdentifier newName(identifierToString(orig));
    newName.append(type);
    if (num != -1) {
        newName.append((const std::string&)std::to_string(num));
    }

    return newName;
}

std::unique_ptr<ast::Relation> makeInfoRelation(
        ast::Clause& originalClause, ast::TranslationUnit& translationUnit) {
    ast::RelationIdentifier name =
            makeRelationName(originalClause.getHead()->getName(), "@info", originalClause.getClauseNum());

    // initialise info relation
    auto infoRelation = new ast::Relation();
    infoRelation->setName(name);

    // create new clause containing a single fact
    auto infoClause = new ast::Clause();
    auto infoClauseHead = new ast::Atom();
    infoClauseHead->setName(name);

    infoRelation->addAttribute(std::make_unique<ast::Attribute>("clause_num", ast::TypeIdentifier("number")));
    infoClauseHead->addArgument(std::make_unique<ast::NumberConstant>(originalClause.getClauseNum()));

    // visit all body literals and add to info clause head
    for (size_t i = 0; i < originalClause.getBodyLiterals().size(); i++) {
        auto lit = originalClause.getBodyLiterals()[i];
        const ast::Atom* atom = lit->getAtom();
        if (atom != nullptr) {
            std::string relName = identifierToString(atom->getName());

            infoRelation->addAttribute(std::unique_ptr<ast::Attribute>(
                    new ast::Attribute(std::string("rel_") + std::to_string(i), ast::TypeIdentifier("symbol"))));

            if (dynamic_cast<ast::Atom*>(lit)) {
                infoClauseHead->addArgument(std::unique_ptr<ast::Argument>(
                        new ast::StringConstant(translationUnit.getSymbolTable(), relName.c_str())));
            } else if (dynamic_cast<ast::Negation*>(lit)) {
                infoClauseHead->addArgument(std::unique_ptr<ast::Argument>(
                        new ast::StringConstant(translationUnit.getSymbolTable(), ("!" + relName).c_str())));
            }
        }
    }

    // generate and add clause representation
    std::stringstream ss;
    originalClause.print(ss);

    infoRelation->addAttribute(std::make_unique<ast::Attribute>("clause_repr", ast::TypeIdentifier("symbol")));
    infoClauseHead->addArgument(std::unique_ptr<ast::Argument>(
            new ast::StringConstant(translationUnit.getSymbolTable(), ss.str().c_str())));

    // set clause head and add clause to info relation
    infoClause->setHead(std::unique_ptr<ast::Atom>(infoClauseHead));
    infoRelation->addClause(std::unique_ptr<ast::Clause>(infoClause));

    return std::unique_ptr<ast::Relation>(infoRelation);
}

/** Transform eqrel relations to explicitly define equivalence relations */
void transformEqrelRelation(ast::Relation& rel) {
    assert(rel.isEqRel() && "attempting to transform non-eqrel relation");
    assert(rel.getArity() == 2 && "eqrel relation not binary");

    rel.setQualifier(rel.getQualifier() - EQREL_RELATION);

    // transitivity
    // transitive clause: A(x, z) :- A(x, y), A(y, z).
    auto transitiveClause = new ast::Clause();
    auto transitiveClauseHead = new ast::Atom(rel.getName());
    transitiveClauseHead->addArgument(std::make_unique<ast::Variable>("x"));
    transitiveClauseHead->addArgument(std::make_unique<ast::Variable>("z"));

    auto transitiveClauseBody = new ast::Atom(rel.getName());
    transitiveClauseBody->addArgument(std::make_unique<ast::Variable>("x"));
    transitiveClauseBody->addArgument(std::make_unique<ast::Variable>("y"));

    auto transitiveClauseBody2 = new ast::Atom(rel.getName());
    transitiveClauseBody2->addArgument(std::make_unique<ast::Variable>("y"));
    transitiveClauseBody2->addArgument(std::make_unique<ast::Variable>("z"));

    transitiveClause->setHead(std::unique_ptr<ast::Atom>(transitiveClauseHead));
    transitiveClause->addToBody(std::unique_ptr<ast::Literal>(transitiveClauseBody));
    transitiveClause->addToBody(std::unique_ptr<ast::Literal>(transitiveClauseBody2));
    rel.addClause(std::unique_ptr<ast::Clause>(transitiveClause));

    // symmetric
    // symmetric clause: A(x, y) :- A(y, x).
    auto symClause = new ast::Clause();
    auto symClauseHead = new ast::Atom(rel.getName());
    symClauseHead->addArgument(std::make_unique<ast::Variable>("x"));
    symClauseHead->addArgument(std::make_unique<ast::Variable>("y"));

    auto symClauseBody = new ast::Atom(rel.getName());
    symClauseBody->addArgument(std::make_unique<ast::Variable>("y"));
    symClauseBody->addArgument(std::make_unique<ast::Variable>("x"));

    symClause->setHead(std::unique_ptr<ast::Atom>(symClauseHead));
    symClause->addToBody(std::unique_ptr<ast::Literal>(symClauseBody));
    rel.addClause(std::unique_ptr<ast::Clause>(symClause));

    // reflexivity
    // reflexive clause: A(x, x) :- A(x, _).
    auto reflexiveClause = new ast::Clause();
    auto reflexiveClauseHead = new ast::Atom(rel.getName());
    reflexiveClauseHead->addArgument(std::make_unique<ast::Variable>("x"));
    reflexiveClauseHead->addArgument(std::make_unique<ast::Variable>("x"));

    auto reflexiveClauseBody = new ast::Atom(rel.getName());
    reflexiveClauseBody->addArgument(std::make_unique<ast::Variable>("x"));
    reflexiveClauseBody->addArgument(std::make_unique<ast::UnnamedVariable>());

    reflexiveClause->setHead(std::unique_ptr<ast::Atom>(reflexiveClauseHead));
    reflexiveClause->addToBody(std::unique_ptr<ast::Literal>(reflexiveClauseBody));
    rel.addClause(std::unique_ptr<ast::Clause>(reflexiveClause));
}

bool ProvenanceTransformer::transform(ast::TranslationUnit& translationUnit) {
    auto program = translationUnit.getProgram();

    // get next level number
    auto getNextLevelNumber = [&](std::vector<ast::Argument*> levels) {
        if (levels.size() == 0) {
            return static_cast<ast::Argument*>(new ast::NumberConstant(0));
        }

        if (levels.size() == 1) {
            return static_cast<ast::Argument*>(new ast::BinaryFunctor(BinaryOp::ADD,
                    std::unique_ptr<ast::Argument>(levels[0]), std::make_unique<ast::NumberConstant>(1)));
        }

        auto currentMax = new ast::BinaryFunctor(BinaryOp::MAX, std::unique_ptr<ast::Argument>(levels[0]),
                std::unique_ptr<ast::Argument>(levels[1]));

        for (size_t i = 2; i < levels.size(); i++) {
            currentMax = new ast::BinaryFunctor(BinaryOp::MAX, std::unique_ptr<ast::Argument>(currentMax),
                    std::unique_ptr<ast::Argument>(levels[i]));
        }

        return static_cast<ast::Argument*>(new ast::BinaryFunctor(BinaryOp::ADD,
                std::unique_ptr<ast::Argument>(currentMax), std::make_unique<ast::NumberConstant>(1)));
    };

    for (auto relation : program->getRelations()) {
        if (relation->isEqRel()) {
            transformEqrelRelation(*relation);
        }

        relation->addAttribute(std::unique_ptr<ast::Attribute>(
                new ast::Attribute(std::string("@rule_number"), ast::TypeIdentifier("number"))));
        relation->addAttribute(std::unique_ptr<ast::Attribute>(
                new ast::Attribute(std::string("@level_number"), ast::TypeIdentifier("number"))));

        // record clause number
        size_t clauseNum = 1;
        for (auto clause : relation->getClauses()) {
            clause->setClauseNum(clauseNum);

            // mapper to add two provenance columns to atoms
            struct M : public ast::NodeMapper {
                using ast::NodeMapper::operator();

                std::unique_ptr<ast::Node> operator()(std::unique_ptr<ast::Node> node) const override {
                    // add provenance columns
                    if (auto atom = dynamic_cast<ast::Atom*>(node.get())) {
                        atom->addArgument(std::make_unique<ast::UnnamedVariable>());
                        atom->addArgument(std::make_unique<ast::UnnamedVariable>());
                    } else if (auto neg = dynamic_cast<ast::Negation*>(node.get())) {
                        auto atom = neg->getAtom();
                        atom->addArgument(std::make_unique<ast::UnnamedVariable>());
                        atom->addArgument(std::make_unique<ast::UnnamedVariable>());
                    }

                    // otherwise - apply mapper recursively
                    node->apply(*this);
                    return node;
                }
            };

            // add unnamed vars to each atom nested in arguments of head
            clause->getHead()->apply(M());

            // if fact, level number is 0
            if (clause->isFact()) {
                clause->getHead()->addArgument(std::make_unique<ast::NumberConstant>(0));
                clause->getHead()->addArgument(std::make_unique<ast::NumberConstant>(0));
            } else {
                std::vector<ast::Argument*> bodyLevels;

                for (size_t i = 0; i < clause->getBodyLiterals().size(); i++) {
                    auto lit = clause->getBodyLiterals()[i];

                    // add unnamed vars to each atom nested in arguments of lit
                    lit->apply(M());

                    // add two provenance columns to lit; first is rule num, second is level num
                    if (auto atom = dynamic_cast<ast::Atom*>(lit)) {
                        atom->addArgument(std::make_unique<ast::UnnamedVariable>());
                        atom->addArgument(std::unique_ptr<ast::Argument>(
                                new ast::Variable("@level_num_" + std::to_string(i))));
                        bodyLevels.push_back(new ast::Variable("@level_num_" + std::to_string(i)));
                        /*
                    } else if (auto neg = dynamic_cast<ast::Negation*>(lit)) {
                        auto atom = neg->getAtom();
                        atom->addArgument(std::make_unique<ast::UnnamedVariable>());
                        atom->addArgument(std::make_unique<ast::UnnamedVariable>());
                        */
                    }
                }

                // add two provenance columns to head lit
                clause->getHead()->addArgument(std::make_unique<ast::NumberConstant>(clauseNum));
                clause->getHead()->addArgument(std::unique_ptr<ast::Argument>(getNextLevelNumber(bodyLevels)));

                clauseNum++;

                // add info relation
                program->addRelation(makeInfoRelation(*clause, translationUnit));
            }
        }
    }

    // program->print(std::cout);
    // std::cout << std::endl;

    return true;
}

}  // end of namespace souffle::ast
