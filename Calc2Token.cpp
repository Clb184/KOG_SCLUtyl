#include "Calc2Token.h"

bool VerifyCalcSyntax(
	std::vector<Token>& from,
	std::vector<Token>* to,
	size_t& idx,
	KEYWORD_KIND proc,
    constant_map& const_map,
    bool constant_exp,
    int num_args
)
{
    bool success = false;
    std::deque<Token> toks;
    int receiver = -1;
    if (Calc2Token(from, toks, idx, proc, const_map, constant_exp, receiver, num_args)) {
        if(PresolveTokens(toks))
            if(Token2ProcessedData(to, toks, receiver))
                success = true;
    }
    return success;
}

bool Calc2Token(
    std::vector<Token>& from, 
    std::deque<Token>& to, 
    size_t& idx, 
    KEYWORD_KIND proc, 
    constant_map& const_map, 
    bool constant_exp,
    int& receiver,
    int num_args
)
{
    bool success = true;
    size_t i = idx;
    int proc_kind = proc;

    //Store the result to analyze later
    int fn = -1;
    std::deque<Token> nums;
    std::stack<Token> syms;

    int args = 1;

    TOKEN_KIND expected_tokens[8] = { TOKEN_IDENTIFIER, TOKEN_ARITHF, TOKEN_ARITHS, TOKEN_NUMBER, TOKEN_IGNORE };
    Token dummy_token = { TOKEN_IGNORE };
    Token guide_token = dummy_token;
    int num_expected = 4;
    bool exit_op = false;
    int nnums = 0;
    int nsyms = 0;
    bool negate = false;

    try {
        for (; i < from.size(); i++) {
            auto& t = from[i];
            for (int s = 0; s <= num_expected; s++) {
                if (expected_tokens[s] == t.kind) break;
            }
            switch (t.kind) {
            case TOKEN_IDENTIFIER:
                if (guide_token.kind == TOKEN_NUMBER || guide_token.kind == TOKEN_REGISTER) {
                    exit_op = true;
                    break;
                }

            {
                bool found_identifier = false;
                switch (proc_kind) {
                case KEY_PROC:
                case KEY_ENEMYPROC:
                    if (g_EnemyRegs.find(t.pStr) != g_EnemyRegs.end()) {
                        t.number = g_EnemyRegs[t.pStr];
                        found_identifier = true;
                    }
                    break;
                case KEY_ATKPROC:
                    if (g_AtkRegs.find(t.pStr) != g_AtkRegs.end()) {
                        t.number = g_AtkRegs[t.pStr];
                        found_identifier = true;
                    }
                    break;
                case KEY_EXANMPROC:
                    if (g_ExAnmRegs.find(t.pStr) != g_ExAnmRegs.end()) {
                        t.number = g_ExAnmRegs[t.pStr];
                        found_identifier = true;
                    }
                    break;
                }
                if (!found_identifier) {
                    if (const_map.find(t.pStr) != const_map.end()) {
                        auto& tkk = const_map[t.pStr];
                        t = tkk;
                        nums.push_back(t);
                        nnums++;
                        expected_tokens[0] = TOKEN_ARITHS;
                        expected_tokens[1] = TOKEN_COMMA;
                        num_expected = 2;
                        guide_token = t;
                    }
                    else throw t;
                }
                else {
                    if (constant_exp) throw t;
                    t.kind = TOKEN_REGISTER;
                    nums.push_back(t);
                    nnums++;
                    expected_tokens[0] = TOKEN_ARITHS;
                    expected_tokens[1] = TOKEN_COMMA;
                    num_expected = 2;
                    guide_token = t;
                }
            }
            break;
            case TOKEN_ARITHF: {

                    guide_token = t;
                    expected_tokens[0] = TOKEN_ARITHC;
                    num_expected = 1;
                    guide_token = dummy_token;
                    fn = t.number;
                    switch (t.number) {
                    case AF_COSL:
                    case AF_SINL:
                    case AF_ATAN:
                    case AF_MIN:
                    case AF_MAX:
                        args = 2; break;
                    case AF_RND:
                        args = 1; break;
                    }
            } break;
            case TOKEN_ARITHC:
                if (t.number == AC_PE) {
                    if (Calc2Token(from, nums, ++i, proc, const_map, constant_exp, receiver, args)) {
                        if (from[i].number != AC_PL) throw from[i];
                        else {
                            if (fn != -1) {
                                Token cmd2;
                                cmd2.kind = TOKEN_COMMAND;
                                switch (fn) {
                                case AF_COSL: cmd2.number = SCR_COSL; break;
                                case AF_SINL: cmd2.number = SCR_SINL; break;
                                case AF_MIN: cmd2.number = SCR_MIN; break;
                                case AF_MAX: cmd2.number = SCR_MAX; break;
                                case AF_ATAN: cmd2.number = SCR_ATAN; break;
                                case AF_RND: cmd2.number = SCR_RND; break;
                                }
                                nums.push_back(cmd2);
                                fn = -1;
                            }
                            nnums++;
                            expected_tokens[0] = TOKEN_ARITHS;
                            expected_tokens[1] = TOKEN_COMMA;
                            num_expected = 2;
                            Token exm;
                            exm.kind = TOKEN_NUMBER;
                            guide_token = exm;
                        }
                    }
                    else throw t;

                }
                else if (t.number == AC_PL) {
                    if (num_args != 1) throw t;
                    idx = i;
                    exit_op = true;
                    break;
                }
                else throw t;

                if (guide_token.kind == TOKEN_ARITHF) {

                }
                break;
            case TOKEN_ARITHS:
                if (t.number == AS_ASSIGN) {
                    if (receiver == -1) {
                        receiver = (nums.end() - 1)->number;
                        nums.pop_back();
                    }
                    else throw t;
                }
                else {
                    switch (t.number) {
                    case AS_SUB:
                        if (nnums == nsyms) {
                            Token ng;
                            ng.kind = TOKEN_COMMAND;
                            ng.number = SCR_NEG;
                            syms.push(ng);
                            break;
                        }
                    case AS_ADD:
                    case AS_EQU:
                    case AS_NOTEQU:
                    case AS_LESS:
                    case AS_LESSEQ:
                    case AS_GREAT:
                    case AS_GREATEQ:
                        while(!syms.empty()) {
                            nums.emplace_back(syms.top());
                            syms.pop();
                        }
                    case AS_AND:
                    case AS_OR:
                    case AS_MUL:
                    case AS_DIV:
                    case AS_MOD:
                        syms.push(t);
                        nsyms++;
                        break;
                    default:
                        break;
                    }
                }
                expected_tokens[0] = TOKEN_ARITHC;
                expected_tokens[1] = TOKEN_ARITHF;
                expected_tokens[2] = TOKEN_NUMBER;
                expected_tokens[3] = TOKEN_IDENTIFIER;
                num_expected = 4;
                guide_token = dummy_token;
                break;
            case TOKEN_NUMBER:
                expected_tokens[0] = TOKEN_ARITHS;
                expected_tokens[1] = TOKEN_COMMA;
                num_expected = 2;
                nums.emplace_back(t);
                nnums++;
                guide_token = t;
                break;
            case TOKEN_COMMA:
                if (num_args > 1) {
                    num_args--;
                    to.insert(to.begin() + to.size(), nums.begin(), nums.end());
                    nums.clear();
                    while (!syms.empty()) {
                        to.emplace_back(syms.top());
                        syms.pop();
                    }
                    nnums = 0;
                    nsyms = 0;
                    expected_tokens[0] = TOKEN_ARITHC;
                    expected_tokens[1] = TOKEN_ARITHF;
                    expected_tokens[2] = TOKEN_NUMBER;
                    expected_tokens[3] = TOKEN_IDENTIFIER;
                    num_expected = 4;
                    guide_token = dummy_token;
                }
                else throw t;
                break;
            default:
                if (num_args != 1) throw t;
                exit_op = true;

            }
            if (exit_op) {
                to.insert(to.begin() + to.size(), nums.begin(), nums.end());
                while (!syms.empty()) {
                    to.emplace_back(syms.top());
                    syms.pop();
                }
                idx = i;
                break;
            }
        }
    }
    catch (...) {
        printf("Calc convert Error: \n");
        success = false;
    }
    if (success) {

    }
    return success;
}

bool PresolveTokens(std::deque<Token>& toks) {
    bool success = true;
    int size = toks.size();

    std::stack<Token> stk;
    std::deque<Token> res;
    try {
        for (auto& t : toks) {
            switch (t.kind) {
            case TOKEN_REGISTER:
            case TOKEN_NUMBER:
                stk.push(t);
                break;
            case TOKEN_ARITHS:
                {
                    bool solved = false;
                    Token p1 = stk.top();
                    stk.pop();
                    if (stk.top().kind == p1.kind && p1.kind == TOKEN_NUMBER) {
                        switch (t.number) {
                            case AS_ADD: stk.top().number += p1.number; break;
                            case AS_SUB: stk.top().number -= p1.number; break;
                            case AS_MUL: stk.top().number *= p1.number; break;
                            case AS_DIV: stk.top().number /= p1.number; break;
                            case AS_MOD: stk.top().number %= p1.number; break;
                        }
                    }
                    else {
                        while (!stk.empty()) {
                            res.push_back(stk.top());
                            stk.pop();
                        }
                        res.push_back(p1);
                        res.push_back(t);
                    }
                }
                break;
            case TOKEN_COMMAND:
            {

                switch (t.number) {
                case SCR_SINL:
                case SCR_COSL:
                case SCR_MIN:
                case SCR_MAX:
                case SCR_ATAN: {
                    Token p1 = stk.top();
                    stk.pop();
                    while (!stk.empty()) {
                        res.push_back(stk.top());
                        stk.pop();
                    }
                    res.push_back(p1);
                    res.push_back(t);
                }break;
                case SCR_RND:
                    while (!stk.empty()) {
                        res.push_back(stk.top());
                        stk.pop();
                    }
                    res.push_back(t);
                    break;
                case SCR_NEG:
                    if (stk.top().kind == TOKEN_NUMBER) {
                        Token& tk = stk.top();
                        tk.number = -tk.number;
                    }
                    else {
                        while (!stk.empty()) {
                            res.push_back(stk.top());
                            stk.pop();
                        }
                        res.push_back(t);
                    }
                    break;
                }
            }
            break;
            }
        }
    }
    catch (...) {
        success = false;
    }
    if (success) {
        toks = res;
    }

    return success;
}

bool Token2ProcessedData(std::vector<Token>* to, const std::deque<Token>& toks, int reg) {
    bool success = true;
    if (toks.size() == 1 && toks[0].kind == TOKEN_NUMBER && reg != -1) {
        Token cmd;
        cmd.kind = TOKEN_COMMAND;
        cmd.number = SCR_MOVC;
        Token cnt;
        cnt.kind = TOKEN_INCOUNT;
        cnt.number = 2;
        to->emplace_back(cmd);
        to->emplace_back(cnt);
        Token rg;
        rg.kind = TOKEN_NUMBER;
        rg.number = reg;
        rg.advance = 1;
        to->emplace_back(rg);
        Token num;
        num.kind = TOKEN_NUMBER;
        num.number = toks[0].number;
        num.advance = 1;
        to->emplace_back(num);
    }

    size_t size = toks.size();
    for (int i = 0; i < size; i++) {
        const auto& tok = toks[i];
        switch (tok.kind) {
        case TOKEN_NUMBER: {
            Token cmd;
            cmd.kind = TOKEN_COMMAND;
            cmd.number = SCR_PUSHC;

            Token cnt;
            cnt.kind = TOKEN_INCOUNT;
            cnt.number = 1;

            Token n = tok;
            n.advance = 4;

            printf("Num: %d\n", n.number);
            to->emplace_back(cmd);
            to->emplace_back(cnt);
            to->emplace_back(n);
        }break;
        case TOKEN_REGISTER:
        {
            Token cmd;
            cmd.kind = TOKEN_COMMAND;
            cmd.number = SCR_PUSHR;

            Token cnt;
            cnt.kind = TOKEN_INCOUNT;
            cnt.number = 1;

            Token n = tok;
            n.kind = TOKEN_NUMBER;
            n.advance = 4;

            printf("Reg: %s\n", n.pStr.c_str());
            to->emplace_back(cmd);
            to->emplace_back(cnt);
            to->emplace_back(n);
        }
            break;
        case TOKEN_ARITHS: {

            Token cmd;
            cmd.kind = TOKEN_COMMAND;
            Token cnt;
            cnt.kind = TOKEN_INCOUNT;
            cnt.number = 0;
            
            switch (tok.number) {
            case AS_ADD: cmd.number = SCR_ADD;
                printf("+\n");
                break;
            case AS_SUB: cmd.number = SCR_SUB;
                printf("-\n");
                break;
            case AS_MUL: cmd.number = SCR_MUL;
                printf("*\n");
                break;
            case AS_DIV: cmd.number = SCR_DIV;
                printf("/\n");
                break;
            case AS_MOD: cmd.number = SCR_MOD;
                printf("%\n");
                break;

            case AS_EQU: cmd.number = SCR_EQUAL;
                printf("==\n");
                break;
            case AS_NOTEQU: cmd.number = SCR_NOTEQ;
                printf("!=\n");
                break;
            case AS_LESS: cmd.number = SCR_LESS;
                printf("<\n");
                break;
            case AS_LESSEQ: cmd.number = SCR_LESSEQ;
                printf("<=\n");
                break;
            case AS_GREAT: cmd.number = SCR_ABOVE;
                printf(">\n");
                break;
            case AS_GREATEQ: cmd.number = SCR_ABOVEEQ;
                printf(">=\n");
                break;
            }

            to->emplace_back(cmd);
            to->emplace_back(cnt);
        }
        break;
        case TOKEN_COMMAND: {
            Token cmd = tok;
            Token cnt;
            cnt.kind = TOKEN_INCOUNT;
            cnt.number = 0;
            switch (cmd.number) {
            case SCR_SINL:
                printf("sinl\n");
                break;
            case SCR_COSL:
                printf("cosl\n");
                break;
            case SCR_MIN:
                printf("min\n");
                break;
            case SCR_MAX:
                printf("max\n");
                break;
            case SCR_ATAN:
                printf("atan\n");
                break;
            case SCR_RND:
                printf("rnd\n");
                break;
            case SCR_NEG:
                printf("neg\n");
                break;
            }
            to->emplace_back(cmd);
            to->emplace_back(cnt);
        }break;
        }
    }
    if (reg != -1) {
        Token cmd;
        cmd.kind = TOKEN_COMMAND;
        cmd.number = SCR_POPR;
        Token cnt;
        cnt.kind = TOKEN_INCOUNT;
        cnt.number = 1;
        to->emplace_back(cmd);
        to->emplace_back(cnt);
        Token rg;
        rg.kind = TOKEN_NUMBER;
        rg.number = reg;
        rg.advance = 1;
        to->emplace_back(rg);
    }

    return success;
}
