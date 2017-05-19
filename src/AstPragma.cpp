#include "AstPragma.h"
#include "Global.h"
#include "AstVisitor.h"

namespace souffle {
	bool AstPragmaChecker::transform(AstTranslationUnit& translationUnit){
		bool changed = false;
		AstProgram* program = translationUnit.getProgram();

		for (const auto& pragma : program->getPragmaDirectives()){
			std::pair<std::string, std::string> kvp = pragma->getkvp();
			// command line options take precedence
			if(!Global::config().has(kvp.first)){
				changed = true;
				Global::config().set(kvp.first, kvp.second);
			}
		}

		return changed;
	}
} // end of namespace souffle
