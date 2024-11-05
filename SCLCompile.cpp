#include "SCLCompile.h"

bool CompileSCL(const char* Name, const char* Header, const char* OutputName) {
    return false;
}

std::vector<Token> TokenizeInput(char* pInputData) {
    return std::vector<Token>();
}

std::vector<Token> CalculateAddresses(const std::vector<Token>& tokens, address_map_ex* pProcData, address_map_ex* pLabelData) {
    return std::vector<Token>();
}

std::vector<SCLInstructionData> ProcessTokens(const std::vector<Token>& tokens, address_map_ex* pProcData, address_map_ex* pLabelData) {
    return std::vector<SCLInstructionData>();
}

SCLHeader ProcessHeader(char* pData, address_map_ex* pProcData) {
    return SCLHeader();
}

void* JoinData(const SCLHeader& header_data, const std::vector<SCLInstructionData>& instruction_data) {
    return nullptr;
}
