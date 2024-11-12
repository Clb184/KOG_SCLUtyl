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
	KEY_INCLUDE,
};

//For the tokenizer...
struct Token {
	TOKEN_KIND kind;
	size_t line;
	std::string pStr;
	int number = 0;
	size_t advance;
	const char* source;
};

struct OutputData {
	void* pData;
	size_t size;
};

struct SCLInstructionDataEx : public SCLInstructionData {
	size_t line = -1;
	const char* source = "";
};

struct ProcDataEx2 {
	address ads;
	std::vector<SCLInstructionDataEx> cmd_data;
	std::map<const std::string, address> label_data;
};

struct SourceInfo {
	const char* pWhereSource;
	const char* pWhatSource;
	size_t line;
};

static std::map<const std::string, KEYWORD_KIND> g_Keystr2Tok = {
	//Procedure types
	{"PROC", KEY_PROC},
	{"TEXINITPROC", KEY_TEXINITPROC},
	{"ENEMY", KEY_ENEMYPROC},
	{"TSET", KEY_ATKPROC},
	{"EXANM", KEY_EXANMPROC},
	{"SETPROC", KEY_SETPROC},
	{"LOADTEXPROC", KEY_LOADTEXPROC},

	//Other Keywords
	{"ENDPROC", KEY_ENDPROC},
	{"const", KEY_CONST},
	{"include", KEY_INCLUDE}
};

typedef std::vector<SCL_INSTRUCTION> valid_instruction_set;
typedef std::vector<SCLInstructionDataEx> ins_data;
typedef std::unordered_map<std::string, ProcDataEx2> address_map_ex;

static valid_instruction_set g_ControlFlow = {
	//Control flow
	SCR_TJMP,
	SCR_FJMP,
	SCR_JMP,
	SCR_OJMP,
	SCR_AJMP,
	SCR_LPOP,
	SCR_LJMP,

	//Arithmetic
	SCR_PUSHR,
	SCR_POPR,
	SCR_MOVC,
	SCR_PUSHC,
	SCR_ADD,
	SCR_SUB,
	SCR_MUL,
	SCR_DIV,
	SCR_MOD,
	SCR_NEG,
	SCR_SINL,
	SCR_COSL,
	SCR_RND,
	SCR_ATAN,
	SCR_MAX,
	SCR_MIN,
	SCR_EQUAL,
	SCR_NOTEQ,
	SCR_ABOVE,
	SCR_LESS,
	SCR_ABOVEEQ,
	SCR_LESSEQ
};

static valid_instruction_set g_TexInitCheck = {
	SCR_ANIME,
	SCR_STOP,
	SCR_RECT,
	SCR_LOAD,
	SCR_EXIT
};

//Also add Arithmetic
static valid_instruction_set g_EnemyCheck = {
	SCR_CALL,
	SCR_RET,
	SCR_ATK,
	SCR_ATK2,
	SCR_ATKNP,
	SCR_FATK,
	SCR_KILL,
	SCR_ESET,
	SCR_CHILD,
	SCR_CHGTASK,
	SCR_WAITATOBJ,
	SCR_ANM,
	SCR_PSE,
	SCR_EFC,
	SCR_MDMG,
	SCR_NOP,
	SCR_PNOP,
	SCR_MOV,
	SCR_PMOV,
	SCR_ACC,
	SCR_PACC,
	SCR_ROL,
	SCR_PROL,
	SCR_DEGS,
	SCR_PARENT,
	SCR_EXIT
};

//Add also flow control
static valid_instruction_set g_TSetCheck = {
	SCR_TAMA,
	SCR_LASER,
	SCR_HLASER,
	SCR_LSPHERE,
	SCR_RLASER,
	SCR_CROSS,
	SCR_FLOWER,
	SCR_GFIRE,
	SCR_IONRING,
	SCR_DEGE,
	SCR_DEGS,
	SCR_LLOPEN,
	SCR_LLCLOSE,
	SCR_LLCHARGE,
	SCR_NOP,
	SCR_PSE,
	SCR_EXIT
};

//Add also flow control
static valid_instruction_set g_ExAnmCheck = {
	SCR_NOP,
	SCR_TASK,
	SCR_TEXMODE,
	SCR_EXIT,
};

static valid_instruction_set g_SetProcCheck = {
	SCR_SET,
	SCR_NOP,
	SCR_EXIT
};


static valid_instruction_set g_TexLoadCheck = {
	SCR_LOADEX,
	SCR_RECT,
	SCR_EXIT
};


//Main function
bool CompileSCL(const char* Name, const char* Header, const char* OutputName);

//Stages of compiling:
 
//Tokenize everything
bool TokenizeInput(
	const char* pSourceFile,
	std::vector<Token>* pToken
);

//Include a source file
bool IncludeSourceFile(
	const char* pSourceFile,
	std::vector<Token>& source,
	size_t index
);

//Verify the syntax and try to make an easier to understand array to process data
bool VerifySyntax(
	std::vector<Token>& tokens,
	std::vector<Token>* pProcessedData
);

//Calculate addresses and parse command data
bool CalculateAddresses(
	const std::vector<Token>& tokens,
	address_map_ex* pProcData,
	size_t* end_file_size,
	std::vector<std::string>& ProcNameOrder
); 

//Fill the corresponding addresses
bool PopulateAddresses(
	address_map_ex& pProcData
); 

//Process header data and set addresses
bool ProcessHeader(
	const char* json, 
	const address_map_ex& pProcData, 
	SCLHeader* pHeader
); 

//Finally, join both data
void* JoinData(
	SCLHeader* header_data,
	const address_map_ex& proc_datass,
	const std::vector<std::string>& ProcNameOrder,
	size_t size
);


#endif