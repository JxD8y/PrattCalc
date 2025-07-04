#include "Expression.h"

Expression::Expression(Token& lhs, Token& rhs, OpType op) {
	this->lhs_tk = &lhs;
	this->rhs_tk = &rhs;
	this->type = op;
}
Expression::Expression(Expression& lhs, Token& rhs, OpType op) {
	this->lhs_expr = &lhs;
	this->rhs_tk = &rhs;
	this->type = op;
}
Expression::Expression(Token& lhs, Expression& rhs, OpType op) {
	this->lhs_tk = &lhs;
	this->rhs_expr = &rhs;
	this->type = op;
}
Expression::Expression(Expression& lhs, Expression& rhs, OpType op) {
	this->lhs_expr = &lhs;
	this->rhs_expr = &rhs;
	this->type = op;
}