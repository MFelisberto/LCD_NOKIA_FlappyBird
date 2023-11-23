/***************************************************************

   Exemplo de como inicializar o gerador de números aleatórios

***************************************************************/
#include <avr/io.h>
#include <time.h>
#include <stdio.h>
#include <util/delay.h>

#include "nokia5110.h"

uint8_t glyph[] = {0b00010000, 0b00100100, 0b11100000, 0b00100100, 0b00010000};

int main(void)
{
    DDRD &= ~(1 << PD0); // entrada em PD0
    PORTD = (1 << PD0);  // habilita pull-up

    nokia_lcd_init();
    nokia_lcd_clear();
    nokia_lcd_custom(1, glyph);
    nokia_lcd_write_string("Not working!", 1);

    // Normalmente usamos a função time(0) para obter o horário do sistema, porém
	// na AVR-LIBC essa função SEMPRE retorna ZERO... portanto, é inútil para inicializar
	// a semente do gerador (com srand)
    int v = time(0);
    char str[20];
    sprintf(str, "%d", v);

    nokia_lcd_set_cursor(0, 12);
    nokia_lcd_write_string("Time: ", 1);
    nokia_lcd_write_string(str, 1);
    nokia_lcd_render();

    // Como precisamos de uma semente mais "aleatória", uma alternativa é mostrar alguma msg
	// de início, aí aguardar o usuário pressionar um botão. Nesse meio tempo, incrementamos
	// um contador inteiro (v)
    while (PIND & (1 << PD0))
        v++;

    // Neste ponto, v terá um valor "razoavelmente aleatório", uma vez que dificilmente
	// conseguiremos "parar" no mesmo ponto
    nokia_lcd_clear();
    sprintf(str, "%d", v);
    nokia_lcd_write_string("Seed: ", 1);
    nokia_lcd_write_string(str, 1);

    // A partir daí, usamos esse v para inicializar o gerador...
    srand(v);

    // ... e a sequência de números agora irá variar a cada execução
    int r = rand() % 100;
    sprintf(str, "%d", r);

    nokia_lcd_set_cursor(0, 12);
    nokia_lcd_write_string("Random: ", 1);
    nokia_lcd_write_string(str, 1);
    nokia_lcd_render();

    while (1)
        ;
}
