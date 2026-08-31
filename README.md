# 🧺 Washing Machine Custom Controller: From Spaghetti to Clean C++

> Uma jornada prática de engenharia de software embarcado: restaurando uma lavadora de roupas e transformando um código monolítico/legado em uma arquitetura C++ moderna, modular, não-bloqueante e testável no PC (Host Unit Tests).

---

## 📖 Sobre o Projeto

Este projeto nasceu da restauração de uma lavadora de roupas antiga cuja placa controladora original havia queimado. O controle original foi substituído por uma placa de desenvolvimento microcontrolada (Arduino / ATmega328P).

O firmware inicial (**v0.1.0**) cumpria o seu papel de lavar as roupas, mas apresentava as limitações clássicas de projetos Arduino prototipados rapidamente:
- Código monolítico em um único arquivo `.ino`.
- Uso intensivo de laços `while()` e chamadas bloqueantes `delay()`.
- Variáveis globais compartilhadas sem proteção ou encapsulamento.
- Impossibilidade de ler sensores em tempo real (como acelerômetro de vibração) durante a execução de um ciclo.
- Ausência de testes automatizados (qualquer alteração exigia testar diretamente na máquina física).

Este repositório documenta passo a passo a **evolução arquitetural** desse firmware.

---

## 🗺️ Roadmap de Evolução & Releases

A evolução do projeto está dividida em marcos arquiteturais com tags Git e Releases detalhadas:

| Versão | Marco Arquitetural | Descrição |
| :---: | :--- | :--- |
| **`v0.1.0`** | 🍝 **Legacy Spaghetti (Baseline)** | Código original monolítico `.ino`, laços bloqueantes e estado global. |
| **`v0.2.0`** | 🔌 **Hardware Abstraction Layer (HAL)** | Isolamento de hardware via interfaces C++ (`IActuators`, `ISensors`) e travas de segurança (*dead-time* do motor contra curto de reversão). |
| **`v0.3.0`** | ⚙️ **Non-Blocking FSM** | Máquina de Estados Finita não-bloqueante baseada em eventos e `millis()`, eliminando todo `delay()`. |
| **`v0.4.0`** | 🧪 **Host Unit Testing (Linux/PC)** | Testes unitários com *Mocks* compilados nativamente no Linux (`g++`), validando tempos, timeouts e transições em milissegundos. |
| **`v0.5.0`** | 🚨 **I2C Watchdog & WS2812B** | Monitoramento de desbalanceamento por acelerômetro I2C e simplificação de 7 LEDs para 1 pino de LED endereçável. |
| **`v1.0.0`** | 🚀 **Production Modern C++** | Firmware final robusto, documentado e pronto para produção. |

---

## 🛠️ Como Compilar e Testar

O projeto foi desenhado para ser **100% amigável para a comunidade Arduino**, sem exigir ferramentas proprietárias ou configurações complexas.

### 1. No Arduino IDE (Interface Gráfica)
1. Abra o arquivo `washing-machine.ino` diretamente no Arduino IDE.
2. Selecione a placa (**Arduino Nano** ou **Arduino Pro Mini** - ATmega328P, 5V, 16MHz).
3. Clique em **Verificar** / **Carregar**.

### 2. Via Terminal / VS Code / Antigravity (`arduino-cli` e `Makefile`)
Se estiver no Linux/Ubuntu:

```bash
# Compilar o firmware do Arduino:
make build

# Gravar na placa via USB:
make flash PORT=/dev/ttyUSB0

# Executar a suíte de Host Tests no PC (usando g++ nativo):
make test
```

---

## 📸 Fotos e Esquemáticos da Placa

Consulte a pasta [`docs/`](docs/) para ver os esquemáticos elétricos, ligação do pressostato e fotos da placa de potência montada.
