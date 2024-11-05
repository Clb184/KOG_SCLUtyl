#ifndef SCLDUMP_INCLUDED
#define SCLDUMP_INCLUDED
#include "SCLData.h"

typedef std::map<address, ProcData> address_map;

static address_map g_ProcData;
static address_map g_LabelData;
static std::vector<SCLInstructionData> g_InstructionData;

//Main function
bool DumpSCL(const char* Name);

//SCL header
void ReadSCLHeader(SCLHeader* pHeader);
void PrintSCLHeader(SCLHeader* pHeader, const char* name);

//SCL body, functions, etc
void ReadSCL(void* pData, size_t size, const char* name);
void PrintSCL();

#endif // !SCLDUMP_INCLUDED
