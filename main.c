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

// glyph para a seleção do menu
uint8_t glyph[] =  {0b00111110,0b00111110,0b00011100,0b00011100,0b00001000};


// struc para os canos
typedef struct{
    int x;   // usamos para mover o cano pela tela
    int y1;  // parte de cima do cano
    int y2;  // parte de cima de cano
    int y11; // parte de baixo do cano
    int y22; // parte de baixo do cano
} Cano; 

// canos que serao utilizados
Cano cano1 = {83, 1, 16, 32, 47};
Cano cano2 = {83, 1, 16, 32, 47};

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
    if(telaAtual == 1){
        
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
                cano2.x = 83;
                if(select % 2 == 0){
                    cano2.y2 =  rand() % 32 + 2;
                    cano2.y11 =  cano2.y2 + 10;
                }else{
                    cano2.y11 =  rand() % 16 + 2;
                    cano2.y2 = cano2.y11 - 10;
                }
            }
            
            if(cano2.x == 43){
                cano1.x = 83;
                if(select % 2 != 0){
                    cano1.y2 =  rand() % 32 + 2;
                    cano1.y11 =  cano1.y2+ 10;
                }else{
                    cano1.y11 =  rand() % 16 + 2;
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
                nokia_lcd_write_string("\001Iniciar",1);
                nokia_lcd_set_cursor(20, 30);
                nokia_lcd_write_string("\tSair",1);
                nokia_lcd_set_cursor(1, 40);
                nokia_lcd_write_string("99",1);
            }
            else if(selecionado == 2){// Normal
                nokia_lcd_write_string("\tIniciar",1);
                nokia_lcd_set_cursor(20, 30);
                nokia_lcd_write_string("\001Sair",1);
                nokia_lcd_set_cursor(1, 40);
                nokia_lcd_write_string("99",1);
            }
           
        break;

        case 1: // TELA DE GAMEPLAY E TELA FINAL

            if(!morreu){
                
                // grama
                nokia_lcd_drawrect(1, 47, 83, 47);
                // teto
                nokia_lcd_drawrect(1, 0, 83, 0);

                if(delayCano>45){
                    if(primeira && cano1.x >43){
                        nokia_lcd_drawrect(cano1.x, cano1.y1, cano1.x + 2, cano1.y2);
                        nokia_lcd_drawrect(cano1.x, cano1.y11, cano1.x + 2, cano1.y22);
                    }else{
                        nokia_lcd_drawrect(cano1.x, cano1.y1, cano1.x + 2, cano1.y2);
                        nokia_lcd_drawrect(cano1.x, cano1.y11, cano1.x + 2, cano1.y22);
                        nokia_lcd_drawrect(cano2.x, cano2.y1, cano2.x + 2, cano2.y2);
                        nokia_lcd_drawrect(cano2.x, cano2.y11, cano2.x + 2, cano2.y22);
                    }
                }
                
                nokia_lcd_drawcircle(xc, yc, r);
            
            }else{
                _delay_ms(500); 
                nokia_lcd_set_cursor(10, 2);
                nokia_lcd_write_string("Fim de jogo!",1);
                nokia_lcd_set_cursor(10, 15);
                nokia_lcd_write_string("Score: ",1);
                nokia_lcd_write_string("score",1);
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
    nokia_lcd_custom(1, glyph);
    
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