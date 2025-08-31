# Jarvis ESP32

## Título e descrição do projeto

Projeto Jarvis ESP32 é um assistente de voz baseado no microcontrolador ESP32, que utiliza o microfone digital INMP441 e o amplificador de áudio MAX98357A para captar e reproduzir áudio. O objetivo é criar um sistema de reconhecimento de voz simples e eficiente para aplicações de automação residencial e projetos IoT.

## Componentes utilizados

- ESP32 Dev Kit
- Microfone digital INMP441
- Amplificador de áudio MAX98357A
- Alto-falante
- Fonte de alimentação adequada

## Ligações de pinos (INMP441 e MAX98357A)

### INMP441 (Microfone digital I2S)

| INMP441 Pin | ESP32 Pin                |
| ----------- | ------------------------ |
| VCC         | 3.3V                     |
| GND         | GND                      |
| SD          | GPIO32 (I2S Data)        |
| SCK         | GPIO14 (I2S Clock)       |
| WS          | GPIO15 (I2S Word Select) |

### MAX98357A (Amplificador I2S)

| MAX98357A Pin | ESP32 Pin                |
| ------------- | ------------------------ |
| VIN           | 3.3V                     |
| GND           | GND                      |
| DIN           | GPIO25 (I2S Data)        |
| BCLK          | GPIO26 (I2S Bit Clock)   |
| LRC           | GPIO27 (I2S Word Select) |

## Estrutura do arquivo secrets.h

O arquivo `secrets.h` deve conter as definições das credenciais de rede e outras informações sensíveis, seguindo o modelo:

```c
#ifndef SECRETS_H
#define SECRETS_H

#define WIFI_SSID "seu_ssid"
#define WIFI_PASSWORD "sua_senha_wifi"

#endif
```

## Lógica atual dos testes

O código atual realiza a captura do áudio do microfone INMP441 via protocolo I2S, processa o sinal e reproduz o áudio diretamente pelo amplificador MAX98357A. Testes iniciais focam na correta aquisição do áudio e na reprodução sem distorções, garantindo a comunicação I2S entre os dispositivos e o ESP32.

## Como usar

1. Configure o arquivo `secrets.h` com suas credenciais Wi-Fi.
2. Conecte os componentes conforme as ligações de pinos indicadas.
3. Compile e faça o upload do código para o ESP32.
4. Abra o monitor serial para acompanhar as mensagens de status.
5. Teste o sistema falando próximo ao microfone e ouvindo a reprodução pelo alto-falante.

## Próximos passos

- Implementar reconhecimento de comandos de voz.
- Integrar com serviços de nuvem para processamento avançado.
- Adicionar suporte a múltiplos idiomas.
- Desenvolver interface web para controle e configuração remota.
- Otimizar consumo de energia para operação autônoma.
