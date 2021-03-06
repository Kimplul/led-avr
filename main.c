#ifndef F_CPU
#define F_CPU 1000000
#endif
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define TRUE 1
#define FALSE 0

typedef enum Operations {
        ADD = 0,
        SUB = 1,
        MULT = 2,
        DIV = 3,
        AND = 4,
        OR = 5
} Operations;

typedef struct State {
        char x;
        char y;
        char counter;
        char number_done;
        Operations operation;
} State;


State state = {0};

char texture[6][5][4] = {
        // add
        {   
                {0, 1, 0, 0, },
                {0, 1, 0, 0, },
                {1, 1, 1, 1, },
                {0, 1, 0, 0, },
                {0, 1, 0, 0, },
        },
        // sub
        {
                {0, 0, 0, 0, },
                {0, 0, 0, 0, },
                {1, 1, 1, 1, },
                {0, 0, 0, 0, },
                {0, 0, 0, 0, },
        },
        // mult
        {
                {0, 0, 0, 0, },
                {1, 0, 0, 1, },
                {0, 1, 1, 0, },
                {0, 1, 1, 0, },
                {1, 0, 0, 1, },
        },
        // div
        {
                {1, 0, 0, 0, },
                {0, 1, 0, 0, },
                {0, 0, 1, 0, },
                {0, 0, 0, 1, },
                {0, 0, 0, 0, },
        },
        // and
        {
                {1, 0, 0, 1, },
                {1, 0, 0, 1, },
                {1, 1, 1, 1, },
                {1, 0, 0, 1, },
                {0, 1, 1, 0, },
        },
        // or
        {
                {1, 0, 0, 1, },
                {1, 0, 0, 1, },
                {1, 0, 0, 1, },
                {1, 0, 0, 1, },
                {1, 0, 0, 1, },
        },
};

void init(void){
        // Initialize LED grid
        DDRA = 0xff;
        DDRB = 0x1f;
        DDRC = 0xff;
        DDRD = 0xff;
        DDRE = 0x07;

        PORTA = 0x07;
        PORTB = 0x1f;
        PORTC = 0x00;
        PORTD = 0xff;
        PORTE = 0x00;

        // Disable Analog Comparator interrupts
        // If this flag isn't cleared, the next instruction might raise an
        // interrupt.
        ACSR &= ~(_BV(ACIE));

        // Disable Analoc Comparator
        ACSR |= _BV(ACD);

        // Enable timer interrupts
        TIMSK |= _BV(TOIE0);

        // Set timer duration, CS01 and CS00 means clk/64 prescaler, so only
        // every 64th clock cycle increments the timer. Timer overflows at 256
        // Leds are a bit dim, but this is the most optimal power/brightness
        // combo
        //TCCR0 |= _BV(CS02);
        TCCR0 |= _BV(CS01);
        TCCR0 |= _BV(CS00);

        // Lower modes than idle won't work with the timer, unfortunately.
        set_sleep_mode(SLEEP_MODE_IDLE);
}

int a = 0x07;
int b = 0x1f;
int c = 0x00;
int d = 0xff;
int e = 0x00;

int flashing = TRUE;
int flash_counter = 1;

// Not necessary, just here for best practices I suppose.
ISR(TIMER0_OVF_vect){}

void sleep(){
        // Enable global interrupts so the timer will trigger
        sei();

        // Enable sleep
        sleep_enable();

        // Go to sleep
        sleep_cpu();
        // Wake up after (any) interrupt

        // Disable sleep
        sleep_disable();

        // Disable global interrupts, saves a bit of power
        cli();
}

void draw_leds(){
        int i = 0;
        int j = 0;

        // x
        PORTA = 0b00001111;
        for(i = 0; i < 8; ++i){ 
                char w = (state.x >> i) & 1;

                if(i % 2 == 0){
                        PORTD ^= w << (i / 2);
                        PORTD = d;

                } else {
                        PORTB ^= w << (4 - ((i - 1) / 2));
                        PORTB = b;
                }
        }

        // y
        PORTA = 0b00100111;
        for(i = 0; i < 8; ++i){ 
                char w = (state.y >> i) & 1;

                if(i == state.counter && !state.number_done)
                        w = flashing;

                if(i % 2 == 0){
                        PORTD ^= w << (i / 2);
                        PORTD = d;

                } else {
                        PORTB ^= w << (4 - ((i - 1) / 2));
                        PORTB = b;
                }
        }

        // image
        PORTA = a;
        for(i = 0; i < 5; ++i){
                int atemp = a ^ (1 << (7 - i));
                PORTA = atemp;

                for(j = 0; j < 4; ++j){
                        int w = texture[state.operation][i][j];
                        int u = 1;

                        if(state.number_done)
                                u = flashing;

                        if(j % 2 == 0){
                                PORTA ^= ((w & u) << 2 - (j / 2));
                                PORTA = atemp;
                        } else {
                                PORTD ^= ((w & u) << 7 - (j - 1)/2);
                                PORTD = d;
                        } 

                }

                PORTA = a;
        }

        flash_counter = (flash_counter + 1) % 72;
        if(flash_counter == 0){
                flashing ^= TRUE;
        }

        sleep();
}

void do_operation(){
        switch(state.operation){
                case ADD:
                        state.x = state.x + state.y;
                        break;
                case SUB:
                        state.x = state.x - state.y;
                        break;
                case MULT:
                        state.x = state.x * state.y;
                        break;
                case DIV:
                        state.x = state.x / state.y;
                        break;
                case AND:
                        state.x = state.x & state.y;
                        break;
                case OR:
                        state.x = state.x | state.y;
                        break;
        }

        state.y = 0;
        state.number_done = FALSE;
}

void update_state(int long_press){
        if(state.number_done){
                if(long_press)
                        do_operation();
                else
                        state.operation = (state.operation + 1) % 6;

        } else {
                if(long_press)
                        state.y |= 1 << state.counter;
                
                state.counter++;

                if(state.counter == 8){
                        state.number_done = TRUE;
                        state.counter = 0;

                        // reset flashing
                        flashing = TRUE;
                        flash_counter = 1;
                }
        }
}

void read_button(){
        int long_press = FALSE;

        while(PIND & 0x04)
                draw_leds();


        for(int i = 0; i < 42; ++i){
                draw_leds();

                if(PIND & 0x04)
                        break;
        }

        if(!(PIND & 0x04))
                long_press = TRUE;

        update_state(long_press);

        while(!(PIND & 0x04))
                draw_leds();
}

int main(){
        init();

        while(1){
                read_button();
        }
}
