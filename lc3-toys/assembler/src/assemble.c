#include "common.h"

// finds pseudo-op .ORIG and returns the first address of PC counter
static void find_orig(FileList *fl, InputBuf *ibuf, OutputBuf *obuf, uint32_t *p_lidx) {
    static char line_str[MAX_LINE_LEN + 1];
    int8_t is_halted = 0;

    for (; fgets(line_str, MAX_LINE_LEN + 1, fl->src); *p_lidx += 1) {
        Line *line = line_new(*p_lidx, line_str);

        if (line->tokcnt == 0) {
            // do nothing
        } else if (line->tokens[0]->type != TK_PSEUDO || line->tokens[0]->subtype != PS_ORIG) {
            set_warning(*p_lidx, "Instruction ignored: %s", line_str);
        } else if (line->tokcnt < 2) {
            set_error(*p_lidx, "No literal operand found for .ORIG");
            is_halted = -1;
        } else if (line->tokens[1]->type != TK_LITERAL) {
            set_error(*p_lidx, "%s is not a literal operand for .ORIG", line->tokens[1]->str);
            is_halted = -1;
        } else {
            uint16_t orig = parse_literal(line->tokens[1]);
            outputbuf_add(obuf, 0, orig, *p_lidx);
            *p_lidx += 1;
            is_halted = 1;
        }

        line_del(line);

        if (is_halted)
            break;
    }

    if (is_halted == -1)
        set_error(0, ".ORIG not found");
}

// checks for possible symbol label and adds to symbol table
static void check_symbol(SymTab *st, Token *tok, uint16_t *p_addr) {
    uint32_t lidx = tok->lidx;
    Symbol *dupe = symtab_find_by_value(st, *p_addr);

    if (dupe == NULL)
        symtab_add(st, tok, *p_addr);
    else
        set_warning(lidx, "Another symbol is pointing to address %04x: %s",
                    *p_addr, dupe->key->str);
}

// actually checks for number of tokens aside from symbol and opcode
// so there's no type checking
static void check_operand_token_count(Token *optok, uint8_t tokcnt_left) {
    static const uint8_t opcode_operand_cnt[] = {
    //  BR    ADD   LD    ST    JSR   AND   LDR
        1,    3,    2,    2,    1,    3,    3,
    //  STR   RTI   NOT   LDI   STI   JMP   RES
        3,    0,    2,    2,    2,    1,    0,
    //  LEA   TRAP  TRAPS JSRR  RET
        2,    1,    0,    1,    0
    };

    static const uint8_t pseudo_operand_cnt[] = {
    //  ORIG  END   BLKW  FILL  STRINGZ
        1,    0,    1,    1,    1
    };

    uint32_t lidx = optok->lidx;
    uint8_t opr_exp = optok->type == TK_OPCODE ? opcode_operand_cnt[optok->subtype]
                                               : pseudo_operand_cnt[optok->subtype];

    if (tokcnt_left > opr_exp)
        set_warning(lidx, "%s requires %d operand(s), %d found",
                    optok->str, opr_exp, tokcnt_left);
    else if (tokcnt_left < opr_exp)
        set_error(lidx, "%s requires %d operand(s) %d found", optok->str, opr_exp, tokcnt_left);
}

// checks type of operands in instructions with opcode
static void check_operand_type_opcode(Token **toks) {
    uint32_t lidx = toks[0]->lidx;
    switch (toks[0]->subtype) {
    case OP_BR:
    case OP_JSR:
        if (toks[1]->type != TK_SYMBOL && toks[1]->type != TK_LITERAL)
            set_error(lidx, "%s is not a symbol or register", toks[1]->str);
        break;

    case OP_ADD:
    case OP_AND:
        if (toks[1]->type != TK_REGISTER)
            set_error(lidx, "%s is not a register", toks[1]->str);
        if (toks[2]->type != TK_REGISTER)
            set_error(lidx, "%s is not a register", toks[2]->str);
        if (toks[3]->type != TK_REGISTER && toks[3]->type != TK_LITERAL)
            set_error(lidx, "%s is not a register or literal", toks[3]->str);
        break;

    case OP_LD:
    case OP_ST:
    case OP_LDI:
    case OP_STI:
    case OP_LEA:
        if (toks[1]->type != TK_REGISTER)
            set_error(lidx, "%s is not a register", toks[1]->str);
        if (toks[2]->type != TK_LITERAL && toks[2]->type != TK_SYMBOL)
            set_error(lidx, "%s is not a literal or symbol", toks[2]->str);
        break;

    case OP_LDR:
    case OP_STR:
        if (toks[1]->type != TK_REGISTER)
            set_error(lidx, "%s is not a register", toks[1]->str);
        if (toks[2]->type != TK_REGISTER)
            set_error(lidx, "%s is not a register", toks[2]->str);
        if (toks[3]->type != TK_LITERAL)
            set_error(lidx, "%s is not a literal", toks[3]->str);
        break;

    case OP_NOT:
        if (toks[1]->type != TK_REGISTER)
            set_error(lidx, "%s is not a register", toks[1]->str);
        if (toks[2]->type != TK_REGISTER)
            set_error(lidx, "%s is not a register", toks[2]->str);
        break;

    case OP_JMP:
    case OP_JSRR:
        if (toks[1]->type != TK_REGISTER)
            set_error(lidx, "%s is not a register", toks[1]->str);
        break;

    case OP_TRAP:
        if (toks[1]->type != TK_LITERAL)
            set_error(lidx, "%s is not a literal", toks[1]->str);
        break;

    case OP_RET:
    case OP_TRAPS:
    case OP_RTI:
        break;

    default:
        set_error(lidx, "Unused opcode %s", toks[0]->str);
    }
}

// checks type of operands in instructions with pseudo-op
static uint16_t check_operand_type_pseudo(Token **toks) {
    Token *opr = toks[1];
    uint32_t lidx = opr->lidx;

    switch (toks[0]->subtype) {
    case PS_BLKW:
        if (opr->type != TK_LITERAL) {
            set_error(lidx, "%s is not a literal", opr->str);
            return 0;
        }
        return parse_literal(opr);
    case PS_FILL:
        if (opr->type != TK_LITERAL && opr->type != TK_SYMBOL) {
            set_error(lidx, "%s is not a literal or symbol", opr->str);
            return 0;
        }
        return 1;
    case PS_STRINGZ:
        if (toks[1]->type != TK_STR) {
            set_error(lidx, "%s is not a string literal", opr->str);
            return 0;
        }
        return opr->size;
    default:
        return 0;
    }
}

// checks line for symbol, validates opcode, pseudo-op, operand count and types
static uint8_t validate_line(InputBuf *ibuf, SymTab *st, Line *line, uint16_t *p_addr) {
    uint8_t cur_tokcnt = 0;

    // check symbol
    if (line->tokens[cur_tokcnt]->type == TK_SYMBOL) {
        check_symbol(st, line->tokens[cur_tokcnt], p_addr);
        if (line->tokcnt == 1) {
            line_del(line);
            return 0;
        }
        cur_tokcnt++;
    }

    // skip the symbol token, move to the main instruction
    Token **toks = line->tokens + cur_tokcnt;
    Token *optok = toks[0];

    uint32_t lidx = optok->lidx;

    // check .end and repeated .orig
    if (optok->type == TK_PSEUDO) {
        switch (optok->subtype) {
        case PS_ORIG:
            set_error(lidx, ".ORIG is already used");
            line_del(line);
            return 0;
        case PS_END:
            line_del(line);
            return 1;
        }
    }

    // check instruction
    if (optok->type != TK_OPCODE && optok->type != TK_PSEUDO) {
        set_error(lidx, "%s is not a valid opcode or pseudo-op", optok->str);
    } else {
        cur_tokcnt++;

        uint8_t tokcnt_left = line->tokcnt - cur_tokcnt;
        check_operand_token_count(optok, tokcnt_left);

        if (optok->type == TK_OPCODE) {
            check_operand_type_opcode(toks);
            *p_addr += 1;
        } else {
            *p_addr += check_operand_type_pseudo(toks);
        }
    }

    inputbuf_add(ibuf, line);
    return 0;
}

// reads lines from assembly file and check for symbols, opcodes, operand count and
// operand type then stores lines in input buffer and symbols in symbol table
static void pass_one(InputBuf *ibuf, OutputBuf *obuf, SymTab *st,
                     FileList *fl, uint32_t *p_lidx) {
    static char line_str[MAX_LINE_LEN + 1];
    uint8_t end_found = 0;
    uint16_t addr = obuf->instr[0];

    for (; fgets(line_str, MAX_LINE_LEN + 1, fl->src) != NULL; *p_lidx += 1) {
        Line *line = line_new(*p_lidx, line_str);
        if (line->tokcnt == 0) {
            line_del(line);
            continue;
        }

        if ((end_found = validate_line(ibuf, st, line, &addr)))
            break;
    }

    if (!end_found)
        set_warning(0, "End of file reached, .END not found");
}

// includes opcode into machine code
static void add_opcode(uint16_t *p_instr, Token *tok) {
    static const uint8_t opcode_val[] = {
    //  BR    ADD   LD    ST    JSR   AND   LDR
        0,    1,    2,    3,    4,    5,    6,
    //  STR   RTI   NOT   LDI   STI   JMP   RES
        7,    8,    9,    10,   11,   12,   13,
    //  LEA   TRAP  TRAPS JSRR  RET
        14,   15,   15,   4,    12
    };
    *p_instr |= opcode_val[tok->subtype] << 12;
}

// check if number can be represented in a specific number of bits
static uint8_t is_fit_5bit(uint16_t x) {
    return (x >= 0 && x <= 31) || (x >= UINT16_MAX - 30 && x <= UINT16_MAX);
}

static uint8_t is_fit_6bit(uint16_t x) {
    return (x >= 0 && x <= 63) || (x >= UINT16_MAX - 62 && x <= UINT16_MAX);
}

static uint8_t is_fit_9bit(uint16_t x) {
    return (x >= 0 && x <= 511) || (x >= UINT16_MAX - 510 && x <= UINT16_MAX);
}

static uint8_t is_fit_11bit(uint16_t x) {
    return (x >= 0 && x <= 2047) || (x >= UINT16_MAX - 2046 && x <= UINT16_MAX);
}

// add operand helpers
static void add_cond(uint16_t *p_instr, Token *tok) {
    *p_instr |= get_branch_type(tok) << 9;
}

static void add_dstsrc(uint16_t *p_instr, Token *tok) {
    *p_instr |= tok->subtype << 9;
}

static void add_src1_direct(uint16_t *p_instr, uint8_t reg) {
    *p_instr |= reg << 6;
}

static void add_src1(uint16_t *p_instr, Token *tok) {
    add_src1_direct(p_instr, tok->subtype);
}

static void add_src2(uint16_t *p_instr, Token *tok) {
    if (tok->type == TK_REGISTER) {
        *p_instr |= tok->subtype;
    } else {
        uint16_t imm5 = parse_literal(tok);
        if (!is_fit_5bit(imm5))
            set_warning(tok->lidx, "Cannot cast %04x fully to 5 bit value", imm5);
        *p_instr |= imm5 & 0x1f;
        *p_instr |= 1 << 5;
    }
}

static void add_pcoff9(uint16_t *p_instr, Token *tok, uint16_t *p_addr, SymTab *st) {
    uint16_t offset;
    uint32_t lidx = tok->lidx;

    if (tok->type == TK_LITERAL) {
        offset = parse_literal(tok);
    } else {
        Symbol *sym = symtab_find(st, tok);
        if (!sym) {
            set_error(lidx, "Symbol %s is not defined", tok->str);
            return;
        }
        offset = sym->addr - (*p_addr + 1);
    }

    if (!is_fit_9bit(offset))
        set_warning(lidx, "Cannot cast %04x fully to 9 bit value", offset);
    *p_instr |= offset & 0x1ff;
}

static void add_pcoff11(uint16_t *p_instr, Token *tok, uint16_t *p_addr, SymTab *st) {
    uint16_t offset;
    uint32_t lidx = tok->lidx;

    if (tok->type == TK_LITERAL) {
        offset = parse_literal(tok);
    } else {
        Symbol *sym = symtab_find(st, tok);
        if (!sym) {
            set_error(lidx, "Symbol %s is not defined", tok->str);
            return;
        }
        offset = sym->addr - (*p_addr + 1);
    }

    if (!is_fit_11bit(offset))
        set_warning(lidx, "Cannot cast %04x fully to 11 bit value", offset);

    *p_instr |= 1 << 11;
    *p_instr |= offset & 0x7ff;
}

static void add_off6(uint16_t *p_instr, Token *tok) {
    uint16_t offset = parse_literal(tok);
    uint32_t lidx = tok->lidx;

    if (!is_fit_6bit(offset))
        set_warning(lidx, "Cannot cast %04x fully to 6 bit value", offset);
    *p_instr |= offset & 0x3f;
}

static void add_trapvect8_literal(uint16_t *p_instr, Token *tok) {
    uint8_t tvec = parse_literal(tok);
    uint32_t lidx = tok->lidx;

    if (tvec < 0 || tvec > 0xff)
        set_error(lidx, "%s is not a valid trapvector", tok->str);
    else if (tvec < 0x20 || tvec > 0x25)
        set_warning(lidx, "Undefined trap routine %s ignored", tok->str);
    *p_instr |= tvec & 0xff;
}

static void add_trapvect8_direct(uint16_t *p_instr, Token *tok) {
    *p_instr |= get_trap_type(tok) & 0xff;
}

// translates asssembly operation to machine code
static uint16_t generate_opcode_instruction(Token **toks, uint16_t *p_addr, SymTab *st) {
    uint16_t instr = 0;
    add_opcode(&instr, toks[0]);

    switch (toks[0]->subtype) {
    case OP_BR:
        add_cond(&instr, toks[0]);
        add_pcoff9(&instr, toks[1], p_addr, st);
        break;
    case OP_ADD:
    case OP_AND:
        add_dstsrc(&instr, toks[1]);
        add_src1(&instr, toks[2]);
        add_src2(&instr, toks[3]);
        break;
    case OP_LD:
    case OP_ST:
    case OP_LDI:
    case OP_STI:
    case OP_LEA:
        add_dstsrc(&instr, toks[1]);
        add_pcoff9(&instr, toks[2], p_addr, st);
        break;
    case OP_JSR:
        add_pcoff11(&instr, toks[1], p_addr, st);
        break;
    case OP_LDR:
    case OP_STR:
        add_dstsrc(&instr, toks[1]);
        add_src1(&instr, toks[2]);
        add_off6(&instr, toks[3]);
        break;
    case OP_NOT:
        add_dstsrc(&instr, toks[1]);
        add_src1(&instr, toks[2]);
        instr |= 0x3f;
        break;
    case OP_JMP:
    case OP_JSRR:
        add_src1(&instr, toks[1]);
        break;
    case OP_RET:
        // RET is JMP to register 7
        add_src1_direct(&instr, 7);
        break;
    case OP_TRAP:
        add_trapvect8_literal(&instr, toks[1]);
        break;
    case OP_TRAPS:
        add_trapvect8_direct(&instr, toks[0]);
        break;
    }
    return instr;
}

// helper functions to handle pseudo-operations .BLKW, .FILL, .STRINGZ

// writes value 0 (blank) to specific number of continuous memory elements
static void handle_blkw(Token **toks, uint16_t *p_addr, OutputBuf *obuf) {
    Token *tok = toks[1];
    uint16_t blank_cnt = parse_literal(tok);
    for (uint16_t i = 0; i < blank_cnt; i++) {
        outputbuf_add(obuf, *p_addr, 0, tok->lidx);
        *p_addr += 1;
    }
}

// writes unsigned 16 bit value to a specific memory element
// the value is interpreted from literal or symbol
static void handle_fill(Token **toks, uint16_t *p_addr, OutputBuf *obuf, SymTab *st) {
    Token *tok = toks[1];
    uint32_t lidx = tok->lidx;
    uint16_t val;

    if (tok->type == TK_LITERAL) {
        val = parse_literal(tok);
    } else {
        Symbol *sym = symtab_find(st, tok);
        if (!sym) {
            set_error(lidx, "Symbol %s is not defined", tok->str);
            return;
        }
        val = sym->addr;
    }
    outputbuf_add(obuf, *p_addr, val, lidx);
    *p_addr += 1;
}

// writes a string literal to multiple continuous memory elements
// note: string literal is added the null character at the end during tokenization
static void handle_stringz(Token **toks, uint16_t *p_addr, OutputBuf *obuf) {
    Token *tok = toks[1];
    for (uint16_t i = 0; i < tok->size; i++) {
        outputbuf_add(obuf, *p_addr, tok->str[i], tok->lidx);
        *p_addr += 1;
    }
}

// handles pseudo-operation (except ORIG and END)
static void handle_pseudo_instruction(Token **toks, uint16_t *p_addr,
                                      OutputBuf *obuf, SymTab *st) {
    switch (toks[0]->subtype) {
    case PS_BLKW:
        handle_blkw(toks, p_addr, obuf);
        break;
    case PS_FILL:
        handle_fill(toks, p_addr, obuf, st);
        break;
    case PS_STRINGZ:
        handle_stringz(toks, p_addr, obuf);
        break;
    }
}

// generates machine code based on assembly instruction
static void generate_code(OutputBuf *obuf, SymTab *st, Line *line, uint16_t *p_addr) {
    Token **toks = line->tokens;
    uint32_t lidx = toks[0]->lidx;

    // check symbol table for possible symbol found
    if (toks[0]->type == TK_SYMBOL) {
        if (line->tokcnt == 1)
            return;
        toks++;
    }

    if (toks[0]->type == TK_PSEUDO) {
        handle_pseudo_instruction(toks, p_addr, obuf, st);
    } else if (toks[0]->type == TK_OPCODE) {
        uint16_t instr = generate_opcode_instruction(toks, p_addr, st);
        outputbuf_add(obuf, *p_addr, instr, lidx);
        *p_addr += 1;
    }
}

// convert instructions in input buffer to machine codes then stores them in output buffer
static void pass_two(InputBuf *ibuf, OutputBuf *obuf, SymTab *st) {
    uint16_t addr = obuf->instr[0];
    for (uint16_t i = 0; i < ibuf->size; i++) {
        Line *line = inputbuf_get(ibuf, i);
        generate_code(obuf, st, line, &addr);
    }
}

// main assembling process
void assemble(InputBuf *ibuf, OutputBuf *obuf, SymTab *st, FileList *fl, char *path) {
    filelist_open(fl, path);

    if (get_error_count() > 0)
        return;

    uint32_t lidx = 1;
    find_orig(fl, ibuf, obuf, &lidx);
    if (get_error_count() > 0)
        return;

    pass_one(ibuf, obuf, st, fl, &lidx);
    if (get_error_count() > 0)
        return;

    pass_two(ibuf, obuf, st);
    if (get_error_count() > 0)
        return;

    // output to files, write_obj() is called last due to the swapping
    // of endianness in the elements of obuf->instr0 there
    write_sym(st, fl->sym);
    write_lst(obuf, fl->lst, fl->src);
    write_bin(obuf, fl->bin);
    write_obj(obuf, fl->obj);
}
