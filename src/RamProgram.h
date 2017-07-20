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
    }

    std::unique_ptr<RamStatement> getMain() {
        return std::move(main);
    }

    void addSubroutine(std::string main, std::unique_ptr<RamStatement> subroutine) {
        subroutines.insert(std::make_pair(main, std::move(subroutine)));
    }

    std::unique_ptr<RamStatement> getSubroutine(std::string name) {
        if (subroutines.find(name) != subroutines.end()) {
            return std::move(subroutines[name]);
        } else {
            return nullptr;
        }
    }
};

}  // end of namespace souffle
