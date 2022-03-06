
const char DATA[] = {9, 8, 7, 6, 5, 4, 3, 2};
const char string[] = "fuckers, guys";

#define E   10
#define RW  11
#define RS  12


void set_data_output() {

    for (int i = 0; i < 8; i++) {
        pinMode(DATA[i], OUTPUT);
    }
}


void set_data_input() {

    for (int i = 0; i < 8; i++) {
        pinMode(DATA[i], INPUT);
    }
}


void setup() {

    pinMode(E, OUTPUT);
    pinMode(RW, OUTPUT);
    pinMode(RS, OUTPUT);
    
    Serial.begin(57600);

    Serial.write("Sending instr 0\r\n");
    send_instr(0x38);
    Serial.write("Sending instr 1\r\n");
    send_instr(0x0e);
    Serial.write("Sending instr 2\r\n");
    send_instr(0x06);
    Serial.write("Sending instr 3\r\n");
    send_instr(0x01);

    for (int i = 0; i < strlen(string); i++) {
        send_char(string[i]);
    }
}


void wait_on_instr(void) {

    set_data_input();

    digitalWrite(RW, 1);
    digitalWrite(RS, 0);
    
    while (1) {
        digitalWrite(E, 0);
        digitalWrite(E, 1);
        if (digitalRead(DATA[7]) == 0) {
            break;
        }
    }
}


void send_instr(int code) {

    wait_on_instr();

    set_data_output();
    digitalWrite(RW, 0);
    digitalWrite(RS, 0);

    digitalWrite(DATA[0], ((code & 0x01) >> 0));
    digitalWrite(DATA[1], ((code & 0x02) >> 1));
    digitalWrite(DATA[2], ((code & 0x04) >> 2));
    digitalWrite(DATA[3], ((code & 0x08) >> 3));
    digitalWrite(DATA[4], ((code & 0x10) >> 4));
    digitalWrite(DATA[5], ((code & 0x20) >> 5));
    digitalWrite(DATA[6], ((code & 0x40) >> 6));
    digitalWrite(DATA[7], ((code & 0x80) >> 7));

    digitalWrite(E, 0);
    digitalWrite(E, 1);
    digitalWrite(E, 0);
}


void send_char(char ch) {

    wait_on_instr();

    set_data_output();
    digitalWrite(RW, 0);
    digitalWrite(RS, 1);
    
    digitalWrite(DATA[0], ((ch & 0x01) >> 0));
    digitalWrite(DATA[1], ((ch & 0x02) >> 1));
    digitalWrite(DATA[2], ((ch & 0x04) >> 2));
    digitalWrite(DATA[3], ((ch & 0x08) >> 3));
    digitalWrite(DATA[4], ((ch & 0x10) >> 4));
    digitalWrite(DATA[5], ((ch & 0x20) >> 5));
    digitalWrite(DATA[6], ((ch & 0x40) >> 6));
    digitalWrite(DATA[7], ((ch & 0x80) >> 7));

    digitalWrite(E, 0);
    digitalWrite(E, 1);
    digitalWrite(E, 0);
}


void loop() {
}
