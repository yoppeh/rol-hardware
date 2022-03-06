/* rol-monitor.ino
 *  
 *  This is a 6502 bus monitor, based on the one by Ben Eater http://eater.net. This version
 *  shows the disassembly of the instructions as they are read. Also, more control lines are 
 *  read, such as chip selects from the address decoder logic.
 */
 
const char ADDR[] = {22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52};
const char DATA[] = {39, 41, 43, 45, 47, 49, 51, 53};

#define FLAG_BRANCH     0x01    // branch instruction
#define FLAG_JUMP       0x02    // jump instruction
#define FLAG_JSR        0x04    // jump subroutine instruction
#define FLAG_RET        0x08    // return instruction
#define FLAG_STACK      0x10    // stack instruction
#define FLAG_MX_ADJUST  0x20    // may adjust m/x, thus change instruction bytes/cycles

typedef struct mnemonic_t {
    char *name;
    unsigned char bytes;
    unsigned char cycles;
    unsigned char xpage_cycles;
    unsigned char flags;
    unsigned char branch_succeeds;
    unsigned char stack_accesses;
    char          x_bytes_adj;
    char          x_cycles_adj;
    char          a_bytes_adj;
    char          a_cycles_adj;
    char          e_bytes_adj;
    char          e_cycles_adj;
} mnemonic_t;

mnemonic_t mnemonics[] = {
    
    {"brk", 2, 7, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {"ora (d, x)", 2, 6, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {"cop", 2, 7, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    {"ora d, s", 2, 4, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {"tsb d", 2, 5, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {"ora d", 2, 3, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {"asl d", 2, 5, 0, 0, 0, 0},
    {"ora [d]", 2, 6, 0, 0, 0, 0},
    {"php", 1, 3, 0, FLAG_STACK, 0, 1},
    {"ora #", 2, 2, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {"asl acc", 1, 2, 0, 0, 0, 0},
    {"phd s", 1, 4, 0, 0, 0, 0},
    {"tsb addr", 3, 6, 0, 0, 0, 0},
    {"ora addr", 3, 4, 0, 0, 0, 0},
    {"asl addr", 3, 6, 0, 0, 0, 0},
    {"ora long, rel", 4, 5, 0, 0, 0, 0},
    
    {"bpl rel", 2, 2, 0, 0, 0, 0},
    {"ora (d), y", 2, 5, 1, 0, 0, 0},
    {"ora (d)", 2, 5, 0, 0, 0, 0},
    {"ora (d, s), y", 2, 7, 0, 0, 0, 0},
    {"trb d", 2, 5, 0, 0, 0, 0},
    {"ora d, x", 2, 4, 0, 0, 0, 0},
    {"asl d, x", 2, 6, 0, 0, 0, 0},
    {"ora [d], y", 2, 6, 0, 0, 0, 0},
    {"clc", 1, 2, 0, 0, 0, 0},
    {"ora addr, y", 3, 4, 1, 0, 0, 0},
    {"inc acc", 1, 2, 0, 0, 0, 0},
    {"tcs", 1, 2, 0, 0, 0, 0},
    {"trb addr", 3, 6, 0, 0, 0, 0},
    {"ora addr, x", 3, 4, 1, 0, 0, 0},
    {"asl addr, x", 3, 7, 0, 0, 0, 0},
    {"ora long, x", 4, 5, 0, 0, 0, 0},
    
    {"jsr addr", 3, 6, 0, FLAG_JSR, 0, 0},
    {"and (d, x)", 2, 6, 0, 0, 0, 0},
    {"jsl long", 4, 8, 0, 0, 0, 0},
    {"and d, s", 2, 4, 0, 0, 0, 0},
    {"bit d", 2, 3, 0, 0, 0, 0},
    {"and d", 2, 3, 0, 0, 0, 0},
    {"rol d", 2, 5, 0, 0, 0, 0},
    {"and [d]", 2, 6, 0, 0, 0, 0},
    {"plp", 1, 4, 0, FLAG_STACK, 0, 2},
    {"and #", 2, 2, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {"rol acc", 1, 2, 0, 0, 0, 0},
    {"pld", 1, 5, 0, 0, 0, 0},
    {"bit addr", 3, 4, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {"and addr", 3, 4, 0, 0, 0, 0},
    {"rol addr", 3, 7, 0, 0, 0, 0},
    {"and long", 4, 5, 0, 0, 0, 0},

    {"bmi", 2, 2, 2, FLAG_BRANCH, 1, 0},
    {"and (d), y", 2, 5, 1, 0, 0, 0},
    {"and (d)", 2, 5, 0, 0, 0, 0},
    {"and (d, s), y", 2, 7, 0, 0, 0, 0},
    {"bit d, x", 2, 4, 0, 0, 0, 0},
    {"and d, x", 2, 4, 0, 0, 0, 0},
    {"rol d, x", 2, 6, 0, 0, 0, 0},
    {"and [d], y", 2, 6, 0, 0, 0, 0},
    {"sec", 1, 2, 0, 0, 0, 0},
    {"and addr, y", 3, 4, 1, 0, 0, 0},
    {"dec acc", 1, 2, 0, 0, 0, 0},
    {"tsc", 1, 2, 0, 0, 0, 0},
    {"bit addr, x", 3, 4, 0, 0, 0, 0},
    {"and addr, x", 3, 4, 1, 0, 0, 0},
    {"rol addr, x", 3, 7, 0, 0, 0, 0},
    {"and long, x", 4, 5, 0, 0, 0, 0},

    {"rti", 1, 7, 0, FLAG_RET, 0, 0},
    {"eor (d, x)", 2, 6, 0, 0, 0, 0},
    {"wdm", 2, 2, 0, 0, 0, 0},
    {"eor d, s", 2, 4, 0, 0, 0, 0},
    {"mvp xyc", 3, 7, 0, 0, 0, 0},
    {"eor d", 2, 3, 0, 0, 0, 0},
    {"lsr d", 2, 5, 0, 0, 0, 0},
    {"eor [d]", 2, 6, 0, 0, 0, 0},
    {"pha", 1, 3, 0, FLAG_STACK, 0, 1},
    {"eor #", 2, 2, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {"lsr acc", 1, 2, 0, 0, 0, 0},
    {"phk", 1, 3, 0, 0, 0, 0},
    {"jmp addr", 3, 3, 0, FLAG_JUMP, 0, 0},
    {"eor addr", 3, 4, 0, 0, 0, 0},
    {"lsr addr", 3, 6, 0, 0, 0, 0},
    {"eor long", 4, 5, 0, 0, 0, 0},

    {"bvc rel", 2, 2, 2, FLAG_BRANCH, 0, 0},
    {"eor (d), y", 2, 5, 1, 0, 0, 0},
    {"eor (d)", 2, 5, 0, 0, 0, 0},
    {"eor (d, s), y", 2, 7, 0, 0, 0, 0},
    {"mvn xyc", 3, 7, 0, 0, 0, 0},
    {"eor d, x", 2, 4, 0, 0, 0, 0},
    {"lsr d, x", 2, 6, 0, 0, 0, 0},
    {"eor [d], y", 2, 6, 0, 0, 0, 0},
    {"cli", 1, 2, 0, 0, 0, 0},
    {"eor addr, y", 3, 4, 1, 0, 0, 0},
    {"phy", 1, 3, 0, FLAG_STACK, 0, 1},
    {"tcd", 1, 2, 0, 0, 0, 0},
    {"jmp long", 4, 4, 0, 0, 0, 0},
    {"eor addr, x", 3, 4, 1, 0, 0, 0},
    {"lsr addr, x", 3, 7, 0, 0, 0, 0},
    {"eor long, x", 4, 5, 0, 0, 0, 0},

    {"rts", 1, 6, 0, FLAG_RET, 0, 0},
    {"adc (d, x)", 2, 6, 0, 0, 0, 0},
    {"per", 3, 6, 0, 0, 0, 0},
    {"adc d, s", 2, 4, 0, 0, 0, 0},
    {"stz d", 2, 3, 0, 0, 0, 0},
    {"adc d", 2, 3, 0, 0, 0, 0},
    {"ror d", 2, 5, 0, 0, 0, 0},
    {"adc [d]", 2, 6, 0, 0, 0, 0},
    {"pla", 1, 4, 0, FLAG_STACK, 0, 2},
    {"adc #", 2, 2, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {"ror acc", 1, 2, 0, 0, 0, 0},
    {"rtl", 1, 6, 0, 0, 0, 0},
    {"jmp (addr)", 3, 5, 0, FLAG_JUMP, 0, 0},
    {"adc addr", 3, 4, 0, 0, 0, 0},
    {"ror addr", 3, 6, 0, 0, 0, 0},
    {"adc long", 4, 5, 0, 0, 0, 0},

    {"bvs rel", 2, 2, 2, FLAG_BRANCH, 1, 0},
    {"adc (d), y", 2, 5, 1, 0, 0, 0},
    {"adc (d)", 2, 5, 0, 0, 0, 0},
    {"adc (d, s), y", 2, 7, 0, 0, 0, 0},
    {"stz d, x", 2, 4, 0, 0, 0, 0},
    {"adc d, x", 2, 4, 0, 0, 0, 0},
    {"ror d, x", 2, 5, 0, 0, 0, 0},
    {"adc [d], y", 2, 6, 0, 0, 0, 0},
    {"sei", 1, 2, 0, 0, 0, 0},
    {"adc addr, y", 3, 4, 1, 0, 0, 0},
    {"ply", 1, 4, 0, FLAG_STACK, 0, 2},
    {"tdc", 1, 2, 0, 0, 0, 0},
    {"jmp (addr, x)", 3, 6, 0, FLAG_JUMP, 0, 0},
    {"adc addr, x", 3, 4, 1, 0, 0, 0},
    {"ror addr, x", 3, 7, 0, 0, 0, 0},
    {"adc long, x", 4, 5, 0, 0, 0, 0},

    {"bra rel", 2, 2, 0, FLAG_BRANCH, 0, 0},
    {"sta (d, x)", 2, 6, 0, 0, 0, 0},
    {"brl rellong", 3, 4, 0, 0, 0, 0},
    {"sta d, s", 2, 4, 0, 0, 0, 0},
    {"sty d", 2, 3, 0, 0, 0, 0},
    {"sta d", 2, 3, 0, 0, 0, 0},
    {"stx d", 2, 3, 0, 0, 0, 0},
    {"sta [d]", 2, 2, 0, 0, 0, 0},
    {"dey", 1, 2, 0, 0, 0, 0},
    {"bit #", 2, 2, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {"txa", 1, 2, 0, 0, 0, 0},
    {"phb", 1, 3, 0, 0, 0, 0},
    {"sty addr", 3, 4, 0, 0, 0, 0},
    {"sta addr", 3, 4, 0, 0, 0, 0},
    {"stx addr", 3, 4, 0, 0, 0, 0},
    {"sta long", 4, 5, 0, 0, 0, 0},

    {"bcc rel", 2, 2, 1, FLAG_BRANCH, 1, 0},
    {"sta (d), y", 2, 6, 0, 0, 0, 0},
    {"sta (d)", 2, 5, 0, 0, 0, 0},
    {"sta (d, s), y", 2, 7, 0, 0, 0, 0},
    {"sty d, x", 2, 4, 0, 0, 0, 0},
    {"sta d, x", 2, 4, 0, 0, 0, 0},
    {"stx d, y", 2, 4, 0, 0, 0, 0},
    {"sta [d], y", 2, 6, 0, 0, 0, 0},
    {"tya", 1, 2, 0, 0, 0, 0},
    {"sta addr, y", 3, 5, 0, 0, 0, 0},
    {"txs", 1, 2, 0, 0, 0, 0},
    {"txy", 1, 2, 0, 0, 0, 0},
    {"stz addr", 3, 4, 0, 0, 0, 0},
    {"sta addr, x", 3, 5, 0, 0, 0, 0},
    {"stz addr, x", 3, 5, 0, 0, 0, 0},
    {"sta long, x", 4, 5, 0, 0, 0, 0},

    {"ldy #", 2, 2, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {"lda (d, x)", 2, 6, 0, 0, 0, 0},
    {"ldx #", 2, 2, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {"lda d, s", 2, 4, 0, 0, 0, 0},
    {"ldy d", 2, 3, 0, 0, 0, 0},
    {"lda d", 2, 3, 0, 0, 0, 0},
    {"ldx d", 2, 3, 0, 0, 0, 0},
    {"lda [d]", 2, 6, 0, 0, 0, 0},
    {"tay", 1, 2, 0, 0, 0, 0},
    {"lda #", 2, 2, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {"tax", 1, 2, 0, 0, 0, 0},
    {"plb", 1, 4, 0, 0, 0, 0},
    {"ldy addr", 3, 4, 0, 0, 0, 0},
    {"lda addr", 3, 4, 0, 0, 0, 0},
    {"ldx addr", 3, 4, 0, 0, 0, 0},
    {"lda long", 4, 5, 0, 0, 0, 0},

    {"bcs rel", 2, 2, 2, FLAG_BRANCH, 1, 0},
    {"lda (d), y", 2, 5, 1, 0, 0, 0},
    {"lda (d)", 2, 5, 0, 0, 0, 0},
    {"lda (d, s), y", 2, 7, 0, 0, 0, 0},
    {"ldy d, x", 2, 4, 0, 0, 0, 0},
    {"lda d, x", 2, 4, 0, 0, 0, 0},
    {"ldx d, y", 2, 4, 0, 0, 0, 0},
    {"lda [d], y", 2, 6, 0, 0, 0, 0},
    {"clv", 1, 2, 0, 0, 0, 0},
    {"lda addr, y", 3, 4, 1, 0, 0, 0},
    {"tsx", 1, 2, 0, 0, 0, 0},
    {"tyx", 1, 2, 0, 0, 0, 0},
    {"ldy addr, x", 3, 4, 1, 0, 0, 0},
    {"lda addr, x", 3, 4, 1, 0, 0, 0},
    {"ldx addr, y", 3, 4, 1, 0, 0, 0},
    {"lda long, x", 4, 5, 0, 0, 0, 0},

    {"cpy #", 2, 2, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {"cmp (d, x)", 2, 6, 0, 0, 0, 0},
    {"rep #", 2, 3, 0, FLAG_MX_ADJUST, 0, 0},
    {"cmp d, s", 2, 4, 0, 0, 0, 0},
    {"cpy d", 2, 3, 0, 0, 0, 0},
    {"cmp d", 2, 3, 0, 0, 0, 0},
    {"dec d", 2, 5, 0, 0, 0, 0},
    {"cmp [d]", 2, 6, 0, 0, 0, 0},
    {"iny", 1, 2, 0, 0, 0, 0},
    {"cmp #", 2, 2, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {"dex", 1, 2, 0, 0, 0, 0},
    {"wai", 1, 3, 0, 0, 0, 0},
    {"cpy addr", 3, 4, 0, 0, 0, 0},
    {"cmp addr", 3, 4, 0, 0, 0, 0},
    {"dec addr", 3, 6, 0, 0, 0, 0},
    {"cmp long", 4, 5, 0, 0, 0, 0},

    {"bne rel", 2, 2, 2, FLAG_BRANCH, 1, 0},
    {"cmp (d), y", 2, 5, 1, 0, 0, 0},
    {"cmp (d)", 2, 5, 0, 0, 0, 0},
    {"cmp (d, s), y", 2, 7, 0, 0, 0, 0},
    {"pei", 2, 6, 0, 0, 0, 0},
    {"cmp d, x", 2, 4, 0, 0, 0, 0},
    {"dec d, x", 2, 6, 0, 0, 0, 0},
    {"cmp [d], y", 2, 6, 0, 0, 0, 0},
    {"cld", 1, 2, 0, 0, 0, 0},
    {"cmp addr, y", 3, 4, 1, 0, 0, 0},
    {"phx", 1, 3, 0, FLAG_STACK, 0, 0},
    {"stp", 1, 3, 0, 0, 0, 0},
    {"jmp (addr)", 3, 6, 0, 0, 0, 0},
    {"cmp addr, x", 3, 4, 1, 0, 0, 0},
    {"dec addr, x", 3, 7, 0, 0, 0, 0},
    {"cmp long, x", 4, 5, 0, 0, 0, 0},

    {"cpx #", 2, 2, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {"sbc (d, x)", 2, 6, 0, 0, 0, 0},
    {"sep #", 2, 3, 0, FLAG_MX_ADJUST, 0, 0},
    {"sbc d, s", 2, 4, 0, 0, 0, 0},
    {"cpx d", 2, 3, 0, 0, 0, 0},
    {"sbc d", 2, 3, 0, 0, 0, 0},
    {"inc d", 2, 5, 0, 0, 0, 0},
    {"sbc [d]", 2, 6, 0, 0, 0, 0},
    {"inx", 1, 2, 0, 0, 0, 0},
    {"sbc #", 2, 2, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {"nop", 1, 2, 0, 0, 0, 0},
    {"xba", 3, 1, 0, 0, 0, 0},
    {"cpx addr", 3, 4, 0, 0, 0, 0},
    {"sbc addr", 3, 4, 0, 0, 0, 0},
    {"inc addr", 3, 6, 0, 0, 0, 0},
    {"sbc long", 4, 5, 0, 0, 0, 0},

    {"beq rel", 2, 2, 2, FLAG_BRANCH, 1, 0},
    {"sbc (d), y", 2, 5, 1, 0, 0, 0},
    {"sbc (d)", 2, 5, 0, 0, 0, 0},
    {"sbc (d, s), y", 2, 7, 0, 0, 0, 0},
    {"pea", 3, 5, 0, 0, 0, 0},
    {"sbc d, x", 2, 4, 0, 0, 0, 0},
    {"inc d, x", 2, 6, 0, 0, 0, 0},
    {"sbc [d], y", 2, 6, 0, 0, 0, 0},
    {"sed", 1, 2, 0, 0, 0, 0},
    {"sbc addr, y", 3, 4, 1, 0, 0, 0},
    {"plx", 1, 4, 0, FLAG_STACK, 0, 2},
    {"xce", 1, 2, 0, 0, 0, 0},
    {"jsr (addr, x)", 3, 8, 0, 0, 0, 0},
    {"sbc addr, x", 3, 4, 1, 0, 0, 0},
    {"inc addr, x", 3, 7, 0, 0, 0, 0},
    {"sbc long, x", 4, 5, 0, 0, 0, 0}
};

#define PHI2    2
#define RESB    3
#define RWB     6
#define NMIB    7
#define IRQB    8
#define VPA     9
#define VDA     10
#define RAM     37
#define VIA0    35
#define VIA1    33
#define UART    31
#define RTC     29
#define ROM     23


unsigned int prev_addr = 0xffff;
unsigned int next_addr[4] = {0xffff, 0xffff, 0xffff, 0xffff};
unsigned int work_addr = 0xffff;
unsigned char prev_mnem = 0xff;
unsigned char expecting_rep_operand = 0;
unsigned char expecting_sep_operand = 0;
unsigned char expecting_abs = 0;
unsigned char expecting_ret = 0;
unsigned char expecting_stack = 0;
unsigned char carry_state = 0;
bool accumulator_size_16 = false;
bool index_size_16 = false;
bool executing = false;
bool expecting_rel = false;
bool native_mode = false;

void setup() {

    for (int i = 0; i < 16; i++) {
        pinMode(ADDR[i], INPUT);
    }
    for (int i = 0; i < 8; i++) {
        pinMode(DATA[i], INPUT);
    }

    pinMode(VPA, INPUT);
    pinMode(VDA, INPUT);
    pinMode(RESB, INPUT);
    pinMode(PHI2, INPUT);
    pinMode(RWB, INPUT);
    pinMode(NMIB, INPUT);
    pinMode(IRQB, INPUT);
    pinMode(RAM, INPUT);
    pinMode(RTC, INPUT);
    pinMode(VIA0, INPUT);
    pinMode(VIA1, INPUT);
    pinMode(ROM, INPUT);
    pinMode(UART, INPUT);
    attachInterrupt(digitalPinToInterrupt(PHI2), onPHI2, RISING);
    
    Serial.begin(57600);
}


void onPHI2() {

    char msg[128] = "";
    char output[128];
    char ch[2];
    unsigned int addr = 0;
    int data = 0;
    char rwb = '<';
    char *resb = "resb";
    char *nmib = "nmib";
    char *irqb = "irqb";
    char *vpa = "vpa";
    char *vda = "vda";
    char *ram = "ram";
    char *rtc = "rtc";
    char *via0 = "via0";
    char *via1 = "via1";
    char *rom = "rom";
    char *uart = "uart";
    char *mnem = "";

    delayMicroseconds(1);

    for (int i = 0; i < 16; i++) {
        addr <<= 1;
        addr |= digitalRead(ADDR[i]) ? 1 : 0;
    }
    for (int i = 0; i < 8; i++) {
        data <<= 1;
        data |= digitalRead(DATA[i]) ? 1 : 0;
    }
    if (digitalRead(RWB)) {
        rwb = '>';
    }

    if (digitalRead(RESB)) {
        resb = "RESB";
    }
    if (digitalRead(NMIB)) {
        nmib = "NMIB";
    }
    if (digitalRead(IRQB)) {
        irqb = "IRQB";
    }
    if (digitalRead(RAM)) {
        ram = "RAM";
    }
    if (digitalRead(RTC)) {
        rtc = "RTC";
    }
    if (digitalRead(VIA0)) {
        via0 = "VIA0";
    }
    if (digitalRead(VIA1)) {
        via1 = "VIA1";
    }
    if (digitalRead(ROM)) {
        rom = "ROM";
    }
    if (digitalRead(UART)) {
        uart = "UART";
    }
    if (digitalRead(VPA)) {
        vpa = "VPA";
    }
    if (digitalRead(VDA)) {
        vda = "VDA";
    }

    if (addr == 0xfffc) {
        next_addr[0] = data;
    } else if (addr == 0xfffd && prev_addr == 0xfffc) {
        next_addr[0] |= data << 8;
    }
    if (prev_addr == 0xfffd && addr == next_addr[0]) {
        executing = true;
    }
    
    if (digitalRead(VDA) && digitalRead(VPA) && executing) {
        mnem = mnemonics[data].name;
    } else if (digitalRead(VDA) && data >= ' ' && data < 127) {
        ch[0] = data;
        ch[1] = 0;
        mnem = ch;
    }

    sprintf(output, "%04x  %c  %02x  %-12s  %s %s %s %s %s %s %s %s %s %s %s  %s", addr, rwb, data, mnem, vda, vpa, resb, nmib, irqb, ram, via0, via1, uart, rtc, rom, msg);
    Serial.println(output);

    prev_addr = addr;
}


void loop() {
}
