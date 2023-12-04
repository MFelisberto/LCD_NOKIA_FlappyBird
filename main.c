#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include "nokia5110.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "nokia5110.h"
#include <stdio.h>
#include <string.h>

// timer clock
#define TIMER_CLK       F_CPU / 1024
#define IRQ_FREQ    30        // -> frquencia
#define BOTAO_UP    PD0       // -> botao up
#define BOTAO_DW    PD1       // -> botao down
#define BOTAO_OK    PD2       // -> botao ok
#define BOTAO_AC    PC3       // -> botao action
volatile int score = 0;       // -> para armazenar a pontuacao
volatile int telaAtual = 0;   // -> para manipulacao de telas
volatile int selecionado = 1; // -> para selecao no menu
volatile int morreu = 0;      // -> para controlar loop de reinicio do jogo e se o personagem morreu
int primeira = 1;             // -> para gerar os canos desde o "inicio" da tela
int delayCano = 1;            // -> pro delay inicial do jogo, para ter um "tempo" para o jogador
int contadorCentesimoDezena;  // -> pro score
int contadorCentesimoUnidade; // -> pro score

// glyph para a seleção do menu
uint8_t glyph[] =  {0b00111110,0b00111110,0b00011100,0b00011100,0b00001000};

// pro score
uint8_t chars[][5]  = {
    { 0x3e, 0x51, 0x49, 0x45, 0x3e }, // 30 0
    { 0x00, 0x42, 0x7f, 0x40, 0x00 }, // 31 1
    { 0x42, 0x61, 0x51, 0x49, 0x46 }, // 32 2
    { 0x21, 0x41, 0x45, 0x4b, 0x31 }, // 33 3
    { 0x18, 0x14, 0x12, 0x7f, 0x10 }, // 34 4
    { 0x27, 0x45, 0x45, 0x45, 0x39 }, // 35 5
    { 0x3c, 0x4a, 0x49, 0x49, 0x30 }, // 36 6
    { 0x01, 0x71, 0x09, 0x05, 0x03 }, // 37 7
    { 0x36, 0x49, 0x49, 0x49, 0x36 }, // 38 8
    { 0x06, 0x49, 0x49, 0x29, 0x1e }  // 39 9
};


// struc para os canos
typedef struct{
    int x;   // usamos para mover o cano pela tela
    int y1;  // parte dent8_t glyph[] =  {0b00111110,0b00111110,0b00011100,0b00011100,0b00001000}; cima do cano
    int y2;  // parte de cima de cano
    int y11; // parte de baixo do cano
    int y22; // parte de baixo do cano
} Cano; 

// canos que serao utilizados
Cano cano1 = {81, 1, 16, 32, 47};
Cano cano2 = {81, 1, 16, 32, 47};

// para o passaro
int yc = 24;
int xc = 20;
int r = 1;

//timer
void timer_init() {
 	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1  = 0;
    TCCR1B |= _BV(WGM12); 
    TCCR1B |= _BV(CS12) | _BV(CS10);  
    OCR1A = (TIMER_CLK / IRQ_FREQ) - 1; 
    TIMSK1 |= _BV(OCIE1A); 
}

// geracao de movimentacao de canos, alem de ifs para a derrota do jogador
ISR(TIMER1_COMPA_vect){
    if(telaAtual == 1 && !morreu){
        
        if(xc - 4 == cano1.x){
            contadorCentesimoUnidade++;
            if(contadorCentesimoUnidade>9){
                contadorCentesimoDezena++;
                if(contadorCentesimoDezena>9){
                    contadorCentesimoDezena = 0;
                }
                contadorCentesimoUnidade = 0;
            }
        }

        if(xc - 4 == cano2.x){
            contadorCentesimoUnidade++;
            if(contadorCentesimoUnidade>9){
                contadorCentesimoDezena++;
                if(contadorCentesimoDezena>9){
                    contadorCentesimoDezena = 0;
                }
                contadorCentesimoUnidade = 0;
            }
        }
        
        if((yc <= cano1.y2 && xc == cano1.x) || (yc >= cano1.y11 && xc == cano1.x) || (yc <= cano2.y2 && xc == cano2.x) || (yc >= cano2.y11 && xc == cano2.x)){
            morreu = 1;
        }
        
        delayCano+=1;
        
        if(delayCano>45){
            cano1.x = cano1.x - 1;
            cano2.x = cano2.x - 1;
            
            int select = rand() % 50;
            
            if(cano1.x == 43){
                primeira = 0;
                cano2.x = 81;
                if(select % 2 == 0){
                    cano2.y2 =  rand() % 32 + 2;
                    cano2.y11 =  cano2.y2 + 10;
                }else{
                    cano2.y11 =  rand() % 5 + 13;
                    cano2.y2 = cano2.y11 - 10;
                }
            }
            
            if(cano2.x == 43){
                cano1.x = 81;
                if(select % 2 != 0){
                    cano1.y2 =  rand() % 32 + 2;
                    cano1.y11 =  cano1.y2+ 10;
                }else{
                    cano1.y11 = rand() % 5 + 13;
                    cano1.y2 =  cano1.y11 - 10;
                }
            }
            
            score+=1;
        }
    }
}

// manipulacao do lcd, telas multiplas
void lcd_atualizar(int tela) {

    nokia_lcd_clear();

    switch(tela){

        case 0: // TELA INICIAL DO JOGO
            
            nokia_lcd_clear();
            nokia_lcd_set_cursor(10, 2);
            nokia_lcd_write_string("Flappy Bird",1);
            nokia_lcd_drawline(5, 14, 78, 15);
            
            nokia_lcd_set_cursor(20, 20);
            
            if(selecionado == 1){
                nokia_lcd_write_string("\005Iniciar",1);
                nokia_lcd_set_cursor(20, 30);
                nokia_lcd_write_string("\tSair",1);
                nokia_lcd_set_cursor(1, 40);
                nokia_lcd_write_string("",1);
            }
            else if(selecionado == 2){// Normal
                nokia_lcd_write_string("\tIniciar",1);
                nokia_lcd_set_cursor(20, 30);
                nokia_lcd_write_string("\005Sair",1);
                nokia_lcd_set_cursor(1, 40);
                nokia_lcd_write_string("",1);
            }
           
        break;

        case 1: // TELA DE GAMEPLAY E TELA FINAL

            if(!morreu){
                
                // grama
                nokia_lcd_drawrect(8, 47, 81, 47);
                // teto
                nokia_lcd_drawrect(8, 0, 81, 0);

                if(delayCano>45){
                    if(primeira && cano1.x >43){
                        nokia_lcd_drawrect(cano1.x+2, cano1.y1, cano1.x, cano1.y2);
                        nokia_lcd_drawrect(cano1.x+2, cano1.y11, cano1.x, cano1.y22);
                    }else{
                        nokia_lcd_drawrect(cano1.x+2, cano1.y1, cano1.x, cano1.y2);
                        nokia_lcd_drawrect(cano1.x+2, cano1.y11, cano1.x, cano1.y22);
                        nokia_lcd_drawrect(cano2.x+2, cano2.y1, cano2.x, cano2.y2);
                        nokia_lcd_drawrect(cano2.x+2, cano2.y11, cano2.x, cano2.y22);
                        nokia_lcd_custom(2,chars[contadorCentesimoUnidade]);
                        nokia_lcd_custom(1,chars[contadorCentesimoDezena]);
                        nokia_lcd_write_string("\001\002", 1);
                    }
                }
                
                nokia_lcd_drawcircle(xc, yc, r);
            
            }else{
                _delay_ms(500); 
                nokia_lcd_set_cursor(10, 2);
                nokia_lcd_write_string("GAME OVER!",1);
                nokia_lcd_set_cursor(10, 15);
                nokia_lcd_custom(2,chars[contadorCentesimoUnidade]);
                nokia_lcd_custom(1,chars[contadorCentesimoDezena]);
                nokia_lcd_write_string("SCORE: \001\002", 1);
                nokia_lcd_render();
                _delay_ms(1500);

                // qualquer coisa tira
                telaAtual = 0;
                selecionado = 1;
                morreu = 0;
                primeira = 1;
                cano1.x = 81;
                cano2.x = 81;
                yc = 24;
                contadorCentesimoDezena = 0;
                contadorCentesimoUnidade = 0;
            }
        
        break;

    }
            
    nokia_lcd_render();
}

int main(void){
    
    // Botao UP
    DDRD &= ~(1 << BOTAO_UP);
    PORTD |= (1 << BOTAO_UP);
   
    // 
    DDRD &= ~(1 << BOTAO_DW);
    PORTD |= (1 << BOTAO_DW);
   
    DDRD &= ~(1 << BOTAO_OK);
    PORTD |= (1 << BOTAO_OK);
   
    DDRC &= ~(1 << BOTAO_AC);
    PORTC |= (1 << BOTAO_AC);

    
    cli();
    nokia_lcd_init();
    nokia_lcd_clear();
    nokia_lcd_custom(5, glyph);
    
    timer_init();
    sei();
    
    while(1){
        
        while(telaAtual == 0){
            lcd_atualizar(0);

            if (!(PIND & (1 << BOTAO_UP))){
                selecionado = 1;
                _delay_ms(150);
            }
            if (!(PIND & (1 << BOTAO_DW))){
                selecionado = 2;
                _delay_ms(150);
            }


            if (!(PIND & (1 << BOTAO_OK)) && selecionado == 1){
                telaAtual = 1;
            }
            else if (!(PIND & (1 << BOTAO_OK)) && selecionado == 2){
                nokia_lcd_power(0);
            }
        }

        lcd_atualizar(1);
        
        yc = yc + 1;

        if(yc==46){
            morreu = 1;
        }
        
        if(!(PINC & (1 << BOTAO_AC))) {
            if(yc>4){
                yc = yc - 3;
            }
            else{
                morreu = 1;
            }      
        }
        
    }
}