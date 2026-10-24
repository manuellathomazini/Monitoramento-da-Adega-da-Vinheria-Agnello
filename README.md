# Monitoramento da Adega da Vinheria Agnello

Projeto IoT com **ESP32 + DHT22 + LDR** simulado no **Wokwi**, enviando dados por **MQTT** para a plataforma **FIWARE**, no padrão **Smart Data Models** (`IndoorEnvironmentObserved`) e **NGSIv2**. O back-end usa **IoT Agent MQTT**, **Orion Context Broker** e **STH-Comet**, e as checagens são feitas por uma **coleção do Postman** incluída neste repositório.

> - **Integrantes:** 
Manuella Sousa Thomazini - RM 573606
Renan Carlos Silva Bonanno - RM 573043
Murillo Dourado Vieira - RM 571912
Henrique Gumbys Pagliato - RM 570914
> - **Repositório no GitHub:** https://github.com/manuellathomazini/Monitoramento-da-Adega-da-Vinheria-Agnello.git
> - **Link do projeto no Wokwi:** https://wokwi.com/projects/477403690792711169
> - **Link do vídeo (hands-on):** https://youtu.be/1pYxvc58QRk

---

## 1. Contexto do desafio

Na fase 1, o sistema da Vinheria avisava quando o ambiente estava muito claro. Os proprietários pediram, na fase 2:

- monitorar **temperatura** e **umidade** do depósito, além da luminosidade;
- guardar **dados históricos** (dias e meses);
- **exportar** os dados para gráficos, análises e relatórios, garantindo a **rastreabilidade** das condições de armazenamento dos vinhos;
- **integrar** a empresa a distribuidoras do mercado europeu.

Para isso, o projeto usa Smart Data Models (União Europeia), o padrão NGSIv2 do FIWARE como back-end, MQTT para o transporte e armazenamento histórico.

## 2. Requisitos atendidos

| Requisito do trabalho | Como foi atendido |
|---|---|
| Enviar dados dos sensores ao FIWARE via MQTT | O ESP32 publica no tópico `/TEF/device888/attrs` |
| Incluir timestamp de cada leitura | Campos `d` (`dateObserved`) e `TimeInstant`, em ISO 8601 UTC, obtidos por NTP |
| Enviar dados pela internet via Wi-Fi | Conexão Wi-Fi do ESP32 (`Wokwi-GUEST` na simulação) |
| Interface em conformidade com NGSI | O **IoT Agent MQTT** converte o Ultralight em entidade NGSIv2 no **Orion Context Broker**, consultada pela requisição 4 da coleção do Postman |
| Leitura e escrita pelo MyMQTT | Leitura assinando `/attrs`; escrita (liga/desliga LED) publicando em `/cmd` |
| Smart Data Models e NGSIv2 | Atributos alinhados ao modelo `IndoorEnvironmentObserved` |
| Armazenamento histórico | **STH-Comet**, alimentado por uma assinatura do Orion e consultado pelas requisições 5 a 7 da coleção |

## 3. Arquitetura

```
 ┌───────────────────────┐   MQTT (Ultralight)    ┌──────────────────┐   NGSIv2   ┌──────────────┐
 │ ESP32 (Wokwi)         │ ─────────────────────▶ │ IoT Agent MQTT   │ ─────────▶ │ Orion        │
 │  DHT22  (temp, umid)  │  /TEF/device888/attrs  │ (Ultralight 2.0) │            │ Context      │
 │  LDR    (lux)         │                        │                  │ ◀───────── │ Broker       │
 │  LED    (atuador)     │ ◀───────────────────── │                  │  comandos  └──────┬───────┘
 └───────────────────────┘  /TEF/device888/cmd    └──────────────────┘                   │ assinatura
            ▲                                              ▲                             │ (notificação)
            │ publica/assina                               │                             ▼
      ┌─────┴─────┐                                        │                      ┌──────────────┐
      │  MyMQTT   │ ───────────────────────────────────────┘                      │ STH-Comet    │
      └───────────┘                                                               │ (histórico)  │
                                                                                  └──────────────┘

 Postman: consulta o Orion (estado atual) e o STH-Comet (histórico) por HTTP.
```

## 4. Estrutura do repositório

```
.
├── README.md                                          # este arquivo
├── sketch.ino                                         # código do ESP32
├── diagram.json                                       # circuito do Wokwi
├── libraries.txt                                      # bibliotecas do Wokwi
└── postman/
    ├── Vinheria_Agnello_FIWARE.postman_collection.json    # coleção com 7 requisições
    └── Vinheria_Agnello_FIWARE.postman_environment.json   # variável url
```

## 5. Hardware e ligações

| Componente | Pino do componente | Pino do ESP32 |
|---|---|---|
| DHT22 | VCC | 3V3 |
| DHT22 | SDA (dados) | GPIO 4 |
| DHT22 | GND | GND |
| Módulo LDR (fotorresistor) | VCC | 3V3 |
| Módulo LDR (fotorresistor) | AO (saída analógica) | GPIO 34 |
| Módulo LDR (fotorresistor) | GND | GND |
| LED | LED onboard | GPIO 2 |

Observações:

- O GPIO 34 é somente entrada, adequado para a leitura analógica do LDR.
- A conversão de lux usa a fórmula do módulo fotorresistor do Wokwi, com resistor fixo de 10 kΩ. Para um LDR físico com outro resistor, ajuste a constante na função `leLux()`.

### `libraries.txt`

```
PubSubClient
DHT sensor library
Adafruit Unified Sensor
```

### `diagram.json` (exemplo)

Se preferir, exporte o `diagram.json` diretamente do seu projeto no Wokwi, que é a versão mais confiável.

```json
{
  "version": 1,
  "author": "Manuella",
  "editor": "wokwi",
  "parts": [
    { "type": "wokwi-esp32-devkit-v1", "id": "esp", "top": 0, "left": 0, "attrs": {} },
    {
      "type": "wokwi-dht22",
      "id": "dht1",
      "top": 0.3,
      "left": 148.2,
      "attrs": { "temperature": "24", "humidity": "60" }
    },
    {
      "type": "wokwi-photoresistor-sensor",
      "id": "ldr1",
      "top": 195,
      "left": 113.2,
      "rotate": 180,
      "attrs": {}
    }
  ],
  "connections": [
    [ "esp:TX0", "$serialMonitor:RX", "", [] ],
    [ "esp:RX0", "$serialMonitor:TX", "", [] ],
    [ "dht1:VCC", "esp:3V3", "red", [ "v0" ] ],
    [ "dht1:GND", "esp:GND.1", "black", [ "v0" ] ],
    [ "dht1:SDA", "esp:D4", "green", [ "v0" ] ],
    [ "ldr1:VCC", "esp:3V3", "red", [ "h0" ] ],
    [ "ldr1:GND", "esp:GND.1", "black", [ "h0" ] ],
    [ "ldr1:AO", "esp:D34", "green", [ "h0" ] ]
  ],
  "dependencies": {}
}
```

## 6. Como replicar no Wokwi

1. Acesse [wokwi.com](https://wokwi.com) e crie um projeto **ESP32**.
2. Adicione os componentes **DHT22** e **Photoresistor Sensor (LDR)** e faça as ligações da seção 5 (ou substitua o `diagram.json` pelo do repositório).
3. Cole o conteúdo de `sketch.ino` no editor.
4. Adicione as bibliotecas da seção 5 pelo *Library Manager* (ou cole o `libraries.txt`).
5. Confira as configurações no topo do `sketch.ino`:

   | Constante | Valor usado | Descrição |
   |---|---|---|
   | `SSID` / `PASSWORD` | `Wokwi-GUEST` / vazio | Rede Wi-Fi da simulação |
   | `BROKER_MQTT` | `3.135.234.101` | IP do broker MQTT do FIWARE da aula |
   | `BROKER_PORT` | `1883` | Porta MQTT |
   | `ID_MQTT` | `fiware_888` | Identificador do cliente MQTT |
   | `INTERVALO_ENVIO` | `5000` | Intervalo de envio, em ms |

6. Clique em **Start Simulation**. No monitor serial devem aparecer:
   - `Conectado. IP: ...`
   - `Conectado ao broker!`
   - `Enviado: t|25.3|h|0.60|p|500|d|...|TimeInstant|...`

Se o IP do broker mudar, atualize `BROKER_MQTT`. Se quiser usar outro dispositivo, troque o número `888` em `TOPICO_SUBSCRIBE`, `TOPICO_ATTRS`, `topicPrefix` e `ID_MQTT`.

## 7. Tópicos MQTT e formato dos dados

O dispositivo usa o padrão **FIWARE Descomplicado (Ultralight 2.0)**, com pares `chave|valor` separados por `|`.

| Tópico | Sentido | Conteúdo |
|---|---|---|
| `/TEF/device888/attrs` | ESP32 → broker | Leituras dos sensores e estado do LED |
| `/TEF/device888/cmd` | broker → ESP32 | Comandos `device888@on\|` e `device888@off\|` |

**Payload dos sensores** (um único payload por leitura, a cada 5 s):

```
t|25.3|h|0.60|p|500|d|2026-10-16T14:30:00Z|TimeInstant|2026-10-16T14:30:00Z
```

| Chave | Atributo no Smart Data Model | Unidade / formato |
|---|---|---|
| `t` | `temperature` | °C |
| `h` | `relativeHumidity` | fração de 0 a 1 (0.60 = 60 %) |
| `p` | `illuminance` | lux (estimado a partir do LDR) |
| `d` | `dateObserved` | ISO 8601, UTC |
| `TimeInstant` | `TimeInstant` | ISO 8601, UTC (timestamp da leitura no FIWARE) |

**Estado do LED**, enviado ao conectar e a cada comando:

```
s|on
s|off
```

Comportamentos importantes:

- **Timestamp obrigatório:** se o NTP ainda não sincronizou, a leitura **não é enviada**.
- **Falha no DHT22:** o ESP32 envia apenas `p`, `d` e `TimeInstant`, e registra o erro no monitor serial.
- **`d` e `TimeInstant`:** `d` alimenta o atributo `dateObserved` do Smart Data Model; `TimeInstant` é o instante de leitura usado pelo IoT Agent e pelos mecanismos de histórico.

## 8. Back-end FIWARE e checagens pelo Postman

### 8.1 Componentes

| Componente | Função no projeto | Porta padrão |
|---|---|---|
| **IoT Agent MQTT** (Ultralight 2.0) | Recebe os payloads MQTT do ESP32, converte em NGSIv2 e repassa os comandos `on`/`off` ao dispositivo | `4041` |
| **Orion Context Broker** | Guarda o estado atual da entidade `IndoorEnvironmentObserved` e dispara as notificações | `1026` |
| **STH-Comet** | Armazena o histórico das leituras (séries temporais) | `8666` |

### 8.2 Configuração do back-end (feita pelo grupo)

A configuração do back-end foi feita pelo grupo, uma única vez e antes dos testes, no servidor FIWARE da disciplina: o dispositivo `device888` foi provisionado no IoT Agent e a assinatura Orion → STH-Comet foi criada no Orion. Essa configuração não faz parte do código do ESP32. A tabela abaixo resume o que foi criado:

| Item | Valor |
|---|---|
| `fiware-service` | `smart` |
| `fiware-servicepath` | `/` |
| `apikey` (grupo de serviço) | `TEF` |
| Dispositivo (`device_id`) | `device888` |
| Entidade | `urn:ngsi-ld:IndoorEnvironmentObserved:888` (tipo `IndoorEnvironmentObserved`) |
| Transporte e timestamp do dispositivo | `MQTT`, `timestamp: true` |
| Assinatura | Orion → STH-Comet (`http://3.135.234.101:8666/notify`, formato `legacy`) para `temperature`, `relativeHumidity` e `illuminance` |

Mapeamento do payload Ultralight para os atributos da entidade no IoT Agent:

| Chave (`object_id`) | Atributo NGSIv2 | Tipo |
|---|---|---|
| `t` | `temperature` | Number |
| `h` | `relativeHumidity` | Number |
| `p` | `illuminance` | Number |
| `d` | `dateObserved` | DateTime |
| `s` | `state` | Text |

Comandos registrados no dispositivo: `on` e `off`.

Para replicar o projeto em outro servidor FIWARE, é preciso repetir essa configuração: criar o grupo de serviço, provisionar o dispositivo com o mapeamento acima e criar a assinatura do STH-Comet, seguindo a documentação do IoT Agent e do Orion.

### 8.3 Coleção do Postman

Os arquivos estão na pasta `postman/`. Todas as requisições são **GET** e servem para **checar** o back-end, sem alterar nada no servidor.

**Como importar:**

1. No Postman, clique em **Import** e selecione os dois arquivos da pasta `postman/`.
2. No canto superior direito, escolha o environment **Vinheria Agnello - FIWARE**.
3. Edite a variável `url` e coloque o IP ou host do servidor FIWARE, sem `http://` e sem porta.
4. Rode as requisições na ordem, com a simulação do Wokwi em execução.

Os demais valores (`smart`, `/`, `TEF`, `device888`, `IndoorEnvironmentObserved` e o `id` da entidade) estão fixos dentro das requisições. Se o seu servidor usar outros, edite as requisições.

**Requisições:**

| # | Requisição | Método e URL | O que verifica |
|---|---|---|---|
| 1 | Health Check - IoT Agent MQTT | `GET http://{{url}}:4041/iot/about` | IoT Agent no ar |
| 2 | Health Check - Orion | `GET http://{{url}}:1026/version` | Orion no ar |
| 3 | Health Check - STH-Comet | `GET http://{{url}}:8666/version` | STH-Comet no ar |
| 4 | Consultar Entidade (Orion) | `GET http://{{url}}:1026/v2/entities/urn:ngsi-ld:IndoorEnvironmentObserved:888` | Estado atual da entidade (NGSIv2) |
| 5 | Histórico - Temperatura (últimas 10) | `GET http://{{url}}:8666/STH/v1/contextEntities/type/IndoorEnvironmentObserved/id/urn:ngsi-ld:IndoorEnvironmentObserved:888/attributes/temperature?lastN=10` | Histórico de temperatura |
| 6 | Histórico - Umidade (últimas 10) | mesma URL, com `attributes/relativeHumidity?lastN=10` | Histórico de umidade |
| 7 | Histórico - Luminosidade (últimas 10) | mesma URL, com `attributes/illuminance?lastN=10` | Histórico de luminosidade |

As requisições 4 a 7 enviam os cabeçalhos `fiware-service: smart` e `fiware-servicepath: /`.

### 8.4 Respostas esperadas

**Requisições 1 a 3 (health check):** status `200 OK` com um JSON que informa a versão do serviço.

**Requisição 4 (entidade no Orion):** status `200 OK` com os atributos do Smart Data Model e valores iguais aos mais recentes enviados pelo ESP32.

```json
{
    "id": "urn:ngsi-ld:IndoorEnvironmentObserved:888",
    "type": "IndoorEnvironmentObserved",
    "TimeInstant": {
        "type": "DateTime",
        "value": "2026-10-23T17:46:37.000Z",
        "metadata": {}
    },
    "dateObserved": {
        "type": "DateTime",
        "value": "2026-10-23T17:46:37.000Z",
        "metadata": {
            "TimeInstant": {
                "type": "DateTime",
                "value": "2026-10-23T17:46:37.000Z"
            }
        }
    },
    "illuminance": {
        "type": "Number",
        "value": 7,
        "metadata": {
            "TimeInstant": {
                "type": "DateTime",
                "value": "2026-10-23T17:46:37.000Z"
            }
        }
    },
    "relativeHumidity": {
        "type": "Number",
        "value": 0.33,
        "metadata": {
            "TimeInstant": {
                "type": "DateTime",
                "value": "2026-10-23T17:46:37.000Z"
            }
        }
    },
    "state": {
        "type": "Text",
        "value": "off",
        "metadata": {
            "TimeInstant": {
                "type": "DateTime",
                "value": "2026-10-23T17:44:07.400Z"
            }
        }
    },
    "temperature": {
        "type": "Number",
        "value": 13.2,
        "metadata": {
            "TimeInstant": {
                "type": "DateTime",
                "value": "2026-10-23T17:46:37.000Z"
            }
        }
    },
    "on": {
        "type": "command",
        "value": "",
        "metadata": {}
    },
    "off": {
        "type": "command",
        "value": "",
        "metadata": {}
    }
}
```

**Requisições 5 a 7 (histórico no STH-Comet):** status `200 OK` com as últimas leituras do atributo, cada uma com o horário de recebimento (`recvTime`) e o valor (`attrValue`).

```json
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "values": [
                            {
                                "_id": "6adb9da5157d0d00077c03fe",
                                "recvTime": "2026-10-23T17:46:32.000Z",
                                "attrName": "temperature",
                                "attrType": "Number",
                                "attrValue": 13.2
                            },
                            {
                                "_id": "6adb9dab157d0d00077c0401",
                                "recvTime": "2026-10-23T17:46:37.000Z",
                                "attrName": "temperature",
                                "attrType": "Number",
                                "attrValue": 13.2
                            }
                        ]
                    }
                ],
                "id": "urn:ngsi-ld:IndoorEnvironmentObserved:888",
                "isPattern": false,
                "type": "IndoorEnvironmentObserved"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

Os resultados vêm em JSON e podem ser exportados para planilhas ou ferramentas de análise, gerando gráficos e relatórios de **rastreabilidade** das condições de armazenamento dos vinhos desde a entrada no depósito.

## 9. Como usar o MyMQTT

1. Instale o **MyMQTT** (Android) ou outro cliente MQTT.
2. Em **Settings**, configure:
   - **Broker address:** `3.135.234.101`
   - **Port:** `1883`
   - sem usuário e senha, a menos que o ambiente exija.
3. **Leitura:** na aba **Subscribe**, assine `/TEF/device888/attrs`. Com a simulação rodando, chegam os payloads dos sensores a cada 5 s e o estado do LED.
4. **Escrita:** na aba **Publish**, publique no tópico `/TEF/device888/cmd`:
   - `device888@on|` → liga o LED e o ESP32 responde `s|on`
   - `device888@off|` → desliga o LED e o ESP32 responde `s|off`

## 10. Teste rápido (passo a passo)

1. Importe a coleção e o environment no Postman e preencha `url` (seção 8.3).
2. Rode as requisições 1 a 3 e confirme `200 OK` nos três serviços.
3. Inicie a simulação no Wokwi e aguarde `Conectado ao broker!`.
4. Abra o MyMQTT e assine `/TEF/device888/attrs`.
5. Altere a temperatura e a umidade no DHT22 e a luminosidade no LDR dentro do Wokwi e observe os novos valores chegando.
6. Rode a requisição 4 (entidade no Orion) e confirme que `temperature`, `relativeHumidity`, `illuminance` e `dateObserved` batem com o que foi enviado.
7. Rode as requisições 5 a 7 e confirme que as leituras novas aparecem no histórico, com o horário correto.
8. Publique `device888@on|` em `/TEF/device888/cmd` e confirme que o LED acende e que chega `s|on`.

## 11. Problemas comuns

| Sintoma | Causa provável | Solução |
|---|---|---|
| `Hora ainda não sincronizada (NTP)` | NTP ainda não respondeu | Aguarde alguns segundos; o envio retoma sozinho |
| `Falha ao ler o DHT22` | Ligação errada no GPIO 4 | Confira o pino de dados do DHT22 |
| `Falha. Nova tentativa em 2s` | Broker inacessível ou IP desatualizado | Confirme `BROKER_MQTT` e a porta 1883 |
| Requisições 1 a 3 sem resposta | `url` errada, serviço fora do ar ou porta bloqueada | Confira a variável `url` e as portas `4041`, `1026` e `8666` |
| Dados chegam no MyMQTT mas a requisição 4 não retorna a entidade (`404`) | Dispositivo não provisionado no IoT Agent ou `fiware-service`/`fiware-servicepath` diferentes | Confirme a configuração do servidor (seção 8.2) |
| Entidade existe no Orion mas as requisições 5 a 7 retornam vazio | Assinatura ausente ou com URL errada, ou ainda não houve envio depois dela | Confirme a assinatura Orion → STH-Comet (seção 8.2) e aguarde novos envios |
| LED não responde | Comando fora do formato | Use exatamente `device888@on\|` ou `device888@off\|` |
| Lux muito diferente do esperado | Fórmula é a do módulo do Wokwi | Ajuste `RL10`, `GAMMA_LDR` e o resistor fixo para um LDR físico |

## 12. Alinhamento com Smart Data Models e integração europeia

Os dados seguem o modelo **IndoorEnvironmentObserved** do projeto Smart Data Models, mantido no ecossistema FIWARE da União Europeia. Por usar nomes de atributos, unidades e formato de data padronizados (por exemplo, `relativeHumidity` como fração de 0 a 1 e datas em ISO 8601 UTC), os dados da Vinheria podem ser consumidos por distribuidoras europeias que utilizem o mesmo padrão, sem conversões específicas.

## 13. Demonstração em vídeo

O vídeo mostra: o circuito e o código no Wokwi, a captura dos dados do DHT22 e do LDR, o MyMQTT recebendo os dados em `/attrs` e o envio dos comandos `on`/`off` acendendo o LED.

Link: https://youtu.be/1pYxvc58QRk

## 14. Tecnologias

ESP32 · Wokwi · DHT22 · LDR · MQTT · MyMQTT · Postman · FIWARE (IoT Agent MQTT, Orion Context Broker, STH-Comet) · NGSIv2 · Smart Data Models · NTP

## 15. Licença e créditos

Projeto acadêmico desenvolvido para a FIAP na matéria de Edge Computing - Professor **Lucas Demetrius Augusto**. Smart Data Models e FIWARE são projetos abertos de seus respectivos mantenedores.