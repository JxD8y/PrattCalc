#pragma once
#include <Token.h>
#include <string>
#include <Expression.h>
#include <Lexer.h>

using namespace std;

class PParser {
public:
	PParser(string expr);
	Expression* Parse();
private:
	Expression* parse_expr(int min_pw);
	Lexer lx;
};
int getBindingPW(OpType op);
std::string OpTypeToString(OpType type);
double SolveExpression(Expression* exp);