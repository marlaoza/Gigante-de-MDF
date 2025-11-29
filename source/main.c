/// @file main.c
#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL ///< Valor do CLOCK da CPU.

#include <util/delay.h>

#define SEG_A PB5
#define SEG_B PC1
#define SEG_C PC2
#define SEG_D PC3
#define SEG_E PC5
#define SEG_G PC4

#define LDR PC0

#define LASER PD5

#define MOTOR1_A PB2
#define MOTOR1_B PB3
#define MOTOR1_PWM PD6

#define MOTOR2_A PB0
#define MOTOR2_B PD7
#define MOTOR2_PWM PB1

#define TIME_180 200 ///< Tempo necessário para a volta de 180 graus. (em ms)

uint8_t alive = 1; ///< Flag que determina se o gigante esta vivo.
uint8_t debug = 0; ///< Flag que determina se o modo de depuração está ativo.

/**
 * Seta um valor a um bit específicio limpando esse bit e fazendo uma comparação OR com o valor dado
 * @param[in] *port
 * @param[in] bit
 * @param[in] value
 */ 
void setBit (volatile uint8_t *port, uint8_t bit, uint8_t value){
	*port = (*port & ~(1 << bit)) | (value << bit);
}


#define RX_COMPLETE_INTERRUPT (1<<RXCIE0)
#define DATA_REGISTER_EMPTY_INTERRUPT (1<<UDRIE0)

/**
 * Configura e inicializa o USART com a BAUD_RATE fornecida
 * @param[in] value
 */ 
void setupUSART(uint32_t value){

	uint16_t BAUD_PRESCALER = (F_CPU / (value * 16UL)) - 1; 

	UBRR0H = BAUD_PRESCALER >> 8;
	UBRR0L = BAUD_PRESCALER;
	
	UCSR0C = (0 << UMSEL00) | (0<<UPM00) | (0<<USBS0) | (3<<UCSZ00);
	
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
}

/**
 * Envia um byte por meio do USART
 * @param[in] DataByte
 */ 
void sendByteUSART(uint8_t DataByte){
	while( (UCSR0A & (1 << UDRE0)) == 0 ) {};
	UDR0 = DataByte;
}

/**
 * Envia uma string ou cadeia de bytes completa por meio do USART
 * @param[in] str
 */ 
void writeUSART(uint8_t * str) {
	str = str + '\0';
	uint8_t c = 1;
	int aux = 0;
	while(c != '\0'){
		c = str[aux];
		sendByteUSART(c);
		aux++;

	}
}



/**
 * Configura e inicializa o PWM com FastPWM
 * Motor 1 - TIMER 0 PORTA A (PD6)
 * Motor 2 - TIMER 1 PORTA A (PB1)
 */ 
void setupMotorsPwm(){
	TCCR0A = (1 << WGM00) | (1 << WGM01) | (1 << COM0A1);   
	TCCR0B = (1 << CS01);
	OCR0A = 0;
	
	TCCR1A = (1 << WGM10) | (1 << COM1A1);
	TCCR1B = (1 << WGM12) | (1 << CS11);
	OCR1A = 0;
}

/**
 * Seta os valores do Pino A, Pino B e o duty cycle do PWM para o Motor 1
 * pinA e pinB controlam a direção (1 0 frente, 0 1 traz) e o freio (0 0 para, 1 1 para)
 * Duty cicle controla a velocidade do motor
 * @param[in] pinA
 * @param[in] pinB
 * @param[in] cycle
 */ 
void setMotor1(uint8_t pinA, uint8_t pinB, uint8_t cycle){
	setBit(&PORTB, MOTOR1_A, pinA);
	setBit(&PORTB, MOTOR1_B, pinB);
	OCR0A = cycle;
}

/**
 * Seta os valores do Pino A, Pino B e o duty cycle do PWM para o Motor 2
 * pinA e pinB controlam a direção (1 0 frente, 0 1 traz) e o freio (0 0 para, 1 1 para)
 * Duty cicle controla a velocidade do motor
 * @param[in] pinA
 * @param[in] pinB
 * @param[in] cycle
 */ 
void setMotor2(uint8_t pinA, uint8_t pinB, uint8_t cycle){
	setBit(&PORTB, MOTOR2_A, pinA);
	setBit(&PORTD, MOTOR2_B, pinB);
	OCR1A = cycle;
}

uint8_t laserCooldown = 0; ///< Flag que determina se o laser esta em cooldown (não pode ser ativado).
uint8_t laserAuto = 1; ///< Flag que controla o modo do motor (1 - AUTO, 2 - MANUAL).

volatile uint16_t laserCounter = 0; ///< Contador de overflows do Timer do Motor

//16 mhz / 1024 prescaler= 15625HZ
// 1/ 15625HZ = 0.000064s
// 255 *  0.000064s = 0.01632s;
// 1 / 0.01632 = 61.2745098039;
//62 = 1.012 segundos
	
#define LASEROVERFLOWS 62 ///< Valor necessário de overflows para timer de 1.01 segundos

/**
 * Configura e inicializa o Timer2
 */ 
void setupLaserTimer() {
	TCCR2A = 0;          
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); 
    TIMSK2 = (1 << TOIE2);
}

/**
 * Interrupção de controle do Timer2
 * Quando laserCounter for maior ou igual a LASEROVERFLOWS um segundo se passou
 * Controla o laser, caso em modo automático, ligando e desligando intercaladamente
 * Caso em modo manual, desligando após um segundo e configurando o @param laserCooldown
 */ 
ISR(TIMER2_OVF_vect) {
	if(debug){
		writeUSART("OVERFLOW TIMER2 \n");
	}
	
	laserCounter++;
	
	if(laserCounter >= LASEROVERFLOWS){
		if(debug){
			writeUSART("1.012 segundos timer2 \n");
		}
		
		if(laserAuto){
			PORTD ^= (1 << LASER);
		}else{	
			if(PIND & (1 << LASER)){
				PORTD &= ~(1 << LASER);
			}else{
				laserCooldown = 0;
			}
		}		
		laserCounter = 0;
	}
	
}


/**
 * Controle de ativação do laser
 * Caso laserCooldown esteja em 0, ativa o laser e reinicia laserCounter
 */ 
void shoot() {
	if(!laserCooldown){
		PORTD |= (1 << LASER);
		laserCounter = 0;
		laserCooldown = 1;
	}else{
		writeUSART("ARMA EM COOLDOWN \n");
	}
}


#define maxLife 3 ///< Valor máximo de vidas
uint8_t life = 3; ///< Valor atual de vidas

const uint8_t lifeDisplay[] = {
	//- - E G D C B (A)
	0b00000000,
	0b00000110,
	0b00111011,
	0b00011111,
}; ///< Lista com a configuração do led de 7 segmentos para cada número de vidas

/**
 * Atualiza o valor do display de 7 segmentos que representa a vida
 */ 
void updateLife() {
	uint8_t value = lifeDisplay[life];
	uint8_t bitA = value & 0b00000001;
	setBit(&PORTB, SEG_A, bitA);
	PORTC = (PORTC & 0b11000001) | (value & 0b00111110);
}

/**
 * Remove uma vida e realiza a rotina de perda de vida (rodar 180 graus e desativar por 5 segundos)
 * Caso não existam mais vidas para remover, desliga os motores e seta @param alive para 0
 */ 
void removeLife() {
	if(alive) {
		if(life > 0){
			writeUSART("Removendo uma Vida");
			life -= 1;
			updateLife();
			setMotor1(1, 0, 255);
			setMotor2(0, 1, 255);
			_delay_ms(TIME_180);
			setMotor1(0, 0, 0);
			setMotor2(0, 0, 0);
			_delay_ms(5000);
		}else{
			alive = 0;
			setMotor1(0, 0, 0);
			setMotor2(0, 0, 0);
		}
	}
}

/**
 * Recarrega as vidas para o valor máximo e seta  @param alive para 1
 */
void resetLife() {
	writeUSART("Resetando Vidas");
	life = maxLife;
	alive = 1;
	updateLife();
}

/**
 * Configura e inicia o conversor analógico digital
 */
void setupADC() {
    ADMUX = (1 << REFS0); 
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

/**
 * Aguarda a conversão e retorna a leitura do conversor analógico digital para o pino PC0
 * @return leitura do pino PC0 de 10 bits (0–1023)
 */
uint16_t readLDR() {
    ADMUX = (ADMUX & 0xF0) | 0; 
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

/**
 * Função de configuração principal, seta os pinos como saída ou entrada e chama as funções de setup
 */
void start(){
	setupUSART(9600);
	
	writeUSART("START\n");
	//SEGMENTOS
	DDRC |= (1 << SEG_B) | (1 << SEG_C) | (1 << SEG_D) | (1 << SEG_E) | (1 << SEG_G);
	DDRB |= (1 << SEG_A);
	
	//MOTOR
	DDRB |= (1 << MOTOR1_A) | (1 << MOTOR1_B) | (1 << MOTOR2_A) | (1 << MOTOR2_PWM);
	DDRD |= (1 << MOTOR1_PWM) | (1 << MOTOR2_B);
	
	//LASER
	DDRD |= (1 << LASER);
	
	writeUSART("SETANDO PWM MOTORES \n");
	setupMotorsPwm();
	
	writeUSART("SETANDO ADC LDR \n");
	setupADC();
	
	writeUSART("SETANDO LASER TIMER \n");
	setupLaserTimer();
	
	resetLife();
	
	sei();
	UCSR0B |= RX_COMPLETE_INTERRUPT;
}

int SPEED = 200; ///< Valor da velocidade do carrinho
volatile uint8_t dataUSART; ///< Byte que guarda a mensagem que vai ser lida pelo USART

/**
 * Interrupção que lida com as mensagens do USART, interpretando os dados para controlar o robô
 */
ISR(USART_RX_vect){
	if(alive){
		dataUSART = UDR0;
		
		if(dataUSART == 'f'){
			writeUSART("MOVENDO FRENTE \n");
			setMotor1(1, 0, SPEED);
			setMotor2(1, 0, SPEED);
		}
		else if(dataUSART == 'b'){
			writeUSART("MOVENDO TRAZ \n");
			setMotor1(0, 1, SPEED);
			setMotor2(0, 1, SPEED);
		}
		else if(dataUSART == 'l'){
			writeUSART("MOVENDO ESQ \n");
			setMotor1(1, 0, SPEED);
			setMotor2(0, 0, 0);
		}
		else if(dataUSART == 'r'){
			writeUSART("MOVENDO DIR \n");
			setMotor1(0, 0, 0);
			setMotor2(1, 0, SPEED);
		}
		else if(dataUSART == 'p'){
			writeUSART("PARANDO \n");
			setMotor1(0, 0, 0);
			setMotor2(0, 0, 0);
		}
		else if(dataUSART == 'm'){
			writeUSART("LASER ENTRANDO EM MODO MANUAL \n");
			laserAuto = 0;
		}
		else if(dataUSART == 'a'){
			writeUSART("LASER ENTRANDO EM MODO AUTOMATICO \n");
			laserAuto = 1;
		}
		else if(dataUSART == 's'){
			if(laserAuto){
				writeUSART("LASER AUTOMATICO NAO PODE ATIRAR \n");
			}else{
				writeUSART("ATIRANDO \n");
				shoot();
			}
			
		}else if(dataUSART == '0'){
			writeUSART("VIRANDO BUNDA\n");
			setMotor1(1, 0, 255);
			setMotor2(0, 1, 255);
			_delay_ms(TIME_180);
			setMotor1(0, 0, 0);
			setMotor2(0, 0, 0);
		}
		else if(dataUSART == '1'){
			writeUSART("CABEÇADA\n");
			setMotor1(1, 0, 255);
			setMotor2(1, 0, 255);
			_delay_ms(100);
			setMotor1(0, 0, 0);
			setMotor2(0, 0, 0);
		}else if(dataUSART == '!'){
			if(debug) {
				writeUSART("DEBUG DESLIGADO\n");
			}else{
				writeUSART("DEBUG LIGADO\n");
			}
			debug = !debug;
		}else if(dataUSART == '3'){
			resetLife();
		}else if(dataUSART == '+'){
			SPEED += 10;
			if(SPEED > 255){
				SPEED = 255;
			}
		}
		else if(dataUSART == '-'){
			SPEED -= 10;
			if(SPEED < 0){
				SPEED = 0;
			}
		}
	}
	
	
}

#define LDR_THRESHOLD 900  ///< Valor mínimo de detecção do LDR
char buffer[6]; ///< Buffer 6 bytes para guardar valor lido do LDR convertido para String

/**
 * Função de Entrada
*/
int main(void){
	start();
	while(1){
			uint16_t ldrValue = readLDR();
			if(debug){
				int i = 0;
				int temp = ldrValue;
				do {
					buffer[i++] = (temp % 10) + '0';
					temp /= 10;
				} while (temp > 0);
			
				buffer[i] = '\0';
				for(int j=0; j<i/2; j++){
					char t = buffer[j];
					buffer[j] = buffer[i-1-j];
					buffer[i-1-j] = t;
				}
				writeUSART("LDR:"); writeUSART(buffer);
			}
			
			if(ldrValue > LDR_THRESHOLD){
				removeLife();
			}	
	}
	
	return 0;
}