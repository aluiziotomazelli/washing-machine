#include <Arduino.h>
#include <Bounce2.h>		//biblioteca debouncer
#include <TimeLib.h>		//biblioteca do tempo


/* A numeração das portas foi usada de forma a facilitar a ligação
 * do Arduino Pro Mini aos outros componentes da placa de circuito impresso.
 */


const byte chaveAmaciante = 1;		//chave para funcao amaciante
const byte ledAmaciante = 6;		//pino do led do amaciante

const byte ledOn = 7;			//led que indica a máquina funcionando
const byte chaveStart = A3;		//inicia ou para a sequencia

const byte motorDir = 8; 		//liga o motor para direita
const byte motorEsq = 9;		//liga o motor para esquerda
const byte atuadorBomba = 4;		//liga o atuador e a bomba
const byte solenoide1_2 = 3;		//liga solenoide 1 e 2
const byte solenoide3 = 2;		//liga solenoide 3

const byte chavePrograma = A5;		//recebe sinal da chave de programas
const byte chaveNivel = A4;		//recebe sinal da chave de niveis

const byte ledLavar = A2;		//led lavar
const byte ledEnxaguar = A1;		//led enxaguar
const byte ledCentrifugar = A0;		//led centrifugar

const byte ledNivelM = 12;		//led nivel medio
const byte ledNivelB = 13;		//led nivel baixo

const byte chaveNivelB = 10;		//recebe sinal do pressostato nivel alto
const byte chaveNivelM = 11;		//recebe sinal do pressostato nivel alto

const byte buzzer = 5;			//buzzer

byte seletorPrograma;			//contador do Programa
byte seletorNivel;			//contador do Nivel

int tempoDelayEntreEtapas = 4000; 	//delay entre as etapas

bool usaAmaciante;			//status do botao amaciante
bool duploEnxague;			//conta o numero de enxagues
bool botaoStart;			//status do botao

bool estadoChaveNivelB;			//indica estado da chave de nivel baixo
bool estadoChaveNivelM;			//indica estado da chave de nivel medio

bool erro;				//indica algum erro
byte tipoErro;				//indica o tipo de erro

unsigned long tempoAvanco; 		//variavel global de tempo usada para temporizar os ciclos


// Inicializa o Bouncer para os objetos:
Bounce programa = Bounce(); 		//Chave de seleção de Programa
Bounce nivel = Bounce();		//Chave de seleção de Nivel
Bounce start = Bounce();	 	//chave Start
Bounce amaciante = Bounce();		//chave amaciante
/* A biblioteca debouncer fornece uma maneira de evitar que ruídos
 * elétricos que possam surgir nos botões atrapalhem a seleção
 * das funções. Maior informação pode ser conseguida na documentção da biblioteca.
 */


void bip(int vezes = 1, int tempo = 150)
/* Faz o bipe do buzzer sem oscilador interno,
 * armazena a quantidade de vezes (padrão 1)
 * e de tempo (padrão 150 ms)
 *
 * Seria mais fácil usar um buzzer com oscilador
 *  interno, mas eu só tinha um sem oscilador.
 */
{
	for (int x = 0; x < vezes; x++)
	{
		tone(buzzer, 3000); 	//seta o pino do buzzer e a frequencia em 3000, frequencia bem audível
		delay(tempo);   	//espera o tempo
		noTone(buzzer);  	//desliga o buzzer
		delay(tempo);		//espera o tempo caso o buzzer toque mais de uma vez.
	}
}

void setup()
{
	pinMode(chaveAmaciante, INPUT_PULLUP);	//seta o pino como entrada e em HIGH devido ao resistor pull-up interno

	pinMode(ledAmaciante, OUTPUT);		//pino do ledAmaciante
	digitalWrite(ledAmaciante, LOW);	//seta em LOW

	pinMode(ledOn, OUTPUT);			//seta pino como saida

	pinMode(chaveStart, INPUT_PULLUP);	//pino do botao de start

	pinMode(motorDir, OUTPUT);		//seta pino como saida
	digitalWrite(motorDir, LOW); 		//seta o pino em LOW

	pinMode(motorEsq, OUTPUT);		//seta pino como saida
	digitalWrite(motorEsq, LOW); 		//seta o pino em LOW

	pinMode(atuadorBomba, OUTPUT);  	//seta pino como saida
	digitalWrite(atuadorBomba, LOW); 	//seta o pino em LOW

	pinMode(solenoide1_2, OUTPUT);		//seta pino como saida
	digitalWrite(solenoide1_2, LOW); 	//seta o pino em LOW

	pinMode(solenoide3, OUTPUT);		//seta pino como saida
	digitalWrite(solenoide3, LOW); 		//seta o pino em LOW

	pinMode(chavePrograma, INPUT_PULLUP); 	//seta o pino como entrada em nivel alto


	pinMode(chaveNivel, INPUT_PULLUP);	//seta o pino como entrada em nivel alto


	pinMode(ledLavar, OUTPUT);		//seta pino como saida
	digitalWrite(ledLavar, LOW); 		//seta o pino em LOW

	pinMode(ledEnxaguar, OUTPUT);		//seta pino como saida
	digitalWrite(ledEnxaguar, LOW); 	//seta o pino em LOW

	pinMode(ledCentrifugar, OUTPUT);	//seta pino como saida
	digitalWrite(ledCentrifugar, LOW); 	//seta o pino em LOW

	pinMode(ledNivelM, OUTPUT);		//seta pino como saida
	digitalWrite(ledNivelM, LOW); 		//seta o pino em LOW

	pinMode(ledNivelB, OUTPUT);		//seta pino como saida
	digitalWrite(ledNivelB, LOW); 		//seta o pino em LOW

	pinMode(chaveNivelM, INPUT_PULLUP);	//seta o pino como entrada em nivel baixo

	pinMode(chaveNivelB, INPUT_PULLUP);	//seta o pino como entrada em nivel baixo

	pinMode(buzzer, OUTPUT);		//seta pino como saida
	digitalWrite(buzzer, LOW);		//seta o pino em LOW

	// Seta o Bounce2 para o Programa
	programa.attach(chavePrograma);	 	//atraca o debouncer ao pino
	programa.interval(100);       		//seta o intervalo do debouncer
	seletorPrograma = 2; 			//coloca o controle de programa no 2 (Enxague)

	// Seta o Bounce2 para o Nivel
	nivel.attach(chaveNivel); 		//atraca o debouncer ao pino
	nivel.interval(100);       		//seta o intervalo do debouncer
	seletorNivel = 2; 			//coloca o controle de nivel no 2 (medio)

	// Seta o Bounce2 para o Start
	start.attach(chaveStart); 	//atraca o debouncer ao pino
	start.interval(100);       	//seta o intervalo do debouncer
	botaoStart = false;		//inicia em falso

	// Seta o Bounce2 para o Amaciante
	amaciante.attach(chaveAmaciante);	//atraca ao pino da chave do amaciante
	amaciante.interval(100);       		//seta o intervalo do debouncer
	usaAmaciante = false; 			//inicia a varivel que armazena o status do amaciante em false
	duploEnxague = false; 			//inicia em apenas um enxague

	bip(3, 100); 			//dá 3 bipe rápidos indicando que a máquina foi ligada

} //edn void setup

void chaves_niveis()
{

	/* O pressostato possui 3 chaves:
	 * Nivel BAIXO: 31-32 = NF (HIGH se ativado, com resistor externo)
	 * Nivel MEDIO:	11-13 = NA (LOW se ativado, com resistor externo)
	 * Nivel ALTO: 	21-23 = NA (LOW se ativado, com resistor externo)
	 *
	 * NIVEL ALTO NAO ESTA SENDO USADO NO PROGRAMA
	 *
	 * Como o pressostato tem chaves NF e NA, fica fácil se confundir
	 * com o seu estado HIGH ou LOW. Assim criei a essa funcao  para
	 * facilitar o uso. Se a chave foi acionada a variavel estaodoChaveX
	 * torna-se true, se foi desacionada, torna-se false.
	*/

	if (digitalRead(chaveNivelB) == LOW)	//true quando o nivel da agua está no mínimo
		estadoChaveNivelB = false;
	else if(digitalRead(chaveNivelB) == HIGH)
		estadoChaveNivelB = true;

	if (digitalRead(chaveNivelM) == HIGH)	//true quando o nivel da agua está no medio
		estadoChaveNivelM = false;
	else if (digitalRead(chaveNivelM) == LOW)
		estadoChaveNivelM = true;

} //end void ChavesNiveis()

void led_lavar()
{
	//aciona o led do programa em Lavar
	digitalWrite(ledLavar, HIGH);
	digitalWrite(ledEnxaguar, LOW);
	digitalWrite(ledCentrifugar, LOW);
}

void led_exaguar()
{
	// aciona o led do programa em Enxaguar
	digitalWrite(ledLavar, LOW);
	digitalWrite(ledEnxaguar, HIGH);
	digitalWrite(ledCentrifugar, LOW);
}

void led_centrifugar()
{
	// aciona o led do programa em centrifugar
	digitalWrite(ledLavar, LOW);
	digitalWrite(ledEnxaguar, LOW);
	digitalWrite(ledCentrifugar, HIGH);
}

void selecao_programa()
{
	programa.update(); 			//dá update no botao seletor de programa usado a biblioteca Bounce2
	if (programa.fell()) 			//se o botao vai de HIGH (normal) para LOW (pressionado)
	{
		seletorPrograma++;   		//o contador do programa aumenta 1
		bip();
		if (seletorPrograma > 3)	//se o contador passar de 3
		{
			seletorPrograma = 1;  	//ele volta para 1
		}
	}

	switch (seletorPrograma)  		//verifica o status da variavel e aciona o LED correspondente
	{
	case 1:
		//programa em Lavar
		led_lavar();
		break;
	case 2:
		// programa em Enxaguar
		led_exaguar();
		break;
	case 3:
		// programa em centrifugar
		led_centrifugar();
		break;
	} //end switch
} //end void SelecaoPrograma()

void selecao_nivel()
{
	nivel.update(); 	   	//da update no botao usando a biblioteca Bounce 2
	if (nivel.fell())   			//se o botao vai de HIGH (normal) para LOW (pressionado)
	{
		seletorNivel++;   		//o contador do programa aumenta 1
		bip();
		if (seletorNivel > 2)		//se o contador passar de 2
		{
			seletorNivel = 1;	//ele volta para 1
		}
	}

	switch (seletorNivel)  		//verifica o status do Nivel e comanda os LEDs correspondentes
	{
	case 1: 	// nivel Baixo
		digitalWrite(ledNivelB, HIGH);
		digitalWrite(ledNivelM, LOW);
		break;
	case 2:		// nivel Médio
		digitalWrite(ledNivelB, LOW);
		digitalWrite(ledNivelM, HIGH);
		break;
	} //end switch
} //end void

void botao_sart()
{
	start.update();  			//dá update no botao
	if (start.fell() && !botaoStart) 	//se o botao vai de HIGH (normal) para LOW (pressionado)
	{
		digitalWrite(ledOn, HIGH);	//acende o LED
		bip();
		botaoStart = true;		//torna a variável true
	}
} // end void

void selecao_amaciante()
{
	amaciante.update();				//da update no botao de amaciante usando a biblioteca Bounce2
	if (amaciante.fell() && !usaAmaciante) 		//se o botao é pressionado E a variavel usaAmaciante é false
	{
		digitalWrite(ledAmaciante, HIGH);	//acende o led indicando enxague duplo e amaciante
		bip();
		usaAmaciante = true; 			//torna a variável true --> será usada no processo de enxague e seleção das solenoides
		duploEnxague = true; 			//ao usar a funcao amaciante um enxague extra é adicionado
	}

	else if (amaciante.fell() && usaAmaciante)	//se o botao é pressionado novamente
	{
		digitalWrite(ledAmaciante, LOW);	// desliga o led
		bip();
		usaAmaciante = false;			//seta ambas variaveis como false
		duploEnxague = false;
	}
} // end void

void avancar()
//funcao usada para avancar programas. Ex: a máquina entrou no modo molho e voce quer pular ele.
{
	delay(1000);
	bip(2, 150);
	tempoAvanco = now(); 	//torna o tempoAvanco, usado nas funcoes de timer igual now(), o tempo atual,
				//assim o programa dependente do timer deixa de ser executado
} //end void

void seleciona_solenoides()
{
	/* A máquina possui 3 solenoides, uma usada para sabão e alvejante,
	 * apenas num ciclo original de super lavagem, outra usada para
	 * sabão em todas lavagens, e a última usada para amaciante.
	 *
	 * Como fiz algo mais simples, com apenas um ciclo de lavar,
	 * liguei as solenoides 1 e 2 juntas (variável solenoide1_2).
	 *
	 * Além disso, se a função amaciante não for selecionada,
	 * todas solenoides são usadas para encher a máquina em todos
	 * os ciclos.
	 *
	 * Caso a função amaciante seja ativada, automaticamente também é
	 * adicionado um segundo enxague. Assim a máquina usa apenas as
	 * solenoides 1 e 2 para encher nos cilcos de lavagem e primeiro enxague,
	 * usando a solenoide 3 apenas no último enxague, para adicionar o amaciante.
	*/

	if (!usaAmaciante) 				//se nao usará amaciante, todas solenoides são ligadas para encher a máquina em todos ciclos
	{
		digitalWrite(solenoide1_2, HIGH); 	//liga TODAS solenoides
		digitalWrite(solenoide3, HIGH);
	}

	if (usaAmaciante && duploEnxague) 		//se for usar amaciante onao liga solenoide do amaciante no primeiro enxague
	{
		digitalWrite(solenoide1_2, HIGH); 	//liga solenoides 1 e 2
	}

	if (usaAmaciante && !duploEnxague)		//já no segundo enxague:
	{
		digitalWrite(solenoide3, HIGH);		//liga a solenoide 3 para jogar o amaciante
		digitalWrite(solenoide1_2, HIGH); 	//liga as solenoides 1 e 2 e mantém a 3 ligada
	}
} // end void

void encher_nivel_baixo() //função para encher a máquina no nível baixo
{
	setTime(0);		//zera o tempo
	unsigned long tempoErro = (12 * 60);	//tempo de erro em 12 min: se em 12 min não encher a máquina
												//há problema na solenoide ou na entrada de água

	while (!estadoChaveNivelB)		//enquanto o nível baixo não foi atingido
	{
		chaves_niveis();		//le o estado das chaves do pressostato
		seleciona_solenoides();		//liga solenoides
		delay(500);			//evitar bouncer

		if (now() > tempoErro)		//se passar do tempo de erro p/ o nivel atingir o nivel baixo
		{
			erro = true;		//indica erro
			tipoErro = 1;		//erro tipo 1, na entrada de água
			erros();		//entra na função de erro, parando a máquina e indicando o erro
		}
		if (digitalRead(chaveStart) == LOW) 	//para avancar a funcao
		{
			delay(1000);
			bip(2, 150);
			digitalWrite(solenoide1_2, LOW);	//DESliga solenoide 1 e 2
			digitalWrite(solenoide3, LOW);		//DESliga solenoide 3
			break;
		}
	}

	if (estadoChaveNivelB) 				//quando atingir o nível baixo
	{
		digitalWrite(solenoide1_2, LOW);	//DESliga solenoide 1 e 2
		digitalWrite(solenoide3, LOW);		//DESliga solenoide 3
	}
}

void encher_nivel_medio()	//função para encher até o nível médio
{
	setTime(0);					//zera o tempo
	unsigned long tempoErro = (12 * 60);		//tempo de erro em 12 min

	while (!estadoChaveNivelM)			//enquanto não atingir o nível médio
	{
		chaves_niveis();			//le o estado das chaves do pressostato
		seleciona_solenoides();			//liga solenoides
		delay(500);				//evitar bouncer

		if ( (now() > tempoErro) && (!estadoChaveNivelB) ) //se passar do tempo de erro p/ o nivel atingir o nivel baixo
		{
			erro = true;		//indica erro
			tipoErro = 1;		//erro tipo 1, na entrada de água
			erros();		//entra na função de erro, parando a máquina e indicando o erro
		}

		if (digitalRead(chaveStart) == LOW) 	//para avancar a funcao
		{
			delay(1000);
			bip(2, 150);
			digitalWrite(solenoide1_2, LOW);	//DESliga solenoide 1 e 2
			digitalWrite(solenoide3, LOW);		//DESliga solenoide 3
			break;
		}

	}
	if (estadoChaveNivelM)				//quando atingir o nível médio
	{
		digitalWrite(solenoide1_2, LOW);	//DESliga solenoide 1 e 2
		digitalWrite(solenoide3, LOW);		//DESliga solenoide 3
	}
}

void encher() //função geral para encher, que faz uso das funções encher_nivel_baixo() e encher_nivel_medio()
{
	switch (seletorNivel)	//identifica qual nivel está selecionado
	{
	case 1:		//Nivel baixo
		encher_nivel_baixo();  //chama a função para encher
		break;

	case 2:		//Nivel medio
		encher_nivel_medio();  //chama a função para encher
		break;
	} //end switch
} //end void

void bater(unsigned long tempo_min, int timeOn, int timeOff)
{
	/* Quando o motor está girando para um lado e logo em seguida é
 	 * acionado para o outro lado, há um pico de corrente, aquele que
 	 * faz as luzes incandescentes ou halógenas ficarem diminuindo
 	 * a luminosidade, ficarem "piscando" enquanto a máquina está
 	 * batendo a roupa.
 	 *
 	 * Isso não acontecia nas máquinas antigas pois o motor girava
 	 * apenas para um lado e uma engrenagem ou qualquer sistema mecânico
 	 * parecido fazia o agitador da máquina girar para um lado e para o
 	 * outro. Há algumas máquinas que usam esse sistema ainda, como as
 	 * chamadas "tanquinhos". Já nas modernas o acionamento do agitador
 	 * é feito através de TRIACs, jogando tensão no fio que faz o motor
 	 * girar para um lado, depois no fio que faz o motor girar para o outro.
	 *
	 * Por esse motivo, deve existir um delay entre quando o motor para
	 * para de girar para um lado e começa a girar para o outro, justamente
	 * para diminuir esses picos de corrente, que gera queda de tensão e distorções
	 * sobre a rede. Eu tentei diminuir esse efeito usando o delay, optoacopladores
	 * com zero crossing e também um filtro snubber na saída da placa, MAS mesmo assim
	 * é perceptível a queda de tenão, basta ligar ua lâmpada incandescente ou halógena
	 * (ou um multímetro) na mesma fase da máquina para perceber.
	 *
	 * Na prática esse delay não é percebido visualmente, pois, além de
	 * ser pequeno, a inércia do motor não deixa ele efetivamente parar
	 * antes de começar a girar para o outro lado.
	 *
	 * Na prática vi que o ideal para uma agitação forte é
	 * o motor girar de 300 a 400 ms, com um delay de 200 ms.
	 * Por isso nas funções de lavar ou enxaguar usei essea função com a sintaxe:
	 * Bater(minutos, 300, 200);
	 *
	 * Já nas operações como bater antes do molho e bater no segundo enxague, quando
	 * é adicionado o amaciante, usei uma agitação um pouco mais lenta:
	 * Bater(minutos, 300, 300);
	 *
	 * Se o timeOn ficar acima de 400 ms, o giro é muito grande e começa embolar a roupa.
	 */

	tempoAvanco = tempo_min * 60; 			//multiplica por 60 para dar o tempo em segundos
	setTime(0);					//seta o tempo em 0 segundos

	while (now() <= tempoAvanco)			//enquanto o tempo atual (now()) não for maior que o tempo de Bater
	{
		digitalWrite(motorDir, HIGH);
		delay(timeOn);				//gira o motor para direita
		digitalWrite(motorDir, LOW);
		delay(timeOff);				//aguarda um tempo
		digitalWrite(motorEsq, HIGH);
		delay(timeOn);				//gira o motor para esquerda
		digitalWrite(motorEsq, LOW);
		delay(timeOff);				//aguarda um pouco

		if (digitalRead(chaveStart) == LOW) 	//para avancar a funcao
		{
			avancar();
		}
	}

	//quando o tempo de bater passar desliga o motor
	digitalWrite(motorEsq, LOW);
	digitalWrite(motorDir, LOW);
} //end void

void drenar()
{
	unsigned long tempoErro = (6* 60);	//tempo de erro em 6 min
	setTime(0);				//zera o tempo

	while (estadoChaveNivelB) 		//enquanto não desativar o nível baixo
	{
		chaves_niveis();
		digitalWrite(atuadorBomba, HIGH); 	//deixa a bomba ligada
		delay(500);				//evitar bouncer

		if (now() > tempoErro)		//se passar de 6 minutos
		{
			erro = true;		//indica erro na bomba
			tipoErro = 2;
			erros();
		}

		if (digitalRead(chaveStart) == LOW) 	//para avancar a funcao
		{
			delay(1000);
			bip(2, 150);
			digitalWrite(atuadorBomba, LOW);	//DESliga bomba
			break;
		}
	}


	if (!estadoChaveNivelB) 	//se desativar a chave de nível baixo
	{
		/*
		 * As chaves do pressostato não são instantâneas,
		 * quanto a máquina está enchendo, ao atingir
		 * o nível baixo, a chave é ativada, mas ela
		 * só é desativada quando o tambor da máquina
		 * está quase vazio.
		 *
		 * Portanto, após a chave do nivel baixo ser desativada
		 * é preciso poucos segundos de drenagem para secar o
		 * tambor da máquina. Esse tempo é setado na variável
		 * abaixo em 30 segundos.
		 */
		tempoAvanco = 30; 		//tempo de drenagem, em seg, após desligar a chave do nivel baixo
		setTime(0);			//zera o tempo

		while (now() <= tempoAvanco)	//enquanto o tempo atual (now()) não for maior que o tempo
		{
			delay(500);				//evitar bouncer
			digitalWrite(atuadorBomba, HIGH);	//liga a bomba
		}
		digitalWrite(atuadorBomba, LOW);		//desliga a bomba quando o tempo passar
	}
} //end void

void molho(unsigned long tempo_min) //função para deixar máquina parada com as roupas de molho
{
	tempoAvanco = tempo_min * 60; 	//multiplica por 60 para dar o tempo em segundos
	setTime(0);			//seta o tempo em 0 segundos

	while (now() <= tempoAvanco)
	{
		digitalWrite(motorDir, LOW);		//DESLIGA equipamentos
		digitalWrite(motorEsq, LOW);
		digitalWrite(atuadorBomba, LOW);
		digitalWrite(solenoide1_2, LOW);
		digitalWrite(solenoide3, LOW);
		delay(1000);

		if (digitalRead(chaveStart) == LOW)	//para avancar a funcao
		{
			avancar();
		}
	} //end while
} //end void

void centrifugar(unsigned long tempo_min, int sprint)
{
	/* Quando máquina centrifuga, ela inicialmente da alguns sprints,
	 * ou seja: liga por um tempo, desligava por outro tempo, e assim por diante,
	 *  dando uns 4 ou 5 sprints até ligar o motor e não desligar mais até o final da centrifugação.
	 *
	 * Provavelmente esses sprints servem, pelo que descobri com testes práticos,
	 * para distribuir melhor a roupa dentro do cesto e evitar vibração.
	 *
	 * Assim é possível escolher usando a função:
	 * Centrifugar(tempo em minutos, número de sprints iniciais)
	 */

	tempoAvanco = tempo_min * 60; 		//multiplica por 60 para dar o tempo em minutos
	digitalWrite(atuadorBomba, HIGH); 	//liga atuadorBomba para acoplar o tambor com o motor
	delay(5000);				//aguarda 5 seg

	setTime(0);				//seta o tempo em 0 segundos


	int tempoOn = 4000;			//tempo ligado
	int tempoOff = 4000;			//tempo desligado

	// o for abaixo executa os sprints
	for (int x = 0; x < sprint; x++)	//executa o numero de sprints
	{
		digitalWrite(motorDir, HIGH);	//liga motor
		delay(tempoOn);
		digitalWrite(motorDir, LOW);	//desliga motor
		delay(tempoOff);

		tempoOn -= 700;			//diminui 0,7s de tempoOn a cada sprint, pois com a inércia
						//do tambor já girando é preciso menos força para manter a rotação
						// e evitar vibração.
		if (tempoOn <= 1500)		//tempoOn mínimo = 1,5 seg
			tempoOn = 1500;

		if (digitalRead(chaveStart) == LOW) 	//para avancar a funcao
		{
			bip(2, 150);
			break;
		}
	}		//end for

	// depois dos sprints o motor gira em modo contínuo até acabar o tempo
	while (now() <= tempoAvanco)			//enquanto o tempo nao for atingido
	{
		digitalWrite(motorDir, HIGH);		//mantem o motor girando

		if (digitalRead(chaveStart) == LOW) 	//para avancar a funcao
		{
			avancar();
		}
	} //end while

	//quando acabar o tempo de Centrifugar
	digitalWrite(motorDir, LOW);		//desliga o motor
	delay(10000); 				//aguarda 10 segundos
	digitalWrite(atuadorBomba, LOW);	//desliga o atuador/bomba

} //end void

void lavar_programa()
{
	//aciona o LED inicando o programa
	led_lavar();

	//Executa as funcoes
	delay(tempoDelayEntreEtapas);
	encher();				//Encher
	delay(tempoDelayEntreEtapas);
	bip();

	bater(4, 300, 300); 			//bater por 4 min, com giro lento
	bip();

	molho(30);				// Molho por 30 min
	bip();

	bater(15, 300, 200);			//Bater por 15 min, com giro rapido
	delay(tempoDelayEntreEtapas);
	bip();

	drenar();				//drenar
	delay(tempoDelayEntreEtapas);
	bip();

	centrifugar(3, 6);			//centrigufar por 3 min com 6 sprints iniciais
} //end void

void enxaguar_programa()
{
	//aciona o LED inicando o programa
	led_exaguar();

	//Executa as funcoes
	if (!usaAmaciante)			//se o botao amaciante NAO foi pressionado há apenas um enxague
	{
		delay(tempoDelayEntreEtapas);
		encher();			//Enche usando todas solenoides, selecionadas atravez da funcao Encher()
		bip();
		delay(tempoDelayEntreEtapas);

		bater(7, 300, 200);		//Bater por 7 min
		bip();
		delay(tempoDelayEntreEtapas);

		drenar();
		//não tem Centrifugar() pois o programa faz o ciclo ir automaticamente para
		//a centrifugação -- ver Programas()
	}

	if (usaAmaciante && duploEnxague) 	//se usar a funcao amaciante, ela adiciona enxague extra
	{
		delay(tempoDelayEntreEtapas);
		encher(); 			//Encher a primeira vez sem usar a solenoide do amaciante: solenoide3
		bip();
		delay(tempoDelayEntreEtapas);

		bater(5, 300, 200);		//Bater por 5 min
		bip();
		delay(tempoDelayEntreEtapas);

		drenar();
		bip();
		delay(tempoDelayEntreEtapas);

		centrifugar(2, 5);		//centrifugar primeiro enxague

		duploEnxague = false; 		//seta a variavel como false para o ultimo enxague
	}

	if (usaAmaciante && !duploEnxague) 	//ultimo enxague
	{
		delay(tempoDelayEntreEtapas);
		encher(); 			//Encher para amaciar usando primeiro a solenoide do amaciante
		bip();
		delay(tempoDelayEntreEtapas);

		bater(2, 300, 300);		//Bater por 3 min
		bip();

		molho(5);
		bip();

		bater(2, 300,200);
		bip();
		delay(tempoDelayEntreEtapas);

		drenar();
		//não tem Centrifugar() pois o programa faz o ciclo ir automaticamente para
		//a centrifugação --> ver executar_programas()
	}
} //end void

void centrifugar_programa()
{
	//aciona o LED inicando o programa
	led_centrifugar();

	//Executa a funcao
	if (estadoChaveNivelB) 		// se a máquina foi deixada parada e está com água
	{
		drenar();		// ela primeiro drena.
	}
	centrifugar(4, 6);		//centrifugar por 4 min
} //end void

void erros()
{
	//Se algum erro acontecer o programa entra em loop infinito
	//e fica indicando o erro no painel.
	//Para voltar a usar a máquina é preciso desligar e ligar novamente
	//através da chave física no painel.

	while (erro)	//se algum erro acontecer
	{
		//desliga todos equipamentos
		digitalWrite(motorDir, LOW);
		digitalWrite(motorEsq, LOW);
		digitalWrite(atuadorBomba, LOW);
		digitalWrite(solenoide1_2, LOW);
		digitalWrite(solenoide3, LOW);

		if (tipoErro == 1)	//erro nas solenoides ou entrada de água
		{
			// apita e pisca as luzes dos niveis
			bip();
			digitalWrite(ledNivelB, HIGH);
			digitalWrite(ledNivelM, HIGH);
			delay(1000);


			digitalWrite(ledNivelB, LOW);
			digitalWrite(ledNivelM, LOW);
			delay(1000);
		}

		if (tipoErro == 2)	//erro na bomba
		{
			// apita e pisca as luzes dos programas
			bip();
			digitalWrite(ledCentrifugar, HIGH);
			digitalWrite(ledEnxaguar, HIGH);
			digitalWrite(ledLavar, HIGH);
			delay(1000);


			digitalWrite(ledCentrifugar, LOW);
			digitalWrite(ledEnxaguar, LOW);
			digitalWrite(ledLavar, LOW);
			delay(1000);
		}
	} //end while infinito
} //end void

void executar_programas()
{

	switch (seletorPrograma)			//verifica o valor da variavel
	{
	case 1: //Programa em Lavar
		while (botaoStart) 			//enquanto botaoStart == true, ou seja, quando o botao é pressionado
		{
			lavar_programa();		//executa o programa de lavar
			bip(1, 300);
			enxaguar_programa();		//na sequencia o programa de enxaguar
			bip(1, 300);
			centrifugar_programa();		//por ultimo o programa de centrifugar
			bip(3, 2000);			//aviso sonoro indicando final
			digitalWrite(ledOn, LOW);	//desliga o LED on

			botaoStart = false;		//coloca a variavel como falsa, para o arduino nao entrar em loop
							//e ficar executando o programa novamente quando chegar ao fim
		}
		break; //nao faz mais nada

	case 2:	//Programa em Enxaguar
		while (botaoStart)
		{
			enxaguar_programa();		//inicia já no programa de enxaguar
			bip(1, 300);
			centrifugar_programa();
			bip(3, 2000);
			digitalWrite(ledOn, LOW);

			botaoStart = false;
		}
		break;

	case 3:		//Programa em Centrifugar
		while (botaoStart)
		{
			centrifugar_programa();		//inicia já em centrifugar
			bip(3, 2000);
			digitalWrite(ledOn, LOW);

			botaoStart = false;
		}
		break;
	} //end switch
} //end void

void loop()
{

	botao_sart();		//chama a funcao que ativa o botao Start
	selecao_nivel();	//chama a funcao que seleciona o nivel
	selecao_amaciante();	//chama a funcao que seleciona ou nao amaciante (e enxague extra)
	selecao_programa();	//chama funcao que seleciona o programa
	chaves_niveis();	//lê as chaves de níveis do pressostato
	executar_programas();	//função que executa os programas selecionados.


} //end loop