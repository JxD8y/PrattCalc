#include <string>
#include <vector>
#include "Token.h"
#include <stdexcept>
#include <algorithm>

using namespace std;


class Lexer {
public:
	Lexer(string expression);
	Lexer() : Lexer("1 + 1") {}
	Token Next();
	Token Peek();
private:
	vector<Token> tokens;
};