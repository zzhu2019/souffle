/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file parser.yy
 *
 * @brief Parser for Datalog
 *
 ***********************************************************************/
%skeleton "lalr1.cc"
%require "3.0.2"
%defines
%define parser_class_name {parser}
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%define api.location.type {ast::SrcLocation}

%code requires {
    #include <config.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <assert.h>
    #include <unistd.h>
    #include <stdarg.h>
    #include <string>
    #include <stack>
    #include <unistd.h>

    #include "Util.h"
    #include "ast/Program.h"
    #include "ast/Clause.h"
    #include "ast/Component.h"
    #include "ast/Relation.h"
    #include "ast/IODirective.h"
    #include "ast/Argument.h"
    #include "ast/Node.h"
    #include "UnaryFunctorOps.h"
    #include "BinaryFunctorOps.h"
    #include "BinaryConstraintOps.h"
    #include "ast/ParserUtils.h"

    #include "ast/SrcLocation.h"
    #include "ast/Types.h"

    using namespace souffle;

    namespace souffle {
        class ParserDriver;
    }

    typedef void* yyscan_t;

    #define YY_NULLPTR nullptr

    /* Macro to update locations as parsing proceeds */
    # define YYLLOC_DEFAULT(Cur, Rhs, N)                       \
    do {                                                       \
       if (N) {                                                \
           (Cur).start        = YYRHSLOC(Rhs, 1).start;        \
           (Cur).end          = YYRHSLOC(Rhs, N).end;          \
           (Cur).filename     = YYRHSLOC(Rhs, N).filename;     \
       } else {                                                \
           (Cur).start    = YYRHSLOC(Rhs, 0).end;              \
           (Cur).filename = YYRHSLOC(Rhs, 0).filename;         \
       }                                                       \
    } while (0)
}

%param { ParserDriver &driver }
%param { yyscan_t yyscanner }

%locations

%define parse.trace
%define parse.error verbose

%code {
    #include "ParserDriver.h"
}

%token <std::string> RESERVED    "reserved keyword"
%token END 0                     "end of file"
%token <std::string> STRING      "symbol"
%token <std::string> IDENT       "identifier"
%token <ast::Domain> NUMBER        "number"
%token <std::string> RELOP       "relational operator"
%token PRAGMA                    "pragma directive"
%token OUTPUT_QUALIFIER          "relation qualifier output"
%token INPUT_QUALIFIER           "relation qualifier input"
%token PRINTSIZE_QUALIFIER       "relation qualifier printsize"
%token BRIE_QUALIFIER            "BRIE datastructure qualifier"
%token BTREE_QUALIFIER           "BTREE datastructure qualifier"
%token EQREL_QUALIFIER           "equivalence relation qualifier"
%token RBTSET_QUALIFIER          "red-black tree set relation qualifier"
%token HASHSET_QUALIFIER         "hashset relation qualifier"
%token OVERRIDABLE_QUALIFIER     "relation qualifier overidable"
%token INLINE_QUALIFIER          "relation qualifier inline"
%token TMATCH                    "match predicate"
%token TCONTAINS                 "checks whether substring is contained in a string"
%token CAT                       "concatenation of two strings"
%token ORD                       "ordinal number of a string"
%token STRLEN                    "length of a string"
%token SUBSTR                    "sub-string of a string"
%token MIN                       "min aggregator"
%token MAX                       "max aggregator"
%token COUNT                     "count aggregator"
%token SUM                       "sum aggregator"
%token TRUE                      "true literal constraint"
%token FALSE                     "false literal constraint"
%token STRICT                    "strict marker"
%token PLAN                      "plan keyword"
%token IF                        ":-"
%token DECL                      "relation declaration"
%token INPUT_DECL                "input directives declaration"
%token OUTPUT_DECL               "output directives declaration"
%token PRINTSIZE_DECL            "printsize directives declaration"
%token OVERRIDE                  "override rules of super-component"
%token TYPE                      "type declaration"
%token COMPONENT                 "component declaration"
%token INSTANTIATE               "component instantiation"
%token NUMBER_TYPE               "numeric type declaration"
%token SYMBOL_TYPE               "symbolic type declaration"
%token AS                        "type cast"
%token NIL                       "nil reference"
%token PIPE                      "|"
%token LBRACKET                  "["
%token RBRACKET                  "]"
%token UNDERSCORE                "_"
%token DOLLAR                    "$"
%token PLUS                      "+"
%token MINUS                     "-"
%token EXCLAMATION               "!"
%token LPAREN                    "("
%token RPAREN                    ")"
%token COMMA                     ","
%token COLON                     ":"
%token SEMICOLON                 ";"
%token DOT                       "."
%token EQUALS                    "="
%token STAR                      "*"
%token SLASH                     "/"
%token CARET                     "^"
%token PERCENT                   "%"
%token LBRACE                    "{"
%token RBRACE                    "}"
%token LT                        "<"
%token GT                        ">"
%token BW_AND                    "band"
%token BW_OR                     "bor"
%token BW_XOR                    "bxor"
%token BW_NOT                    "bnot"
%token L_AND                     "land"
%token L_OR                      "lor"
%token L_NOT                     "lnot"

%type <uint32_t>                         qualifiers
%type <ast::TypeIdentifier *>              type_id
%type <ast::RelationIdentifier *>          rel_id
%type <ast::Type *>                        type
%type <ast::Component *>                   component component_head component_body
%type <ast::ComponentType *>               comp_type
%type <ast::ComponentInit *>               comp_init
%type <ast::Relation *>                    attributes non_empty_attributes relation_body
%type <std::vector<ast::Relation *>>       relation_list relation_head
%type <ast::Argument *>                    arg
%type <ast::Atom *>                        arg_list non_empty_arg_list atom
%type <std::vector<ast::Atom*>>            head
%type <ast::RuleBody *>                       literal term disjunction conjunction body
%type <ast::Clause *>                      fact
%type <ast::Pragma *>                      pragma
%type <std::vector<ast::Clause*>>          rule rule_def
%type <ast::ExecutionOrder *>              exec_order exec_order_list
%type <ast::ExecutionPlan *>               exec_plan exec_plan_list
%type <ast::RecordInit *>                  recordlist
%type <ast::RecordType *>                  recordtype
%type <ast::UnionType *>                   uniontype
%type <std::vector<ast::TypeIdentifier>>   type_params type_param_list
%type <std::string>                      comp_override
%type <ast::IODirective *>                 key_value_pairs non_empty_key_value_pairs iodirective iodirective_body
%printer { yyoutput << $$; } <*>;

%precedence AS
%left L_OR
%left L_AND
%left BW_OR
%left BW_XOR
%left BW_AND
%left PLUS MINUS
%left STAR SLASH PERCENT
%precedence BW_NOT L_NOT
%precedence NEG
%left CARET

%%
%start program;

/* Program */
program
  : unit

/* Top-level statement */
unit
  : unit type {
        driver.addType(std::unique_ptr<ast::Type>($2));
    }
  | unit relation_head {
        for(const auto& cur : $2) driver.addRelation(std::unique_ptr<ast::Relation>(cur));
    }
  | unit iodirective {
        driver.addIODirectiveChain(std::unique_ptr<ast::IODirective>($2));
    }
  | unit fact {
        driver.addClause(std::unique_ptr<ast::Clause>($2));
    }
  | unit rule {
        for(const auto& cur : $2) driver.addClause(std::unique_ptr<ast::Clause>(cur));
    }
  | unit component {
        driver.addComponent(std::unique_ptr<ast::Component>($2));
    }
  | unit comp_init {
        driver.addInstantiation(std::unique_ptr<ast::ComponentInit>($2));
    }
  | unit pragma {
        driver.addPragma(std::unique_ptr<ast::Pragma>($2));
    }
  | %empty {
    }

/* Pragma directives */
pragma
  : PRAGMA STRING STRING {
        $$ = new ast::Pragma($2,$3);
        $$->setSrcLoc(@$);
    }

  | PRAGMA STRING {
        $$ = new ast::Pragma($2, "");
        $$->setSrcLoc(@$);
    }



/* Type Identifier */
type_id
  : IDENT {
        $$ = new ast::TypeIdentifier($1);
    }
  | type_id DOT IDENT {
        $$ = $1;
        $$->append($3);
    }

/* Type Declaration */
type
  : NUMBER_TYPE IDENT {
        $$ = new ast::PrimitiveType($2, true);
        $$->setSrcLoc(@$);
    }
  | SYMBOL_TYPE IDENT {
        $$ = new ast::PrimitiveType($2, false);
        $$->setSrcLoc(@$);
    }
  | TYPE IDENT {
        $$ = new ast::PrimitiveType($2);
        $$->setSrcLoc(@$);
    }
  | TYPE IDENT EQUALS uniontype {
        $$ = $4;
        $$->setName($2);
        $$->setSrcLoc(@$);
    }
  | TYPE IDENT EQUALS LBRACKET recordtype RBRACKET {
        $$ = $5;
        $$->setName($2);
        $$->setSrcLoc(@$);
    }
  | TYPE IDENT EQUALS LBRACKET RBRACKET {
        $$ = new ast::RecordType();
        $$->setName($2);
        $$->setSrcLoc(@$);
    }

recordtype
  : IDENT COLON type_id {
        $$ = new ast::RecordType();
        $$->add($1, *$3); delete $3;
    }
  | recordtype COMMA IDENT COLON type_id {
         $$ = $1;
        $1->add($3, *$5); delete $5;
    }

uniontype
  : type_id {
        $$ = new ast::UnionType();
        $$->add(*$1); delete $1;
    }
  | uniontype PIPE type_id {
        $$ = $1;
        $1->add(*$3); delete $3;
    }


/* Relation Identifier */

rel_id
  : IDENT {
        $$ = new ast::RelationIdentifier($1);
    }
  | rel_id DOT IDENT {
        $$ = $1;
        $$->append($3);
    }


/* Relations */
non_empty_attributes
  : IDENT COLON type_id {
        $$ = new ast::Relation();
        ast::Attribute *a = new ast::Attribute($1, *$3);
        a->setSrcLoc(@3);
        $$->addAttribute(std::unique_ptr<ast::Attribute>(a));
        delete $3;
    }
  | attributes COMMA IDENT COLON type_id {
        $$ = $1;
        ast::Attribute *a = new ast::Attribute($3, *$5);
        a->setSrcLoc(@5);
        $$->addAttribute(std::unique_ptr<ast::Attribute>(a));
        delete $5;
    }

attributes
  : non_empty_attributes {
        $$ = $1;
    }
  | %empty {
        $$ = new ast::Relation();
    }

qualifiers
  : qualifiers OUTPUT_QUALIFIER {
        if($1 & OUTPUT_RELATION) driver.error(@2, "output qualifier already set");
        $$ = $1 | OUTPUT_RELATION;
    }
  | qualifiers INPUT_QUALIFIER {
        if($1 & INPUT_RELATION) driver.error(@2, "input qualifier already set");
        $$ = $1 | INPUT_RELATION;
    }
  | qualifiers PRINTSIZE_QUALIFIER {
        if($1 & PRINTSIZE_RELATION) driver.error(@2, "printsize qualifier already set");
        $$ = $1 | PRINTSIZE_RELATION;
    }
  | qualifiers OVERRIDABLE_QUALIFIER {
        if($1 & OVERRIDABLE_RELATION) driver.error(@2, "overridable qualifier already set");
        $$ = $1 | OVERRIDABLE_RELATION;
    }
  | qualifiers INLINE_QUALIFIER {
        if($1 & INLINE_RELATION) driver.error(@2, "inline qualifier already set");
        $$ = $1 | INLINE_RELATION;
    }
  | qualifiers BRIE_QUALIFIER {
        if($1 & (BRIE_RELATION|BTREE_RELATION|EQREL_RELATION|RBTSET_RELATION|HASHSET_RELATION)) driver.error(@2, "btree/brie/eqrel/rbtset/hashset qualifier already set");
        $$ = $1 | BRIE_RELATION;
    }
  | qualifiers BTREE_QUALIFIER {
        if($1 & (BRIE_RELATION|BTREE_RELATION|EQREL_RELATION|RBTSET_RELATION|HASHSET_RELATION)) driver.error(@2, "btree/brie/eqrel/rbtset/hashset qualifier already set");
        $$ = $1 | BTREE_RELATION;
    }
  | qualifiers EQREL_QUALIFIER {
        if($1 & (BRIE_RELATION|BTREE_RELATION|EQREL_RELATION|RBTSET_RELATION|HASHSET_RELATION)) driver.error(@2, "btree/brie/eqrel/rbtset/hashset qualifier already set");
        $$ = $1 | EQREL_RELATION;
    }
  | qualifiers RBTSET_QUALIFIER {
        if($1 & (BRIE_RELATION|BTREE_RELATION|EQREL_RELATION|RBTSET_RELATION|HASHSET_RELATION)) driver.error(@2, "btree/brie/eqrel/rbtset/hashset qualifier already set");
        $$ = $1 | RBTSET_RELATION;
    }
  | qualifiers HASHSET_QUALIFIER {
        if($1 & (BRIE_RELATION|BTREE_RELATION|EQREL_RELATION|RBTSET_RELATION|HASHSET_RELATION)) driver.error(@2, "btree/brie/eqrel/rbtset/hashset qualifier already set");
        $$ = $1 | HASHSET_RELATION;
    }
  | %empty {
        $$ = 0;
    }

relation_head
  : DECL relation_list {
      $$.swap($2);
    }

relation_list
  : relation_body {
      $$.push_back($1);
    }
  | IDENT COMMA relation_list {
      $$.swap($3);
      auto tmp = $$.back()->clone();
      tmp->setName($1);
      tmp->setSrcLoc(@$);
      $$.push_back(tmp);
    }

relation_body
  : IDENT LPAREN attributes RPAREN qualifiers {
        $$ = $3;
        $$->setName($1);
        $$->setQualifier($5);
        $$->setSrcLoc(@$);
    }

non_empty_key_value_pairs
  : IDENT EQUALS STRING {
        $$ = new ast::IODirective();
        $$->addKVP($1, $3);
    }
  | key_value_pairs COMMA IDENT EQUALS STRING {
        $$ = $1;
        $$->addKVP($3, $5);
    }
  | IDENT EQUALS IDENT {
        $$ = new ast::IODirective();
        $$->addKVP($1, $3);
    }
  | key_value_pairs COMMA IDENT EQUALS IDENT {
        $$ = $1;
        $$->addKVP($3, $5);
    }
  | IDENT EQUALS TRUE {
        $$ = new ast::IODirective();
        $$->addKVP($1, "true");
    }
  | key_value_pairs COMMA IDENT EQUALS TRUE {
        $$ = $1;
        $$->addKVP($3, "true");
    }
 | IDENT EQUALS FALSE {
        $$ = new ast::IODirective();
        $$->addKVP($1, "false");
    }
 | key_value_pairs COMMA IDENT EQUALS FALSE {
        $$ = $1;
        $$->addKVP($3, "false");
    }

key_value_pairs
  : non_empty_key_value_pairs {
        $$ = $1;
    }
  | %empty {
        $$ = new ast::IODirective();
        $$->setSrcLoc(@$);
    }

iodirective_body
  : rel_id LPAREN key_value_pairs RPAREN {
        $$ = $3;
        $3->addName(*$1);
        $3->setSrcLoc(@1);
        delete $1;
    }
  | rel_id {
        $$ = new ast::IODirective();
        $$->setName(*$1);
        $$->setSrcLoc(@1);
        delete $1;
    }
  | rel_id COMMA iodirective_body {
        $$ = $3;
        $3->addName(*$1);
        $3->setSrcLoc(@1);
        delete $1;
    }

iodirective
  : INPUT_DECL iodirective_body {
        $$ = $2;
        $$->setAsInput();
        $$->setSrcLoc(@2);
    }
  | OUTPUT_DECL iodirective_body {
        $$ = $2;
        $$->setAsOutput();
        $$->setSrcLoc(@2);
    }
  | PRINTSIZE_DECL iodirective_body {
        $$ = $2;
        $$->setAsPrintSize();
        $$->setSrcLoc(@2);
    }

/* Atom */
arg
  : STRING {
        $$ = new ast::StringConstant(driver.getSymbolTable(), $1.c_str());
        $$->setSrcLoc(@$);
    }
  | UNDERSCORE {
        $$ = new ast::UnnamedVariable();
        $$->setSrcLoc(@$);
    }
  | DOLLAR {
        $$ = new ast::Counter();
        $$->setSrcLoc(@$);
    }
  | IDENT {
        $$ = new ast::Variable($1);
        $$->setSrcLoc(@$);
    }
  | NUMBER {
        $$ = new ast::NumberConstant($1);
        $$->setSrcLoc(@$);
    }
  | LPAREN arg RPAREN {
        $$ = $2;
    }
  | arg BW_OR arg {
        $$ = new ast::BinaryFunctor(BinaryOp::BOR, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        $$->setSrcLoc(@$);
    }
  | arg BW_XOR arg {
        $$ = new ast::BinaryFunctor(BinaryOp::BXOR, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        $$->setSrcLoc(@$);
    }
  | arg BW_AND arg {
        $$ = new ast::BinaryFunctor(BinaryOp::BAND, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        $$->setSrcLoc(@$);
    }
  | arg L_OR arg {
        $$ = new ast::BinaryFunctor(BinaryOp::LOR, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        $$->setSrcLoc(@$);
    }
  | arg L_AND arg {
        $$ = new ast::BinaryFunctor(BinaryOp::LAND, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        $$->setSrcLoc(@$);
    }
  | arg PLUS arg {
        $$ = new ast::BinaryFunctor(BinaryOp::ADD, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        $$->setSrcLoc(@$);
    }
  | arg MINUS arg {
        $$ = new ast::BinaryFunctor(BinaryOp::SUB, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        $$->setSrcLoc(@$);
    }
  | arg STAR arg {
        $$ = new ast::BinaryFunctor(BinaryOp::MUL, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        $$->setSrcLoc(@$);
    }
  | arg SLASH arg {
        $$ = new ast::BinaryFunctor(BinaryOp::DIV, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        $$->setSrcLoc(@$);
    }
  | arg PERCENT arg {
        $$ = new ast::BinaryFunctor(BinaryOp::MOD, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        $$->setSrcLoc(@$);
    }
  | arg CARET arg {
        $$ = new ast::BinaryFunctor(BinaryOp::EXP, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        $$->setSrcLoc(@$);
    }
  | MAX LPAREN arg COMMA arg RPAREN {
        $$ = new ast::BinaryFunctor(BinaryOp::MAX, std::unique_ptr<ast::Argument>($3), std::unique_ptr<ast::Argument>($5));
        $$->setSrcLoc(@$);
    }
  | MIN LPAREN arg COMMA arg RPAREN {
        $$ = new ast::BinaryFunctor(BinaryOp::MIN, std::unique_ptr<ast::Argument>($3), std::unique_ptr<ast::Argument>($5));
        $$->setSrcLoc(@$);
    }
  | CAT LPAREN arg COMMA arg RPAREN {
        $$ = new ast::BinaryFunctor(BinaryOp::CAT, std::unique_ptr<ast::Argument>($3), std::unique_ptr<ast::Argument>($5));
        $$->setSrcLoc(@$);
    }
  | ORD LPAREN arg RPAREN {
        $$ = new ast::UnaryFunctor(UnaryOp::ORD, std::unique_ptr<ast::Argument>($3));
        $$->setSrcLoc(@$);
    }
  | STRLEN LPAREN arg RPAREN {
        $$ = new ast::UnaryFunctor(UnaryOp::STRLEN, std::unique_ptr<ast::Argument>($3));
        $$->setSrcLoc(@$);
    }
  | SUBSTR LPAREN arg COMMA arg COMMA arg RPAREN {
        $$ = new ast::TernaryFunctor(TernaryOp::SUBSTR,
                std::unique_ptr<ast::Argument>($3),
                std::unique_ptr<ast::Argument>($5),
                std::unique_ptr<ast::Argument>($7));
        $$->setSrcLoc(@$);
    }
  | arg AS IDENT {
        $$ = new ast::TypeCast(std::unique_ptr<ast::Argument>($1), $3);
        $$->setSrcLoc(@$);
    }
  | MINUS arg %prec NEG {
        std::unique_ptr<ast::Argument> arg;
        if (const ast::NumberConstant* original = dynamic_cast<const ast::NumberConstant*>($2)) {
            $$ = new ast::NumberConstant(-1*original->getIndex());
            $$->setSrcLoc($2->getSrcLoc());
            delete $2;
        } else {
            $$ = new ast::UnaryFunctor(UnaryOp::NEG, std::unique_ptr<ast::Argument>($2));
            $$->setSrcLoc(@$);
        }
    }
  | BW_NOT arg {
        $$ = new ast::UnaryFunctor(UnaryOp::BNOT, std::unique_ptr<ast::Argument>($2));
        $$->setSrcLoc(@$);
    }
  | L_NOT arg {
        $$ = new ast::UnaryFunctor(UnaryOp::LNOT, std::unique_ptr<ast::Argument>($2));
        $$->setSrcLoc(@$);
    }
  | LBRACKET RBRACKET {
        $$ = new ast::RecordInit();
        $$->setSrcLoc(@$);
    }
  | LBRACKET recordlist RBRACKET {
        $$ = $2;
        $$->setSrcLoc(@$);
    }
  | NIL {
        $$ = new ast::NullConstant();
        $$->setSrcLoc(@$);
    }
  | COUNT COLON atom {
        auto res = new ast::Aggregator(ast::Aggregator::count);
        res->addBodyLiteral(std::unique_ptr<ast::Literal>($3));
        $$ = res;
        $$->setSrcLoc(@$);
    }
  | COUNT COLON LBRACE body RBRACE {
        auto res = new ast::Aggregator(ast::Aggregator::count);
        auto bodies = $4->toClauseBodies();
        if (bodies.size() != 1) {
            std::cerr << "ERROR: currently not supporting non-conjunctive aggregation clauses!";
            exit(1);
        }
        for(const auto& cur : bodies[0]->getBodyLiterals()) {
            res->addBodyLiteral(std::unique_ptr<ast::Literal>(cur->clone()));
        }
        delete bodies[0];
        delete $4;
        $$ = res;
        $$->setSrcLoc(@$);
    }
  | SUM arg COLON atom {
        auto res = new ast::Aggregator(ast::Aggregator::sum);
        res->setTargetExpression(std::unique_ptr<ast::Argument>($2));
        res->addBodyLiteral(std::unique_ptr<ast::Literal>($4));
        $$ = res;
        $$->setSrcLoc(@$);
    }
  | SUM arg COLON LBRACE body RBRACE {
        auto res = new ast::Aggregator(ast::Aggregator::sum);
        res->setTargetExpression(std::unique_ptr<ast::Argument>($2));
        auto bodies = $5->toClauseBodies();
        if (bodies.size() != 1) {
            std::cerr << "ERROR: currently not supporting non-conjunctive aggregation clauses!";
            exit(1);
        }
        for(const auto& cur : bodies[0]->getBodyLiterals()) {
	    res->addBodyLiteral(std::unique_ptr<ast::Literal>(cur->clone()));
        }
        delete bodies[0];
        delete $5;
        $$ = res;
        $$->setSrcLoc(@$);
    }
  | MIN arg COLON atom {
        auto res = new ast::Aggregator(ast::Aggregator::min);
        res->setTargetExpression(std::unique_ptr<ast::Argument>($2));
        res->addBodyLiteral(std::unique_ptr<ast::Literal>($4));
        $$ = res;
        $$->setSrcLoc(@$);
    }
  | MIN arg COLON LBRACE body RBRACE {
        auto res = new ast::Aggregator(ast::Aggregator::min);
        res->setTargetExpression(std::unique_ptr<ast::Argument>($2));
        auto bodies = $5->toClauseBodies();
        if (bodies.size() != 1) {
            std::cerr << "ERROR: currently not supporting non-conjunctive aggregation clauses!";
            exit(1);
        }
        for(const auto& cur : bodies[0]->getBodyLiterals()) {
            res->addBodyLiteral(std::unique_ptr<ast::Literal>(cur->clone()));
        }
        delete bodies[0];
        delete $5;
        $$ = res;
        $$->setSrcLoc(@$);
    }
  | MAX arg COLON atom {
        auto res = new ast::Aggregator(ast::Aggregator::max);
        res->setTargetExpression(std::unique_ptr<ast::Argument>($2));
        res->addBodyLiteral(std::unique_ptr<ast::Literal>($4));
        $$ = res;
        $$->setSrcLoc(@$);
    }
  | MAX arg COLON LBRACE body RBRACE {
        auto res = new ast::Aggregator(ast::Aggregator::max);
        res->setTargetExpression(std::unique_ptr<ast::Argument>($2));
        auto bodies = $5->toClauseBodies();
        if (bodies.size() != 1) {
            std::cerr << "ERROR: currently not supporting non-conjunctive aggregation clauses!";
            exit(1);
        }
        for(const auto& cur : bodies[0]->getBodyLiterals()) {
            res->addBodyLiteral(std::unique_ptr<ast::Literal>(cur->clone()));
        }
        delete bodies[0];
        delete $5;
        $$ = res;
        $$->setSrcLoc(@$);
    }
  | RESERVED LPAREN arg RPAREN {
        std::cerr << "ERROR: '" << $1 << "' is a keyword reserved for future implementation!" << std::endl;
        exit(1);
    }

recordlist
  : arg {
        $$ = new ast::RecordInit();
        $$->add(std::unique_ptr<ast::Argument>($1));
    }
  | recordlist COMMA arg {
        $$ = $1;
        $$->add(std::unique_ptr<ast::Argument>($3));
    }

non_empty_arg_list
  : arg {
        $$ = new ast::Atom();
        $$->addArgument(std::unique_ptr<ast::Argument>($1));
    }
  | arg_list COMMA arg {
        $$ = $1;
        $$->addArgument(std::unique_ptr<ast::Argument>($3));
    }

arg_list
  : non_empty_arg_list {
        $$ = $1;
    }
  | %empty {
        $$ = new ast::Atom();
    }

atom
  : rel_id LPAREN arg_list RPAREN {
        $$ = $3;
        $3->setName(*$1);
        delete $1;
        $$->setSrcLoc(@$);
    }

/* Literal */
literal
  : arg RELOP arg {
        auto* res = new ast::BinaryConstraint($2, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        res->setSrcLoc(@$);
        $$ = new ast::RuleBody(ast::RuleBody::constraint(res));
    }
  | arg LT arg {
        auto* res = new ast::BinaryConstraint(BinaryConstraintOp::LT, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        res->setSrcLoc(@$);
        $$ = new ast::RuleBody(ast::RuleBody::constraint(res));
    }
  | arg GT arg {
        auto* res = new ast::BinaryConstraint(BinaryConstraintOp::GT, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        res->setSrcLoc(@$);
        $$ = new ast::RuleBody(ast::RuleBody::constraint(res));
    }
  | arg EQUALS arg {
        auto* res = new ast::BinaryConstraint(BinaryConstraintOp::EQ, std::unique_ptr<ast::Argument>($1), std::unique_ptr<ast::Argument>($3));
        res->setSrcLoc(@$);
        $$ = new ast::RuleBody(ast::RuleBody::constraint(res));
    }
  | atom {
        $1->setSrcLoc(@$);
        $$ = new ast::RuleBody(ast::RuleBody::atom($1));
    }
  | TMATCH LPAREN arg COMMA arg RPAREN {
        auto* res = new ast::BinaryConstraint(BinaryConstraintOp::MATCH, std::unique_ptr<ast::Argument>($3), std::unique_ptr<ast::Argument>($5));
        res->setSrcLoc(@$);
        $$ = new ast::RuleBody(ast::RuleBody::constraint(res));
    }
  | TCONTAINS LPAREN arg COMMA arg RPAREN {
        auto* res = new ast::BinaryConstraint(BinaryConstraintOp::CONTAINS, std::unique_ptr<ast::Argument>($3), std::unique_ptr<ast::Argument>($5));
        res->setSrcLoc(@$);
        $$ = new ast::RuleBody(ast::RuleBody::constraint(res));
    }
  | TRUE {
        auto* res = new ast::BooleanConstraint(true);
        res->setSrcLoc(@$);
        $$ = new ast::RuleBody(ast::RuleBody::constraint(res));
    }
  | FALSE {
        auto* res = new ast::BooleanConstraint(false);
        res->setSrcLoc(@$);
        $$ = new ast::RuleBody(ast::RuleBody::constraint(res));
    }

/* Fact */
fact
  : atom DOT {
        $$ = new ast::Clause();
        $$->setHead(std::unique_ptr<ast::Atom>($1));
        $$->setSrcLoc(@$);
    }

/* Head */
head
  : atom {
        $$.push_back($1);
    }
  | head COMMA atom {
        $$.swap($1);
        $$.push_back($3);
    }

/* Term */
term
  : literal {
        $$ = $1;
    }
  | EXCLAMATION term {
        $$ = $2;
        $$->negate();
    }
  | LPAREN disjunction RPAREN {
        $$ = $2;
    }

/* Conjunction */
conjunction
  : term {
        $$ = $1;
    }
  | conjunction COMMA term {
        $$ = $1;
        $$->conjunct(std::move(*$3));
        delete $3;
    }

/* Disjunction */
disjunction
  : conjunction {
        $$ = $1;
    }
  | disjunction SEMICOLON conjunction {
        $$ = $1;
        $$->disjunct(std::move(*$3));
        delete $3;
    }

/* Body */
body
  : disjunction                        { $$ = $1;
    }

/* execution order list */
exec_order_list
  : NUMBER {
        $$ = new ast::ExecutionOrder();
        $$->appendAtomIndex($1);
    }
  | exec_order_list COMMA NUMBER {
        $$ = $1;
        $$->appendAtomIndex($3);
    }

/* execution order */
exec_order
  : LPAREN exec_order_list RPAREN {
        $$ = $2;
        $$->setSrcLoc(@$);
    }

/* execution plan list */
exec_plan_list
  : NUMBER COLON exec_order {
        $$ = new ast::ExecutionPlan();
        $$->setOrderFor($1, std::unique_ptr<ast::ExecutionOrder>($3));
    }
  | exec_plan_list COMMA NUMBER COLON exec_order {
        $$ = $1;
        $$->setOrderFor($3, std::unique_ptr<ast::ExecutionOrder>($5));
    }

/* execution plan */
exec_plan
  : PLAN exec_plan_list {
        $$ = $2;
        $$->setSrcLoc(@$);
    }

/* Rule Definition */
rule_def
  : head IF body DOT {
        auto bodies = $3->toClauseBodies();
        for(const auto& head : $1) {
            for(ast::Clause* body : bodies) {
                ast::Clause* cur = body->clone();
                cur->setHead(std::unique_ptr<ast::Atom>(head->clone()));
                cur->setSrcLoc(@$);
                cur->setGenerated($1.size() != 1 || bodies.size() != 1);
                $$.push_back(cur);
            }
        }
        for(auto& head : $1) {
            delete head;
        }
        for(ast::Clause* body : bodies) {
            delete body;
        }
        delete $3;
    }

/* Rule */
rule
  : rule_def {
        $$ = $1;
    }
  | rule STRICT {
        $$ = $1;
        for(const auto& cur : $$) cur->setFixedExecutionPlan();
    }
  | rule exec_plan {
        $$ = $1;
        for(const auto& cur : $$) cur->setExecutionPlan(std::unique_ptr<ast::ExecutionPlan>($2->clone()));
    }

/* Type Parameters */

type_param_list
  : IDENT {
        $$.push_back($1);
    }
  | type_param_list COMMA type_id {
        $$ = $1;
        $$.push_back(*$3);
        delete $3;
    }

type_params
  : %empty {
    }
  | LT type_param_list GT {
        $$ = $2;
    }

/* Component type */

comp_type
  : IDENT type_params {
        $$ = new ast::ComponentType($1,$2);
    }

/* Component */

component_head
  : COMPONENT comp_type {
        $$ = new ast::Component();
        $$->setComponentType(std::unique_ptr<ast::ComponentType>($2));
    }
  | component_head COLON comp_type {
        $$ = $1;
        $$->addBaseComponent(std::unique_ptr<ast::ComponentType>($3));
    }
  | component_head COMMA comp_type {
        $$ = $1;
        $$->addBaseComponent(std::unique_ptr<ast::ComponentType>($3));
    }

component_body
  : component_body type {
        $$ = $1;
        $$->addType(std::unique_ptr<ast::Type>($2));
    }
  | component_body relation_head {
        $$ = $1;
        for(const auto& cur : $2) $$->addRelation(std::unique_ptr<ast::Relation>(cur));
    }
  | component_body iodirective {
        $$ = $1;
        $$->addIODirective(std::unique_ptr<ast::IODirective>($2));
    }
  | component_body fact {
        $$ = $1;
        $$->addClause(std::unique_ptr<ast::Clause>($2));
    }
  | component_body rule {
        $$ = $1;
        for(const auto& cur : $2) {
            $$->addClause(std::unique_ptr<ast::Clause>(cur));
        }
    }
  | component_body comp_override {
        $$ = $1;
        $$->addOverride($2);
    }
  | component_body component {
        $$ = $1;
        $$->addComponent(std::unique_ptr<ast::Component>($2));
    }
  | component_body comp_init {
        $$ = $1;
        $$->addInstantiation(std::unique_ptr<ast::ComponentInit>($2));
    }
  | %empty {
        $$ = new ast::Component();
    }

component
  : component_head LBRACE component_body RBRACE {
        $$ = $3;
        $$->setComponentType(std::unique_ptr<ast::ComponentType>($1->getComponentType()->clone()));
        $$->copyBaseComponents($1);
        delete $1;
        $$->setSrcLoc(@$);
    }

/* Component Instantition */
comp_init
  : INSTANTIATE IDENT EQUALS comp_type {
        $$ = new ast::ComponentInit();
        $$->setInstanceName($2);
        $$->setComponentType(std::unique_ptr<ast::ComponentType>($4));
        $$->setSrcLoc(@$);
    }

/* Override rules of a relation */
comp_override
  : OVERRIDE IDENT {
        $$ = $2;
}

%%
void yy::parser::error(const location_type &l, const std::string &m) {
   driver.error(l, m);
}
