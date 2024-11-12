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
    bool success = false;

        //Relevant data
        SCLHeader head;
        address_map_ex proc_data;
        std::vector<Token> tok_data;
        std::vector<Token> processed_token;
        size_t bin_size;
        ins_data ins_data;
        std::vector<std::string> procname_data;
        InitializeString2Command();
        if (TokenizeInput(Name, &tok_data)) {
            if (VerifySyntax(tok_data, &processed_token)) {
                if (CalculateAddresses(processed_token, &proc_data, &bin_size, procname_data)) {
                    if (PopulateAddresses(proc_data)) {
                        if (ProcessHeader(Header, proc_data, &head)) {
                            void* pData = JoinData(&head, proc_data, procname_data, bin_size);
                            if (FILE* out = fopen(OutputName, "wb")) {
                                fwrite(pData, bin_size, 1, out);
                                fclose(out);
                                free(pData);
                                success = true;
                            }
                        }
                    }
                }
            }
        }
    return success;
}

bool TokenizeInput(
    const char* pSourceFile,
    std::vector<Token>* pToken
) 
{
    bool success = true;
    bool on_open = false;
    FILE* fp;
    try {
        fp = fopen(pSourceFile, "rb");
        on_open = fp != nullptr;
    }
    catch (...) {
        on_open = false;
    }

    if (on_open) {
        size_t size;
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        rewind(fp);
        char* pSourceData = (char*)malloc(size);
        fread(pSourceData, size, 1, fp);
        fclose(fp);

        size_t idx = 0;
        size_t line = 1;
        bool is_comment = false;
        while (idx < size) {
            char c = pSourceData[idx];
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
                        while (idx < size && c != '\n') {
                            idx++;
                            c = pSourceData[idx];
                        }
                        line++;
                    }
                    //Get string, if no quote is found before new line, throw an error
                    else if (c == '\"') {
                        bool find_quote = false;
                        idx++;
                        if (idx < size) {
                            c = pSourceData[idx];
                            while ((idx < size && c != '\"')) {
                                tok.pStr.push_back(c);
                                idx++;
                                c = pSourceData[idx];
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
                        if (GetInteger(pSourceData, size, idx, &tok.number)) {
                            tok.kind = TOKEN_NUMBER;
                            if (sign) tok.number = -tok.number;
                        }
                        else throw 1;
                    }
                    else if (IsValidFirstIdentifier(c)) {
                        if (GetIdentifier(pSourceData, size, idx, tok.pStr)) {
                            tok.kind = (IsValidKeyword(tok.pStr, &tok.number)) ? TOKEN_KEYWORD : (IsValidCommand(tok.pStr, &tok.number)) ? TOKEN_COMMAND : TOKEN_IDENTIFIER;
                        }
                        else throw 0;
                    }
                }
                tok.source = pSourceFile;
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
                printf("%s> Error line %d: %s\n", pSourceFile, line, err);
                success = false;
                break;
            }
            catch (...) {
                printf("%s> Error line %d: Unspecified error\n", pSourceFile, line);
                success = false;
                break;
            }
        }

        free(pSourceData);
    }
    else {
        printf("Can't open source %s\n", pSourceFile);
        success = false;
    }
    return success;
}

bool IncludeSourceFile(const char* pSourceFile, std::vector<Token>& source, size_t index) {
    std::vector<Token> source_2;
    bool success = false;
    if (TokenizeInput(pSourceFile, &source_2)) {
        source.insert(source.begin() + index, source_2.begin(), source_2.end());
        success = true;
    }
    return success;
}

bool VerifySyntax(
    std::vector<Token>& tokens,
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
    KEYWORD_KIND proc_kind;

    //Constants got by the const keyword
    std::map<const std::string, Token> const_map;
    Token ProcName { TOKEN_PROC, 0, ""};
    Token LabName { TOKEN_PROC, 0, ""};
    Token* pAnimP = nullptr;
    int ident_size = 0;


    for (int it = 0; it < tokens.size(); it++) {
        auto& t = tokens[it];
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
                    case KEY_INCLUDE:
                        possible_tokens[0] = TOKEN_STRING;
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
                        {
                            SCL_DATATYPE datat = def.paramdatatype[arg_idx];
                            Token tk = t;
                            if (datat != ADDRESS) {
                                if (const_map.find(t.pStr) != const_map.end()) {
                                    //Expand constant data
                                    size_t of = 0;
                                    const Token& tok = const_map[t.pStr];
                                    switch (datat) {
                                    case U32: case I32: 
                                        if (tok.kind == TOKEN_NUMBER) { 
                                            of += 2;
                                        }
                                        else throw t;
                                    case U16: case I16: 
                                        if (tok.kind == TOKEN_NUMBER) { 
                                            of++;
                                        }
                                        else throw t;
                                    case U8: case I8:  
                                        if (tok.kind == TOKEN_NUMBER) {
                                            of++;
                                            tk.number = tok.number;
                                        }
                                        else throw t; break;
                                    case STRING: 
                                        if (tok.kind == TOKEN_STRING) {
                                            of = tok.pStr.length() + 1;
                                            tk.pStr = tok.pStr;
                                        }
                                        else throw t; break;
                                    }
                                    tk.advance = of;
                                }
                            }
                            else {
                                tk.advance = 4;
                            }
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
                            proc_kind = (KEYWORD_KIND)guide_token.number;
                            {
                                Token tmp = { TOKEN_PROC, guide_token.number, t.pStr };
                                pProcessedData->emplace_back(tmp);
                            }
                            guide_token = dummy_token;
                            break;
                        case KEY_CONST:
                            possible_tokens[0] = TOKEN_NUMBER;
                            possible_tokens[1] = TOKEN_STRING;
                            guide_token.pStr = t.pStr;
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
                    {
                        bool is_correct_instruction = false;
                        switch (proc_kind) {
                        case KEY_TEXINITPROC:
                            for (auto& c : g_TexInitCheck) {
                                if (t.number == c) {
                                    is_correct_instruction = true;
                                    break;
                                }
                            }break;
                        case KEY_PROC:
                        case KEY_ENEMYPROC:
                            for (auto& c : g_ControlFlow) {
                                if (t.number == c) {
                                    is_correct_instruction = true;
                                    break;
                                }
                            }
                            for (auto& c : g_EnemyCheck) {
                                if (t.number == c) {
                                    is_correct_instruction = true;
                                    break;
                                }
                            }break;

                        case KEY_ATKPROC:
                            for (auto& c : g_ControlFlow) {
                                if (t.number == c) {
                                    is_correct_instruction = true;
                                    break;
                                }
                            }
                            for (auto& c : g_TSetCheck) {
                                if (t.number == c) {
                                    is_correct_instruction = true;
                                    break;
                                }
                            }break;

                        case KEY_EXANMPROC:
                            for (auto& c : g_ControlFlow) {
                                if (t.number == c) {
                                    is_correct_instruction = true;
                                    break;
                                }
                            }
                            for (auto& c : g_ExAnmCheck) {
                                if (t.number == c) {
                                    is_correct_instruction = true;
                                    break;
                                }
                            }break;
                        case KEY_SETPROC:
                            for (auto& c : g_SetProcCheck) {
                                if (t.number == c) {
                                    is_correct_instruction = true;
                                    break;
                                }
                            }break;
                        case KEY_LOADTEXPROC:
                            for (auto& c : g_TexLoadCheck) {
                                if (t.number == c) {
                                    is_correct_instruction = true;
                                    break;
                                }
                            }break;
                        } 
                        if (!is_correct_instruction)
                            throw t;
                    }
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
                        const_map.insert({guide_token.pStr, t});
                    }
                    else if (guide_token.number == KEY_INCLUDE && t.kind == TOKEN_STRING) {
                        if (IncludeSourceFile(t.pStr.c_str(), tokens, it + 1)) {
                            possible_tokens[0] = TOKEN_IDENTIFIER;
                            possible_tokens[1] = TOKEN_COMMAND;
                            possible_tokens[2] = TOKEN_KEYWORD;
                            num_possible = 3;
                            guide_token = dummy_token;
                        }
                        else throw SourceInfo{ t.source, t.pStr.c_str(),  t.line};
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
                        tmp.advance = of;
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
            printf("Invalid token in line %d @ %s\n", t.line, t.source);
            raise_error = true;
        }
        catch (const SourceInfo& src) {
            printf("%s - %d: Couldn't include source %s\n", src.pWhereSource, src.line, src.pWhatSource);
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
    address_map_ex* pProcData,
    size_t* end_file_size,
    std::vector<std::string>& ProcNameOrder
) 
{
    bool raise_error = false;
    address offset = sizeof(SCLHeader);
    size_t size = tokens.size();
    bool found_texproc = false;
    int err_code = -1;
    std::string dupe;
    for (int i = 0; i < size; i++) {
        if (tokens[i].kind == TOKEN_PROC) {
            ProcDataEx2 chunk;
            chunk.ads = offset;
            const std::string name = tokens[i].pStr;
            if (pProcData->find(name) != pProcData->end()) {
                raise_error = true;
                err_code = 1;
                dupe = tokens[i].pStr;
                break;
            }
            if (tokens[i].number == KEY_TEXINITPROC) {
                if (found_texproc) {
                    raise_error = true;
                    err_code = 0;
                    break;
                }
                else found_texproc = true;
            }
            i++;
            while (tokens[i].kind != TOKEN_ENDPROC) {
                switch (tokens[i].kind){
                    case TOKEN_COMMAND: {
                        SCLInstructionDataEx data;
                        data.line = tokens[i].line;
                        data.source = tokens[i].source;
                        data.cmd = (SCL_INSTRUCTION)tokens[i].number;
                        i++;
                        if (tokens[i].kind == TOKEN_INCOUNT) {
                            size_t pc = tokens[i].number;
                            data.cnt = pc;
                            offset++;
                            i++;
                            for (int j = 0; j < pc; j++, i++) {
                                SCLParamData param;
                                if (tokens[i].kind == TOKEN_STRING || tokens[i].kind == TOKEN_IDENTIFIER)
                                    param.stringdata = (char*)tokens[i].pStr.data();
                                else if (tokens[i].kind == TOKEN_NUMBER)
                                    param.sdword = tokens[i].number;

                                data.param.emplace_back(param);
                                offset += tokens[i].advance;
                            }
                        }
                        chunk.cmd_data.emplace_back(std::move(data));
                    } break;
                    case TOKEN_LABEL: {
                        if (chunk.label_data.find(tokens[i].pStr) != chunk.label_data.end()) {
                            raise_error = true;
                            err_code = 2;
                            dupe = tokens[i].pStr;
                            break;
                        }
                        chunk.label_data.insert({tokens[i].pStr, offset});
                        i++;
                    }break;
                }
                if (raise_error)
                    break;
            }
            pProcData->insert({name, chunk});
            ProcNameOrder.emplace_back(name);
        }
        if (raise_error)
            break;
    }
    if (!raise_error)
        *end_file_size = offset;
    else {
        const char* msg = "";
        switch (err_code) {
        case 0: printf("Error: There can only be one TexInit Procedure."); break;
        case 1: printf("Duplicate procedure name: %s\n", dupe.c_str()); break;
        case 2: printf("Duplicate label name: %s\n", dupe.c_str()); break;
        }
    }
    return !raise_error;
}

bool PopulateAddresses(
    address_map_ex& pProcData
)
{

    bool on_success = true;

    try {
        for (auto& p : pProcData) {
            ProcDataEx2& chunk = p.second;
            for (auto& c : chunk.cmd_data) {
                switch (c.cmd) {
                case SCR_TJMP:
                case SCR_FJMP:
                case SCR_JMP:
                case SCR_OJMP:
                case SCR_AJMP:
                case SCR_LJMP:
                {
                    const string& str = c.param[0].stringdata;
                    if (chunk.label_data.find(str) != chunk.label_data.end())
                        c.param[0].ads = chunk.label_data[str];
                    else throw SourceInfo{c.source, str, c.line};
                }break;
                case SCR_CALL:
                case SCR_ESET:
                case SCR_FATK:
                case SCR_ATKNP:
                case SCR_TASK:
                {
                    const string& str = c.param[0].stringdata;
                    if (pProcData.find(str) != pProcData.end())
                        c.param[0].ads = pProcData[str].ads;
                    else throw SourceInfo{ c.source, str, c.line };
                }break;
                case SCR_CHILD:
                case SCR_CHGTASK:
                {
                    const string& str = c.param[1].stringdata;
                    if (pProcData.find(str) != pProcData.end())
                        c.param[1].ads = pProcData[str].ads;
                    else throw SourceInfo{ c.source, str, c.line };
                }break;
                case SCR_SET:
                case SCR_ATK:
                {
                    const string& str = c.param[2].stringdata;
                    if (pProcData.find(str) != pProcData.end())
                        c.param[2].ads = pProcData[str].ads;
                    else throw SourceInfo{ c.source, str, c.line };
                }break;
                case SCR_ATK2:
                {
                    const string& str = c.param[3].stringdata;
                    if (pProcData.find(str) != pProcData.end())
                        c.param[3].ads = pProcData[str].ads;
                    else throw SourceInfo{ c.source, str, c.line };
                }break;
                }
            }
        }
    }
    catch (const SourceInfo& inf) {
        printf("%s : %d > Procedure or label \"%s\" doesn't exist.\n", inf.pWhereSource, inf.line, inf.pWhatSource);
        on_success = false;
    }
    return on_success;
}

bool ProcessHeader(
    const char* json, 
    const address_map_ex& pProcData,
    SCLHeader* pHeader
) 
{
    memset(pHeader, 0xffffffff, sizeof(SCLHeader));
    std::ifstream header(json);
    bool raise_error = false;
    nlohmann::json jheader = nlohmann::json::parse(header);
    if (jheader.find("TexInit") != jheader.end()) {
        try {
            const std::string& k = jheader["TexInit"];
            if (pProcData.find(k) != pProcData.end()) {
                address ads = pProcData.at(k).ads;
                pHeader->TexInitializer = ads;
            }
            else raise_error = true;
        }
        catch (...) {
            printf("Argument for TexInit must be an String.\n");
        }
    }
    else raise_error = true;
    if (raise_error) return false;

    const std::string SCLLevels[] = {"SCL1", "SCL2", "SCL3", "SCL4"};
    int cnt = 0;

    //I don't know if this is necessary, but just in case
    pHeader->NumLv1SCL = 0;
    pHeader->NumLv2SCL = 0;
    pHeader->NumLv3SCL = 0;
    pHeader->NumLv4SCL = 0;
    memset(pHeader->SCL_Lv1, 0xffffffff, sizeof(int) * SCLBUFFER_SIZE);
    memset(pHeader->SCL_Lv2, 0xffffffff, sizeof(int) * SCLBUFFER_SIZE);
    memset(pHeader->SCL_Lv3, 0xffffffff, sizeof(int) * SCLBUFFER_SIZE);
    memset(pHeader->SCL_Lv4, 0xffffffff, sizeof(int) * SCLBUFFER_SIZE);

    for (auto& scl : SCLLevels) {
        if (jheader.find(scl) != jheader.end()) {
            try {
                const std::vector<std::string>& k = jheader[scl];
                try {
                    int func_idx = 0;
                    for (auto& s : k) {
                        if (func_idx >= SCLBUFFER_SIZE) {
                            printf("Warning in JSON header: Truncating procedure fetching.\n");
                            break;
                        }
                        if (pProcData.find(s) != pProcData.end()) {
                            address ads = pProcData.at(s).ads;
                            switch (cnt) {
                            case 0: pHeader->SCL_Lv1[func_idx] = ads; break;
                            case 1: pHeader->SCL_Lv2[func_idx] = ads; break;
                            case 2: pHeader->SCL_Lv3[func_idx] = ads; break;
                            case 3: pHeader->SCL_Lv4[func_idx] = ads; break;
                            }
                        }
                        else throw s;
                        func_idx++;
                    }
                    switch (cnt) {
                    case 0: pHeader->NumLv1SCL = func_idx; break;
                    case 1: pHeader->NumLv2SCL = func_idx; break;
                    case 2: pHeader->NumLv3SCL = func_idx; break;
                    case 3: pHeader->NumLv4SCL = func_idx; break;
                    }
                    cnt++;
                }
                catch (const std::string& func) {
                    printf("Error in JSON header: Procedure \"%s\" doesn't exist.\n", func.c_str());
                    raise_error = true;
                    break;
                }
            }
            catch(...) {
                printf("Argument for SCL level must be an Array of strings.\n");
                raise_error = true;
                break;
            }
        }
        else raise_error = true;
    }
    if (raise_error) return false;
    const std::string player_table_name[] = {"Boss", "Combo", "Atk1", "Atk2", "Anm1", "Anm2","BossAnm", "WinAnm"};
    //Zero initialize this
    for (int i = 0; i < NUM_CHARACTERS; i++) {
        pHeader->LTEntry[i].NumTextures = 0;
        for (auto& tinit: pHeader->LTEntry[i].EntryPoint) {
            tinit = 0xffffffff;
        }
    }
    for (int i = 0; i < 9; i++) {
        const std::string character = ID2String(i);
        if (jheader.find(character) != jheader.end()) {
            const auto& k = jheader[character];
            try {
                int table = 0;
                for (auto& e : player_table_name) {
                    const std::string& proc = k[e];
                    try {
                        if (pProcData.find(proc) != pProcData.end()) {
                            address ads = pProcData.at(proc).ads;
                            switch (table) {
                            case 0: pHeader->BossAddr[i] = ads; break;
                            case 1: pHeader->ComboAddr[i] = ads;  break;
                            case 2: pHeader->Lv1Attack[i] = ads;  break;
                            case 3: pHeader->Lv2Attack[i] = ads;  break;
                            case 4: pHeader->ExAnmLv1[i] = ads;  break;
                            case 5: pHeader->ExAnmLv2[i] = ads;  break;
                            case 6: pHeader->ExAnmBoss[i] = ads;  break;
                            case 7: pHeader->ExAnmWin[i] = ads;  break;
                            }
                            table++;
                        }
                        else throw proc;
                    }
                    catch (const std::string& func) {
                        printf("Error in JSON header: Procedure \"%s\" doesn't exist.\n", func.c_str());
                        raise_error = true;
                        break;
                    }
                }
            }
            catch (...) {
                printf("Arguments for player attack and animation procedures must be strings.\n");
                raise_error = true;
                break;
            }
            if (raise_error) break;
            if (k.find("LoadTex") != k.end()) {
                try {
                    const std::vector<std::string>& ltex = k["LoadTex"];
                    try {
                        int ltentry = 0;
                        for (auto& s : ltex) {
                            if (ltentry >= LOADTEXTURE_MAX) {
                                printf("Warning in JSON header: Truncating LOADTEX procedure fetching.\n");
                                break;
                            }
                            if (pProcData.find(s) != pProcData.end()) {
                                address ads = pProcData.at(s).ads;
                                pHeader->LTEntry[i].EntryPoint[ltentry] = ads;
                                ltentry++;
                            }
                            else throw s;
                        }
                        pHeader->LTEntry[i].NumTextures = ltentry;
                    }
                    catch (const std::string& func) {
                        printf("Error in JSON header: function \"%s\" doesn't exist.\n", func.c_str());
                        raise_error = true;
                        break;
                    }
                }
                catch (...) {
                    printf("Argument for LoadTex must be an Array of strings.\n");
                    raise_error = true;
                    break;
                }
            }
        }
        else raise_error = true;
    }

    return !raise_error;
}

void* JoinData(
    SCLHeader* header_data, 
    const address_map_ex& proc_data,
    const std::vector<std::string>& ProcNameOrder,
    size_t size
)
{
    char* pData = (char*)malloc(size);
    size_t idx = sizeof(SCLHeader);
    memset(pData, 0x00, size);
    memcpy(pData, header_data, sizeof(SCLHeader));
    for (const auto& chunk_name : ProcNameOrder) {
        const ins_data& ref_ins = proc_data.at(chunk_name).cmd_data;
        for (const auto& data : ref_ins) {
            switch (data.cmd) {
            case SCR_LOAD:
                pData[idx] = char(data.param[0].sdword % 0x100);
                pData[idx + 1] = data.cmd;
                pData[idx + 2] = char(data.param[1].sdword % 0x100);
                memcpy(pData + 3 + idx, data.param[2].stringdata, std::strlen(data.param[2].stringdata));
                idx += 3 + std::strlen(data.param[2].stringdata) + 1;
                break;
            case SCR_PARENT:
            case SCR_PUSHR:
            case SCR_POPR:
                pData[idx] = char(data.param[0].sdword % 0x100);
                pData[idx + 1] = data.cmd;
                idx += 2;
                break;
            default: {
                SCLInstructionDefine def = g_InstructionSize[(SCL_INSTRUCTION)data.cmd];
                pData[idx] = data.cmd;
                if (data.cmd == SCR_ANIME) {
                    int anim_cnt = data.param[1].sdword;
                    def.cnt += anim_cnt;
                    for (int f = 0; f < anim_cnt; f++) {
                        def.paramdatatype.emplace_back(U8);
                    }
                }
                idx++;
                for (int i = 1; i < def.cnt; i++) {
                    switch (def.paramdatatype[i]) {
                    case U8: case I8: {
                        char bytedata = (char)(data.param[i - 1].sdword % 0x100);
                        pData[idx] = bytedata;
                        idx++;
                        break;
                    }
                    case U16: case I16: {
                        short shortdata = (short)(data.param[i - 1].sdword % 0x10000);
                        *(short*)(pData + idx) = shortdata;
                        idx += 2;
                        break;
                    }
                    case ADDRESS:
                    case U32: case I32: {
                        int intdata = (data.param[i - 1].sdword);
                        *(int*)(pData + idx) = intdata;
                        idx += 4;
                        break;
                    }
                    case STRING: {
                        size_t str = std::strlen(data.param[i - 1].stringdata);
                        memcpy(pData + idx, data.param[i - 1].stringdata, str);
                        idx += str + 1;
                    }
                    }
                }
            }
                   break;
            }
        }
    }
    return pData;
}
