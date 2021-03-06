//
// Created by safoex on 05.09.19.
//

#ifndef ABTM2_JSSYMBOLSFINDER_H
#define ABTM2_JSSYMBOLSFINDER_H

#include "esprima.h"

namespace abtm_js {


#define DUMB(Node, props) \
void visit(esprima::Node *node) { \
     visitChildren(node); \
    }

#define VisitLiteral(Node, props)\
void visit(esprima::Node *node) {\
    numOfMemberExpr--; \
    if(numOfMemberExpr == 0) identifierCanBeSet = 1;\
    visitChildren(node);\
}

#define VisitMemberExpression(Node, props)\
void visit(esprima::Node *node) {\
    numOfMemberExpr++;\
    visitChildren(node);\
}

#define VisitIdentifier(Node, vars)\
void visit(esprima::Node *node) {\
    if(identifierCanBeSet) {\
        vars.insert(node->name);\
        if(numOfMemberExpr != 0) identifierCanBeSet = 0;\
    } else { numOfMemberExpr--; if(numOfMemberExpr == 0) identifierCanBeSet = 1;}\
    visitChildren(node);\
}

//    std::cout << node->name << '\t' << numOfMemberExpr << '\t' << identifierCanBeSet << std::endl;\

    class FindVarsVisitor : public esprima::Visitor {
        std::unordered_set<std::string> &vars;
        int numOfMemberExpr, identifierCanBeSet;
    public:
        FindVarsVisitor(std::unordered_set<std::string> &vars) : vars(vars), numOfMemberExpr(0),
                                                                 identifierCanBeSet(1) {};

        DUMB(Program, NONE)

        VisitIdentifier(Identifier, vars)

        DUMB(BlockStatement, NONE)

        DUMB(EmptyStatement, NONE)

        DUMB(ExpressionStatement, NONE)

        DUMB(IfStatement, NONE)

        DUMB(LabeledStatement, NONE)

        DUMB(BreakStatement, NONE)

        DUMB(ContinueStatement, NONE)

        DUMB(WithStatement, NONE)

        DUMB(SwitchCase, NONE)

        DUMB(SwitchStatement, NONE)

        DUMB(ReturnStatement, NONE)

        DUMB(ThrowStatement, NONE)

        DUMB(CatchClause, NONE)

        DUMB(TryStatement, NONE)

        DUMB(WhileStatement, NONE)

        DUMB(DoWhileStatement, NONE)

        DUMB(ForStatement, NONE)

        DUMB(ForInStatement, NONE)

        DUMB(DebuggerStatement, NONE)

        DUMB(FunctionDeclaration, NONE)

        DUMB(VariableDeclarator, NONE)

        DUMB(VariableDeclaration, PROP(kind))

        DUMB(ThisExpression, NONE)

        DUMB(ArrayExpression, NONE)

        DUMB(Property, PROP(kind))

        DUMB(ObjectExpression, NONE)

        DUMB(FunctionExpression, NONE)

        DUMB(SequenceExpression, NONE)

        DUMB(UnaryExpression, (PROP(operator_), PROP(prefix)))

        DUMB(BinaryExpression, PROP(operator_))

        DUMB(AssignmentExpression, PROP(operator_))

        DUMB(UpdateExpression, PROP(operator_))

        DUMB(LogicalExpression, PROP(operator_))

        DUMB(ConditionalExpression, NONE)

        DUMB(NewExpression, NONE)

        DUMB(CallExpression, NONE)

        VisitMemberExpression(MemberExpression, PROP(computed))

        DUMB(NullLiteral, NONE)

        VisitLiteral(RegExpLiteral, (PROP(pattern), PROP(flags)))

        VisitLiteral(StringLiteral, PROP(value))

        VisitLiteral(NumericLiteral, PROP(value))

        VisitLiteral(BooleanLiteral, PROP(value))
    };

    std::unordered_set<std::string> get_used_vars_from_expr(std::string const &expr) {
        std::unordered_set<std::string> result;
        try {
            esprima::Pool pool;
            esprima::Program *program = esprima::parse(pool, expr);
            FindVarsVisitor fv(result);
            program->accept(&fv);
        }
        catch (const esprima::ParseError &error) {
            std::cout << "load error: " << error.description << std::endl;
            throw std::runtime_error("Error while finding used vars in " + expr);
        }
        return result;
    }

}

#endif //ABTM2_JSSYMBOLSFINDER_H
