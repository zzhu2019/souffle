#include "AstTransforms.h"

namespace souffle {

/**
 * Helper functions
 */
const std::string identifierToString(const AstRelationIdentifier& name) {
    std::stringstream ss;
    ss << name;
    return *(new std::string(ss.str()));
}

inline AstRelationIdentifier makeRelationName(
        const AstRelationIdentifier& orig, const std::string& type, int num = -1) {
    auto newName = new AstRelationIdentifier(identifierToString(orig));
    newName->append(type);
    if (num != -1) {
        newName->append((const std::string&)std::to_string(num));
    }

    return *newName;
}

std::unique_ptr<AstRelation> makeInfoRelation(
        AstClause& originalClause, AstTranslationUnit& translationUnit) {
    AstRelationIdentifier name =
            makeRelationName(originalClause.getHead()->getName(), "@info", originalClause.getClauseNum());

    // initialise info relation
    auto infoRelation = new AstRelation();
    infoRelation->setName(name);

    // create new clause containing a single fact
    auto infoClause = new AstClause();
    auto infoClauseHead = new AstAtom();
    infoClauseHead->setName(name);

    infoRelation->addAttribute(
            std::unique_ptr<AstAttribute>(new AstAttribute("clause_num", AstTypeIdentifier("number"))));
    infoClauseHead->addArgument(
            std::unique_ptr<AstArgument>(new AstNumberConstant(originalClause.getClauseNum())));

    // visit all body literals and add to info clause head
    for (size_t i = 0; i < originalClause.getBodyLiterals().size(); i++) {
        auto lit = originalClause.getBodyLiterals()[i];
        const AstAtom* atom = lit->getAtom();
        if (atom != nullptr) {
            std::string relName = identifierToString(atom->getName());

            infoRelation->addAttribute(std::unique_ptr<AstAttribute>(
                    new AstAttribute(std::string("rel_") + std::to_string(i), AstTypeIdentifier("symbol"))));

            if (dynamic_cast<AstAtom*>(lit)) {
                infoClauseHead->addArgument(std::unique_ptr<AstArgument>(
                        new AstStringConstant(translationUnit.getSymbolTable(), relName.c_str())));
            } else if (dynamic_cast<AstNegation*>(lit)) {
                infoClauseHead->addArgument(std::unique_ptr<AstArgument>(
                        new AstStringConstant(translationUnit.getSymbolTable(), ("!" + relName).c_str())));
            }
        }
    }

    // generate and add clause representation
    std::stringstream ss;
    originalClause.print(ss);

    infoRelation->addAttribute(
            std::unique_ptr<AstAttribute>(new AstAttribute("clause_repr", AstTypeIdentifier("symbol"))));
    infoClauseHead->addArgument(std::unique_ptr<AstArgument>(
            new AstStringConstant(translationUnit.getSymbolTable(), ss.str().c_str())));

    // set clause head and add clause to info relation
    infoClause->setHead(std::unique_ptr<AstAtom>(infoClauseHead));
    infoRelation->addClause(std::unique_ptr<AstClause>(infoClause));

    return std::unique_ptr<AstRelation>(infoRelation);
}

bool NaiveProvenanceTransformer::transform(AstTranslationUnit& translationUnit) {
    auto program = translationUnit.getProgram();

    // get next level number
    auto getNextLevelNumber = [&](std::vector<AstArgument*> levels) {
        if (levels.size() == 0) {
            return static_cast<AstArgument*>(new AstNumberConstant(0));
        }

        if (levels.size() == 1) {
            return static_cast<AstArgument*>(
                    new AstBinaryFunctor(BinaryOp::ADD, std::unique_ptr<AstArgument>(levels[0]),
                            std::unique_ptr<AstArgument>(new AstNumberConstant(1))));
        }

        auto currentMax = new AstBinaryFunctor(BinaryOp::MAX, std::unique_ptr<AstArgument>(levels[0]),
                std::unique_ptr<AstArgument>(levels[1]));

        for (size_t i = 2; i < levels.size(); i++) {
            currentMax = new AstBinaryFunctor(BinaryOp::MAX, std::unique_ptr<AstArgument>(currentMax),
                    std::unique_ptr<AstArgument>(levels[i]));
        }

        return static_cast<AstArgument*>(
                new AstBinaryFunctor(BinaryOp::ADD, std::unique_ptr<AstArgument>(currentMax),
                        std::unique_ptr<AstArgument>(new AstNumberConstant(1))));
    };

    for (auto relation : program->getRelations()) {
        relation->addAttribute(std::unique_ptr<AstAttribute>(
                new AstAttribute(std::string("@rule_number"), AstTypeIdentifier("number"))));
        relation->addAttribute(std::unique_ptr<AstAttribute>(
                new AstAttribute(std::string("@level_number"), AstTypeIdentifier("number"))));

        // record clause number
        size_t clauseNum = 0;
        for (auto clause : relation->getClauses()) {
            clause->setClauseNum(clauseNum);

            // mapper to add two provenance columns to atoms
            struct M : public AstNodeMapper {
                using AstNodeMapper::operator();

                std::unique_ptr<AstNode> operator()(std::unique_ptr<AstNode> node) const override {
                    // add provenance columns
                    if (auto atom = dynamic_cast<AstAtom*>(node.get())) {
                        atom->addArgument(std::unique_ptr<AstArgument>(new AstUnnamedVariable()));
                        atom->addArgument(std::unique_ptr<AstArgument>(new AstUnnamedVariable()));
                    } else if (auto neg = dynamic_cast<AstNegation*>(node.get())) {
                        auto atom = neg->getAtom();
                        atom->addArgument(std::unique_ptr<AstArgument>(new AstUnnamedVariable()));
                        atom->addArgument(std::unique_ptr<AstArgument>(new AstUnnamedVariable()));
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
                clause->getHead()->addArgument(
                        std::unique_ptr<AstArgument>(new AstNumberConstant(clauseNum)));
                clause->getHead()->addArgument(std::unique_ptr<AstArgument>(new AstNumberConstant(0)));
            } else {
                std::vector<AstArgument*> bodyLevels;

                for (size_t i = 0; i < clause->getBodyLiterals().size(); i++) {
                    auto lit = clause->getBodyLiterals()[i];

                    // add unnamed vars to each atom nested in arguments of lit
                    lit->apply(M());

                    // add two provenance columns to lit; first is rule num, second is level num
                    if (auto atom = dynamic_cast<AstAtom*>(lit)) {
                        atom->addArgument(std::unique_ptr<AstArgument>(new AstUnnamedVariable()));
                        atom->addArgument(std::unique_ptr<AstArgument>(
                                new AstVariable("@level_num_" + std::to_string(i))));
                        bodyLevels.push_back(new AstVariable("@level_num_" + std::to_string(i)));
                        /*
                    } else if (auto neg = dynamic_cast<AstNegation*>(lit)) {
                        auto atom = neg->getAtom();
                        atom->addArgument(std::unique_ptr<AstArgument>(new AstUnnamedVariable()));
                        atom->addArgument(std::unique_ptr<AstArgument>(new AstUnnamedVariable()));
                        */
                    }
                }

                // add two provenance columns to head lit
                clause->getHead()->addArgument(
                        std::unique_ptr<AstArgument>(new AstNumberConstant(clauseNum)));
                clause->getHead()->addArgument(std::unique_ptr<AstArgument>(getNextLevelNumber(bodyLevels)));
            }

            // add info relation
            program->addRelation(makeInfoRelation(*clause, translationUnit));

            clauseNum++;
        }
    }

    // program->print(std::cout);
    // std::cout << std::endl;

    return true;
}

}  // end of namespace souffle
