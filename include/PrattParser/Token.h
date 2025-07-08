#include <string>
#pragma once

enum class TYPE {
	Atom,
	Op
};
enum class OpType {
	Add,
	Sub,
	Mul,
	Div,
	LPRN,
	RPRN,
	OPEND
};

struct Token {
	union {
		double value;
		OpType opType;
	};
	TYPE type;
	Token() {
		value = 0;
		type = TYPE::Atom;
	}
	Token(TYPE _type, double _value) :value(_value), type(_type){}
	Token(TYPE _type, OpType _opType) :opType(_opType), type(_type){}
};