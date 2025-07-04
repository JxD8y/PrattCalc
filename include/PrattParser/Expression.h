#include <vector>
#include "Token.h"
#pragma once
using namespace std;

class Expression {
public:
	Expression(Token& lhs,Token& rhs,OpType op);
	Expression(Expression& lhs,Token& rhs,OpType op);
	Expression(Token& lhs,Expression& rhs,OpType op);
	Expression(Expression& lhs,Expression& rhs,OpType op);
	Token* lhs_tk = NULL;
	Token* rhs_tk = NULL;
	Expression* lhs_expr = NULL;
	Expression* rhs_expr = NULL;
	OpType type = OpType::OPEND;
};