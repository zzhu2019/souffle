#pragma once

#include "RamStatement.h"

namespace souffle {

class RamProgram : public RamNode {
private:
    std::unique_ptr<RamStatement> main;
    std::map<std::string, std::unique_ptr<RamStatement>> subroutines;

public:
    RamProgram(std::unique_ptr<RamStatement> main) : RamNode(RN_Program), main(std::move(main)) {}

    virtual ~RamProgram() = default;

    std::vector<const RamNode*> getChildNodes() const {
        return toVector<const RamNode*>();
    }

    void print(std::ostream& out) const {
        out << "PROGRAM " << std::endl;
        main->print(out);
        // iterator over all subrountines
        //  -> print subroutine code

        for (auto it = subroutines.begin(); it != subroutines.end(); it++) {
            out << std::endl << "SUBROUTINE " << it->first << std::endl;
            it->second->print(out);
        }
    }

    const RamStatement* getMain() const {
        return main.get();
    }

    void addSubroutine(std::string main, std::unique_ptr<RamStatement> subroutine) {
        subroutines.insert(std::make_pair(main, std::move(subroutine)));
    }

    const RamStatement& getSubroutine(std::string name) const {
        return *subroutines.at(name);
    }
};

}  // end of namespace souffle
