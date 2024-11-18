#include "SCLCompile.h"
#include "Calc2Token.h"

static int g_IfCnt = 0;
static int g_WhileCnt = 0;
static int g_LoopCnt = 0;

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
/*
+ - * / % operation
+= -= *= /= %= operation and assign
== <= >= < > !=
&&-> min ||-> max

(stuff) && stuff && stuff && stuff
(((1 1)min 1)min 0)min
1 1 min 0 min
1 0 min
0
false


if not (40 > 60){
    ...
       block
    ...
}

    40
    60
    great
    tjmp ifblock
    ...
        block
    ...
ifblock0e:
    
*/
constexpr inline bool IsValidBlockControl(char c) {
    return c == '{' || c == '}';
}

constexpr inline bool IsValidArithmeticControl(char c) {
    return c == '(' || c == ')';
}

constexpr inline bool IsValidArithmeticSymbol(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '=' || c == '>' || c == '<' || c == '&' || c == '|';
}

constexpr inline bool IsValidCharacter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == ',' || c == ';' || c == '\"' || c == '-' || c == ':'
        || IsValidArithmeticSymbol(c) || IsValidArithmeticControl(c) || IsValidBlockControl(c);
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
    return c == ',' || c == ' ' || c == '\t' || c == ';' || c == '\n' || c == ':' || c == 0x0d || IsValidArithmeticSymbol(c) || IsValidBlockControl(c) || IsValidArithmeticSymbol(c);
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

bool GetArithSign(char* pData, size_t bufSize, size_t& buf_idx, ARITHMETIC_SYMBOL& result) {
    bool success = true;
    std::string res = "";
    while (buf_idx < bufSize) {
        char c = pData[buf_idx];
        if (IsValidArithmeticSymbol(c)) {
            res.push_back(c);
        }
        else {
            break;
        }
        buf_idx++;
    }
    if (res.length() >= 1) {
        char base = res.at(0);
        switch(res.length()) {
        default: success = false;
        case 1:
            switch (base) {
            case '+': result = AS_ADD; break;
            case '-': result = AS_SUB; break;
            case '*': result = AS_MUL; break;
            case '/': result = AS_DIV; break;
            case '%': result = AS_MOD; break;

            case '<': result = AS_LESS; break;
            case '>': result = AS_GREAT; break;
            case '=': result = AS_ASSIGN; break;
            default: success = false; break;
            }
            break;
        case 2: {
            char next = res.at(1);
            if (next == base) {
                switch (next) {
                case '+': result = AS_INC; break;
                case '-': result = AS_DEC; break;
                case '&': result = AS_AND; break;
                case '|': result = AS_OR; break;
                case '=': result = AS_EQU; break;
                default: success = false; break;
                }
            }
            else if (next == '=') {
                switch (base) {
                case '+': result = AS_ADDA; break;
                case '-': result = AS_SUBA; break;
                case '*': result = AS_MULA; break;
                case '/': result = AS_DIVA; break;
                case '%': result = AS_MODA; break;

                case '<': result = AS_LESSEQ; break;
                case '>': result = AS_GREATEQ; break;
                case '!': result = AS_NOTEQU; break;
                default: success = false; break;
                }
            }
            else success = false;
            }break;
        }
    }
    else
        success = false;
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
            if (VerifySyntaxAndParse(tok_data, &processed_token)) {
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
                tok.source = pSourceFile;
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
                        if (!find_quote) throw 2;
                    }
                    else if (IsValidNumberCharacter(c)) {
                        if (GetInteger(pSourceData, size, idx, &tok.number)) {
                            tok.kind = TOKEN_NUMBER;
                        }
                        else throw 1;
                    }
                    else if (IsValidFirstIdentifier(c)) {
                        if (GetIdentifier(pSourceData, size, idx, tok.pStr)) {
                            tok.kind =
                                (IsValidKeyword(tok.pStr, &tok.number)) ? TOKEN_KEYWORD :
                                (IsValidCommand(tok.pStr, &tok.number)) ? TOKEN_COMMAND :
                                TOKEN_IDENTIFIER;
                            if (g_ArithFunc.find(tok.pStr) != g_ArithFunc.end()) {
                                tok.kind = TOKEN_ARITHF;
                                tok.number = g_ArithFunc[tok.pStr];
                                tok.line = line;
                                tok.source = pSourceFile;
                            }
                        }
                        else throw 0;
                    }
                    else if (IsValidArithmeticSymbol(c)) {
                        ARITHMETIC_SYMBOL sym;
                        if (GetArithSign(pSourceData, size, idx, sym)) {
                            tok.kind = TOKEN_ARITHS;
                            tok.number = sym;
                        }
                    }
                    else if (IsValidArithmeticControl(c)) {
                        tok.kind = (c == '(') ? TOKEN_BEGINPARENTHESIS : TOKEN_ENDPARENTHESIS;
                        idx++;
                    }
                    else if (IsValidBlockControl(c)) {
                        tok.kind = (c == '{') ? TOKEN_BEGINBLOCK : TOKEN_ENDBLOCK;
                        idx++;
                    }
                    else throw c;

                }
                else throw c;
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
            catch (char c) {
                printf("%s> Error line %c: Unrecognized character %c\n", pSourceFile, line, c);
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

bool VerifySyntaxAndParse(
    std::vector<Token>& tokens,
    std::vector<Token>* pProcessedData
)
{
    //Something went wrong or not
    bool raise_error = false;
    //Activated by ANIME which needs an array
    bool anime = false;

    //Constants got by the const keyword
    constant_map const_map;
    Token ProcName { TOKEN_PROC, 0, ""};
    Token LabName { TOKEN_PROC, 0, ""};
    Token* pAnimP = nullptr;
    int ident_size = 0;
    size_t size = tokens.size();

    for (size_t i = 0; i < size; ) {
        auto& t = tokens[i];
        try {
            if (t.kind == TOKEN_KEYWORD) {
                switch (t.number) {
                case KEY_TEXINIT:
                case KEY_PROC:
                case KEY_ENEMY:
                case KEY_TSET:
                case KEY_EXANM:
                case KEY_EXTRATEX:
                case KEY_SET:
                    if (i + 2 < tokens.size() && tokens[i + 1].kind == TOKEN_IDENTIFIER) {
                        i++;
                        Token tmp = { TOKEN_PROC, t.number, tokens[i].pStr };
                        pProcessedData->emplace_back(tmp);
                        if (tokens[i + 1].kind == TOKEN_BEGINBLOCK) {
                            i += 2;
                            bool parse_success = false;
                            switch (t.number) {
                            case KEY_TEXINIT: parse_success = ProcessTexInitBlock(tokens, pProcessedData, const_map, i); break;
                            case KEY_PROC: parse_success = ProcessEnemyBlock(tokens, pProcessedData, const_map, i, 2); break;
                            case KEY_ENEMY: parse_success = ProcessEnemyBlock(tokens, pProcessedData, const_map, i, 0); break;
                            case KEY_TSET: parse_success = ProcessTSetBlock(tokens, pProcessedData, const_map, i, 0); break;
                            case KEY_EXANM: parse_success = ProcessExAnmBlock(tokens, pProcessedData, const_map, i, 0); break;
                            case KEY_EXTRATEX: parse_success = ProcessExtraTexBlock(tokens, pProcessedData, const_map, i); break;
                            case KEY_SET: parse_success = ProcessSetBlock(tokens, pProcessedData, const_map, i); break;
                            }
                            if (!parse_success) throw 1;
                        }
                        else throw tokens[i + 1];
                    }
                    else throw 0;
                    break;
                case KEY_CONST:
                    if (i + 2 < tokens.size() && tokens[i + 1].kind == TOKEN_IDENTIFIER) {
                        i++;
                        Token tmp = { TOKEN_PROC, tokens[i + 1].line };
                        const std::string& str = tokens[i].pStr;
                        i++;
                        switch (tokens[i].kind) {
                        case TOKEN_STRING: tmp.kind = TOKEN_STRING; tmp.pStr = tokens[i].pStr; break;
                        default: tmp.kind = TOKEN_NUMBER; if (!CalcConvertConst(tokens, pProcessedData, i, KEY_GLOBAL, const_map, tmp.number)) throw 2; break;
                        }
                        const_map.insert({ str, tmp });
                    }
                    else throw 0;
                    break;
                case KEY_INCLUDE:
                    if (i + 1 < tokens.size() && tokens[i + 1].kind == TOKEN_STRING) {
                        i++;
                        if (!IncludeSourceFile(tokens[i].pStr.c_str(), tokens, i + 2)) throw SourceInfo{ t.source, t.pStr.c_str(),  t.line };
                        size = tokens.size();
                    }
                    else throw 0;
                }

            }
            else throw t;
        }
        catch (const Token& t) {
            printf("Unexpected token in line %d @ %s\n", t.line, t.source);
            raise_error = true;
        }
        catch (const SourceInfo& src) {
            printf("%s - %d: Failed including source file %s\n", src.pWhereSource, src.line, src.pWhatSource);
            raise_error = true;
        }
        catch (const int code) {
            const char* msg = "";
            switch (code) {
            case 0: msg = "Unexpected End of File"; break;
            case 1: msg = "Failed processing procedure block"; break;
            case 2: msg = "Failed processing const"; break;
            }
            printf("Error while parsing: %s\n", msg);
        }
        catch (...) {
            raise_error = true;
        }
    }

    return !raise_error;
}

bool ProcessCommonCmdData(
    std::vector<Token>& tokens, 
    std::vector<Token>* pProcessedData, 
    constant_map& const_map,
    SCL_INSTRUCTION command, 
    SCL_INSTRUCTION& last_cmd, 
    KEYWORD_KIND proc_type,
    size_t& idx
)
{
    bool success = true;
    size_t size = tokens.size();
    try {
        //Used to look at reference for arguments
        SCLInstructionDefine def = g_InstructionSize[command];

        //Instruction and how many parameters it has
        Token cmd;
        cmd.kind = TOKEN_COMMAND;
        cmd.number = command;
        last_cmd = command;

        int num_args = def.cnt - 1;
        Token cnt;
        cnt.kind = TOKEN_INCOUNT;
        cnt.number = num_args;

        pProcessedData->emplace_back(cmd);
        pProcessedData->emplace_back(cnt);
        idx++;
        if (idx + (num_args) * 2 - 1 < size) {
            for (int x = num_args, y = 0; x > 0;) {
                Token rg;
                rg.advance = 0;
                const Token& rf = tokens[idx];
                if (command == SCR_PUSHR || command == SCR_PARENT || command == SCR_POPR || command == SCR_LOAD) {
                    int reg = def.paramdatatype[0];
                    def.paramdatatype[0] = COMMAND;
                    def.paramdatatype[1] = U8;
                }

                switch (def.paramdatatype[1 + y]) {
                case U32: case I32:
                    rg.advance += 2;
                case U16: case I16:
                    rg.advance++;
                case U8: case I8:
                    rg.advance++;
                    {
                        int result = 0;
                        rg.kind = TOKEN_NUMBER;
                        rg.line = rf.line;
                        rg.source = rf.source;
                        if (!CalcConvertConst(tokens, pProcessedData, idx, proc_type, const_map, rg.number)) throw 3;
                    }    break;
                case ADDRESS:
                    rg.kind = TOKEN_IDENTIFIER;
                    rg.advance = 4;
                    rg.pStr = rf.pStr;
                    idx++;
                    break;
                case STRING :
                    rg.advance = rf.pStr.size();
                    rg.kind = TOKEN_STRING;
                    rg.pStr = rf.pStr;
                    idx++;
                    break;
                default:
                    throw 2;
                }
                pProcessedData->emplace_back(rg);
                x--;
                if (x > 0 && tokens[idx].kind == TOKEN_COMMA) {
                    idx++;
                    y++;
                }
            }
        }
        else throw 0;
    }
    catch (...) {
        success = false;
    }

    return success;
}

bool ProcessConditionOrLoopBlock(
    std::vector<Token>& tokens, 
    std::vector<Token>* pProcessedData, 
    constant_map& const_map, 
    KEYWORD_KIND proc_kind,
    KEYWORD_KIND type, 
    size_t& idx
) {
    bool success = true;
    size_t size = tokens.size();
    try {
        switch (type) {
        case KEY_IF:
            if (idx + 3 < size) {
                idx++;
                if (tokens[idx].kind == TOKEN_BEGINPARENTHESIS) {
                    if (CalcConvert(tokens, pProcessedData, idx, KEY_ENEMY, const_map, false)) {
                        if (tokens[idx].kind == TOKEN_BEGINBLOCK) {
                            idx++;
                            std::string lab_if = "@if_" + std::to_string(g_IfCnt);
                            Token if_cmd;
                            if_cmd.kind = TOKEN_COMMAND;
                            if_cmd.number = SCR_FJMP;
                            pProcessedData->emplace_back(if_cmd);
                            Token if_cnt;
                            if_cnt.kind = TOKEN_INCOUNT;
                            if_cnt.number = 1;
                            pProcessedData->emplace_back(if_cnt);
                            Token if_label;
                            if_label.kind = TOKEN_IDENTIFIER;
                            if_label.advance = 4;
                            if_label.pStr = lab_if;
                            pProcessedData->emplace_back(if_label);
                            if (!ProcessEnemyBlock(tokens, pProcessedData, const_map, idx, 2)) throw 0;

                            Token tmp{ TOKEN_LABEL, 0, lab_if };
                            pProcessedData->emplace_back(tmp);
                            g_IfCnt++;
                        }
                    }
                    else throw 0;
                }


            }
            else throw 0;

        case KEY_WHILE:
            if (idx + 3 < size) {
                idx++;
                if (tokens[idx].kind == TOKEN_BEGINPARENTHESIS) {
                    std::string w_cond = "@whilec_" + std::to_string(g_WhileCnt);
                    std::string w_end = "@whilee_" + std::to_string(g_WhileCnt);
                    Token tmp1{ TOKEN_LABEL, 0, w_cond };
                    pProcessedData->emplace_back(tmp1);

                    Token w_cmd1;
                    w_cmd1.kind = TOKEN_COMMAND;
                    w_cmd1.number = SCR_FJMP;
                    pProcessedData->emplace_back(w_cmd1);
                    Token w_cnt1;
                    w_cnt1.kind = TOKEN_INCOUNT;
                    w_cnt1.number = 1;
                    pProcessedData->emplace_back(w_cnt1);
                    Token w_label1;
                    w_label1.kind = TOKEN_IDENTIFIER;
                    w_label1.advance = 4;
                    w_label1.pStr = w_end;
                    pProcessedData->emplace_back(w_label1);
                    if (CalcConvert(tokens, pProcessedData, idx, KEY_ENEMY, const_map, false)) {
                        if (tokens[idx].kind == TOKEN_BEGINBLOCK) {
                            idx++;
                            if (!ProcessEnemyBlock(tokens, pProcessedData, const_map, idx, 2)) throw 0;

                            Token w_cmd2;
                            w_cmd2.kind = TOKEN_COMMAND;
                            w_cmd2.number = SCR_JMP;
                            pProcessedData->emplace_back(w_cmd2);
                            Token w_cnt2;
                            w_cnt2.kind = TOKEN_INCOUNT;
                            w_cnt2.number = 1;
                            pProcessedData->emplace_back(w_cnt2);
                            Token w_label2;
                            w_label2.kind = TOKEN_IDENTIFIER;
                            w_label2.advance = 4;
                            w_label2.pStr = w_cond;
                            pProcessedData->emplace_back(w_label2);

                            Token tmp2{ TOKEN_LABEL, 0, w_end };
                            pProcessedData->emplace_back(tmp2);
                            g_WhileCnt++;
                        }
                    }
                    else throw 0;
                }
            }
            else throw 0;
            break;


        case KEY_LOOP:
            if (idx + 3 < size) {
                idx++;
                if (tokens[idx].kind == TOKEN_BEGINPARENTHESIS) {
                    if (CalcConvert(tokens, pProcessedData, idx, KEY_ENEMY, const_map, false)) {
                        if (tokens[idx].kind == TOKEN_BEGINBLOCK) {
                            idx++;
                            std::string w_cond = "@loopb_" + std::to_string(g_LoopCnt);
                            std::string w_end = "@loope_" + std::to_string(g_LoopCnt);

                            Token lpop_cmd;
                            lpop_cmd.kind = TOKEN_COMMAND;
                            lpop_cmd.number = SCR_LPOP;
                            pProcessedData->emplace_back(lpop_cmd);
                            Token lpop_cnt;
                            lpop_cnt.kind = TOKEN_INCOUNT;
                            lpop_cnt.number = 0;
                            pProcessedData->emplace_back(lpop_cnt);

                            Token tmp1{ TOKEN_LABEL, 0, w_cond };
                            pProcessedData->emplace_back(tmp1);

                            Token w_cmd1;
                            w_cmd1.kind = TOKEN_COMMAND;
                            w_cmd1.number = SCR_LJMP;
                            pProcessedData->emplace_back(w_cmd1);
                            Token w_cnt1;
                            w_cnt1.kind = TOKEN_INCOUNT;
                            w_cnt1.number = 1;
                            pProcessedData->emplace_back(w_cnt1);
                            Token w_label1;
                            w_label1.kind = TOKEN_IDENTIFIER;
                            w_label1.advance = 4;
                            w_label1.pStr = w_end;
                            pProcessedData->emplace_back(w_label1);
                            if (!ProcessEnemyBlock(tokens, pProcessedData, const_map, idx, 1)) throw 0;

                            Token w_cmd2;
                            w_cmd2.kind = TOKEN_COMMAND;
                            w_cmd2.number = SCR_JMP;
                            pProcessedData->emplace_back(w_cmd2);
                            Token w_cnt2;
                            w_cnt2.kind = TOKEN_INCOUNT;
                            w_cnt2.number = 1;
                            pProcessedData->emplace_back(w_cnt2);
                            Token w_label2;
                            w_label2.kind = TOKEN_IDENTIFIER;
                            w_label2.advance = 4;
                            w_label2.pStr = w_cond;
                            pProcessedData->emplace_back(w_label2);

                            Token tmp2{ TOKEN_LABEL, 0, w_end };
                            pProcessedData->emplace_back(tmp2);
                            g_LoopCnt++;
                        }
                    }
                    else throw 0;
                }
            }
            else throw 0;
            break;
        }
    }
    catch (...) {
        success = false;
    }
    return success;
}

bool ProcessTexInitBlock(
    std::vector<Token>& tokens, 
    std::vector<Token>* pProcessedData, 
    constant_map& const_map, 
    size_t& idx
)
{
    bool success = true;
    SCL_INSTRUCTION last_cmd = SCR_NOP;
    const size_t size = tokens.size();

    try {
        bool on_exit = false;
        for (; idx < size; ) {
            const Token& t = tokens[idx];
            switch (t.kind) {
                //Just comands
            case TOKEN_COMMAND: {
                bool found_command = false;
                for (auto c : g_TexInitCheck) if (c == t.number) found_command = true;
                if (found_command) {
                    if (t.number == SCR_ANIME) {
                        
                    }
                    else if (!ProcessCommonCmdData(tokens, pProcessedData, const_map, (SCL_INSTRUCTION)t.number, last_cmd, KEY_TEXINIT, idx)) throw 0;
                }
                else throw t;
            }    break;
                              //Const for local constants
            case TOKEN_KEYWORD:
                switch (t.number) {
                case KEY_CONST:
                    if (idx + 2 < tokens.size() && tokens[idx + 1].kind == TOKEN_IDENTIFIER) {
                        idx++;
                        Token tmp = { TOKEN_PROC, tokens[idx + 1].line };
                        const std::string& str = tokens[idx].pStr;
                        idx++;
                        switch (tokens[idx].kind) {
                        case TOKEN_STRING: tmp.kind = TOKEN_STRING; tmp.pStr = tokens[idx].pStr; break;
                        default: tmp.kind = TOKEN_NUMBER; if (!CalcConvertConst(tokens, pProcessedData, idx, KEY_GLOBAL, const_map, tmp.number)) throw 2; break;
                        }
                        const_map.insert({ str, tmp });
                    }
                    else throw 0;
                    break;
                default:
                    throw 0;
                }
                break;
            case TOKEN_ENDBLOCK:
            {
                idx++;
                if (last_cmd != SCR_EXIT) {
                    Token cmd;
                    cmd.kind = TOKEN_COMMAND;
                    cmd.number = SCR_EXIT;

                    Token cnt;
                    cnt.kind = TOKEN_INCOUNT;
                    cnt.number = 0;
                    pProcessedData->emplace_back(cmd);
                    pProcessedData->emplace_back(cnt);
                }
                Token end;
                end.kind = TOKEN_ENDPROC;
                pProcessedData->emplace_back(end);

                on_exit = true;
            }
            break;
            default:
                throw 0;
                break;
            }
            if (on_exit) break;
        }
    }
    catch (...) {
        success = false;
    }
    return success;
}

bool ProcessEnemyBlock(
    std::vector<Token>& tokens, 
    std::vector<Token>* pProcessedData, 
    constant_map& const_map,
    size_t& idx,
    int exit_type
)
{
    bool success = true;
    SCL_INSTRUCTION last_cmd = SCR_NOP;
    const size_t size = tokens.size();

    try {
        bool on_exit = false;
        for (; idx < size; ) {
            const Token& t = tokens[idx];
            switch (t.kind) {
            //Just comands
            case TOKEN_COMMAND: {
                bool found_command = false;
                for (auto c : g_EnemyCheck) if (c == t.number) found_command = true;
                for (auto c : g_ControlFlow) if (c == t.number) found_command = true;
                if (found_command) {
                    if(!ProcessCommonCmdData(tokens, pProcessedData, const_map, (SCL_INSTRUCTION)t.number, last_cmd, KEY_ENEMY, idx)) throw 0;
                }
                else throw t;
            }    break;
            //For normal math expressions
            case TOKEN_IDENTIFIER: {
                if (idx + 1 < size && tokens[idx + 1].kind == TOKEN_DOTS) {
                    Token tmp{ TOKEN_LABEL, t.line, t.pStr };
                    pProcessedData->emplace_back(tmp);
                    idx += 2;
                }
                else if (!CalcConvert(tokens, pProcessedData, idx, KEY_ENEMY, const_map, true)) throw 0;
            } break;
            //Const for local constants
            case TOKEN_KEYWORD:
                if (t.number == KEY_IF || t.number == KEY_WHILE || t.number == KEY_LOOP) {
                    if(!ProcessConditionOrLoopBlock(tokens, pProcessedData, const_map, KEY_ENEMY, (KEYWORD_KIND)t.number, idx)) throw 0;
                }
                else {
                    switch (t.number) {
                    case KEY_CONST:
                        if (idx + 2 < tokens.size() && tokens[idx + 1].kind == TOKEN_IDENTIFIER) {
                            idx++;
                            Token tmp = { TOKEN_PROC, tokens[idx + 1].line };
                            const std::string& str = tokens[idx].pStr;
                            idx++;
                            switch (tokens[idx].kind) {
                            case TOKEN_STRING: tmp.kind = TOKEN_STRING; tmp.pStr = tokens[idx].pStr; break;
                            default: tmp.kind = TOKEN_NUMBER; if (!CalcConvertConst(tokens, pProcessedData, idx, KEY_GLOBAL, const_map, tmp.number)) throw 2; break;
                            }
                            const_map.insert({ str, tmp });
                        }
                        else throw 0;
                        break;
                    default:
                        throw 0;
                    }
                }
                break;
            case TOKEN_ENDBLOCK:
                idx++;
                if (exit_type != 1) {
                    if (last_cmd != SCR_EXIT) {
                        Token cmd;
                        cmd.kind = TOKEN_COMMAND;
                        cmd.number = (exit_type == 0) ? SCR_EXIT : SCR_RET;

                        Token cnt;
                        cnt.kind = TOKEN_INCOUNT;
                        cnt.number = !(exit_type == 0);
                        pProcessedData->emplace_back(cmd);
                        pProcessedData->emplace_back(cnt);
                        if (exit_type == 2) {
                            Token rt;
                            rt.kind = TOKEN_NUMBER;
                            rt.number = 0;
                            rt.advance = 1;
                            pProcessedData->emplace_back(rt);
                        }
                    }
                    Token end;
                    end.kind = TOKEN_ENDPROC;
                    pProcessedData->emplace_back(end);
                    
                }
                on_exit = true;
                break;
            default:
                break;
            }
            if (on_exit) break;
        }
    }
    catch(...){
        success = false;
    }
    return success;
}

bool ProcessTSetBlock(
    std::vector<Token>& tokens,
    std::vector<Token>* pProcessedData, 
    constant_map& const_map, 
    size_t& idx,
    int exit_type
)
{
    bool success = true;
    SCL_INSTRUCTION last_cmd = SCR_NOP;
    const size_t size = tokens.size();

    try {
        bool on_exit = false;
        for (; idx < size; ) {
            const Token& t = tokens[idx];
            switch (t.kind) {
                //Just comands
            case TOKEN_COMMAND: {
                bool found_command = false;
                for (auto c : g_TSetCheck) if (c == t.number) found_command = true;
                for (auto c : g_ControlFlow) if (c == t.number) found_command = true;
                if (found_command) {
                    if (!ProcessCommonCmdData(tokens, pProcessedData, const_map, (SCL_INSTRUCTION)t.number, last_cmd, KEY_TSET, idx)) throw 0;
                }
                else throw t;
            }    break;
                              //For normal math expressions
            case TOKEN_IDENTIFIER: {
                if (idx + 1 < size && tokens[idx + 1].kind == TOKEN_DOTS) {
                    Token tmp{ TOKEN_LABEL, t.line, t.pStr };
                    pProcessedData->emplace_back(tmp);
                    idx += 2;
                }
                else if (!CalcConvert(tokens, pProcessedData, idx, KEY_TSET, const_map, true)) throw 0;
            } break;
                                 //Const for local constants
            case TOKEN_KEYWORD:
                if (t.number == KEY_IF || t.number == KEY_WHILE || t.number == KEY_LOOP) {
                    if (!ProcessConditionOrLoopBlock(tokens, pProcessedData, const_map, KEY_TSET, (KEYWORD_KIND)t.number, idx)) throw 0;
                }
                else {
                    switch (t.number) {
                    case KEY_CONST:
                        if (idx + 2 < tokens.size() && tokens[idx + 1].kind == TOKEN_IDENTIFIER) {
                            idx++;
                            Token tmp = { TOKEN_PROC, tokens[idx + 1].line };
                            const std::string& str = tokens[idx].pStr;
                            idx++;
                            switch (tokens[idx].kind) {
                            case TOKEN_STRING: tmp.kind = TOKEN_STRING; tmp.pStr = tokens[idx].pStr; break;
                            default: tmp.kind = TOKEN_NUMBER; if (!CalcConvertConst(tokens, pProcessedData, idx, KEY_GLOBAL, const_map, tmp.number)) throw 2; break;
                            }
                            const_map.insert({ str, tmp });
                        }
                        else throw 0;
                        break;
                    default:
                        throw 0;
                    }
                }
                break;
            case TOKEN_ENDBLOCK:
                idx++;
                if(exit_type != 1){
                    if (last_cmd != SCR_EXIT) {
                        Token cmd;
                        cmd.kind = TOKEN_COMMAND;
                        cmd.number = SCR_EXIT;

                        Token cnt;
                        cnt.kind = TOKEN_INCOUNT;
                        cnt.number = 0;
                        pProcessedData->emplace_back(cmd);
                        pProcessedData->emplace_back(cnt);
                    }
                    Token end;
                    end.kind = TOKEN_ENDPROC;
                    pProcessedData->emplace_back(end);

                }
                on_exit = true;
                break;
            default:
                break;
            }
            if (on_exit) break;
        }
    }
    catch (...) {
        success = false;
    }
    return success;
}

bool ProcessExAnmBlock(
    std::vector<Token>& tokens, 
    std::vector<Token>* pProcessedData,
    constant_map& const_map, 
    size_t& idx,
    int exit_type
)
{
    bool success = true;
    SCL_INSTRUCTION last_cmd = SCR_NOP;
    const size_t size = tokens.size();

    try {
        bool on_exit = false;
        for (; idx < size; ) {
            const Token& t = tokens[idx];
            switch (t.kind) {
                //Just comands
            case TOKEN_COMMAND: {
                bool found_command = false;
                for (auto c : g_ExAnmCheck) if (c == t.number) found_command = true;
                for (auto c : g_ControlFlow) if (c == t.number) found_command = true;
                if (found_command) {
                    if (!ProcessCommonCmdData(tokens, pProcessedData, const_map, (SCL_INSTRUCTION)t.number, last_cmd, KEY_EXANM, idx)) throw 0;
                }
                else throw t;
            }    break;
                              //For normal math expressions
            case TOKEN_IDENTIFIER: {
                if (idx + 1 < size && tokens[idx + 1].kind == TOKEN_DOTS) {
                    Token tmp{ TOKEN_LABEL, t.line, t.pStr };
                    pProcessedData->emplace_back(tmp);
                    idx += 2;
                }
                else if (!CalcConvert(tokens, pProcessedData, idx, KEY_EXANM, const_map, true)) throw 0;
            } break;
                                 //Const for local constants
            case TOKEN_KEYWORD:
                if (t.number == KEY_IF || t.number == KEY_WHILE || t.number == KEY_LOOP) {
                    if (!ProcessConditionOrLoopBlock(tokens, pProcessedData, const_map, KEY_EXANM, (KEYWORD_KIND)t.number, idx)) throw 0;
                }
                else {
                    switch (t.number) {
                    case KEY_CONST:
                        if (idx + 2 < tokens.size() && tokens[idx + 1].kind == TOKEN_IDENTIFIER) {
                            idx++;
                            Token tmp = { TOKEN_PROC, tokens[idx + 1].line };
                            const std::string& str = tokens[idx].pStr;
                            idx++;
                            switch (tokens[idx].kind) {
                            case TOKEN_STRING: tmp.kind = TOKEN_STRING; tmp.pStr = tokens[idx].pStr; break;
                            default: tmp.kind = TOKEN_NUMBER; if (!CalcConvertConst(tokens, pProcessedData, idx, KEY_GLOBAL, const_map, tmp.number)) throw 2; break;
                            }
                            const_map.insert({ str, tmp });
                        }
                        else throw 0;
                        break;
                    default:
                        throw 0;
                    }
                }
                break;
            case TOKEN_ENDBLOCK:
                idx++; 
                if (exit_type != 1) {
                    if (last_cmd != SCR_EXIT) {
                        Token cmd;
                        cmd.kind = TOKEN_COMMAND;
                        cmd.number = SCR_EXIT;

                        Token cnt;
                        cnt.kind = TOKEN_INCOUNT;
                        cnt.number = 0;
                        pProcessedData->emplace_back(cmd);
                        pProcessedData->emplace_back(cnt);

                    }

                    Token end;
                    end.kind = TOKEN_ENDPROC;
                    pProcessedData->emplace_back(end);
                }
                on_exit = true;
                break;
            default:
                break;
            }
            if (on_exit) break;
        }
    }
    catch (...) {
        success = false;
    }
    return success;
}

bool ProcessSetBlock(
    std::vector<Token>& tokens,
    std::vector<Token>* pProcessedData,
    constant_map& const_map,
    size_t& idx
)
{
    bool success = true;
    SCL_INSTRUCTION last_cmd = SCR_NOP;
    const size_t size = tokens.size();

    try {
        bool on_exit = false;
        for (; idx < size; ) {
            const Token& t = tokens[idx];
            switch (t.kind) {
                //Just comands
            case TOKEN_COMMAND: {
                bool found_command = false;
                for (auto c : g_SetCheck) if (c == t.number) found_command = true;
                if (found_command) {
                    if (!ProcessCommonCmdData(tokens, pProcessedData, const_map, (SCL_INSTRUCTION)t.number, last_cmd, KEY_SET, idx)) throw 0;
                }
                else throw t;
            }    break;
                              //Const for local constants
            case TOKEN_KEYWORD:
                switch (t.number) {
                case KEY_CONST:
                    if (idx + 2 < tokens.size() && tokens[idx + 1].kind == TOKEN_IDENTIFIER) {
                        idx++;
                        Token tmp = { TOKEN_PROC, tokens[idx + 1].line };
                        const std::string& str = tokens[idx].pStr;
                        idx++;
                        switch (tokens[idx].kind) {
                        case TOKEN_STRING: tmp.kind = TOKEN_STRING; tmp.pStr = tokens[idx].pStr; break;
                        default: tmp.kind = TOKEN_NUMBER; if (!CalcConvertConst(tokens, pProcessedData, idx, KEY_GLOBAL, const_map, tmp.number)) throw 2; break;
                        }
                        const_map.insert({ str, tmp });
                    }
                    else throw 0;
                    break;
                default:
                    throw 0;
                }
                break;
            case TOKEN_ENDBLOCK:
            {
                idx++;
                if (last_cmd != SCR_EXIT) {
                    Token cmd;
                    cmd.kind = TOKEN_COMMAND;
                    cmd.number = SCR_EXIT;

                    Token cnt;
                    cnt.kind = TOKEN_INCOUNT;
                    cnt.number = 0;
                    pProcessedData->emplace_back(cmd);
                    pProcessedData->emplace_back(cnt);
                }
                Token end;
                end.kind = TOKEN_ENDPROC;
                pProcessedData->emplace_back(end);

                on_exit = true;
            }
            break;
            default:
                throw 0;
                break;
            }
            if (on_exit) break;
        }
    }
    catch (...) {
        success = false;
    }
    return success;
}

bool ProcessExtraTexBlock(
    std::vector<Token>& tokens,
    std::vector<Token>* pProcessedData, 
    constant_map& const_map, 
    size_t& idx
)
{
    bool success = true;
    SCL_INSTRUCTION last_cmd = SCR_NOP;
    const size_t size = tokens.size();

    try {
        bool on_exit = false;
        for (; idx < size; ) {
            const Token& t = tokens[idx];
            switch (t.kind) {
                //Just comands
            case TOKEN_COMMAND: {
                bool found_command = false;
                for (auto c : g_ExtraTexCheck) if (c == t.number) found_command = true;
                if (found_command) {
                    if (!ProcessCommonCmdData(tokens, pProcessedData, const_map, (SCL_INSTRUCTION)t.number, last_cmd, KEY_EXTRATEX, idx)) throw 0;
                }
                else throw t;
            }    break;
                                 //Const for local constants
            case TOKEN_KEYWORD:
                    switch (t.number) {
                    case KEY_CONST:
                        if (idx + 2 < tokens.size() && tokens[idx + 1].kind == TOKEN_IDENTIFIER) {
                            idx++;
                            Token tmp = { TOKEN_PROC, tokens[idx + 1].line };
                            const std::string& str = tokens[idx].pStr;
                            idx++;
                            switch (tokens[idx].kind) {
                            case TOKEN_STRING: tmp.kind = TOKEN_STRING; tmp.pStr = tokens[idx].pStr; break;
                            default: tmp.kind = TOKEN_NUMBER; if (!CalcConvertConst(tokens, pProcessedData, idx, KEY_GLOBAL, const_map, tmp.number)) throw 2; break;
                            }
                            const_map.insert({ str, tmp });
                        }
                        else throw 0;
                        break;
                    default:
                        throw 0;
                    }
                break;
            case TOKEN_ENDBLOCK:
            {
                idx++;
                if (last_cmd != SCR_EXIT) {
                    Token cmd;
                    cmd.kind = TOKEN_COMMAND;
                    cmd.number = SCR_EXIT;

                    Token cnt;
                    cnt.kind = TOKEN_INCOUNT;
                    cnt.number = 0;
                    pProcessedData->emplace_back(cmd);
                    pProcessedData->emplace_back(cnt);
                }
                Token end;
                end.kind = TOKEN_ENDPROC;
                pProcessedData->emplace_back(end);

                on_exit = true;
            }
                break;
            default:
                throw 0;
                break;
            }
            if (on_exit) break;
        }
    }
    catch (...) {
        success = false;
    }
    return success;
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
            if (tokens[i].number == KEY_TEXINIT) {
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
                                if (tokens[i].advance > 100 || tokens[i].advance < 0) {
                                    __debugbreak();
                                    auto& debb = tokens[i];
                                    while (1) {
                                        ;
                                    }
                                }
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
    if (!header.is_open()) return false;
    nlohmann::json jheader = nlohmann::json::parse(header);
    if (jheader.find("TexInit") != jheader.end()) {
        try {
            const std::string& k = jheader["TexInit"];
            if (pProcData.find(k) != pProcData.end()) {
                address ads = pProcData.at(k).ads;
                pHeader->TexInitializer = ads;
            }
            else {
                printf("TexInit procedure \"%s\" not found\n", k.c_str());
                raise_error = true;
            }
        }
        catch (...) {
            printf("Argument for TexInit must be an String.\n");
        }
    }
    else {
        printf("TexInit key not found\n");
        raise_error = true;
    }
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
