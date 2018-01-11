#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "c8asm.h"

static uint_fast32_t
string_hash(const char *str) {
    uint_fast32_t hash = 5381, c;
    while ((c = *(const unsigned char *)str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

#define TABLE_OF_LABELS_SIZE (MAX_LABELS * 5 / 4 + 1)
struct LabelAddress {
    uint_fast32_t   hash;
    char            *label;
    uint_fast16_t   address;
    bool            undefined;
};
static uint_fast16_t        number_of_labels;
static uint_fast16_t        number_of_undefined_labels;
static struct LabelAddress  labels[TABLE_OF_LABELS_SIZE];

extern uint_fast16_t
asm_get_number_of_undefined_labels(void) {
    return number_of_undefined_labels;
}

static struct LabelAddress *
lookup_label(char *label) {
    uint_fast32_t hash = string_hash(label);
    size_t n = TABLE_OF_LABELS_SIZE;
    for (size_t i = hash % n; labels[i].label; i = (i + 1) % n) {
        if (hash == labels[i].hash && !strcmp(label, labels[i].label)) {
            return &labels[i];
        }
    }
    return NULL;
}

static enum AsmError
add_label(struct LabelAddress new_label_address) {
    if (number_of_labels >= MAX_LABELS) {
        return ASM_TOO_MANY_LABELS;
    }
    char *label = new_label_address.label;
    uint_fast32_t hash = new_label_address.hash = string_hash(label);
    size_t i, n = TABLE_OF_LABELS_SIZE;
    for (i = hash % n; labels[i].label; i = (i + 1) % n) {
        if (hash == labels[i].hash && !strcmp(label, labels[i].label)) {
            return ASM_SECOND_DEFINITION;
        }
    }
    ++number_of_labels;
    labels[i] = new_label_address;
    return ASM_OK;
}

static uint_least8_t machine_code[BUFFER_SIZE];
static uint_fast16_t instruction_pointer;

extern uint_least8_t *
asm_get_machine_code(void) {
    return machine_code;
}

extern uint_fast16_t
asm_get_instruction_pointer(void) {
    return instruction_pointer;
}

extern const char *
asm_error_string(enum AsmError e) {
    switch (e) {
        default:                            return "unknown error";
        case ASM_OK:                        return "OK";
        case ASM_TOO_MANY_LABELS:           return "Too many labels";
        case ASM_SECOND_DEFINITION:         return "Second label definition";
        case ASM_TOO_BIG_ARGUMENT:          return "Too big argument";
        case ASM_TOO_MANY_INSTRUCTIONS:     return "Too many instructions";
    }
}

extern void
asm_init(void) {
    instruction_pointer = 0;
    number_of_labels = 0;
    number_of_undefined_labels = 0;
    for (size_t i = 0; i < TABLE_OF_LABELS_SIZE; ++i) {
        labels[i].label = NULL;
    }
}

extern enum AsmError
asm_emit_label(char *label) {
    struct LabelAddress *la = lookup_label(label);
    if (la) {
        if (!la->undefined) return ASM_SECOND_DEFINITION;
        /* First definition. Label was used before. */
        uint_fast16_t address = MIN_ADDRESS + instruction_pointer;
        uint_fast16_t i, j = la->address;
        do {
            i = j;
            j = (machine_code[i] & 0x0f) << 8 | machine_code[i + 1];
            machine_code[i    ] = (machine_code[i] & 0xf0) | ((address >> 8) & 0x0f);
            machine_code[i + 1] = address & 0xff;
        } while (j);
        la->undefined = false;
        la->address   = address;
        --number_of_undefined_labels;
    } else { /* First definition. Label was not used before. */
        enum AsmError e = add_label((struct LabelAddress) {
            .label     = label,
            .address   = MIN_ADDRESS + instruction_pointer,
            .undefined = false,
        });
        if (ASM_OK != e) return e;
    }
    return ASM_OK;
}

static enum AsmError
emit(uint_fast16_t instruction) {
    if (BUFFER_SIZE <= instruction_pointer + 1) return ASM_TOO_MANY_INSTRUCTIONS;
    machine_code[instruction_pointer++] = (instruction >> 8u) & 0xff;
    machine_code[instruction_pointer++] = instruction & 0xff;
    return ASM_OK;
}

#define CHECK_ARGUMENT_SIZE(argument, size) \
    if ((uint_fast16_t)(1 << (size)) < (argument)) return ASM_TOO_BIG_ARGUMENT;

static enum AsmError
emit_hnnni(uint_fast16_t h, uint_fast16_t nnn) {
    CHECK_ARGUMENT_SIZE(h, 4);
    CHECK_ARGUMENT_SIZE(nnn, 12);
    return emit(h << 12 | nnn);
}

static enum AsmError
emit_hnnnl(uint_fast16_t h, char *label) {
    struct LabelAddress *la = lookup_label(label);
    if (!la) { /* If label was neither defined nor used before. */
        /* Save instruction pointer in the table of labels to fill address when
         * it will be defined. */
        enum AsmError e = add_label((struct LabelAddress) {
            .label      = label,
            .address    = instruction_pointer,
            .undefined  = true,
        });
        if (ASM_OK != e) return e;
        e = emit_hnnni(h, 0);
        ++number_of_undefined_labels;
        return e;
    } else if (la->undefined) { /* If label was not defined but was used before. */
        uint_fast16_t previous_usage = la->address;
        /* Update table of labels with new last usage site. */
        la->address = instruction_pointer;
        return emit_hnnni(h, previous_usage);
    } else { /* If label was defined. */
        return emit_hnnni(h, la->address);
    }
}

static enum AsmError
emit_hxkk(uint_fast16_t h, uint_fast16_t x, uint_fast16_t kk) {
    CHECK_ARGUMENT_SIZE(h, 4);
    CHECK_ARGUMENT_SIZE(x, 4);
    CHECK_ARGUMENT_SIZE(kk, 8);
    return emit(h << 12 | x << 8 | kk);
}

static enum AsmError
emit_hxyn(uint_fast16_t h, uint_fast16_t x, uint_fast16_t y, uint_fast16_t n) {
    CHECK_ARGUMENT_SIZE(h, 4);
    CHECK_ARGUMENT_SIZE(x, 4);
    CHECK_ARGUMENT_SIZE(y, 4);
    CHECK_ARGUMENT_SIZE(n, 4);
    return emit(h << 12 | x << 8 | y << 4 | n);
}

extern enum AsmError
asm_emit_data(uint_fast16_t data) {
    if (BUFFER_SIZE < instruction_pointer) return ASM_TOO_MANY_INSTRUCTIONS;
    machine_code[instruction_pointer++] = data & 0xff;
    return ASM_OK;
}

extern enum AsmError
asm_emit_cls(void) {
    return emit(0x00e0);
}

extern enum AsmError
asm_emit_ret(void) {
    return emit(0x00ee);
}

extern enum AsmError
asm_emit_jp_addr(uint_fast16_t addr) {
    return emit_hnnni(0x1, addr);
}

extern enum AsmError
asm_emit_jp_label(char *label) {
    return emit_hnnnl(0x1, label);
}

extern enum AsmError
asm_emit_call_addr(uint_fast16_t addr) {
    return emit_hnnni(0x2, addr);
}

extern enum AsmError
asm_emit_call_label(char *label) {
    return emit_hnnnl(0x2, label);
}

extern enum AsmError
asm_emit_se_vx_byte(uint_fast16_t x, uint_fast16_t byte) {
    return emit_hxkk(0x3, x, byte);
}

extern enum AsmError
asm_emit_sne_vx_byte(uint_fast16_t x, uint_fast16_t byte) {
    return emit_hxkk(0x4, x, byte);
}

extern enum AsmError
asm_emit_se_vx_vy(uint_fast16_t x, uint_fast16_t y) {
    return emit_hxyn(0x5, x, y, 0);
}

extern enum AsmError
asm_emit_ld_vx_byte(uint_fast16_t x, uint_fast16_t byte) {
    return emit_hxkk(0x6, x, byte);
}

extern enum AsmError
asm_emit_add_vx_byte(uint_fast16_t x, uint_fast16_t byte) {
    return emit_hxkk(0x7, x, byte);
}

extern enum AsmError
asm_emit_ld_vx_vy(uint_fast16_t x, uint_fast16_t y) {
    return emit_hxyn(0x8, x, y, 0);
}

extern enum AsmError
asm_emit_or_vx_vy(uint_fast16_t x, uint_fast16_t y) {
    return emit_hxyn(0x8, x, y, 1);
}

extern enum AsmError
asm_emit_and_vx_vy(uint_fast16_t x, uint_fast16_t y) {
    return emit_hxyn(0x8, x, y, 2);
}

extern enum AsmError
asm_emit_xor_vx_vy(uint_fast16_t x, uint_fast16_t y) {
    return emit_hxyn(0x8, x, y, 3);
}

extern enum AsmError
asm_emit_add_vx_vy(uint_fast16_t x, uint_fast16_t y) {
    return emit_hxyn(0x8, x, y, 4);
}

extern enum AsmError
asm_emit_sub_vx_vy(uint_fast16_t x, uint_fast16_t y) {
    return emit_hxyn(0x8, x, y, 5);
}

extern enum AsmError
asm_emit_shr_vx(uint_fast16_t x) {
    return emit_hxkk(0x8, x, 0x06);
}

extern enum AsmError
asm_emit_subn_vx_vy(uint_fast16_t x, uint_fast16_t y) {
    return emit_hxyn(0x8, x, y, 7);
}

extern enum AsmError
asm_emit_shl_vx(uint_fast16_t x) {
    return emit_hxkk(0x8, x, 0x0E);
}

extern enum AsmError
asm_emit_sne_vx_vy(uint_fast16_t x, uint_fast16_t y) {
    return emit_hxyn(0x9, x, y, 0);
}

extern enum AsmError
asm_emit_ld_i_addr(uint_fast16_t addr) {
    return emit_hnnni(0xA, addr);
}

extern enum AsmError
asm_emit_ld_i_label(char *label) {
    return emit_hnnnl(0xA, label);
}

extern enum AsmError
asm_emit_jp_v0_addr(uint_fast16_t addr) {
    return emit_hnnni(0xB, addr);
}

extern enum AsmError
asm_emit_jp_v0_label(char *label) {
    return emit_hnnnl(0xB, label);
}

extern enum AsmError
asm_emit_rnd_vx_byte(uint_fast16_t x, uint_fast16_t byte) {
    return emit_hxkk(0xC, x, byte);
}

extern enum AsmError
asm_emit_drw_vx_vy_nibble(uint_fast16_t x, uint_fast16_t y, uint_fast16_t nibble) {
    return emit_hxyn(0xD, x, y, nibble);
}

extern enum AsmError
asm_emit_skp_vx(uint_fast16_t x) {
    return emit_hxkk(0xE, x, 0x9E);
}

extern enum AsmError
asm_emit_sknp_vx(uint_fast16_t x) {
    return emit_hxkk(0xE, x, 0xA1);
}

extern enum AsmError
asm_emit_ld_vx_dt(uint_fast16_t x) {
    return emit_hxkk(0xF, x, 0x07);
}

extern enum AsmError
asm_emit_ld_vx_k(uint_fast16_t x) {
    return emit_hxkk(0xF, x, 0x0A);
}

extern enum AsmError
asm_emit_ld_dt_vx(uint_fast16_t x) {
    return emit_hxkk(0xF, x, 0x15);
}

extern enum AsmError
asm_emit_ld_st_vx(uint_fast16_t x) {
    return emit_hxkk(0xF, x, 0x18);
}

extern enum AsmError
asm_emit_add_i_vx(uint_fast16_t x) {
    return emit_hxkk(0xF, x, 0x1E);
}

extern enum AsmError
asm_emit_ld_f_vx(uint_fast16_t x) {
    return emit_hxkk(0xF, x, 0x29);
}

extern enum AsmError
asm_emit_ld_b_vx(uint_fast16_t x) {
    return emit_hxkk(0xF, x, 0x33);
}

extern enum AsmError
 asm_emit_ld_ii_vx(uint_fast16_t x) {
    return emit_hxkk(0xF, x, 0x55);
}

extern enum AsmError
asm_emit_ld_vx_ii(uint_fast16_t x) {
    return emit_hxkk(0xF, x, 0x65);
}
