#include "Lexer.h"
#include <iostream>
Lexer::Lexer(string expression) {
	if (expression.length() < 3)
		throw std::runtime_error("Cannot tokenize and parse this expr");
	
	for (int i = 0; i < expression.length(); i++) {
		char c = expression[i];
		if (c == ' ')
			continue;

		if (i == expression.length() - 1)
			tokens.push_back(Token(TYPE::Op,OpType::OPEND));

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
			tokens.push_back(Token(TYPE::Atom, std::stod(sub)));

			if (i == expression.length())
				tokens.push_back(Token(TYPE::Op, OpType::OPEND));//could be better
		}
	}
}