#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "pico/time.h"
#include "pico/bootrom.h"     // Para a função reset_usb_boot()
#include "hardware/gpio.h"
#include "hardware/pwm.h"     // Para controle do PWM do buzzer
#include <string.h>
#include <stdio.h>

// Definição dos pinos
#define LED1_PIN    11   // LED para o cômodo 1 (ex.: Sala)
#define LED2_PIN    12   // LED para o cômodo 2 (ex.: Cozinha)
#define LED3_PIN    13   // LED para o cômodo 3 (ex.: Quarto)
#define BUZZER_PIN  21   // Pino do buzzer (usado com PWM)

#define BUTTON1_PIN 5    // Sensor de movimento - Botão A
#define BUTTON2_PIN 6    // Botão B (usado para acionar o modo BOOTSEL)

#define WIFI_SSID "Iago"          // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_PASS "Iago@8022"      // Substitua pela senha da sua rede Wi-Fi

// Mensagens de estado
char sensor1_message[50] = "Nenhum movimento (Sensor A)";
char sensor2_message[50] = "Botão B: pressione para BOOTSEL";

// Buffer para resposta HTTP com HTML estilizado
char http_response[2048];

// Controle do alarme disparado pelo sensor A
volatile bool sensor_alarm_triggered = false;
uint32_t alarm_start_ms = 0;
const uint32_t ALARM_DURATION_MS = 2000; // 2 segundos

// Variáveis para debounce via interrupção (em milissegundos)
volatile uint32_t last_interrupt_time_sensor1 = 0;
volatile uint32_t last_interrupt_time_sensor2 = 0;
const uint32_t debounce_delay_ms = 50;

// Flags atualizadas pelas interrupções indicando o estado dos botões
volatile bool sensor1_pressed = false;  // Botão A
volatile bool sensor2_pressed = false;  // Botão B

// Funções para controle do buzzer via PWM
void buzzer_start(uint frequency) {
    // Configura o pino do buzzer para PWM
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint channel = pwm_gpio_to_channel(BUZZER_PIN);  // Obtém o canal correto
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    // Configuração para gerar a frequência desejada:
    // frequency = clock / (divider * (wrap + 1))
    float divider = 125.0f;             // Divisor fixo
    uint32_t clock = 125000000;           // Clock padrão do Pico (125 MHz)
    uint32_t wrap = clock / (divider * frequency) - 1;
    pwm_set_clkdiv(slice_num, divider);
    pwm_set_wrap(slice_num, wrap);
    // Configura duty cycle de 50% usando o canal correto
    pwm_set_chan_level(slice_num, channel, (wrap + 1) / 2);
    pwm_set_enabled(slice_num, true);
}

void buzzer_stop() {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, false);
}

// Callback de interrupção para os sensores (botões)
void gpio_callback(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (gpio == BUTTON1_PIN) {
        if (current_time - last_interrupt_time_sensor1 < debounce_delay_ms) return;
        last_interrupt_time_sensor1 = current_time;
        // Botão com pull-up: pressionado = LOW (0)
        sensor1_pressed = (gpio_get(BUTTON1_PIN) == 0);
    } else if (gpio == BUTTON2_PIN) {
        if (current_time - last_interrupt_time_sensor2 < debounce_delay_ms) return;
        last_interrupt_time_sensor2 = current_time;
        sensor2_pressed = (gpio_get(BUTTON2_PIN) == 0);
    }
}

// Função para criar a resposta HTTP com HTML estilizado
void create_http_response() {
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 200 OK\r\n"
        "Cache-Control: no-cache, no-store, must-revalidate\r\n"
        "Pragma: no-cache\r\n"
        "Expires: 0\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n\r\n"
        "<!DOCTYPE html>"
        "<html lang=\"pt\">"
        "<head>"
        "  <meta charset=\"UTF-8\">"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "  <link href=\"https://fonts.googleapis.com/css2?family=Roboto:wght@400;500;700&display=swap\" rel=\"stylesheet\">"
        "  <title>House Control</title>"
        "  <style>"
        "    body {"
        "      margin: 0;"
        "      padding: 0;"
        "      font-family: 'Roboto', sans-serif;"
        "      background: #f0f2f5;"
        "    }"
        "    .container {"
        "      max-width: 600px;"
        "      margin: 50px auto;"
        "      background: #ffffff;"
        "      border-radius: 8px;"
        "      box-shadow: 0 4px 12px rgba(0,0,0,0.1);"
        "      padding: 20px;"
        "    }"
        "    h1, h2 {"
        "      text-align: center;"
        "      color: #333333;"
        "    }"
        "    .control-section {"
        "      margin: 20px 0;"
        "    }"
        "    .control-section p {"
        "      text-align: center;"
        "      margin: 10px 0;"
        "    }"
        "    .button {"
        "      display: inline-block;"
        "      padding: 12px 24px;"
        "      margin: 5px;"
        "      border: none;"
        "      border-radius: 4px;"
        "      background: #6200EE;"
        "      color: #ffffff;"
        "      text-decoration: none;"
        "      font-weight: 500;"
        "      transition: background 0.3s ease;"
        "    }"
        "    .button:hover {"
        "      background: #3700B3;"
        "    }"
        "    .status {"
        "      font-size: 0.9em;"
        "      color: #555555;"
        "      text-align: center;"
        "      margin-top: 20px;"
        "    }"
        "  </style>"
        "</head>"
        "<body>"
        "  <div class=\"container\">"
        "    <h1>House Control</h1>"
        "    <div class=\"control-section\">"
        "      <h2>LEDs (Cômodos)</h2>"
        "      <p>Sala (LED 1): <a class=\"button\" href=\"/led1/on\">Ligar</a> <a class=\"button\" href=\"/led1/off\">Desligar</a></p>"
        "      <p>Cozinha (LED 2): <a class=\"button\" href=\"/led2/on\">Ligar</a> <a class=\"button\" href=\"/led2/off\">Desligar</a></p>"
        "      <p>Quarto (LED 3): <a class=\"button\" href=\"/led3/on\">Ligar</a> <a class=\"button\" href=\"/led3/off\">Desligar</a></p>"
        "    </div>"
        "    <div class=\"control-section\">"
        "      <h2>Buzzer (Alarme)</h2>"
        "      <p><a class=\"button\" href=\"/buzzer/on\">Ativar</a> <a class=\"button\" href=\"/buzzer/off\">Desativar</a></p>"
        "    </div>"
        "  </div>"
        "</body>"
        "</html>\r\n"
    );
}


// Callback para processar as requisições HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *request = (char *)p->payload;

    // Comandos para controle dos LEDs
    if (strstr(request, "GET /led1/on")) {
        gpio_put(LED1_PIN, 1);
    } else if (strstr(request, "GET /led1/off")) {
        gpio_put(LED1_PIN, 0);
    } else if (strstr(request, "GET /led2/on")) {
        gpio_put(LED2_PIN, 1);
    } else if (strstr(request, "GET /led2/off")) {
        gpio_put(LED2_PIN, 0);
    } else if (strstr(request, "GET /led3/on")) {
        gpio_put(LED3_PIN, 1);
    } else if (strstr(request, "GET /led3/off")) {
        gpio_put(LED3_PIN, 0);
    }
    // Comandos para controle do Buzzer via PWM
    else if (strstr(request, "GET /buzzer/on")) {
        buzzer_start(2000);  // Liga o buzzer com 2000 Hz
        sensor_alarm_triggered = false;
    } else if (strstr(request, "GET /buzzer/off")) {
        buzzer_stop();
        sensor_alarm_triggered = false;
    }

    create_http_response();
    tcp_write(tpcb, http_response, strlen(http_response), TCP_WRITE_FLAG_COPY);

    pbuf_free(p);
    return ERR_OK;
}

// Callback de conexão: associa o callback HTTP à nova conexão
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);
    return ERR_OK;
}

// Função para iniciar o servidor HTTP
static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB\n");
        return;
    }
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    printf("Servidor HTTP rodando na porta 80...\n");
}

int main() {
    stdio_init_all();
    sleep_ms(10000);
    printf("Iniciando servidor HTTP\n");

    // Inicializa o Wi-Fi
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        return 1;
    } else {
        printf("Conectado.\n");
        uint8_t *ip_address = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
        printf("Endereço IP %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }
    printf("Wi-Fi conectado!\n");

    // Configura os pinos dos LEDs
    gpio_init(LED1_PIN);
    gpio_set_dir(LED1_PIN, GPIO_OUT);
    gpio_init(LED2_PIN);
    gpio_set_dir(LED2_PIN, GPIO_OUT);
    gpio_init(LED3_PIN);
    gpio_set_dir(LED3_PIN, GPIO_OUT);

    // Configura o pino do Buzzer (a função buzzer_start() configura o pino para PWM)
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    // Configura os botões com pull-up
    gpio_init(BUTTON1_PIN);
    gpio_set_dir(BUTTON1_PIN, GPIO_IN);
    gpio_pull_up(BUTTON1_PIN);
    
    gpio_init(BUTTON2_PIN);
    gpio_set_dir(BUTTON2_PIN, GPIO_IN);
    gpio_pull_up(BUTTON2_PIN);

    // Configura as interrupções para os botões (rising e falling edge)
    gpio_set_irq_enabled_with_callback(BUTTON1_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    gpio_set_irq_enabled(BUTTON2_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

    // Inicia o servidor HTTP
    start_http_server();

    // Loop principal
    while (true) {
        cyw43_arch_poll();

        // Atualiza a mensagem do sensor A baseada no estado do alarme
        if (sensor_alarm_triggered) {
            snprintf(sensor1_message, sizeof(sensor1_message), "Movimento detectado!");
        } else {
            snprintf(sensor1_message, sizeof(sensor1_message), "Nenhum movimento (Sensor A)");
        }

        // Se o botão B for pressionado, atualiza a mensagem e entra em BOOTSEL
        if (sensor2_pressed) {
            snprintf(sensor2_message, sizeof(sensor2_message), "Entrando em BOOTSEL...");
            printf("Botão B pressionado: entrando em modo BOOTSEL\n");
            sleep_ms(100);  // Pequena pausa para estabilização
            reset_usb_boot(0, 0);
        } else {
            snprintf(sensor2_message, sizeof(sensor2_message), "Botão B: pressione para BOOTSEL");
        }

        // Se o sensor A for acionado e o alarme ainda não estiver ativo, ativa o buzzer com PWM
        if (sensor1_pressed && !sensor_alarm_triggered) {
            buzzer_start(2000);
            sensor_alarm_triggered = true;
            alarm_start_ms = to_ms_since_boot(get_absolute_time());
        }
        // Desativa automaticamente o alarme após o tempo definido
        if (sensor_alarm_triggered) {
            uint32_t current_ms = to_ms_since_boot(get_absolute_time());
            if (current_ms - alarm_start_ms >= ALARM_DURATION_MS) {
                buzzer_stop();
                sensor_alarm_triggered = false;
                printf("Alarme desligado automaticamente após %d ms\n", ALARM_DURATION_MS);
            }
        }

        sleep_ms(100);
    }

    cyw43_arch_deinit();
    return 0;
}
