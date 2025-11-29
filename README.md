```
      _,---.   .=-.-.    _,---.   ,---.      .-._        ,--.--------.    ,----.  
  _.='.'-,  \ /==/_ /_.='.'-,  \.--.'  \    /==/ \  .-._/==/,  -   , -\,-.--` , \ 
 /==.'-     /|==|, |/==.'-     /\==\-/\ \   |==|, \/ /, |==\.-.  - ,-./==|-  _.-` 
/==/ -   .-' |==|  /==/ -   .-' /==/-|_\ |  |==|-  \|  | `--`\==\- \  |==|   `.-. 
|==|_   /_,-.|==|- |==|_   /_,-.\==\,   - \ |==| ,  | -|      \==\_ \/==/_ ,    / 
|==|  , \_.' )==| ,|==|  , \_.' )==/ -   ,| |==| -   _ |      |==|- ||==|    .-'  
\==\-  ,    (|==|- \==\-  ,    (==/-  /\ - \|==|  /\ , |      |==|, ||==|_  ,`-._ 
 /==/ _  ,  //==/. //==/ _  ,  |==\ _.\=\.-'/==/, | |- |      /==/ -//==/ ,     / 
 `--`------' `--`-` `--`------' `--`        `--`./  `--`      `--`--``--`-----``

                     ,----.                ___                  _,---.  
      _,..---._   ,-.--` , \        .-._ .'=.'\  _,..---._   .-`.' ,  \ 
    /==/,   -  \ |==|-  _.-`       /==/ \|==|  /==/,   -  \ /==/_  _.-' 
    |==|   _   _\|==|   `.-.       |==|,|  / - |==|   _   _Y==/-  '..-. 
    |==|  .=.   /==/_ ,    /       |==|  \/  , |==|  .=.   |==|_ ,    / 
    |==|,|   | -|==|    .-'        |==|- ,   _ |==|,|   | -|==|   .--'  
    |==|  '='   /==|_  ,`-._       |==| _ /\   |==|  '='   /==|-  |     
    |==|-,   _`//==/ ,     /       /==/  / / , /==|-,   _`//==/   \     
    `-.`.____.' `--`-----``        `--`./  `--``-.`.____.' `--`---'     
```

O **Gigante de MDF** é um carrinho de batalha controlado por comandos serial via **Bluetooth**, interpretados pelo USART do microcontrolador **ATmega328P**.  

O projeto inclui:
- Dois motores DC 5V controlados por um C.I L298N (movimentação)
- Um sensor **LDR** (detecção de acerto de laser)  
- Um **display de 7 segmentos** (contador de vidas)  
- Um **laser 5 V** (arma)
- Um módulo bluetooth HC-10 (controle)

## Descrição De Recursos

Todas as funcionalidades do carrinho são controladas via bytes enviados pelo HC-10.
Os comandos aceitos são:
| Comando | Ação |
|---------|------|
| `f` | Move o carrinho para frente |
| `b` | Move o carrinho para trás |
| `l` | Vira o carrinho para esquerda |
| `r` | Vira o carrinho para direita |
| `p` | Para os motores |
| `m` | Troca o modo do laser para manual |
| `a` | Troca o modo do laser para automático |
| `s` | Dispara laser (no modo manual) |
| `0` | Girar 180° |
| `1` | Impulso para frente |
| `3` | Reseta vidas |
| `!` | Alterna o modo de depuração |
| `+` / `-` | Ajusta velocidade |

Os motores são controlados pelo C.I de ponte H L298N, com a velocidade sendo modulada via PWM
Cada um dos motores tem um Timer diferente (*Timer0* e *Timer1*) controlando sua saida PWM.

O laser possui dois modos de operação:

### • Modo Automático  
Liga e desliga a cada 1 segundo.

### • Modo Manual  
Controlado por comando Bluetooth.  
Quando ativado, dispara por 1 segundo e entra em *cooldown* por mais 1 segundo.

O tempo do timer é controlado pelo *Timer2* do **Atmega328P** utilizando a interrupção de overflow para contar até 1 segundo

O **LDR** detecta disparos inimigos e remove vidas.  
O display de 7 segmentos exibe o número atual de vidas.

### Modo de Depuração
Quanto ativado, o **Atmega328P** envia para o HC-10 informações extras como os valores lidos pelo LDR e os resultados das interrupções

## Estrutura do Projeto
```bash
/
├── source/
│ └── main.c - Código-fonte em C do firmware
│
├── html/
│ └── Documentação HTML gerada pelo Doxygen
│
├── latex/
│ └── Documentação LaTeX gerada pelo Doxygen
│
├── schematics/
│ ├── schina-ponte-h.kicad_sch - Esquemático do KiCad
│ ├── schina-ponte-h.kicad_pcb -PCB do KiCad
│ └── circSchina.sim1 - Arquivo .simu do SimulIDE
│
└── README.md
```

## Executando no SimulIDE
1. Instale o **SimulIDE** de https://simulide.com/p/
2. Abra o arquivo schematics/circSchina.sim1 com o **SimulIDE**
3. Abra o arquivo source/main.c como código
4. Configure o arquivo como compilador avrgcc
5. Configure o compilador passando o endereço da toolchain do avr-gcc
6. no Device, coloque ATMEGA328P
7. Compile e envie o código para o microcontrolador simulado.
