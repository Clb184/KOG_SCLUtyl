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
	TOKEN_ARITHS,
	TOKEN_ARITHF,
	TOKEN_ARITHC,
	TOKEN_REGISTER,
	TOKEN_BEGINPARENTHESIS,
	TOKEN_ENDPARENTHESIS,
	TOKEN_BEGINBLOCK,
	TOKEN_ENDBLOCK,

	//Extra tokens to make some stuff faster to process, i think
	TOKEN_PROC, //This holds the name of the procedure
	TOKEN_ENDPROC, //This tells to stop reading a subroutine
	TOKEN_LABEL, //the name of a label
	TOKEN_INCOUNT, //number of instructions to read
	TOKEN_CONST, //has the name of a const
	TOKEN_ARITHBEG, //has the name of a const
	TOKEN_ARITHEND, //has the name of a const

	//TOKEN_COMMENT, // ; as comment, like in a real assembler (Never touched one lol)
};

enum ARITHMETIC_SYMBOL {
	AS_ASSIGN, // var = ...

	AS_ADD, //var + var
	AS_SUB, //var - var
	AS_MUL, //var * var
	AS_DIV, //var / var
	AS_MOD, //var % var

	AS_INC, //var++
	AS_DEC, //var--

	AS_ADDA, //var += var
	AS_SUBA, //var -= var
	AS_MULA, //var *= var
	AS_DIVA, //var /= var
	AS_MODA, //var %= var

	AS_EQU, //var == var
	AS_NOTEQU, //var != var
	AS_LESS, //var < var
	AS_LESSEQ, //var <= var
	AS_GREAT, //var > var
	AS_GREATEQ, //var >= var

	AS_AND, //var && var
	AS_OR, //var || var

};

enum ARITHMETIC_CONTROL {
	AC_PE, // (
	AC_PL, // )
};

enum ARITHMETIC_FUNCTION {
	AF_MAX, //max(var, var)
	AF_MIN, //min(var, var)
	AF_RND, //rnd(var)
	AF_ATAN, //atan(var, var)
	AF_COSL, //cosl(var, var)
	AF_SINL, //sinl(var, var)
};

enum KEYWORD_KIND {
	KEY_GLOBAL,
	KEY_PROC,
	KEY_TEXINIT,
	KEY_ENEMY,
	KEY_TSET,
	KEY_EXANM,
	KEY_SET,
	KEY_EXTRATEX,
	KEY_ENDPROC,

	KEY_IF,
	KEY_WHILE,
	KEY_LOOP,

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

//Registers used on normal and enemy procedures
static std::map<const std::string, int> g_EnemyRegs = {
	{"x", 0},
	{"y", 1},
	{"vel", 2},
	{"hp", 3},
	{"count", 4},
	{"score", 5},
	{"gr0", 6},
	{"gr1", 7},
	{"gr2", 8},
	{"gr3", 9},
	{"gr4", 10},
	{"gr5", 11},
	{"gr6", 12},
	{"gr7", 13},
	{"flag", 14},
	{"dir", 15},
	{"alpha", 19},
};

//Registers used on TCL (tama(bullet) control language)
static std::map<const std::string, int> g_AtkRegs = {
	{"x", 0},
	{"y", 1},
	{"cmd", 2},
	{"vel", 3},
	{"acc", 4},
	{"ang", 5}, //Separation for fans
	{"ang2", 6}, //Angle separation between shots
	{"cnt", 7},
	{"cnt2", 8},
	{"dir", 9},
	{"vdir", 10},
	{"color", 11},
	{"type", 12},
	{"option", 13},
	{"anmspd", 14},
	{"llength", 15},
	{"gr0", 16},
	{"gr1", 17},
	{"gr2", 18},
	{"gr3", 19},
	{"gr4", 20},
	{"gr5", 21},
	{"gr6", 22},
	{"gr7", 23},
	{"parentdir", 24},
	{"param", 26},
};

//Registers used on ExAnm
static std::map<const std::string, int> g_ExAnmRegs = {
	{"x", 0},
	{"y", 1},
	{"sx", 2},
	{"sy", 3},
	{"dir", 4},
	{"alpha", 5}, //Separation for fans
	{"spriteu", 6}, //Angle separation between shots
	{"sprited", 7},
	{"clipx", 8},
	{"clipy", 9},
	{"clipw", 10},
	{"cliph", 11},
	{"clipr", 12},
	{"clipg", 13},
	{"clipb", 14},
	{"clipa", 15},
	{"gr0", 16},
	{"gr1", 17},
	{"gr2", 18},
	{"gr3", 19},
	{"gr4", 20},
	{"gr5", 21},
	{"gr6", 22},
	{"gr7", 23},
	{"gr8", 24},
	{"gr9", 25},
};

static std::map<const std::string, ARITHMETIC_FUNCTION> g_ArithFunc = {
	{"cosl",AF_COSL},
	{"sinl",AF_SINL},
	{"atan",AF_ATAN},
	{"min",AF_MIN},
	{"max",AF_MAX},
	{"rnd",AF_RND},
};

static std::map<const std::string, KEYWORD_KIND> g_Keystr2Tok = {
	//Procedure types
	{"PROC", KEY_PROC},
	{"TEXINITPROC", KEY_TEXINIT},
	{"ENEMY", KEY_ENEMY},
	{"TSET", KEY_TSET},
	{"EXANM", KEY_EXANM},
	{"SETPROC", KEY_SET},
	{"LOADTEXPROC", KEY_EXTRATEX},

	//Conditions and loops (lol)
	{"if", KEY_IF},
	{"while", KEY_WHILE},
	{"loop", KEY_LOOP},

	//Other Keywords
	{"ENDPROC", KEY_ENDPROC},
	{"const", KEY_CONST},
	{"include", KEY_INCLUDE}
};

typedef std::vector<SCL_INSTRUCTION> valid_instruction_set;
typedef std::vector<SCLInstructionDataEx> ins_data;
typedef std::unordered_map<std::string, ProcDataEx2> address_map_ex;
typedef std::map<const std::string, Token> constant_map;

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

static valid_instruction_set g_SetCheck = {
	SCR_SET,
	SCR_NOP,
	SCR_EXIT
};


static valid_instruction_set g_ExtraTexCheck = {
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
bool VerifySyntaxAndParse(
	std::vector<Token>& tokens,
	std::vector<Token>* pProcessedData
);

//Parse common stuff for comand data
bool ProcessCommonCmdData(
	std::vector<Token>& tokens,
	std::vector<Token>* pProcessedData,
	constant_map& const_map,
	SCL_INSTRUCTION cmd,
	SCL_INSTRUCTION& last_cmd,
	KEYWORD_KIND proc_type,
	size_t& idx
);

//Process if statemets, while loops and normal loops
bool ProcessConditionOrLoopBlock(
	std::vector<Token>& tokens,
	std::vector<Token>* pProcessedData,
	constant_map& const_map,
	KEYWORD_KIND proc_kind,
	KEYWORD_KIND type,
	size_t& idx
);

//Process different kinds of blocks
bool ProcessTexInitBlock(
	std::vector<Token>& tokens,
	std::vector<Token>* pProcessedData,
	constant_map& const_map,
	size_t& idx
);

bool ProcessEnemyBlock(
	std::vector<Token>& tokens,
	std::vector<Token>* pProcessedData,
	constant_map& const_map,
	size_t& idx,
	int exit_type
);

bool ProcessTSetBlock(
	std::vector<Token>& tokens,
	std::vector<Token>* pProcessedData,
	constant_map& const_map,
	size_t& idx,
	int exit_type
);

bool ProcessExAnmBlock(
	std::vector<Token>& tokens,
	std::vector<Token>* pProcessedData,
	constant_map& const_map,
	size_t& idx,
	int exit_type
);

bool ProcessSetBlock(
	std::vector<Token>& tokens,
	std::vector<Token>* pProcessedData,
	constant_map& const_map,
	size_t& idx
);

bool ProcessExtraTexBlock(
	std::vector<Token>& tokens,
	std::vector<Token>* pProcessedData,
	constant_map& const_map,
	size_t& idx
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