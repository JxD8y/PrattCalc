#include "Expression.h"

Expression::Expression(double value) {
	this->SetValue(value);
}
Expression::Expression(Expression* lhs, Expression* rhs, OpType op) {
	this->pLhs = lhs;
	this->pRhs = rhs;
}

void Expression::SetType(OpType type) {
	this->type = type;
	this->value = 0;
}
OpType Expression::GetType() {
	return this->type;
}

void Expression::SetValue(double value) {
	this->value = value;
	this->type = OpType::OPEND;
}
double Expression::GetValue() {
	return this->value;
}