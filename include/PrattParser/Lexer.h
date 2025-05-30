#include <string>
#include <vector>
#include "Token.h"
#include <stdexcept>
#include <algorithm>

using namespace std;


class Lexer {
public:
	Lexer(string expression);
	Token Next();
	Token Peek();
private:
	vector<Token> tokens;
};