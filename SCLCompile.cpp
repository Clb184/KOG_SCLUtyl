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
        std::vector<Token> tok_data;
        std::vector<Token> processed_token;
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
            if (VerifySyntax(tok_data, &processed_token)) {
                if (CalculateAddresses(processed_token, &proc_data)) {
                    PopulateAddresses(proc_data);
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
    const std::vector<Token>& tokens,
    std::vector<Token>* pProcessedData
)
{
    //Something went wrong or not
    bool raise_error = false;
    //For resetting the guide
    Token dummy_token = { TOKEN_IGNORE };
    //This is used as a guide on what to do, following certain rules
    Token guide_token = dummy_token;

    //This stores what token are needed
    int num_possible = 1;
    TOKEN_KIND possible_tokens[9] = {TOKEN_KEYWORD};
    //Used to look at reference for arguments
    SCLInstructionDefine def;
    int arg_idx = 0;
    //Activated by ANIME which needs an array
    bool anime = false;
    //Activated when entering a subroutine
    bool on_sub = false;

    Token ProcName { TOKEN_PROC, 0, ""};
    Token LabName { TOKEN_PROC, 0, ""};
    Token* pAnimP = nullptr;

    for (auto& t : tokens) {
        try {
            for (int i = 0; i <= num_possible && !raise_error; i++) {
                if (i >= num_possible) throw t;
                else if (t.kind == possible_tokens[i]) break;
                
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
                        if (on_sub) throw t;
                        possible_tokens[0] = TOKEN_IDENTIFIER;
                        num_possible = 1;
                        on_sub = true;
                        guide_token = t;
                        break;
                    case KEY_CONST:
                        possible_tokens[0] = TOKEN_IDENTIFIER;
                        num_possible = 1;
                        guide_token = t;
                        break;
                    case KEY_ENDPROC:
                        if (!on_sub) throw t;
                        possible_tokens[2] = TOKEN_KEYWORD;
                        num_possible = 3;
                        guide_token = dummy_token;
                        on_sub = false;
                        {
                            Token tmp{TOKEN_ENDPROC};
                            pProcessedData->emplace_back(tmp);
                        }
                        break;
                    default:
                        throw t;
                    }
                    break;
                case TOKEN_IDENTIFIER:
                    switch (guide_token.kind) {
                    case TOKEN_COMMAND:
                        if (!on_sub) throw t;
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
                        {
                            Token tk = t;
                            tk.line = 4;
                            pProcessedData->emplace_back(tk);
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
                            {
                                Token tmp = { TOKEN_PROC, 0, t.pStr };
                                pProcessedData->emplace_back(tmp);
                            }
                            guide_token = dummy_token;
                            break;
                        case KEY_CONST:
                            possible_tokens[0] = TOKEN_NUMBER;
                            possible_tokens[1] = TOKEN_STRING;
                            num_possible = 2;
                            break;
                        default:
                            throw t;
                        }
                        break;
                    default:
                        throw t;
                    case TOKEN_IGNORE:
                        if (!on_sub) throw t;
                        possible_tokens[0] = TOKEN_DOTS;
                        num_possible = 1;
                        guide_token = t;
                        break;
                    }
                    break;
                case TOKEN_COMMAND:
                    if (!on_sub) throw t;
                    def = g_InstructionSize[(SCL_INSTRUCTION)t.number];
                    guide_token = t;
                    arg_idx = 1;
                    {
                        Token args;
                        args.kind = TOKEN_INCOUNT;
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
                        case SCR_ATK:
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
                            args.number = def.cnt;
                        }
                        else {
                            possible_tokens[0] = TOKEN_IDENTIFIER;
                            possible_tokens[1] = TOKEN_COMMAND;
                            possible_tokens[2] = TOKEN_KEYWORD;
                            num_possible = 3;
                            guide_token = dummy_token;
                            args.number = 0;
                        }
                        pProcessedData->emplace_back(t);
                        pProcessedData->emplace_back(args);
                            
                    }
                    break;
                case TOKEN_NUMBER:
                case TOKEN_STRING:
                    if (guide_token.number == KEY_CONST) {
                        possible_tokens[0] = TOKEN_IDENTIFIER;
                        possible_tokens[1] = TOKEN_COMMAND;
                        possible_tokens[2] = TOKEN_KEYWORD;
                        num_possible = 3;
                        guide_token = dummy_token;
                    }
                    else if (guide_token.kind == TOKEN_COMMAND) {
                        if (anime && arg_idx == 2) {
                            def.cnt += t.number;
                            (pProcessedData->end() - 2)->number += t.number;
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
                        size_t of = 0;
                        if (t.kind == TOKEN_NUMBER) {
                            switch (def.paramdatatype[arg_idx]) {
                            case U32: case I32: of = 4; break;
                            case U16: case I16: of = 2; break;
                            case U8: case I8: of = 1; break;
                            }
                        }
                        else if (t.kind == TOKEN_STRING) {
                            of = t.pStr.length() + 1;
                        }
                        Token tmp = t;
                        tmp.line = of;
                        pProcessedData->emplace_back(tmp);
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
                case TOKEN_DOTS:
                    if (guide_token.kind == TOKEN_IDENTIFIER) {
                        possible_tokens[0] = TOKEN_IDENTIFIER;
                        possible_tokens[1] = TOKEN_COMMAND;
                        possible_tokens[2] = TOKEN_KEYWORD;
                        num_possible = 3;
                        Token tmp { TOKEN_LABEL, 0, guide_token.pStr };
                        pProcessedData->emplace_back(tmp);
                        guide_token = dummy_token;
                    }
                    break;
                }

            }
            else {
                printf("An error has ocurred L: %d\n", t.line);
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
    if (on_sub)
        raise_error = true;
    return !raise_error;
}

bool CalculateAddresses(
    const std::vector<Token>& tokens, 
    address_map_ex* pProcData
) 
{
    bool raise_error = false;
    address offset = sizeof(SCLHeader);
    size_t size = tokens.size();
    for (int i = 0; i < size; i++) {
        if (tokens[i].kind == TOKEN_PROC) {
            ProcDataEx2 chunk;
            chunk.ads = offset;
            const std::string name = tokens[i].pStr;
            i++;
            while (tokens[i].kind != TOKEN_ENDPROC) {
                switch (tokens[i].kind){
                    case TOKEN_COMMAND: {
                        SCLInstructionData data;
                        data.cmd = (SCL_INSTRUCTION)tokens[i].number;
                        i++;
                        if (tokens[i].kind == TOKEN_INCOUNT) {
                            size_t pc = tokens[i].number;
                            data.cnt = pc;
                            i++;
                            for (int j = 0; j < pc; j++, i++) {
                                SCLParamData param;
                                if (tokens[i].kind == TOKEN_STRING)
                                    param.stringdata = (char*)tokens[i].pStr.data();
                                else
                                    param.sdword = tokens[i].number;
                                data.param.emplace_back(param);
                            }
                        }
                        chunk.cmd_data.emplace_back(std::move(data));
                    } break;
                    case TOKEN_LABEL: {
                        chunk.label_data.insert({tokens[i].pStr, offset});
                        i++;
                    }break;
                }
            }
            pProcData->insert({name, chunk});
        }
    }
    return !raise_error;
}

bool PopulateAddresses(
    address_map_ex& pProcData
)
{
    return false;
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
