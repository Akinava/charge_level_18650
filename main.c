// ============================================================================
//
// lent from http://tinusaur.org
//
// ============================================================================

#define F_CPU 8000000UL

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>

//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//                ATtiny 45
//              +----------+
//  ----(RST)---+ PB5  Vcc +---(+)-------
// -(CRGING)----+ PB3  PB2 +---[TWI/SCL]-
//  ----(ADC)---+ PB4  PB1 +---(BUZZER)--
//  ------(-)---+ GND  PB0 +---[TWI/SDA]-
//              +----------+
//              
//  ---- (SDA)--+------------------------+
//  ---- (SCK)--+  oled display i2c      +
//  ---- (VCC)--+  ssd1306 driver 32x128 +
//  ---- (GND)--+------------------------+
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// -----(+)--------------->   // Vcc, Pin 1 on SSD1306 Board
// -----(-)--------------->   // GND, Pin 2 on SSD1306 Board
#define SSD1306_SCL          PB2   // SCL, Pin 3 on SSD1306 Board
#define SSD1306_SDA          PB0   // SDA, Pin 4 on SSD1306 Board
#define SSD1306_SA           0x78  // Slave address

#define BUZZER_PIN           PB1   //
#define CHARGING_SIGNAL_PIN  PB3   // charging singnal

#define CHAR_HEIGHT          8     // lines
#define CHAR_WIDTH           10    // px
#define CHAR_LINE            2     // px
#define CHAR_INDENT          4     // px

#define POSITION_V           114   // px          
#define POSITION_PERCENT     48    // px


#define UPDATE_PERIOD        600   // 1  min
#define BUZZER_PERIOD        12000 // 10 min
#define ON                   1     // buzzer mode
#define OFF                  0     // buzzer mode

#define LOW_LEVEL            25    // %
#define PERCENT_100          3.9   // v max
#define PERCENT_0            2.7   // v min

#define VOLTS_VALUE_1        4.88
#define ADC_VALUE_1          804
#define VOLTS_VALUE_2        2.79
#define ADC_VALUE_2          440

#define SET_HIGH(PORT) PORTB |= (1 << PORT)
#define SET_LOW(PORT) PORTB &= ~(1 << PORT)

// ----------------------------------------------------------------------------

void send_byte(uint8_t byte);
void g_line(uint8_t x_start, uint8_t x_end, uint8_t y, uint8_t b);

const uint8_t SYMBOL_V [] = {
    0x0F, 0xFC, 0xC0, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFC, 0x0F,
    0x00, 0x00, 0x0F, 0x38, 0xF0, 0xF0, 0x38, 0x0F, 0x00, 0x00,
};

const uint8_t SYMBOL_PERCENT [] = {
    0x7E, 0xFF, 0xFF, 0x7E, 0x00, 0x00, 0x00, 0xF0, 0xFF, 0x0F,
    0x00, 0x00, 0x00, 0x00, 0xC0, 0xF0, 0x7F, 0x0F, 0x00, 0x00,
    0x00, 0x00, 0xF0, 0xFE, 0x0F, 0x03, 0x00, 0x00, 0x00, 0x00,
    0xF0, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x7E, 0xFF, 0xFF, 0x7E,
};

const uint8_t SYMBOL_FLASH [] = {
    0x00, 0x00, 0x00, 0x00, 0x80, 0x60, 0x10, 0x08, 0x06, 0x01,
    0x70, 0xF8, 0xEE, 0xC7, 0xC1, 0x80, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x83, 0xE3, 0x77, 0x1F, 0x0E,
    0x80, 0x60, 0x10, 0x08, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00,
};

const uint8_t init_sequence [] = {  // Initialization Sequence
    0xAE,           // Display OFF (sleep mode)
    0x20, 0b00,     // Set Memory Addressing Mode
                    // 00=Horizontal Addressing Mode; 01=Vertical Addressing Mode;
                    // 10=Page Addressing Mode (RESET); 11=Invalid
    0xB0,           // Set Page Start Address for Page Addressing Mode, 0-7
    0xC8,           // Set COM Output Scan Direction
    0x00,           // ---set low column address
    0x10,           // ---set high column address
    0x40,           // --set start line address
    0x81, 0x3F,     // Set contrast control register
    0xA1,           // Set Segment Re-map. A0=address mapped; A1=address 127 mapped. 
    0xA6,           // Set display mode. A6=Normal; A7=Inverse
    0xA8, 0x3F,     // Set multiplex ratio(1 to 64)
    0xA4,           // Output RAM to Display
                    // 0xA4=Output follows RAM content; 0xA5,Output ignores RAM content
    0xD3, 0x00,     // Set display offset. 00 = no offset
    0xD5,           // --set display clock divide ratio/oscillator frequency
    0xF0,           // --set divide ratio
    0xD9, 0x22,     // Set pre-charge period
    0xDA, 0x12,     // Set com pins hardware configuration      
    0xDB,           // --set vcomh
    0x20,           // 0x20,0.77xVcc
    0x8D, 0x14,     // Set DC-DC enable
    0xAF            // Display ON in normal mode
    
};
//                             0     1     2     3     4     5     6     7     8     9     0
const uint8_t num_map [] = {0xBE, 0x14, 0xEC, 0xEA, 0x5A, 0xF2, 0xF6, 0x8A, 0xFE, 0xFA, 0xBE};

void send_byte(uint8_t byte){
    uint8_t i;
    for (i = 0; i < 8; i++){
        if ((byte << i) & 0x80)
            SET_HIGH(SSD1306_SDA);
        else
            SET_LOW(SSD1306_SDA);
        
        SET_HIGH(SSD1306_SCL);
        SET_LOW(SSD1306_SCL);
    }
    SET_HIGH(SSD1306_SDA);
    SET_HIGH(SSD1306_SCL);
    SET_LOW(SSD1306_SCL);
}

void xfer_start(void){
    SET_HIGH(SSD1306_SCL);  // Set to HIGH
    SET_HIGH(SSD1306_SDA);  // Set to HIGH
    SET_LOW(SSD1306_SDA);   // Set to LOW
    SET_LOW(SSD1306_SCL);   // Set to LOW
}

void xfer_stop(void){
    SET_LOW(SSD1306_SCL);   // Set to LOW
    SET_LOW(SSD1306_SDA);   // Set to LOW
    SET_HIGH(SSD1306_SCL);  // Set to HIGH
    SET_HIGH(SSD1306_SDA);  // Set to HIGH
}

void send_command_start(void){
    xfer_start();
    send_byte(SSD1306_SA);  // Slave address, SA0=0
    send_byte(0x00);        // write command
}

void send_command_stop(void){
    xfer_stop();
}

void send_command(uint8_t command){
    send_command_start();
    send_byte(command);
    send_command_stop();
}

void send_data_start(void){
    xfer_start();
    send_byte(SSD1306_SA);
    send_byte(0x40);    //write data
}

void send_data_stop(void){
    xfer_stop();
}

void setpos(uint8_t x, uint8_t y){
    send_command_start();
    send_byte(0xb0 + y);
    send_byte(((x & 0xf0) >> 4) | 0x10); // | 0x10
    send_byte((x & 0x0f));               // | 0x01
    send_command_stop();
}

void clean_area(uint8_t x1, uint8_t x2){
    uint8_t y;
    for (y = 0; y < 8; y++){
        g_line(x1, x2, y, 0x00);
    }
}

void display_clear(void){
    clean_area(0, 128);
}

void display_init(void){
    DDRB |= (1 << SSD1306_SDA); // Set port as output
    DDRB |= (1 << SSD1306_SCL); // Set port as output
    uint8_t i;  
    for (i = 0; i < sizeof(init_sequence); i++){
        send_command(init_sequence[i]);
    }
}

void g_line(uint8_t x_start, uint8_t x_end, uint8_t y, uint8_t b){
    setpos(x_start, y);
    uint8_t i;
    send_data_start();
    for (i = x_start; i < x_end; i++){
        send_byte(b);
    }
    send_data_stop();
}

void v_line(uint8_t x, uint8_t y_start, uint8_t y_end, uint8_t b){
    uint8_t i;
    uint8_t k;
    for (i = y_start; i <= y_end; i++){
        setpos(x, i);
        send_data_start();
        for (k = 0; k < CHAR_LINE; k++){
            send_byte(b);
        }
        send_data_stop();
    }
}

uint8_t l_indent(uint8_t indent, uint8_t x){
    if (indent){
        return x + CHAR_LINE;
    }
    return x;
}

uint8_t r_indent(uint8_t indent, uint8_t x){
    if (indent){
        return x + CHAR_WIDTH - CHAR_LINE;
    }
    return x + CHAR_WIDTH;
}


uint8_t print_n(uint8_t x, uint8_t n){
    // x is the right border of number
    uint8_t m = num_map[n];
    uint8_t x1;
    uint8_t x2;
    uint8_t y1;
    uint8_t y2;
    
    if (n == 1){
        x = x - CHAR_LINE;
    }else{
        x = x - CHAR_WIDTH;
    }

    if (m & 0x80){
        x1 = l_indent(m & 0x10, x);
        x2 = r_indent(m & 0x08, x);
        g_line(x1, x2, 0, 0x0A);
    }
    if (m & 0x40){
        x1 = l_indent(m & 0x10, x);
        x2 = r_indent(m & 0x08, x);
        g_line(x1, x2, CHAR_HEIGHT/2-1, 0x80);
        x1 = l_indent(m & 0x04, x);
        x2 = r_indent(m & 0x02, x);
        g_line(x1, x2, CHAR_HEIGHT/2, 0x02);
    }
    if (m & 0x20){
        x1 = l_indent(m & 0x04, x);
        x2 = r_indent(m & 0x02, x);
        g_line(x1, x2, CHAR_HEIGHT-1, 0xA0);
    }
    if (m & 0x10){
        v_line(x, 0, CHAR_HEIGHT/2-1, 0xAA);
    }
    if (m & 0x08){
        v_line(x+CHAR_WIDTH-CHAR_LINE, 0, CHAR_HEIGHT/2-1, 0xAA);
    }
    if (m & 0x04){
        v_line(x, CHAR_HEIGHT/2, CHAR_HEIGHT, 0xAA);
    }
    if (m & 0x02){
        v_line(x+CHAR_WIDTH-CHAR_LINE, CHAR_HEIGHT/2, CHAR_HEIGHT, 0xAA);
    }
    return x;
}

uint8_t map_bit(uint8_t lv, uint8_t b){
    uint8_t dv;
    uint8_t res = 0x00;
    if (lv){
        b = b >> 4;
    }
    for (dv = 0; dv < 4; dv++){
        res |= ((1 << (dv*2+1)) & b << (dv+1));
    }
    return res;
}

void drow_map(uint8_t x, uint8_t y, uint8_t height, const uint8_t *map){
    uint8_t i;
    uint8_t k;
    setpos(x, y);
    for (i = 0; i < height*2; i++){
        setpos(x, y+i);
        send_data_start();
        for (k = 0; k < CHAR_WIDTH; k++){
            send_byte(map_bit(i%2, map[i/2*CHAR_WIDTH+k]));
        }
        send_data_stop();
    }
}

uint8_t print_number(uint8_t eos, uint16_t n){
    // eos / end of string
    eos = print_n(eos, n%10);
    while(n/10 > 0){
        n /= 10;
        eos = print_n(eos-CHAR_INDENT, n%10);
    }
    return eos;
}

void print_float(uint8_t eos, float f){
    uint16_t i = f * 1000.;
    // fractional part
    eos = print_number(eos, i / 10 % 10);
    eos = print_number(eos-CHAR_INDENT, i / 100 % 10);
    // dot
    g_line(eos - CHAR_INDENT - CHAR_LINE, eos - CHAR_INDENT, 7, 0xA0);
    // integer part
    print_number(eos - 2 * CHAR_INDENT - CHAR_LINE, i / 1000 % 10);
}

void beep(void){
    uint16_t i;
    for (i = 0; i < 30; i++){

        SET_HIGH(BUZZER_PIN);
        _delay_us(50);
        SET_LOW(BUZZER_PIN);
        _delay_us(550);
    }
}

void init_adc(void){
    ADMUX =
        (0 << ADLAR) |     // left shift result
        (0 << REFS2) |     // 
        (1 << REFS1) |     // Sets ref. voltage to internal 1.1V, bit 1
        (0 << REFS0) |     // 
        (0 << MUX3)  |     // use ADC2 for input (PB4), MUX bit 3
        (0 << MUX2)  |     // use ADC2 for input (PB4), MUX bit 2
        (1 << MUX1)  |     // use ADC2 for input (PB4), MUX bit 1
        (0 << MUX0);       // use ADC2 for input (PB4), MUX bit 0

    ADCSRA = 
        (1 << ADEN)  |     // Enable ADC 
        (1 << ADPS2) |     // set prescaler to 64, bit 2 
        (1 << ADPS1) |     // set prescaler to 64, bit 1 
        (1 << ADPS0);      // set prescaler to 64, bit 0  
}

uint16_t read_adc(void){
    uint8_t adc_lobyte;
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    adc_lobyte = ADCL;
    return ADCH<<8 | adc_lobyte;
}

uint8_t check_charger(void){
    if (PINB & (1 << CHARGING_SIGNAL_PIN)){
        drow_map(0, 0, sizeof(SYMBOL_FLASH)/CHAR_WIDTH, SYMBOL_FLASH);
        return OFF;
    }
    clean_area(0, 10);
    return ON;
}

int main(void){

    // Small delay is necessary if init is the first operation in the application.
    _delay_ms(40);
    DDRB |= (1 << BUZZER_PIN);         // Set port as output
    DDRB &= ~(1<<CHARGING_SIGNAL_PIN); // Set port as input
    display_init();
    display_clear();
    init_adc();
    _delay_ms(50);

    const float volts_k = (VOLTS_VALUE_1 - VOLTS_VALUE_2) / (ADC_VALUE_1 - ADC_VALUE_2);
    const float volts_b = VOLTS_VALUE_1 - volts_k * ADC_VALUE_1;
    const float PERCENT_K = 100 / (PERCENT_100 - PERCENT_0) * volts_k;
    const float PERCENT_B = 100 * (volts_b - PERCENT_100)/(PERCENT_100 - PERCENT_0) + 100;

    drow_map(POSITION_PERCENT + CHAR_INDENT, 0, sizeof(SYMBOL_PERCENT)/CHAR_WIDTH, SYMBOL_PERCENT);
    drow_map(POSITION_V + CHAR_INDENT, 4, sizeof(SYMBOL_V)/CHAR_WIDTH, SYMBOL_V);

    uint16_t adc_10b;
    uint8_t adc_prc;
    float adc_v;
    uint8_t buzzer_mode = ON;
    uint16_t i;
    for (;;){
        // calibrate mode
        /*
        print_number(127, read_adc());
        display_clear();
        */

        // main mode start
        for (i=0; i< BUZZER_PERIOD; i++){
            buzzer_mode = check_charger();
            if (i % UPDATE_PERIOD == 0){
                adc_10b = read_adc();
                
                adc_v = adc_10b * volts_k + volts_b;
                clean_area(POSITION_V - (CHAR_WIDTH * 3 + CHAR_INDENT * 5 + 2), POSITION_V);
                print_float(POSITION_V, adc_v);
                 
                if (adc_10b * PERCENT_K + PERCENT_B < 0){
                    adc_prc = 0;
                }else{
                    adc_prc = adc_10b * PERCENT_K + PERCENT_B;
                }
                if (adc_prc > 100)
                    adc_prc = 100;
                clean_area(POSITION_PERCENT- (CHAR_WIDTH * 2 + CHAR_INDENT * 2 + 2), POSITION_PERCENT);
                print_number(POSITION_PERCENT, adc_prc);
            }

            if (buzzer_mode & i % BUZZER_PERIOD == 0){
                if (adc_prc <= LOW_LEVEL){
                    beep();
                }
            }
        }
        // main mode end
    }
    return 0;
}
