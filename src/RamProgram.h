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
        std::vector<const RamNode*> children;
        children.push_back(main.get());

        // add subroutines
        for (auto& s : subroutines) {
            children.push_back(s.second.get());
        }

        return children;
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

    void addSubroutine(std::string name, std::unique_ptr<RamStatement> subroutine) {
        subroutines.insert(std::make_pair(name, std::move(subroutine)));
    }

    const std::map<std::string, RamStatement&> getSubroutines() const {
        std::map<std::string, RamStatement&> subroutineRefs;
        for (auto& s : subroutines) {
            subroutineRefs.insert({s.first, *s.second});
        }
        return subroutineRefs;
    }

    const RamStatement& getSubroutine(std::string name) const {
        return *subroutines.at(name);
    }
};

}  // end of namespace souffle
