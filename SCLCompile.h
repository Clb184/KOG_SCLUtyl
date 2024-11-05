#ifndef SCLCOMPILE_INCLUDED
#define SCLCOMPILE_INCLUDED
#include "SCLData.h"

enum TOKEN_KIND {
	TOKEN_KEYWORD, //Words like PROC, TEXINIT, TSET
	TOKEN_COMMAND, //Words for commans, like SET, CALL, ADD, etc
	TOKEN_IDENTIFIER, //the name of a procedure, label or define
	TOKEN_NUMBER, // Just numbers... Like 5,3,2,1
	TOKEN_STRING, // Words between quotes like "TEX_YUUKA"
	TOKEN_COMMA, // the comma
	TOKEN_COMMENT, // ; as comment, like in a real assembler (Never touched one lol)
};

struct Token {
	TOKEN_KIND kind;
	union {
		std::string pStr;
		int number;
		std::string pKeyword;
		std::string pIdentifier;
	};
};

#endif