#include "AstTransforms.h"

namespace souffle {

bool NaiveProvenanceTransformer::transform(AstTranslationUnit& translationUnit) {
    auto program = translationUnit.getProgram();
    
    // get next level number
    auto getNextLevelNumber = [&](std::vector<AstArgument*> levels) {
        if (levels.size() < 2) {
            return new AstBinaryFunctor(BinaryOp::ADD, std::unique_ptr<AstArgument>(levels[0]), std::unique_ptr<AstArgument>(new AstNumberConstant(1)));
        }

        auto currentMax = new AstBinaryFunctor(
                    BinaryOp::MAX, std::unique_ptr<AstArgument>(levels[0]), std::unique_ptr<AstArgument>(levels[1]));

        for (size_t i = 2; i < levels.size(); i++) {
            currentMax = new AstBinaryFunctor(BinaryOp::MAX, std::unique_ptr<AstArgument>(currentMax),
                    std::unique_ptr<AstArgument>(levels[i]));
        }

        return new AstBinaryFunctor(BinaryOp::ADD, std::unique_ptr<AstArgument>(currentMax), std::unique_ptr<AstArgument>(new AstNumberConstant(1)));
    };

    for (auto relation : program->getRelations()) {
        relation->addAttribute(std::unique_ptr<AstAttribute>(new AstAttribute(std::string("@rule_number"), *(new AstTypeIdentifier("number")))));
        relation->addAttribute(std::unique_ptr<AstAttribute>(new AstAttribute(std::string("@level_number"), *(new AstTypeIdentifier("number")))));
        
        // record clause number
        size_t clauseNum = 0;
        for (auto clause : relation->getClauses()) {
            // if fact, level number is 0
            if (clause->isFact()) {
                clause->getHead()->addArgument(std::unique_ptr<AstArgument>(new AstNumberConstant(clauseNum)));
                clause->getHead()->addArgument(std::unique_ptr<AstArgument>(new AstNumberConstant(0)));
            } else {
                std::vector<AstArgument*> bodyLevels;

                for (size_t i = 0; i < clause->getBodyLiterals().size(); i++) {
                    auto lit = clause->getBodyLiterals()[i];

                    if (auto atom = dynamic_cast<AstAtom*>(lit)) {
                        atom->addArgument(std::unique_ptr<AstArgument>(new AstUnnamedVariable()));
                        atom->addArgument(std::unique_ptr<AstArgument>(new AstVariable("@level_num_" + std::to_string(i))));
                        bodyLevels.push_back(new AstVariable("@level_num_" + std::to_string(i)));
                    } else if (auto neg = dynamic_cast<AstNegation*>(lit)) {
                        auto atom = neg->getAtom();
                        atom->addArgument(std::unique_ptr<AstArgument>(new AstUnnamedVariable()));
                        atom->addArgument(std::unique_ptr<AstArgument>(new AstUnnamedVariable()));
                    }
                }

                clause->getHead()->addArgument(std::unique_ptr<AstArgument>(new AstNumberConstant(clauseNum)));
                clause->getHead()->addArgument(std::unique_ptr<AstArgument>(getNextLevelNumber(bodyLevels)));
            }
            clauseNum++;
        }
    }

    return true;
}

} // end of namespace souffle
