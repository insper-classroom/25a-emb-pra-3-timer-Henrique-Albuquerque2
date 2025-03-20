#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"

#define PINO_ECHO 11
#define PINO_TRIGGER 10

#define TAXA_BAUD 115200
#define UART_INSTANCIA uart0
#define PINO_UART_RX 1
#define PINO_UART_TX 0

volatile bool dados_prontos = false;
volatile uint64_t tempo_inicio = 0;
volatile uint64_t tempo_fim = 0;

void callback_echo(uint gpio, uint32_t eventos) {
    if (eventos & GPIO_IRQ_EDGE_RISE) {
        tempo_inicio = time_us_64();  
    } 
    if (eventos & GPIO_IRQ_EDGE_FALL) {
        tempo_fim = time_us_64();   
        dados_prontos = true;            
    }
}

void inicializar_hardware() {
    stdio_init_all();
    uart_init(UART_INSTANCIA, TAXA_BAUD);
    gpio_set_function(PINO_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PINO_UART_RX, GPIO_FUNC_UART);

    gpio_init(PINO_TRIGGER);
    gpio_set_dir(PINO_TRIGGER, GPIO_OUT);
    gpio_put(PINO_TRIGGER, 0);

    gpio_init(PINO_ECHO);
    gpio_set_dir(PINO_ECHO, GPIO_IN);
    gpio_set_irq_enabled_with_callback(PINO_ECHO, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &callback_echo);
}

void enviar_pulso_trigger() {
    gpio_put(PINO_TRIGGER, 1);
    sleep_us(10);
    gpio_put(PINO_TRIGGER, 0);
}

void imprimir_tempo_atual() {
    datetime_t tempo_atual;
    rtc_get_datetime(&tempo_atual);
    printf("%02d:%02d:%02d - ", tempo_atual.hour, tempo_atual.min, tempo_atual.sec);
}

void medir_distancia() {
    enviar_pulso_trigger();
    sleep_ms(60);

    if (dados_prontos) {
        uint64_t duracao_pulso = tempo_fim - tempo_inicio;
        float distancia_cm = duracao_pulso / 58.0;  
        imprimir_tempo_atual();
        printf("%.2f cm\n", distancia_cm);
        dados_prontos = false;
    } else {
        imprimir_tempo_atual();
        printf("Falha\n");
    }
}

bool processar_entrada_terminal(bool leitura_ativa) {
    int caractere = getchar_timeout_us(1000);
    if (caractere == 's') {
        printf("Iniciando \n");
        return true;
    } else if (caractere == 'p') {
        printf("Pausa \n");
        return false;
    }
    return leitura_ativa;
}

int main() {
    stdio_init_all();
    inicializar_hardware();

    bool leitura_ativa = false;

    rtc_init();
    datetime_t tempo_inicial = {.year = 2025, .month = 3, .day = 20, .dotw = 4, .hour = 16, .min = 56, .sec = 0};
    rtc_set_datetime(&tempo_inicial);

    printf("Data e hora inicial configuradas\n");
    printf("Tudo certo para começar\n");
    printf("Pressione 's' para iniciar ou 'p' para parar.\n");


    while (1) {
        leitura_ativa = processar_entrada_terminal(leitura_ativa);
        if (leitura_ativa) {
            medir_distancia();
        }
        sleep_ms(1000);
    }

    return 0;
}