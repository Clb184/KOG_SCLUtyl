#ifndef SCLCOMPILE_INCLUDED
#define SCLCOMPILE_INCLUDED
#include "SCLData.h"

//I have to apply certain restrictions for commands...
enum TOKEN_KIND {
	TOKEN_KEYWORD, //Words like PROC, TEXINIT, TSET
	TOKEN_COMMAND, //Words for commans, like SET, CALL, ADD, etc
	TOKEN_IDENTIFIER, //the name of a procedure, label or define
	TOKEN_NUMBER, // Just numbers... Like 5,3,2,1
	TOKEN_STRING, // Words between quotes like "TEX_YUUKA"
	TOKEN_COMMA, // the comma
	TOKEN_COMMENT, // ; as comment, like in a real assembler (Never touched one lol)
};

//For the tokenizer...
struct Token {
	TOKEN_KIND kind;
	union {
		std::string pStr;
		int number;
		std::string pKeyword;
		std::string pIdentifier;
	};
};

typedef std::map<std::string, ProcDataEx> address_map_ex;

//Main function
bool CompileSCL(const char* Name, const char* Header, const char* OutputName);

//Stages of compiling
std::vector<Token> TokenizeInput(char* pInputData); //Tokenize everything
std::vector<Token> CalculateAddresses(const std::vector<Token>& tokens,address_map_ex* pProcData, address_map_ex* pLabelData); //Calculate addresses and return only command data
std::vector<SCLInstructionData> ProcessTokens(const std::vector<Token>& tokens,address_map_ex* pProcData, address_map_ex* pLabelData); //Copy all data in the corresponding buffer
SCLHeader ProcessHeader(char* pData, address_map_ex* pProcData); //Process header data and set addresses
void* JoinData(const SCLHeader& header_data, const std::vector<SCLInstructionData>& instruction_data); //Finally, join both data


#endif