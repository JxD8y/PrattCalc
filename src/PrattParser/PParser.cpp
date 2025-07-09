#include <PParser.h>

int getBindingPW(OpType op) {
	switch (op) {
	case OpType::Add:
		return 1;
	case OpType::Sub:
		return 1;
	case OpType::Mul:
		return 2;
	case OpType::Div:
		return 2;

	case OpType::LPRN: 
		return 2;
	case OpType::RPRN:
		return 2;

	}
}

PParser::PParser(string expr) {
	this->lx = Lexer(expr);
}
Expression* PParser::Parse() {
	return this->parse_expr(0);
}
Expression* PParser::parse_expr(int min_pw) {
	Token left = this->lx.Next();
	if (left.type == TYPE::Op)
		throw std::runtime_error("Infix operator at wrong pos.");
	Expression* l = new Expression(left.value);

	while (1) {
		Token op = this->lx.Peek();
		if (op.type != TYPE::Op)
			throw std::runtime_error("no Infix operator at pos.");

		if (op.opType == OpType::OPEND)
			break;

		int pw = getBindingPW(op.opType);
		if (pw < min_pw)
			break;

		this->lx.Next();
		Expression* r = this->parse_expr(pw);
		l = new Expression(l, r, op.opType);
	}
	return l;
}
std::string OpTypeToString(OpType type) {
	switch (type) {
	case OpType::Add:
		return "+";
	case OpType::Sub:
		return "-";
	case OpType::Mul:
		return "*";
	case OpType::Div:
		return "/";
	default:
		return "???";
	}
}
double SolveExpression(Expression* exp) {
	if (exp == NULL)
		return 0;
	double r = 0, l = 0;
	if (exp->pLhs != NULL)
		l = SolveExpression(exp->pLhs);
	if (exp->pRhs != NULL)
		r = SolveExpression(exp->pRhs);
	if (exp->GetType() == OpType::OPEND)
		return exp->GetValue();
	else {
		switch (exp->GetType()) {
			case OpType::Add:
				return l + r;
			case OpType::Sub:
				return l - r;
			case OpType::Mul:
				return l * r;
			case OpType::Div:
				return l / r;
			default:
				return 0;
		}
	}
}