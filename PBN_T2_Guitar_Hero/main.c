#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include "nokia5110.h"
#include <avr/interrupt.h>

/*
    Dimensoes tela:
        x: 0 a 83 (horizontal) 
        y: 0 a 47 (vertical)

*/

#define TIMER_CLK		F_CPU / 1024
// Freq. de interrupcao: 30 Hz --> Interrupcao de aprox. 33,33ms (1 seg = 1000ms)
#define IRQ_FREQ		30

// uint8_t glyph[] =  {0b00000000, 
//                     0b00000000, 
//                     0b00000000, 
//                     0b00000000, 
//                     0b00000000};

// glyph gira 90 graus 'a esquerda no texto
uint8_t glyph[] =  {0b00111110, 
                    0b00111110, 
                    0b00011100, 
                    0b00011100, 
                    0b00001000};

/*

    Prototipos de Funcoes

*/
void atualizaTela(int tela);
void deslocaColuna();


/*

    Variaveis globais principais com dados do jogo

*/
volatile int score = 0; // pontuacao
volatile uint8_t notasMax = 0; // quantidade de notas (gerada por seed)
volatile uint8_t notasGeradas = 0; // contador para notas criadas (indo ate notasMax)
volatile int8_t lives = 0; // vidas --> definido a partir da seed (ou de notasMax)
volatile uint8_t streak = 0; // notas acertadas em sequencia
volatile uint8_t streakRecorde = 0; // maior streak realizada
volatile uint8_t notasPos = 0b0000; // posicao das notas (gerado por seed e manipulado como binario)

/*

    Variaveis globais booleanas

*/
volatile uint8_t iniciar = 0; // define inicio da execucao do programa
volatile uint8_t playing = 0; // define inicio do jogo (surgimento de notas)
volatile uint8_t geraNota = 0; // ativa area de leitura no while(1)
volatile uint8_t existeNota = 0; // indica se ainda existem notas a passar por toda a tela
volatile uint8_t acao = 0; // usado para definir acao nos menus do jogo (alterado por w/s)
volatile uint8_t telaAtual = 0; // usado para alternacao entre telas 
/*
    0 --> menu incial/final
    1 --> jogo carregando
    2 --> jogo rodando
    3 --> resultados vitoria/derrota
*/

/*

    Variaveis globais auxiliares (limites, vetores)

*/
// contador para gerar notas com espaçamento correto
volatile uint8_t tempoGeraNotas = 48;  
#define limiteGeraNotas 48

//vetores que atuam como colunas de cada nota na tela
volatile uint8_t notasColuna1[48];
volatile uint8_t notasColuna2[48];
volatile uint8_t notasColuna3[48];
volatile uint8_t notasColuna4[48];


// int que informam se a notas são cheias (0) ou vazias (1)
// Nota cheia --> pontua se nao apertar o botao da coluna
// Nota vazia --> pontua se apertar o botao da coluna
volatile uint8_t tipoDaNota[4] = {0, 0, 0, 0};

//booleanos para permitir leitura da nota ao alcancar a linha
uint8_t leituraInicialNotas[4] = {1, 1, 1, 1}; 

// booleanos para permitir leitura da nota apos ter alcancado a linha
uint8_t leituraContinuaNotas[4] = {0, 0, 0, 0}; 

// indicadores de posicao das notas nas suas colunas, inicializados na posicao de leitura
uint8_t posicaoLeitura1 = 25;
uint8_t posicaoLeitura2 = 25;
uint8_t posicaoLeitura3 = 25;
uint8_t posicaoLeitura4 = 25;

// boolean para indicar se errou uma nota
uint8_t ocorreuErro = 0;




/*
    // botao iniciar --> PD3
*/
ISR(INT1_vect)
{
    if(acao == 1){// se escolheu sair do jogo
        nokia_lcd_power(0);
    }
        else if(!iniciar){
            iniciar =  !iniciar;
            playing = 1;
            geraNota = 1;
            telaAtual = 1;
            notasGeradas = 0;
            streak = 0;
            streakRecorde = 0;
            score = 0;
            tempoGeraNotas = 48;
            
        }
}

/*
    // botoes das setas em menus --> PD0 - PD1
*/
ISR(PCINT2_vect) {
    if(PIND & ((1 << PD0) | (1 << PD1))){
       acao = !acao;
    }
}


/*
    // TIMER1 --> interrupcao baseada na frequência (IRQ_FREQ)
*/
ISR(TIMER1_COMPA_vect)
{
    if(!iniciar){
        // notasMax = ((rand() % 6) + 5) * 10; // gera espaco para 50 - 100 notas
        notasMax = ((rand() % 11) + 5) * 10; // gera espaco para 50 - 150 notas
        lives = notasMax/10;
    }

    if(telaAtual == 2){

        //se contador de espacamento entre notas == limite E ainda pode criar notas
        if(tempoGeraNotas == limiteGeraNotas && notasGeradas < notasMax){

            notasPos = (rand() % 15) +1; // gera posicoes aleatorias para notas, usa 16 pois o valor tem 4 bits

            tempoGeraNotas = 0;

            if(((notasPos & 0b1000) >> 3) && notasGeradas < notasMax){// nota 1 --> PC3
                int tipoNota = (rand() % 2);// define tipo da nota --> 0 = cheia / 1 = vazia
                tipoDaNota[0] = tipoNota;

                notasGeradas++;
                notasColuna1[0] = 1;
            }
            if(((notasPos & 0b0100) >> 2) && notasGeradas < notasMax){// nota 2 --> PC2
                int tipoNota = (rand() % 2);// define tipo da nota --> 0 = cheia / 1 = vazia
                tipoDaNota[1] = tipoNota;

                notasGeradas++;
                notasColuna2[0] = 1;
            }
            if(((notasPos & 0b0010) >> 1) && notasGeradas < notasMax){// nota 3 --> PC1
                int tipoNota = (rand() % 2);// define tipo da nota --> 0 = cheia / 1 = vazia
                tipoDaNota[2] = tipoNota;

                notasGeradas++;
                notasColuna3[0] = 1;
            }
            if(((notasPos & 0b0001) && notasGeradas < notasMax)){// nota 4 --> PC0
                int tipoNota = (rand() % 2);// define tipo da nota --> 0 = cheia / 1 = vazia
                tipoDaNota[3] = tipoNota;

                notasGeradas++;
                notasColuna4[0] = 1;
            }     
        }

        
        tempoGeraNotas++;

        deslocaColuna();// desloca os 4 vetores
        
    }
}


/*
    main (leitura de notas e fluxo entre telas)
*/
int main(void)
{
    cli(); // desabilita interrupções
    /*
        Timer1 de 16 bits

    */
	// resseta contadores para TIMER1
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1  = 0;
	// seta o registrador output compare
	OCR1A = (TIMER_CLK / IRQ_FREQ) - 1;

	// liga modo CTC (Clear Time on Compare Match)
	TCCR1B |= (1 << WGM12);

	// seta CS10 e CS12 para prescaler = 1024
	TCCR1B |= (1 << CS12) | (1 << CS10);

	// habilita máscara do timer1
    //(OCIE1A -> Output Compare Interrupt Enable 1A)
	TIMSK1 |= (1 << OCIE1A);


    /*
        Configuracao de entradas
    */

    DDRC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3)); // set PC0-PC3 como entrada
    DDRD &= ~(1 << PD3);// set PD3 como entrada
    PORTD = (1 << PD3);// ativa pull-up em PD3

    // configura interrupcao de PD3
    EICRA = (1 << ISC11) | (1 << ISC10);// borda de subida para INT1 (botao de iniciar)
    EIMSK |= (1 << INT1); // ativa INT1

    // habilita interrupcao em Port D (PCIE2)
    PCICR |= (1 << PCIE2); 
    
    // configura interrupcao de PD0 - PD1 para teclas cima-baixo (apenas menus)
    PCMSK2 |= (1 << PCINT16) | (1 << PCINT17); // ativa PCINT16 - PCINT17 (PD0 - PD1)
    
    sei(); //habilita interrupcoes

    srand(time(NULL)); //permite gerar numeros aleatorios em ordem diferente a cada execucao

    nokia_lcd_custom(1, glyph);

    nokia_lcd_init();
    while (1)
    {
        nokia_lcd_clear();

        if(telaAtual == 0 || telaAtual == 1 || telaAtual == 3){
            atualizaTela(telaAtual);
            nokia_lcd_render();

            if(telaAtual == 3){
                _delay_ms(10000);
                telaAtual = 0;
            }
        }

        if(geraNota){
            atualizaTela(telaAtual);

            if(telaAtual == 2){
                for(int i = 0; i<48; i++)//print notas
                {
                    if(notasColuna1[i]){// nota 1 --> PC3
                        if(tipoDaNota[0]){
                            nokia_lcd_drawrect(38,i, 41, i+12);
                        }else {
                            nokia_lcd_fullrect(38,i, 41, i+12);
                        }
                    }

                    if(notasColuna2[i]){// nota 2 --> PC2
                        if(tipoDaNota[1]){// nota 
                            nokia_lcd_drawrect(50, i, 53, i+12);
                        }else {
                            nokia_lcd_fullrect(50, i, 53, i+12);
                        }
                    }

                    if(notasColuna3[i]){// nota 3 --> PC1
                        if(tipoDaNota[2]){
                            nokia_lcd_drawrect(62, i, 65, i+12);
                        }else {
                            nokia_lcd_fullrect(62, i, 65, i+12);
                        }
                    }

                    if(notasColuna4[i]){// nota 4 --> PC0
                        if(tipoDaNota[3]){
                            nokia_lcd_drawrect(74, i, 77, i+12);
                        }else {
                            nokia_lcd_fullrect(74, i, 77, i+12);
                        }
                    }
                }

                /*
                    Leitura da nota 1 (PC3)
                */
                //se esta no inicio de leitura e tem nota
                if(posicaoLeitura1 == 25 && notasColuna1[posicaoLeitura1]){
                    // se leitura inicial ativada
                    if(leituraInicialNotas[0]){
                        // se botao foi presssionado com nota vazia, ou nao pressionado com nota cheia
                        if((tipoDaNota[0] == 1 && PINC & (1 << PC3)) || (tipoDaNota[0] == 0 && !(PINC & (1 << PC3)))){
                            streak++;
                            leituraInicialNotas[0] = 0;
                            leituraContinuaNotas[0] = 1;
                            score+=5;

                            // se continuacao da leitura ativada
                            if(leituraContinuaNotas[0]){
                                score+=5;
                            }
                        }
                            // se nao pressionou o botao no momento de leitura inicial
                            else{
                                leituraInicialNotas[0] = 0;
                                ocorreuErro = 1;
                                leituraContinuaNotas[0] = 0;
                            }
                       posicaoLeitura1++;         
                    }
                    
                }
                    //se nao, se posicao de leitura esta no meio da nota
                    else if(posicaoLeitura1 > 25 && posicaoLeitura1 < 36){
                        // se leitura continua ativada, aumenta score
                        if(leituraContinuaNotas[0]){
                            score+=5;
                        }
                        posicaoLeitura1++;

                        // se nota chegou na posicao final, reinicia contadores da coluna
                        if(posicaoLeitura1 >= 36){
                            posicaoLeitura1 = 25;
                            leituraInicialNotas[0] = 1;
                            leituraContinuaNotas[0] = 0;
                        }
                    }


                /*
                    Leitura da nota 2 (PC2)
                */
                //se esta no inicio de leitura e tem nota
                if(posicaoLeitura2 == 25 && notasColuna2[posicaoLeitura2]){
                    // se leitura inicial ativada
                    if(leituraInicialNotas[1]){
                        // se botao foi presssionado com nota vazia, ou nao pressionado com nota cheia
                        if((tipoDaNota[1] == 1 && PINC & (1 << PC2)) || (tipoDaNota[1] == 0 && !(PINC & (1 << PC2)))){
                            streak++;
                            leituraInicialNotas[1] = 0;
                            leituraContinuaNotas[1] = 1;
                            score+=5;

                            // se continuacao da leitura ativada
                            if(leituraContinuaNotas[1]){
                                score+=5;
                            }
                        }
                            // se nao pressionou o botao no momento de leitura inicial
                            else{
                                leituraInicialNotas[1] = 0;
                                ocorreuErro = 1;
                                leituraContinuaNotas[1] = 0;
                            }
                       posicaoLeitura2++;         
                    }
                    
                }
                    //se nao, se posicao de leitura esta no meio da nota
                    else if(posicaoLeitura2 > 25 && posicaoLeitura2 < 36){
                        // se leitura continua ativada, aumenta score
                        if(leituraContinuaNotas[1]){
                            score+=5;
                        }
                        posicaoLeitura2++;
                        // se nota chegou na posicao final, reinicia contadores da coluna
                        if(posicaoLeitura2 >= 36){
                            posicaoLeitura2 = 25;
                            leituraInicialNotas[1] = 1;
                            leituraContinuaNotas[1] = 0;
                        }
                    }


                /*
                    Leitura da nota 3 (PC1)
                */
                //se esta no inicio de leitura e tem nota
                if(posicaoLeitura3 == 25 && notasColuna3[posicaoLeitura3]){
                    // se leitura inicial ativada
                    if(leituraInicialNotas[2]){
                        // se botao foi presssionado com nota vazia, ou nao pressionado com nota cheia
                        if((tipoDaNota[2] == 1 && PINC & (1 << PC1)) || (tipoDaNota[2] == 0 && !(PINC & (1 << PC1)))){
                            streak++;
                            leituraInicialNotas[2] = 0;
                            leituraContinuaNotas[2] = 1;
                            score+=5;
                        
                            // se continuacao da leitura ativada
                            if(leituraContinuaNotas[2]){
                                score+=5;
                            }
                        }
                            // se nao pressionou o botao no momento de leitura inicial
                            else{
                                leituraInicialNotas[2] = 0;
                                ocorreuErro = 1;
                                leituraContinuaNotas[2] = 0;
                            }
                       posicaoLeitura3++;         
                    }
                    
                }
                    //se nao, se posicao de leitura esta no meio da nota
                    else if(posicaoLeitura3 > 25 && posicaoLeitura3 < 36){
                        // se leitura continua ativada, aumenta score
                        if(leituraContinuaNotas[2]){
                            score+=5;
                        }
                        posicaoLeitura3++;
                        // se nota chegou na posicao final, reinicia contadores da coluna
                        if(posicaoLeitura3 >= 36){
                            posicaoLeitura3 = 25;
                            leituraInicialNotas[2] = 1;
                            leituraContinuaNotas[2] = 0;
                        }
                    }


                /*
                    Leitura da nota 4 (PC0)
                */
                //se esta no inicio de leitura e tem nota
                if(posicaoLeitura4 == 25 && notasColuna4[posicaoLeitura4]){
                    // se leitura inicial ativada
                    if(leituraInicialNotas[3]){
                        // se botao foi presssionado com nota vazia, ou nao pressionado com nota cheia
                        if((tipoDaNota[3] == 1 && PINC & (1 << PC0)) || (tipoDaNota[3] == 0 && !(PINC & (1 << PC0)))){
                            streak++;
                            leituraInicialNotas[3] = 0;
                            leituraContinuaNotas[3] = 1;
                            score+=5;
                        
                            // se continuacao da leitura ativada
                            if(leituraContinuaNotas[3]){
                                score+=5;
                            }
                        }
                            // se nao pressionou o botao no momento de leitura inicial
                            else{
                                leituraInicialNotas[3] = 0;
                                ocorreuErro = 1;
                                leituraContinuaNotas[3] = 0;
                            }
                       posicaoLeitura4++;         
                    }
                    
                }
                    //se nao, se posicao de leitura esta no meio da nota
                    else if(posicaoLeitura4 > 25 && posicaoLeitura4 < 36){
                        // se leitura continua ativada, aumenta score
                        if(leituraContinuaNotas[3]){
                            score+=5;
                        }
                        posicaoLeitura4++;
                        // se nota chegou na posicao final, reinicia contadores da coluna
                        if(posicaoLeitura4 >= 36){
                            posicaoLeitura4 = 25;
                            leituraInicialNotas[3] = 1;
                            leituraContinuaNotas[3] = 0;
                        }
                    }

                //se nao leu ao menos uma nota
                if(ocorreuErro){
                    streak = 0;
                    lives--;
                    ocorreuErro = 0;
                }

                if(streak > streakRecorde){streakRecorde = streak;}

                if(lives < 0){
                    telaAtual = 3;
                    playing = 0;
                    geraNota = 0;
                    iniciar = 0;
                    continue; // encerra ciclo atual do while(1)
                }
            }  

            if(playing){
                nokia_lcd_render(); 
            }
                
            //se todas as notas foram criadas, e nao existe nenhuma nos vetores de coluna
            if(notasGeradas == notasMax && !existeNota){
                    telaAtual = 3;
                    playing = 0;
                    geraNota = 0;
                    iniciar = 0;
                    
            }   
        }

        
        if(telaAtual == 1){
            _delay_ms(5000);
            telaAtual = 2;
        } 
    }
}

void atualizaTela(int tela)
{
    char espacoLives[3];
    char espacoScore[5];
    char espacoStreak[3];
    switch (tela)
    {
    case 0: // menu inicial iniciar
        nokia_lcd_clear();
        nokia_lcd_set_cursor(10, 2);
        nokia_lcd_write_string("Guitar Hero",1);

        nokia_lcd_drawline(5, 14, 78, 15); // linha horizontal baixo

        nokia_lcd_set_cursor(20, 20);
        if(!acao){// escolheu iniciar
            nokia_lcd_write_string("\001New Game",1);
            nokia_lcd_set_cursor(20, 35);
            nokia_lcd_write_string("\tQuit",1);
        }
            else{// escolheu sair
                nokia_lcd_write_string("\tNew Game",1);
                nokia_lcd_set_cursor(20, 35);
                nokia_lcd_write_string("\001Quit",1);
            }

        break;

    case 1: // jogo carregando
        
        nokia_lcd_clear();
        nokia_lcd_drawline(31, 0, 32, 47);
        nokia_lcd_drawline(32, 36, 83, 37);// linha horizontal para limite das notas

        nokia_lcd_set_cursor(0, 0);//posiciona pontos
        dtostrf(0, 4, 0, espacoScore);
        nokia_lcd_write_string(espacoScore,1);

        nokia_lcd_set_cursor(5, 13);
        nokia_lcd_write_string(">GET READY!<",1);

        nokia_lcd_set_cursor(0, 25);//posiciona streak
        dtostrf(0, 4, 0, espacoStreak);
        nokia_lcd_write_string(espacoStreak,1);

        nokia_lcd_set_cursor(0, 37);//posiciona vidas
        dtostrf(0, 4, 0, espacoLives);
        nokia_lcd_write_string(espacoLives,1);
        
        break;

    case 2: // jogo rodando

        nokia_lcd_clear();
        nokia_lcd_drawline(31, 0, 32, 47);
        nokia_lcd_drawline(32, 36, 83, 37);// linha horizontal para limite das notas

        nokia_lcd_set_cursor(0, 0);//posiciona pontos
        dtostrf(score, 4, 0, espacoScore);
        nokia_lcd_write_string(espacoScore,1);

        nokia_lcd_set_cursor(0, 25);//posiciona streak
        dtostrf(streak, 4, 0, espacoStreak);
        nokia_lcd_write_string(espacoStreak,1);

        nokia_lcd_set_cursor(0, 37);//posiciona vidas
        dtostrf(lives, 4, 0, espacoLives);
        nokia_lcd_write_string(espacoLives,1);
        
        break;

    case 3: // fim vitoria/derrota
        nokia_lcd_clear();

        nokia_lcd_set_cursor(0, 2);

        if(lives < 0){
            nokia_lcd_write_string("Game Over! :(",1);
        }
            else{
                nokia_lcd_write_string("You Won!!! :D",1);
            }
        nokia_lcd_drawline(5, 14, 78, 15); // linha horizontal baixo

        nokia_lcd_set_cursor(5, 20);
        nokia_lcd_write_string("Score: ",1);
        dtostrf(score, 5, 0, espacoScore);
        nokia_lcd_write_string(espacoScore,1);


        nokia_lcd_set_cursor(5, 30);
        nokia_lcd_write_string("Streak: ",1);
        dtostrf(streakRecorde, 4, 0, espacoStreak);
        nokia_lcd_write_string(espacoStreak,1);

        break;
        
    default: // erro
        nokia_lcd_clear();
        nokia_lcd_drawrect(5, 14, 78, 20);
        nokia_lcd_render();
        break;
    }
}

void deslocaColuna()
{
    existeNota = 0;
    for(int i = 47; i>0; i--){
        notasColuna1[i] = notasColuna1[i-1];
        notasColuna2[i] = notasColuna2[i-1];
        notasColuna3[i] = notasColuna3[i-1];
        notasColuna4[i] = notasColuna4[i-1];
        if(notasColuna1[i] || notasColuna2[i] || notasColuna3[i] || notasColuna4[i]){
            existeNota = 1;
        }
    }
    notasColuna1[0] = 0;
    notasColuna2[0] = 0;
    notasColuna3[0] = 0;
    notasColuna4[0] = 0;
}

