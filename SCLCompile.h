#ifndef SCLCOMPILE_INCLUDED
#define SCLCOMPILE_INCLUDED
#include "SCLData.h"
#include <unordered_map>

//I have to apply certain restrictions for commands...
enum TOKEN_KIND {
	TOKEN_IGNORE,
	TOKEN_KEYWORD, //Words like PROC, TEXINIT, TSET
	TOKEN_COMMAND, //Words for commans, like SET, CALL, ADD, etc
	TOKEN_IDENTIFIER, //the name of a procedure, label or define
	TOKEN_NUMBER, // Just numbers... Like 5,3,2,1
	TOKEN_STRING, // Words between quotes like "TEX_YUUKA"
	TOKEN_COMMA, // the comma for separating things
	TOKEN_DOTS, // the comma for separating things

	//Extra tokens to make some stuff faster to process, i think
	TOKEN_PROC, //This holds the name of the procedure
	TOKEN_ENDPROC, //This tells to stop reading a subroutine
	TOKEN_LABEL, //the name of a label
	TOKEN_INCOUNT, //number of instructions to read
	TOKEN_CONST, //has the name of a const

	//TOKEN_COMMENT, // ; as comment, like in a real assembler (Never touched one lol)
};

enum KEYWORD_KIND {
	KEY_PROC,
	KEY_TEXINITPROC,
	KEY_ENEMYPROC,
	KEY_ATKPROC,
	KEY_EXANMPROC,
	KEY_SETPROC,
	KEY_LOADTEXPROC,
	KEY_ENDPROC,
	KEY_CONST,
};

//For the tokenizer...
struct Token {
	TOKEN_KIND kind;
	size_t line;
	std::string pStr;
	int number = 0;
};

struct OutputData {
	void* pData;
	size_t size;
};

struct ProcDataEx2 {
	address ads;
	std::vector<SCLInstructionData> cmd_data;
	std::map<const std::string, address> label_data;
};

static std::map<const std::string, KEYWORD_KIND> g_Keystr2Tok = {
	{"PROC", KEY_PROC},
	{"TEXINITPROC", KEY_TEXINITPROC},
	{"ENEMY", KEY_ENEMYPROC},
	{"TSET", KEY_ATKPROC},
	{"EXANM", KEY_EXANMPROC},
	{"SETPROC", KEY_SETPROC},
	{"LOADTEXPROC", KEY_LOADTEXPROC},
	{"ENDPROC", KEY_ENDPROC},
	{"const", KEY_CONST},
};

static std::map<const std::string, Token> g_DefMap;

typedef std::map<std::string, ProcDataEx2> address_map_ex;

//Main function
bool CompileSCL(const char* Name, const char* Header, const char* OutputName);

//Stages of compiling:
 
//Tokenize everything
bool TokenizeInput(
	char* pInputData, 
	size_t size,
	std::vector<Token>* pToken
); 

//Verify the syntax and try to make an easier to understand array to process data
bool VerifySyntax(
	const std::vector<Token>& tokens,
	std::vector<Token>* pProcessedData
);

//Calculate addresses and parse command data
bool CalculateAddresses(
	const std::vector<Token>& tokens,
	address_map_ex* pProcData
); 

//Fill the corresponding addresses
bool PopulateAddresses(
	address_map_ex& pProcData
); 

//Process header data and set addresses
bool ProcessHeader(
	const char* json, 
	address_map_ex* pProcData, 
	SCLHeader* pHeader
); 

//Finally, join both data
OutputData JoinData(
	const SCLHeader& header_data,
	const std::vector<SCLInstructionData>& instruction_data
);


#endif