 
 #include "hardware/rtc.h"          
 #include "pico/util/datetime.h"    
 #include "pico/stdlib.h"
 #include "hardware/gpio.h"
 #include "hardware/timer.h"        
 #include <stdio.h>
 #include <string.h>
 
 int PINO_TRIGGER = 10;
 int PINO_ECHO = 11;
 
 volatile absolute_time_t tempo_inicio;
 volatile absolute_time_t tempo_fim;
 
 volatile int sinal_descida = 0;
 
 static alarm_id_t id_alarme;             
 volatile bool aguardando_subida = false;  
 static const uint TIMEOUT_MS = 50;       
 
 int64_t alarme(alarm_id_t id, void *user_data) {
     if (aguardando_subida) {
         datetime_t agora;
         rtc_get_datetime(&agora);
         printf("%02d:%02d:%02d - Falha\n", agora.hour, agora.min, agora.sec);
         aguardando_subida = false;
     }
     return 0; // Não repete 
 }
 
 void callback_echo(uint gpio, uint32_t eventos) {
     if (eventos & GPIO_IRQ_EDGE_RISE) { 
         tempo_inicio = get_absolute_time();
         
         if (aguardando_subida) {
             cancel_alarm(id_alarme);
             aguardando_subida = false;
         }
     } else if (eventos & GPIO_IRQ_EDGE_FALL) {
         tempo_fim = get_absolute_time();
         sinal_descida = 1; 
     }
 }
 
 int main() {
     stdio_init_all();
     
     datetime_t tempo_inicial = {
         .year  = 2025,
         .month = 3,
         .day   = 18,
         .dotw  = 0,  // 0 = domingo
         .hour  = 0,
         .min   = 0,
         .sec   = 0
     };
     rtc_init();
     rtc_set_datetime(&tempo_inicial);
     
     gpio_init(PINO_TRIGGER);
     gpio_set_dir(PINO_TRIGGER, GPIO_OUT);
     
     gpio_init(PINO_ECHO);
     gpio_set_dir(PINO_ECHO, GPIO_IN);
     gpio_set_irq_enabled_with_callback(PINO_ECHO,
                                      GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                      true,
                                      callback_echo);
     
     int medindo = 0;
     
     while (true) {
         // Checa entrada do usuário (não bloqueante)
         int caractere = getchar_timeout_us(500);
         if (caractere == 's') {
             medindo = !medindo;
             if (medindo) {
                 printf("Iniciando leitura \n");
             } else {
                 printf("Pausadas leituras\n");
             }
         }
         
         if (medindo) {
             gpio_put(PINO_TRIGGER, 1);
             sleep_us(10);
             gpio_put(PINO_TRIGGER, 0);
             
             aguardando_subida = true;
             id_alarme = add_alarm_in_ms(TIMEOUT_MS, alarme, NULL, false);
         
             if (sinal_descida == 1) {
                 sinal_descida = 0;
                 int diferenca = absolute_time_diff_us(tempo_inicio, tempo_fim);
                 double distancia = (diferenca * 0.0343) / 2.0;
                 datetime_t agora = {0};
                 rtc_get_datetime(&agora);
         
                 char buffer_data_hora[256];
                 char *data_hora_str = &buffer_data_hora[0];
                 datetime_to_str(data_hora_str, sizeof(buffer_data_hora), &agora);
                 rtc_get_datetime(&agora);
                 printf("%s - %.2f cm\n", data_hora_str, distancia);
             }
         }
         
         sleep_ms(1000);
     }
 }