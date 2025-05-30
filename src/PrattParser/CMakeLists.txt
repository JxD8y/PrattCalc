add_library(PrattLib STATIC 
			Lexer.cpp)

target_include_directories(PrattLib PUBLIC ../../include/PrattParser)