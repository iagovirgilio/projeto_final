#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "pico/time.h"
#include "pico/bootrom.h"     // Para a função reset_usb_boot()
#include "hardware/gpio.h"
#include "hardware/pwm.h"     // Para controle do PWM do buzzer
#include "hardware/i2c.h"     // Para comunicação I2C com o display
#include <string.h>
#include <stdio.h>
#include "inc/ssd1306.h"      // Biblioteca do display SSD1306
#include "inc/font.h"         // Fonte utilizada pelo display
#include "template.h"

// Configuração do I2C para o display OLED
#define I2C_SDA_PIN    14
#define I2C_SCL_PIN    15
#define I2C_PORT       i2c1

// Definição dos pinos do projeto
#define LED1_PIN    11   // LED para o cômodo 1 (ex.: Sala)
#define LED2_PIN    12   // LED para o cômodo 2 (ex.: Cozinha)
#define LED3_PIN    13   // LED para o cômodo 3 (ex.: Quarto)
#define BUZZER_PIN  21   // Pino do buzzer (usado com PWM)

#define BUTTON1_PIN 5    // Sensor de movimento - Botão A
#define BUTTON2_PIN 6    // Botão B (usado para acionar o modo BOOTSEL)

#define WIFI_SSID "Iago"          // Nome da rede Wi-Fi
#define WIFI_PASS "Iago@8022"      // Senha da rede Wi-Fi

// Mensagens de estado
char sensor1_message[50] = "Nenhum movimento (Sensor A)";
char sensor2_message[50] = "Botão B: pressione para BOOTSEL";

// Buffer para resposta HTTP com HTML estilizado
char http_response[10240];
char html_body[10240];

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

// Instância do display OLED
static ssd1306_t ssd;

// Funções para controle do buzzer via PWM
void buzzer_start(uint frequency) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint channel = pwm_gpio_to_channel(BUZZER_PIN);  // Obtém o canal correto
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    float divider = 125.0f;             // Divisor fixo
    uint32_t clock = 125000000;         // Clock padrão do Pico (125 MHz)
    uint32_t wrap = clock / (divider * frequency) - 1;
    pwm_set_clkdiv(slice_num, divider);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, channel, (wrap + 1) / 2);  // Duty cycle de 50%
    pwm_set_enabled(slice_num, true);
}

void buzzer_stop() {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, false);
}

// Callback de interrupção para os botões (sensores)
void gpio_callback(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (gpio == BUTTON1_PIN) {
        if (current_time - last_interrupt_time_sensor1 < debounce_delay_ms) return;
        last_interrupt_time_sensor1 = current_time;
        sensor1_pressed = (gpio_get(BUTTON1_PIN) == 0);  // Botão com pull-up: pressionado = 0
    } else if (gpio == BUTTON2_PIN) {
        if (current_time - last_interrupt_time_sensor2 < debounce_delay_ms) return;
        last_interrupt_time_sensor2 = current_time;
        sensor2_pressed = (gpio_get(BUTTON2_PIN) == 0);
    }
}

// Função para criar a resposta HTTP com HTML estilizado
void create_http_response() {
    // Preenche o corpo HTML substituindo os placeholders (%s)
    snprintf(html_body, sizeof(html_body), html_template, sensor1_message, sensor2_message);

    int body_length = strlen(html_body);

    // Constrói o header HTTP com o Content-Length calculado e Connection: close
    char header[256];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Cache-Control: no-cache, no-store, must-revalidate\r\n"
        "Pragma: no-cache\r\n"
        "Expires: 0\r\n"
        "Content-Length: %d\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Connection: close\r\n\r\n", body_length);

    // Concatena o header e o corpo para formar a resposta final
    snprintf(http_response, sizeof(http_response), "%s%s", header, html_body);
}

// Callback para processar as requisições HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    char *request = (char *)p->payload;
    if (strstr(request, "GET /status")) {
        // Atualiza o sensor1_message, se necessário
        // Responde apenas com o status do sensor A
        char status[100];
        snprintf(status, sizeof(status), "%s", sensor1_message);
    } else if (strstr(request, "GET /led1/on")) {
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

// Função para atualizar o display OLED com informações do sistema
void update_display() {
    char buffer[32];
    ssd1306_fill(&ssd, false);
    // Exibe o cabeçalho
    ssd1306_draw_string(&ssd,"House Control", 0, 0);

    // Exibe o status do Wi-Fi e o IP, se conectado
    if (cyw43_state.netif[0].ip_addr.addr != 0) {
        snprintf(buffer, sizeof(buffer), "IP: %d.%d.%d.%d",
                 ((uint8_t*)&(cyw43_state.netif[0].ip_addr.addr))[0],
                 ((uint8_t*)&(cyw43_state.netif[0].ip_addr.addr))[1],
                 ((uint8_t*)&(cyw43_state.netif[0].ip_addr.addr))[2],
                 ((uint8_t*)&(cyw43_state.netif[0].ip_addr.addr))[3]);
        ssd1306_draw_string(&ssd, "WiFi: Conectado",0, 10);
        ssd1306_draw_string(&ssd, buffer, 0, 20);
    } else {
        ssd1306_draw_string(&ssd,"WiFi: Desconectado", 0, 10);
    }
    // Exibe os estados dos sensores
    ssd1306_draw_string(&ssd,sensor1_message,0, 30);
    ssd1306_draw_string(&ssd, sensor2_message,0, 40);
    ssd1306_send_data(&ssd);
}

int main() {
    stdio_init_all();
    sleep_ms(10000);
    printf("Iniciando servidor HTTP\n");

    // Inicializa o I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    // Inicializa o display OLED (verifique o endereço 0x3C, ajuste se necessário)
    ssd.width  = 128;
    ssd.height = 64;
    ssd1306_init(&ssd, 128, 64, false, 0x3C, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

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

    // Configura o pino do Buzzer
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

        // Atualiza as mensagens dos sensores
        if (sensor_alarm_triggered) {
            snprintf(sensor1_message, sizeof(sensor1_message), "Movimento detectado!");
        } else {
            snprintf(sensor1_message, sizeof(sensor1_message), "Nenhum movimento (Sensor A)");
        }
        if (sensor2_pressed) {
            snprintf(sensor2_message, sizeof(sensor2_message), "Entrando em BOOTSEL...");
            printf("Botão B pressionado: entrando em modo BOOTSEL\n");
            sleep_ms(100);  // Pausa para estabilização
            reset_usb_boot(0, 0);
        } else {
            snprintf(sensor2_message, sizeof(sensor2_message), "Botão B: pressione para BOOTSEL");
        }
        
        // Se o sensor A for acionado e o alarme não estiver ativo, ativa o buzzer
        if (sensor1_pressed && !sensor_alarm_triggered) {
            buzzer_start(2000);
            sensor_alarm_triggered = true;
            alarm_start_ms = to_ms_since_boot(get_absolute_time());
        }
        // Desativa o alarme após 2 segundos
        if (sensor_alarm_triggered) {
            uint32_t current_ms = to_ms_since_boot(get_absolute_time());
            if (current_ms - alarm_start_ms >= ALARM_DURATION_MS) {
                buzzer_stop();
                sensor_alarm_triggered = false;
                printf("Alarme desligado automaticamente após %d ms\n", ALARM_DURATION_MS);
            }
        }
        
        // Atualiza o display OLED com as informações atuais
        update_display();

        sleep_ms(100);
    }

    cyw43_arch_deinit();
    return 0;
}
