/* This library contains code for generating machine code for chip-8. It
 * maintains internal buffer with machine code. Call to asm_emit* appends
 * code of the instruction to buffer. */

enum AsmError {
    ASM_OK,
    ASM_TOO_MANY_LABELS,
    ASM_SECOND_DEFINITION,
    ASM_TOO_BIG_ARGUMENT,
    ASM_TOO_MANY_INSTRUCTIONS,
};

#define ASM_MIN_ADDRESS     0x200   /* Machine code starts at this address. */
#define ASM_MAX_ADDRESS     0x1000  /* Size of address space. */
#define ASM_MAX_LABELS      4096    /* Maximum number of labels that can be used. */
#define ASM_BUFFER_SIZE     (ASM_MAX_ADDRESS - ASM_MIN_ADDRESS) /* Size of internal buffer. */

/* Before calling any other function from this header first call this. */
extern void asm_init(void);

/* Label can be used before definition i.e. before its location is specified.
 * Such label is called undefined. This function allows to get number of such
 * labels.*/
extern uint_fast16_t asm_get_number_of_undefined_labels(void);

/* Returns pointer to internal buffer with machine code. */
extern uint_least8_t *asm_get_machine_code(void);

/* Returns current position within internal buffer of machine code. */
extern uint_fast16_t asm_get_instruction_pointer(void);

/* Returns string description of error code. */
extern const char *asm_error_string(enum AsmError e);

/* Defines label, i.e. specifies address of the label. Its address can be calculated as
 * ASM_MIN_ADDRESS + asm_get_instruction_pointer(). */
extern enum AsmError asm_emit_label(const char *label);

/* Puts SINGLE byte into memory as is. */
extern enum AsmError asm_emit_data(uint_fast16_t data);

/* Next functions append buffer with corresponding chip-8 instruction.
 * Instructions taking address as an argument can alternatively take label. */
extern enum AsmError asm_emit_cls(void);
extern enum AsmError asm_emit_ret(void);
extern enum AsmError asm_emit_jp_addr(uint_fast16_t addr);
extern enum AsmError asm_emit_jp_label(const char *label);
extern enum AsmError asm_emit_call_addr(uint_fast16_t addr);
extern enum AsmError asm_emit_call_label(const char *label);
extern enum AsmError asm_emit_se_vx_byte(uint_fast16_t x, uint_fast16_t byte);
extern enum AsmError asm_emit_sne_vx_byte(uint_fast16_t x, uint_fast16_t byte);
extern enum AsmError asm_emit_se_vx_vy(uint_fast16_t x, uint_fast16_t y);
extern enum AsmError asm_emit_ld_vx_byte(uint_fast16_t x, uint_fast16_t byte);
extern enum AsmError asm_emit_add_vx_byte(uint_fast16_t x, uint_fast16_t byte);
extern enum AsmError asm_emit_ld_vx_vy(uint_fast16_t x, uint_fast16_t y);
extern enum AsmError asm_emit_or_vx_vy(uint_fast16_t x, uint_fast16_t y);
extern enum AsmError asm_emit_and_vx_vy(uint_fast16_t x, uint_fast16_t y);
extern enum AsmError asm_emit_xor_vx_vy(uint_fast16_t x, uint_fast16_t y);
extern enum AsmError asm_emit_add_vx_vy(uint_fast16_t x, uint_fast16_t y);
extern enum AsmError asm_emit_sub_vx_vy(uint_fast16_t x, uint_fast16_t y);
extern enum AsmError asm_emit_shr_vx(uint_fast16_t x);
extern enum AsmError asm_emit_subn_vx_vy(uint_fast16_t x, uint_fast16_t y);
extern enum AsmError asm_emit_shl_vx(uint_fast16_t x);
extern enum AsmError asm_emit_sne_vx_vy(uint_fast16_t x, uint_fast16_t y);
extern enum AsmError asm_emit_ld_i_addr(uint_fast16_t addr);
extern enum AsmError asm_emit_ld_i_label(const char *label);
extern enum AsmError asm_emit_jp_v0_addr(uint_fast16_t addr);
extern enum AsmError asm_emit_jp_v0_label(char *label);
extern enum AsmError asm_emit_rnd_vx_byte(uint_fast16_t x, uint_fast16_t byte);
extern enum AsmError asm_emit_drw_vx_vy_nibble(uint_fast16_t x, uint_fast16_t y,
                                               uint_fast16_t nibble);
extern enum AsmError asm_emit_skp_vx(uint_fast16_t x);
extern enum AsmError asm_emit_sknp_vx(uint_fast16_t x);
extern enum AsmError asm_emit_ld_vx_dt(uint_fast16_t x);
extern enum AsmError asm_emit_ld_vx_k(uint_fast16_t x);
extern enum AsmError asm_emit_ld_dt_vx(uint_fast16_t x);
extern enum AsmError asm_emit_ld_st_vx(uint_fast16_t x);
extern enum AsmError asm_emit_add_i_vx(uint_fast16_t x);
extern enum AsmError asm_emit_ld_f_vx(uint_fast16_t x);
extern enum AsmError asm_emit_ld_b_vx(uint_fast16_t x);
extern enum AsmError asm_emit_ld_ii_vx(uint_fast16_t x);
extern enum AsmError asm_emit_ld_vx_ii(uint_fast16_t x);
