add_library(PrattLib STATIC 
			Lexer.cpp Expression.cpp "PParser.cpp")

target_include_directories(PrattLib PUBLIC ../../include/PrattParser)