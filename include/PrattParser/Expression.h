#include <vector>
#include "Token.h"
#pragma once
using namespace std;

class Expression {
public:
	Expression(Expression* lhs,Expression* rhs,OpType op);
	Expression(double value);
	Expression() {}
	Expression* pLhs = NULL;
	Expression* pRhs = NULL;

	void SetType(OpType);
	OpType GetType();
	void SetValue(double value);
	double GetValue();
private:
	OpType type = OpType::OPEND;
	double value = 0;
};