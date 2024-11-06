#include "SCLCompile.h"

inline bool IsValidKeyword(const std::string& word, int* pint) {
    if (g_Keystr2Tok.find(word) != g_Keystr2Tok.end()) {
        *pint = g_Keystr2Tok[word];
        return true;
    }
    return false;
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
            //p();
            if (VerifySyntax(tok_data)) {
                if (CalculateAddresses(tok_data, &proc_data, &label_data, &tok_data)) {
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
                            tok.kind = TOKEN_STRING;
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
                        tok.kind = (IsValidKeyword(tok.pStr, &tok.number)) ? TOKEN_KEYWORD : (IsValidCommand(tok.pStr, &tok.number)) ? TOKEN_COMMAND : TOKEN_IDENTIFIER;
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

bool VerifySyntax(
    const std::vector<Token>& tokens
)
{
    bool raise_error = false;
    Token dummy_token = { TOKEN_IGNORE };
    Token guide_token = dummy_token;
    TOKEN_KIND token = TOKEN_KEYWORD;

    int num_possible = 1;
    TOKEN_KIND possible_tokens[9] = {TOKEN_KEYWORD};
    SCLInstructionDefine def;
    int arg_idx = 0;
    bool anime = false;
    for (auto& t : tokens) {
        try {
            for (int i = 0; i <= num_possible && !raise_error; i++) {
                if (t.kind == possible_tokens[i]) break;
                else if (i == num_possible) throw t;
            }

            if (!raise_error) {
                switch (t.kind) {
                case TOKEN_KEYWORD:
                    switch (t.number) {
                    case KEY_ENEMYPROC:
                    case KEY_ATKPROC:
                    case KEY_SETPROC:
                    case KEY_EXANMPROC:
                    case KEY_TEXINITPROC:
                    case KEY_LOADTEXPROC:
                    case KEY_PROC:
                        possible_tokens[0] = TOKEN_IDENTIFIER;
                        num_possible = 1;
                        break;
                    case KEY_CONST:
                        possible_tokens[0] = TOKEN_NUMBER;
                        num_possible = 1;
                        break;
                    default:
                        throw t;
                    }
                    guide_token = t;
                    break;
                case TOKEN_IDENTIFIER:
                    switch (guide_token.kind) {
                    case TOKEN_COMMAND:
                        if (arg_idx < def.cnt) {
                            possible_tokens[0] = TOKEN_COMMA;
                            num_possible = 1;
                        }
                        else {
                            possible_tokens[0] = TOKEN_IDENTIFIER;
                            possible_tokens[1] = TOKEN_COMMAND;
                            possible_tokens[2] = TOKEN_KEYWORD;
                            num_possible = 3;
                            guide_token = dummy_token;
                        }
                        break;
                    case TOKEN_KEYWORD:
                        switch (guide_token.number) {
                        case KEY_ENEMYPROC:
                        case KEY_ATKPROC:
                        case KEY_SETPROC:
                        case KEY_EXANMPROC:
                        case KEY_TEXINITPROC:
                        case KEY_LOADTEXPROC:
                        case KEY_PROC:
                            possible_tokens[0] = TOKEN_IDENTIFIER;
                            possible_tokens[1] = TOKEN_COMMAND;
                            possible_tokens[2] = TOKEN_KEYWORD;
                            num_possible = 3;
                            guide_token = dummy_token;
                            break;
                        case KEY_CONST:
                            break;
                        default:
                            throw t;
                        }
                        break;
                    default:
                    case TOKEN_IGNORE:
                        possible_tokens[0] = TOKEN_DOTS;
                        num_possible = 1;
                        guide_token = t;
                        break;
                    }
                    break;
                case TOKEN_COMMAND:
                    def = g_InstructionSize[(SCL_INSTRUCTION)t.number];
                    guide_token = t;
                    arg_idx = 1;
                    {
                        SCL_DATATYPE req;
                        switch (t.number) {
                        case SCR_LOAD:
                        case SCR_PARENT:
                        case SCR_PUSHR:
                        case SCR_POPR: {
                            SCL_DATATYPE d = def.paramdatatype[1];
                            def.paramdatatype[1] = def.paramdatatype[0];
                            def.paramdatatype[0] = d;
                        }break;
                        case SCR_CALL:
                            break;
                        case SCR_ANIME:
                            anime = true;
                        }
                        if (def.cnt > 1) {
                            req = def.paramdatatype[1];

                            possible_tokens[0] = (req >= U8 && req <= I32) ? TOKEN_NUMBER : (req == STRING) ? TOKEN_STRING : (req == ADDRESS) ? TOKEN_IDENTIFIER : TOKEN_IGNORE;
                            possible_tokens[1] = TOKEN_IDENTIFIER;
                            num_possible = 2;
                            def.cnt--;
                        }
                        else {
                            possible_tokens[0] = TOKEN_IDENTIFIER;
                            possible_tokens[1] = TOKEN_COMMAND;
                            possible_tokens[2] = TOKEN_KEYWORD;
                            num_possible = 3;
                            guide_token = dummy_token;
                        }
                    }
                    break;
                case TOKEN_NUMBER:
                case TOKEN_STRING:
                    if (guide_token.kind == TOKEN_COMMAND) {
                        if (anime && arg_idx == 2) {
                            def.cnt += t.number;
                            for (int i = 0; i < t.number; i++) {
                                def.paramdatatype.emplace_back(U8);
                            }
                        }
                        if (arg_idx < def.cnt) {
                            possible_tokens[0] = TOKEN_COMMA;
                            num_possible = 1;
                        }
                        else {
                            if (anime) anime = false;
                            possible_tokens[0] = TOKEN_IDENTIFIER;
                            possible_tokens[1] = TOKEN_COMMAND;
                            possible_tokens[2] = TOKEN_KEYWORD;
                            num_possible = 3;
                            guide_token = dummy_token;
                        }
                    }
                    else throw t;
                    break;
                case TOKEN_COMMA:
                    if (guide_token.kind == TOKEN_COMMAND) {
                        arg_idx++;
                        SCL_DATATYPE req = def.paramdatatype[arg_idx];
                        possible_tokens[0] = (req >= U8 && req <= I32) ? TOKEN_NUMBER : (req == STRING) ? TOKEN_STRING : (req == ADDRESS) ? TOKEN_IDENTIFIER : TOKEN_IGNORE;
                        possible_tokens[1] = TOKEN_IDENTIFIER;
                        num_possible = 2;
                    }
                    else  throw t;
                    break;
                }

            }
            else {
                raise_error = true;
                break;
            }
        }
        catch (const Token& t) {
            printf("Invalid token in line %d\n", t.line);
            raise_error = true;
        }
        catch (...) {
            raise_error = true;
        }
    }
    return !raise_error;
}

bool CalculateAddresses(
    const std::vector<Token>& tokens, 
    address_map_ex* pProcData, 
    address_map_ex* pLabelData,
    std::vector<Token>* pTokenOut
) 
{
    return false;
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
