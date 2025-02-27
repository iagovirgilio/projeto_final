# Projeto Final: Sistema de Monitoramento com Raspberry Pi Pico

Este projeto implementa um sistema de monitoramento baseado no Raspberry Pi Pico, utilizando Wi-Fi, um display OLED, e um buzzer para notificações. O sistema monitora sensores de movimento e botões, enviando informações via HTTP para um servidor web.


## Funcionalidades:

* **Monitoramento de Sensores:** O sistema monitora dois sensores:
    * **Sensor de Movimento (Botão A):** Detecta movimento e aciona um alarme (buzzer) por 2 segundos.  A informação do status do sensor é atualizada no display OLED e está disponível via requisição HTTP.
    * **Botão B:**  Ao ser pressionado, reinicia o dispositivo em modo BOOTSEL para facilitar o flashing de novo firmware. O status é atualizado no display e disponível via HTTP.

* **Controle Remoto de LEDs:** Permite controlar remotamente três LEDs (LED1, LED2, LED3) via requisições HTTP (liga/desliga).

* **Controle do Buzzer:**  O buzzer pode ser acionado e desligado remotamente via requisições HTTP.

* **Interface com Display OLED:** Um display OLED SSD1306 exibe informações sobre o status do Wi-Fi, o endereço IP, e os estados dos sensores.

* **Comunicação Wi-Fi:** O Pico se conecta a uma rede Wi-Fi para disponibilizar os dados via HTTP.

* **Servidor HTTP embutido:**  Um servidor HTTP simples é executado na porta 80, respondendo a requisições para obter o status dos sensores e controlar os LEDs e o buzzer.


## Hardware Utilizado:

* Raspberry Pi Pico
* Display OLED SSD1306
* Buzzer
* 3 LEDs
* 2 Botões (ou sensores que atuam como botões)


## Software Utilizado:

* SDK do Raspberry Pi Pico
* LwIP (Lightweight IP stack)
* Biblioteca para o display SSD1306
* Biblioteca para comunicação I2C


## Instalação:

1. **Instalar o SDK do Raspberry Pi Pico:** Siga as instruções disponíveis em [https://raspberrypi.com/documentation/pico/](https://raspberrypi.com/documentation/pico/).
2. **Instalar as bibliotecas necessárias:** As bibliotecas usadas no projeto estão incluídas no código-fonte.
3. **Configurar a rede Wi-Fi:** Modifique o arquivo `projeto_final.c` para configurar o SSID e a senha da sua rede Wi-Fi.
4. **Compilar e transferir o código:** Compile o código usando o compilador do SDK e transfira o firmware para o Raspberry Pi Pico.

## Como executar:

1. Conecte o hardware conforme o esquema.
2. Ligue o Raspberry Pi Pico.
3. O sistema irá se conectar à rede Wi-Fi e iniciar o servidor HTTP na porta 80.
4. Acesse o servidor via navegador web para monitorar os dados e controlar os LEDs e o buzzer.

## Requisições HTTP:

* `/status`: Retorna o status dos sensores (somente Sensor A neste momento).
* `/led1/on`, `/led1/off`: Liga/desliga o LED1.
* `/led2/on`, `/led2/off`: Liga/desliga o LED2.
* `/led3/on`, `/led3/off`: Liga/desliga o LED3.
* `/buzzer/on`, `/buzzer/off`: Liga/desliga o buzzer.


## Melhorias Futuras:

* Implementar um servidor web completo para uma melhor visualização dos dados.
* Adicionar mais sensores e funcionalidades.
* Implementar uma interface de usuári# Projeto Joystick com RP2040 e Display SSD1306

Este projeto demonstra o uso do conversor analógico-digital (ADC) do RP2040 para ler um joystick, controlar o brilho de LEDs RGB via PWM e exibir informações gráficas em um display OLED SSD1306 utilizando o protocolo I2C. O objetivo é consolidar os conhecimentos sobre ADC, PWM e comunicação I2C, além de explorar o uso de interrupções e *debouncing* para tratamento de botões.

---

## Funcionalidades

- **Leitura do Joystick:** Os valores analógicos dos eixos X e Y (pinos ADC 26 e ADC 27) controlam o brilho dos LEDs vermelho e azul e a posição de um quadrado no display OLED.

- **Controle de LEDs via PWM:** Os LEDs vermelho e azul são controlados pelo PWM, com brilho variando de acordo com a posição do joystick. O LED verde alterna seu estado ao pressionar o botão do joystick.

- **Exibição Gráfica no Display OLED:** Um quadrado de 8x8 pixels no display OLED representa a posição do joystick. A borda do display muda ao pressionar o botão do joystick.

- **Interrupções e *Debouncing*:** Botões (joystick e botão A) são gerenciados por interrupções com *debouncing* para evitar leituras falsas.

- **Botão A:** Alterna a ativação/desativação do controle PWM dos LEDs.


---

## Componentes Utilizados

- **Microcontrolador:** RP2040 (ex: BitDogLab)
- **Joystick:** Eixos X e Y (ADC 26 e 27), botão (GPIO 22)
- **LED RGB:** Vermelho (GPIO 13, PWM), Azul (GPIO 12, PWM), Verde (GPIO 11)
- **Botão A:** GPIO 5
- **Display OLED SSD1306:** SDA (GPIO 14), SCL (GPIO 15), Endereço I2C: 0x3C


---

## Requisitos do Projeto

- **Hardware:** RP2040, Joystick com botão, LED RGB, Botão A, Display OLED SSD1306 (128x64)
- **Software:** SDK do Raspberry Pi Pico, Compilador C/C++ para ARM, Ferramentas de build (Make, CMake)


---

## Conexões de Hardware

| Componente      | Pino RP2040 |
|-----------------|-------------|
| Joystick X      | ADC 26      |
| Joystick Y      | ADC 27      |
| Botão Joystick | GPIO 22     |
| LED Vermelho    | GPIO 13     |
| LED Azul        | GPIO 12     |
| LED Verde       | GPIO 11     |
| Botão A         | GPIO 5      |
| Display SDA     | GPIO 14     |
| Display SCL     | GPIO 15     |


---

## Compilação e Upload

1. **Configuração do Ambiente:** Instale o SDK do Raspberry Pi Pico e configure seu ambiente de desenvolvimento.

2. **Preparação do Projeto:** Copie os arquivos fonte para um diretório. Configure o `CMakeLists.txt` (se necessário).

3. **Compilação:** Utilize o `cmake` e `make` para compilar o código.  (Incluir comandos específicos se disponíveis)

4. **Upload:** Transfira o arquivo `.uf2` gerado para o RP2040.


---

## Como Usar o Projeto

- **Inicialização:** O quadrado é exibido centralizado no display OLED.

- **Controle do Joystick:** Movimentando o joystick, o brilho dos LEDs vermelho e azul e a posição do quadrado no display são alterados.

- **Botões:** O botão do joystick alterna o LED verde e muda o estilo da borda do display. O botão A ativa/desativa o controle PWM dos LEDs.


---

## Observações

- **Debouncing:** Implementado com um tempo de espera para evitar leituras falsas dos botões.

---


## Contato:

iagovirgilio@gmail.com


