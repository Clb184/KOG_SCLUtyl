#include "SCLCompile.h"

inline bool IsValidKeyword(const std::string& word) {
    return
        word == "TEXINITPROC" || 
        word == "ENEMY" || 
        word == "TSET" || 
        word == "EXANM" || 
        word == "SETPROC" ||
        word == "LOADTEXPROC" || 
        word == "PROC" ||
        word == "const";
}

inline bool IsValidCommand(const std::string& word, int* pint) {
    if (g_Str2Cmd.find(word) != g_Str2Cmd.end()) {
        *pint = g_Str2Cmd[word];
        return true;
    }
    return false;
}

constexpr inline bool IsValidCharacter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == ',' || c == ';' || c == '\"' || c == '-' || c == ':';
}

constexpr inline bool IsValidFirstIdentifier(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

constexpr inline bool IsValidIdentifier(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

constexpr inline bool IsValidFirstNumberCharacter(char c, bool& sign) {
    if (c == '-') sign = true;
    return (c >= '0' && c <= '9') || c == '-';
}

constexpr inline bool IsValidNumberCharacter(char c) {
    return (c >= '0' && c <= '9');
}

constexpr inline bool IsSeparator(char c) {
    return c == ',' || c == ' ' || c == '\t' || c == ';' || c == '\n' || c == ':' || c == 0x0d;
}

bool GetInteger(char* pData, size_t bufSize, size_t& buf_idx, int* pResult) {
    bool success = true;
    int ret = 0;
    while (buf_idx < bufSize) {
        char c = pData[buf_idx];
        if (IsValidNumberCharacter(c)) {
            ret = ret * 10 + (c - '0');
        }
        else if (IsSeparator(c)) break;
        else if(IsValidIdentifier(c)) {
            success = false;
            break;
        }
        else break;
        buf_idx++;
    }
    *pResult = ret;
    return success;
}

bool GetIdentifier(char* pData, size_t bufSize, size_t& buf_idx, std::string& result) {
    bool success = true;
    while (buf_idx < bufSize) {
        char c = pData[buf_idx];
        if (IsValidIdentifier(c)) {
            result.push_back(c);
        }
        else {
            break;
        }
        buf_idx++;
    }
    return success;
}

bool CompileSCL(const char* Name, const char* Header, const char* OutputName) {
    FILE* fp;
    size_t size;
    bool success = false;
    if (fp = fopen(Name, "rb")) {
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        rewind(fp);
        char* pTextData = (char*)malloc(size);
        fread(pTextData, size, 1, fp);
        fclose(fp);

        //Relevant data
        SCLHeader head;
        address_map_ex proc_data;
        address_map_ex label_data;
        std::vector<Token> tok_data;
        std::vector<SCLInstructionData> instruction_data;
        InitializeString2Command();
        if (TokenizeInput(pTextData, size, &tok_data)) {
            auto p = [&]() {
                for (auto& t : tok_data) {
                    switch (t.kind) {
                    case TOKEN_COMMAND:
                    case TOKEN_KEYWORD:
                    case TOKEN_STRING:
                    case TOKEN_IDENTIFIER: printf("L %d: %s\n", t.line, t.pStr.c_str()); break;
                    case TOKEN_NUMBER: printf("L %d: %d\n", t.line, t.number); break;

                    }
                }
                return;
                };
            p();
            tok_data = CalculateAddresses(tok_data, &proc_data, &label_data);
            instruction_data = ProcessTokens(tok_data, proc_data, label_data);
            if (ProcessHeader(Header, &proc_data, &head)) {
                OutputData pSCLData = JoinData(head, instruction_data);
                FILE* out = fopen(OutputName, "wb");
                fwrite(pSCLData.pData, pSCLData.size, 1, out);
                fclose(out);
                free(pSCLData.pData);
                success = true;
            }
        }
        free(pTextData);
    }
    return success;
}

bool TokenizeInput(
    char* pInputData, 
    size_t size,
    std::vector<Token>* pToken
) 
{
    bool success = true;
    size_t idx = 0;
    size_t line = 1;
    bool is_comment = false;
    while (idx < size) {
        char c = pInputData[idx];
        try {
            Token tok;
            tok.kind = TOKEN_IGNORE;
            tok.line = line;
            if (c == ' ' || c == '\t' || c == 0x0d) {
                idx++;
            }
            else if (c == '\n') {
                idx++;
                line++;
            }
            else if (IsValidCharacter(c)) {
                bool sign = false;
                if (c == ',') {
                    tok.kind = TOKEN_COMMA;
                    idx++;
                }
                else if (c == ':') {
                    tok.kind = TOKEN_DOTS;
                    idx++;
                }
                else if (c == ';') {
                    while (idx < size || c != '\n') {
                        idx++;
                        c = pInputData[idx];
                    }
                    line++;
                }
                //Get string, if no quote is found before new line, throw an error
                else if (c == '\"') {
                    bool find_quote = false;
                    idx++;
                    if (idx < size) {
                        c = pInputData[idx];
                        while ((idx < size && c != '\"')) {
                            tok.pStr.push_back(c);
                            idx++;
                            c = pInputData[idx];
                        }
                        if (c == '\"') {
                            find_quote = true;
                            idx++;
                        }
                    }
                    if(!find_quote) throw 2;
                }
                else if (IsValidFirstNumberCharacter(c, sign)) {
                    if (sign) idx++;
                    if (GetInteger(pInputData, size, idx, &tok.number)) {
                        tok.kind = TOKEN_NUMBER;
                        if (sign) tok.number = -tok.number;
                    }
                    else throw 1;
                }
                else if (IsValidFirstIdentifier(c)) {
                    if (GetIdentifier(pInputData, size, idx, tok.pStr)) {
                        tok.kind = (IsValidKeyword(tok.pStr)) ? TOKEN_KEYWORD : (IsValidCommand(tok.pStr, &tok.number)) ? TOKEN_COMMAND : TOKEN_IDENTIFIER;
                    }
                    else throw 0;
                }
            }
            if (tok.kind != TOKEN_IGNORE)
                pToken->emplace_back(tok);
        }
        //Handle errors
        catch (int i) {
            const char* err = "";
            switch (i) {
            case 0: err = "Invalid identifier"; break;
            case 1: err = "Invalid combination with integer"; break;
            case 2: err = "String quote not found"; break;
            }
            printf("Error line %d: %s\n", line, err);
            success = false;
            break;
        }
        catch (...) {
            printf("Error line %d: Unspecified error\n", line);
            success = false;
            break;
        }
    }

    return success;
}

std::vector<Token> CalculateAddresses(
    const std::vector<Token>& tokens, 
    address_map_ex* pProcData, 
    address_map_ex* pLabelData
) 
{
    return std::vector<Token>();
}

std::vector<SCLInstructionData> ProcessTokens(
    const std::vector<Token>& tokens, 
    const address_map_ex& pProcData, 
    const address_map_ex& pLabelData
)
{
    return std::vector<SCLInstructionData>();
}

bool ProcessHeader(
    const char* json, 
    address_map_ex* pProcData,
    SCLHeader* pHeader
) 
{
    std::ifstream header(json);
    
    nlohmann::json jheader = nlohmann::json::parse(header);
    try {
        std::string tes = jheader["asd"];
    }
    catch (...) {
        return false;
    }
    return true;
}

OutputData JoinData(
    const SCLHeader& header_data, 
    const std::vector<SCLInstructionData>& instruction_data
)
{
    OutputData ret = {};
    return ret;
}
