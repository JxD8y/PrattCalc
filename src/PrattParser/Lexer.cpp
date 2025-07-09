#include "Lexer.h"
#include <iostream>
#include <algorithm>
using namespace std;

Lexer::Lexer(string expression) {
	if (expression.length() < 3)
		throw std::runtime_error("Cannot tokenize and parse this expr");

	for (int i = 0; i < expression.length(); i++) {
		char c = expression[i];
		if (c == ' ')
			continue;

		if (c == '+')
			tokens.push_back(Token(TYPE::Op, OpType::Add));
		else if (c == '-')
			tokens.push_back(Token(TYPE::Op, OpType::Sub));
		else if (c == '*')
			tokens.push_back(Token(TYPE::Op, OpType::Mul));
		else if (c == '/')
			tokens.push_back(Token(TYPE::Op, OpType::Div));
		else if (c == '(')
			tokens.push_back(Token(TYPE::Op, OpType::LPRN));
		else if (c == ')')
			tokens.push_back(Token(TYPE::Op, OpType::RPRN));
		else {
			string sub;
			char s = c;
			while (i < expression.length() && s != '+' && s != '\0' && s != ' ' && s != '-' && s != '*' && s != '/' && s != '(' && s != ')') {
				sub.push_back(s);
				s = expression[++i];
			}
			i--;
			tokens.push_back(Token(TYPE::Atom, std::stod(sub)));

			if (i == expression.length())
				tokens.push_back(Token(TYPE::Op, OpType::OPEND));
		}
	}
	tokens.push_back(Token(TYPE::Op, OpType::OPEND));
	std::reverse(this->tokens.begin(), this->tokens.end());
}
//Pop the token from stack storage
Token Lexer::Next() {
	auto tk = this->tokens.back();
	this->tokens.pop_back();
	return tk;
}
//Look at the next Token
Token Lexer::Peek() {
	auto tk = this->tokens.back();
	return tk;
}