#include <Arduino.h>
#include <Bounce2.h> // Debouncer library
#include <TimeLib.h> // Time library

/* Pin numbering was chosen to facilitate routing and wiring
 * from the Arduino Pro Mini to the other PCB components.
 */

const byte chaveAmaciante = 1; // Switch pin for softener function
const byte ledAmaciante = 6;   // Softener LED indicator pin

const byte ledOn = 7;       // LED indicating the machine is running
const byte chaveStart = A3; // Start/Stop sequence button

const byte motorDir = 8;     // Turn motor clockwise (right)
const byte motorEsq = 9;     // Turn motor counter-clockwise (left)
const byte atuadorBomba = 4; // Turn on brake actuator and drain pump
const byte solenoide1_2 = 3; // Turn on solenoids 1 and 2 (main water valves)
const byte solenoide3 = 2;   // Turn on solenoid 3 (softener valve)

const byte chavePrograma = A5; // Program selector switch input
const byte chaveNivel = A4;    // Water level selector switch input

const byte ledLavar = A2;       // Wash LED
const byte ledEnxaguar = A1;    // Rinse LED
const byte ledCentrifugar = A0; // Spin LED

const byte ledNivelM = 12; // Medium water level LED
const byte ledNivelB = 13; // Low water level LED

const byte chaveNivelB = 10; // Low level pressure switch input
const byte chaveNivelM = 11; // Medium level pressure switch input

const byte buzzer = 5; // Buzzer pin

byte seletorPrograma; // Program selection counter
byte seletorNivel;    // Water level selection counter

int tempoDelayEntreEtapas = 4000; // Delay between stages (ms)

bool usaAmaciante; // Softener mode status
bool duploEnxague; // Double rinse counter/flag
bool botaoStart;   // Start button status

bool estadoChaveNivelB; // Low water level switch state
bool estadoChaveNivelM; // Medium water level switch state

bool erro;     // Error flag
byte tipoErro; // Error code/type

unsigned long tempoAvanco; // Global time variable used for timing cycles

// Initialize Bounce2 debouncer objects:
Bounce programa = Bounce();  // Program selector switch
Bounce nivel = Bounce();     // Water level selector switch
Bounce start = Bounce();     // Start switch
Bounce amaciante = Bounce(); // Softener switch
/* The debouncer library prevents electrical contact noise
 * from disrupting button presses and function selection.
 */

void bip(int vezes = 1, int tempo = 150)
/* Emits beeps using a passive buzzer (without internal oscillator),
 * takes repetition count (default 1) and duration (default 150 ms).
 */
{
    for (int x = 0; x < vezes; x++) {
        tone(buzzer, 3000); // Set buzzer pin and frequency to 3000 Hz
        delay(tempo);       // Wait tone duration
        noTone(buzzer);     // Turn off buzzer
        delay(tempo);       // Wait interval between consecutive beeps
    }
}

void setup()
{
    pinMode(chaveAmaciante, INPUT_PULLUP); // Set pin as input with internal pull-up resistor

    pinMode(ledAmaciante, OUTPUT);   // Softener LED output
    digitalWrite(ledAmaciante, LOW); // Start turned off

    pinMode(ledOn, OUTPUT); // Machine running LED output

    pinMode(chaveStart, INPUT_PULLUP); // Start button input

    pinMode(motorDir, OUTPUT);   // Motor clockwise output
    digitalWrite(motorDir, LOW); // Start turned off

    pinMode(motorEsq, OUTPUT);   // Motor counter-clockwise output
    digitalWrite(motorEsq, LOW); // Start turned off

    pinMode(atuadorBomba, OUTPUT);   // Brake actuator & drain pump output
    digitalWrite(atuadorBomba, LOW); // Start turned off

    pinMode(solenoide1_2, OUTPUT);   // Solenoids 1 & 2 output
    digitalWrite(solenoide1_2, LOW); // Start turned off

    pinMode(solenoide3, OUTPUT);   // Solenoid 3 output
    digitalWrite(solenoide3, LOW); // Start turned off

    pinMode(chavePrograma, INPUT_PULLUP); // Program selector input

    pinMode(chaveNivel, INPUT_PULLUP); // Water level selector input

    pinMode(ledLavar, OUTPUT);   // Wash LED output
    digitalWrite(ledLavar, LOW); // Start turned off

    pinMode(ledEnxaguar, OUTPUT);   // Rinse LED output
    digitalWrite(ledEnxaguar, LOW); // Start turned off

    pinMode(ledCentrifugar, OUTPUT);   // Spin LED output
    digitalWrite(ledCentrifugar, LOW); // Start turned off

    pinMode(ledNivelM, OUTPUT);   // Medium level LED output
    digitalWrite(ledNivelM, LOW); // Start turned off

    pinMode(ledNivelB, OUTPUT);   // Low level LED output
    digitalWrite(ledNivelB, LOW); // Start turned off

    pinMode(chaveNivelM, INPUT_PULLUP); // Medium level pressure switch input

    pinMode(chaveNivelB, INPUT_PULLUP); // Low level pressure switch input

    pinMode(buzzer, OUTPUT);   // Buzzer output
    digitalWrite(buzzer, LOW); // Start turned off

    // Configure Bounce2 for Program selector
    programa.attach(chavePrograma); // Attach debouncer to pin
    programa.interval(100);         // Set debounce interval to 100 ms
    seletorPrograma = 2;            // Default program = 2 (Rinse)

    // Configure Bounce2 for Water Level selector
    nivel.attach(chaveNivel); // Attach debouncer to pin
    nivel.interval(100);      // Set debounce interval to 100 ms
    seletorNivel = 2;         // Default water level = 2 (Medium)

    // Configure Bounce2 for Start button
    start.attach(chaveStart); // Attach debouncer to pin
    start.interval(100);      // Set debounce interval to 100 ms
    botaoStart = false;       // Initial status is false (stopped)

    // Configure Bounce2 for Softener button
    amaciante.attach(chaveAmaciante); // Attach debouncer to pin
    amaciante.interval(100);          // Set debounce interval to 100 ms
    usaAmaciante = false;             // Softener status starts false
    duploEnxague = false;             // Double rinse starts false

    bip(3, 100); // 3 quick beeps indicating power-on initialization

} // end setup

void chaves_niveis()
{
    /* The pressure switch has 3 contacts:
     * LOW Level:    31-32 = NC (HIGH when active, with external resistor)
     * MEDIUM Level: 11-13 = NO (LOW when active, with external resistor)
     * HIGH Level:   21-23 = NO (LOW when active, with external resistor)
     *
     * HIGH LEVEL IS NOT CURRENTLY USED IN THIS PROGRAM
     *
     * Because the pressure switch mixes NC and NO contacts, this helper
     * maps raw levels to normalized boolean states (true = level reached).
     */

    if (digitalRead(chaveNivelB) == LOW) // Water level has not reached minimum
        estadoChaveNivelB = false;
    else if (digitalRead(chaveNivelB) == HIGH) // Minimum level reached
        estadoChaveNivelB = true;

    if (digitalRead(chaveNivelM) == HIGH) // Medium level not reached
        estadoChaveNivelM = false;
    else if (digitalRead(chaveNivelM) == LOW) // Medium level reached
        estadoChaveNivelM = true;

} // end chaves_niveis()

void led_lavar()
{
    // Activate Wash program LED
    digitalWrite(ledLavar, HIGH);
    digitalWrite(ledEnxaguar, LOW);
    digitalWrite(ledCentrifugar, LOW);
}

void led_exaguar()
{
    // Activate Rinse program LED
    digitalWrite(ledLavar, LOW);
    digitalWrite(ledEnxaguar, HIGH);
    digitalWrite(ledCentrifugar, LOW);
}

void led_centrifugar()
{
    // Activate Spin program LED
    digitalWrite(ledLavar, LOW);
    digitalWrite(ledEnxaguar, LOW);
    digitalWrite(ledCentrifugar, HIGH);
}

void selecao_programa()
{
    programa.update();   // Update debouncer state
    if (programa.fell()) // If button transitions from HIGH to LOW (pressed)
    {
        seletorPrograma++; // Increment program counter
        bip();
        if (seletorPrograma > 3) // Wrap around if exceeding 3
        {
            seletorPrograma = 1;
        }
    }

    switch (seletorPrograma) // Update corresponding program LED
    {
    case 1:
        // Wash program
        led_lavar();
        break;
    case 2:
        // Rinse program
        led_exaguar();
        break;
    case 3:
        // Spin program
        led_centrifugar();
        break;
    } // end switch
} // end selecao_programa()

void selecao_nivel()
{
    nivel.update();   // Update debouncer state
    if (nivel.fell()) // If button transitions from HIGH to LOW (pressed)
    {
        seletorNivel++; // Increment water level counter
        bip();
        if (seletorNivel > 2) // Wrap around if exceeding 2
        {
            seletorNivel = 1;
        }
    }

    switch (seletorNivel) // Update corresponding level LEDs
    {
    case 1: // Low level
        digitalWrite(ledNivelB, HIGH);
        digitalWrite(ledNivelM, LOW);
        break;
    case 2: // Medium level
        digitalWrite(ledNivelB, LOW);
        digitalWrite(ledNivelM, HIGH);
        break;
    } // end switch
} // end selecao_nivel()

void botao_sart()
{
    start.update();                  // Update debouncer state
    if (start.fell() && !botaoStart) // If button pressed and machine not running
    {
        digitalWrite(ledOn, HIGH); // Turn on running LED
        bip();
        botaoStart = true; // Set running status to true
    }
} // end botao_sart()

void selecao_amaciante()
{
    amaciante.update();                    // Update debouncer state
    if (amaciante.fell() && !usaAmaciante) // If pressed and softener mode was false
    {
        digitalWrite(ledAmaciante, HIGH); // Turn on softener LED
        bip();
        usaAmaciante = true; // Enable softener and valve sequencing
        duploEnxague = true; // Automatically add a double rinse cycle
    }

    else if (amaciante.fell() && usaAmaciante) // If pressed again while active
    {
        digitalWrite(ledAmaciante, LOW); // Turn off softener LED
        bip();
        usaAmaciante = false; // Disable softener and double rinse
        duploEnxague = false;
    }
} // end selecao_amaciante()

void avancar()
// Helper to skip stages (e.g. advance through soak cycle immediately)
{
    delay(1000);
    bip(2, 150);
    tempoAvanco = now(); // Force timer expiry so stage terminates
} // end avancar()

void seleciona_solenoides()
{
    /* The machine has 3 solenoid valves:
     * - Solenoid 1 & 2: Main wash & bleach water valves (grouped as solenoide1_2)
     * - Solenoid 3: Fabric softener compartment valve
     *
     * If softener mode is disabled, all solenoids are energized to fill faster.
     * If softener mode is enabled, only solenoids 1 & 2 are used for wash and
     * first rinse, and solenoid 3 is energized on the second rinse to dispense softener.
     */

    if (!usaAmaciante) // If softener not used, open all valves for faster fill
    {
        digitalWrite(solenoide1_2, HIGH); // Turn on all solenoids
        digitalWrite(solenoide3, HIGH);
    }

    if (usaAmaciante && duploEnxague) // First rinse: do not dispense softener
    {
        digitalWrite(solenoide1_2, HIGH); // Turn on solenoids 1 & 2 only
    }

    if (usaAmaciante && !duploEnxague) // Second/final rinse: dispense softener
    {
        digitalWrite(solenoide3, HIGH);   // Turn on softener solenoid
        digitalWrite(solenoide1_2, HIGH); // Also keep main solenoids open
    }
} // end seleciona_solenoides()

void encher_nivel_baixo() // Fill machine up to low water level
{
    setTime(0);                          // Reset timer
    unsigned long tempoErro = (12 * 60); // 12-minute timeout error threshold

    while (!estadoChaveNivelB) // While low level contact is not reached
    {
        chaves_niveis();        // Read pressure switch states
        seleciona_solenoides(); // Open appropriate solenoids
        delay(500);             // Debounce / poll delay

        if (now() > tempoErro) // Water inlet timeout reached
        {
            erro = true;  // Flag error
            tipoErro = 1; // Error type 1: water inlet failure
            erros();      // Enter error handler
        }
        if (digitalRead(chaveStart) == LOW) // Button press advances/skips stage
        {
            delay(1000);
            bip(2, 150);
            digitalWrite(solenoide1_2, LOW); // Close solenoids 1 & 2
            digitalWrite(solenoide3, LOW);   // Close solenoid 3
            break;
        }
    }

    if (estadoChaveNivelB) // When low level is reached
    {
        digitalWrite(solenoide1_2, LOW); // Close solenoids 1 & 2
        digitalWrite(solenoide3, LOW);   // Close solenoid 3
    }
}

void encher_nivel_medio() // Fill machine up to medium water level
{
    setTime(0);                          // Reset timer
    unsigned long tempoErro = (12 * 60); // 12-minute timeout error threshold

    while (!estadoChaveNivelM) // While medium level contact is not reached
    {
        chaves_niveis();        // Read pressure switch states
        seleciona_solenoides(); // Open appropriate solenoids
        delay(500);             // Debounce / poll delay

        if ((now() > tempoErro) && (!estadoChaveNivelB)) // Error if even low level is not reached in 12 min
        {
            erro = true;  // Flag error
            tipoErro = 1; // Error type 1: water inlet failure
            erros();      // Enter error handler
        }

        if (digitalRead(chaveStart) == LOW) // Button press advances/skips stage
        {
            delay(1000);
            bip(2, 150);
            digitalWrite(solenoide1_2, LOW); // Close solenoids 1 & 2
            digitalWrite(solenoide3, LOW);   // Close solenoid 3
            break;
        }
    }
    if (estadoChaveNivelM) // When medium level is reached
    {
        digitalWrite(solenoide1_2, LOW); // Close solenoids 1 & 2
        digitalWrite(solenoide3, LOW);   // Close solenoid 3
    }
}

void encher() // General fill function delegating to low or medium fill routines
{
    switch (seletorNivel) // Check selected water level
    {
    case 1:                   // Low level
        encher_nivel_baixo();
        break;

    case 2:                   // Medium level
        encher_nivel_medio();
        break;
    } // end switch
} // end encher()

void bater(unsigned long tempo_min, int timeOn, int timeOff)
{
    /* When the motor is rotating in one direction and abruptly reversed,
     * high inrush current spikes occur, causing line voltage dips.
     * To protect the TRIACs/relays and minimize mains disturbances,
     * a dead-time (timeOff) is enforced between direction changes.
     *
     * Empirical tuning showed:
     * - Strong agitation: 300 to 400 ms timeOn, 200 ms timeOff.
     * - Gentle agitation: 300 ms timeOn, 300 ms timeOff.
     * - timeOn above 400 ms causes clothes tangling.
     */

    tempoAvanco = tempo_min * 60; // Convert minutes to seconds
    setTime(0);                   // Reset timer to 0 seconds

    while (now() <= tempoAvanco) // While duration has not expired
    {
        digitalWrite(motorDir, HIGH);
        delay(timeOn); // Rotate motor clockwise (right)
        digitalWrite(motorDir, LOW);
        delay(timeOff); // Dead-time pause
        digitalWrite(motorEsq, HIGH);
        delay(timeOn); // Rotate motor counter-clockwise (left)
        digitalWrite(motorEsq, LOW);
        delay(timeOff); // Dead-time pause

        if (digitalRead(chaveStart) == LOW) // Button press advances/skips stage
        {
            avancar();
        }
    }

    // Turn off motor when agitation duration is reached
    digitalWrite(motorEsq, LOW);
    digitalWrite(motorDir, LOW);
} // end bater()

void drenar()
{
    unsigned long tempoErro = (6 * 60); // 6-minute drain timeout
    setTime(0);                         // Reset timer

    while (estadoChaveNivelB) // While low level pressure switch is still triggered
    {
        chaves_niveis();
        digitalWrite(atuadorBomba, HIGH); // Keep pump energized
        delay(500);                       // Debounce / poll delay

        if (now() > tempoErro) // Exceeded 6 minutes without draining
        {
            erro = true;  // Flag error
            tipoErro = 2; // Error type 2: drainage failure
            erros();
        }

        if (digitalRead(chaveStart) == LOW) // Button press advances/skips stage
        {
            delay(1000);
            bip(2, 150);
            digitalWrite(atuadorBomba, LOW); // Turn off pump
            break;
        }
    }

    if (!estadoChaveNivelB) // Once low level contact opens
    {
        /* Pressure switch contacts have hysteresis; once the low level
         * contact opens, a few additional seconds of pumping are needed
         * to completely empty the tub. This is set to 30 seconds below.
         */
        tempoAvanco = 30; // 30 seconds extra drain duration
        setTime(0);       // Reset timer

        while (now() <= tempoAvanco) // Keep draining for the extra period
        {
            delay(500);
            digitalWrite(atuadorBomba, HIGH); // Keep pump energized
        }
        digitalWrite(atuadorBomba, LOW); // Turn off pump when done
    }
} // end drenar()

void molho(unsigned long tempo_min) // Soak stage: keeps machine idle with water & clothes
{
    tempoAvanco = tempo_min * 60; // Convert minutes to seconds
    setTime(0);                   // Reset timer to 0

    while (now() <= tempoAvanco) {
        digitalWrite(motorDir, LOW); // Turn off all actuators
        digitalWrite(motorEsq, LOW);
        digitalWrite(atuadorBomba, LOW);
        digitalWrite(solenoide1_2, LOW);
        digitalWrite(solenoide3, LOW);
        delay(1000);

        if (digitalRead(chaveStart) == LOW) // Button press advances/skips stage
        {
            avancar();
        }
    } // end while
} // end molho()

void centrifugar(unsigned long tempo_min, int sprint)
{
    /* Spin stage initiates with short intermittent sprints to evenly distribute
     * clothes inside the drum before continuous high-speed spinning, preventing violent vibration.
     * Parameters: Centrifugar(duration_in_minutes, sprint_count).
     */

    tempoAvanco = tempo_min * 60;     // Convert minutes to seconds
    digitalWrite(atuadorBomba, HIGH); // Engage clutch actuator and drain pump
    delay(5000);                      // Wait 5 seconds for mechanical clutch engagement

    setTime(0); // Reset timer to 0 seconds

    int tempoOn = 4000;  // Initial sprint ON duration
    int tempoOff = 4000; // Sprint OFF duration

    // Execute spin sprints
    for (int x = 0; x < sprint; x++)
    {
        digitalWrite(motorDir, HIGH); // Energize motor
        delay(tempoOn);
        digitalWrite(motorDir, LOW); // De-energize motor
        delay(tempoOff);

        tempoOn -= 700; // Reduce ON time each sprint as drum builds inertia
        if (tempoOn <= 1500) // Minimum ON time clamp = 1.5 seconds
            tempoOn = 1500;

        if (digitalRead(chaveStart) == LOW) // Button press advances/skips stage
        {
            bip(2, 150);
            break;
        }
    } // end for

    // Continuous spin mode until timer expiry
    while (now() <= tempoAvanco)
    {
        digitalWrite(motorDir, HIGH); // Keep motor spinning

        if (digitalRead(chaveStart) == LOW) // Button press advances/skips stage
        {
            avancar();
        }
    } // end while

    // Spin stage termination
    digitalWrite(motorDir, LOW);     // Turn off motor
    delay(10000);                    // Wait 10 seconds for drum coast-down
    digitalWrite(atuadorBomba, LOW); // Disengage clutch actuator / pump

} // end centrifugar()

void lavar_programa()
{
    // Activate Wash indicator LED
    led_lavar();

    // Execute Wash cycle sequence
    delay(tempoDelayEntreEtapas);
    encher(); // Fill water
    delay(tempoDelayEntreEtapas);
    bip();

    bater(4, 300, 300); // Agitate 4 min, gentle mode
    bip();

    molho(30); // Soak 30 min
    bip();

    bater(15, 300, 200); // Agitate 15 min, normal mode
    delay(tempoDelayEntreEtapas);
    bip();

    drenar(); // Drain water
    delay(tempoDelayEntreEtapas);
    bip();

    centrifugar(3, 6); // Spin 3 min with 6 initial sprints
} // end lavar_programa()

void enxaguar_programa()
{
    // Activate Rinse indicator LED
    led_exaguar();

    // Execute Rinse cycle sequence
    if (!usaAmaciante) // Single rinse mode (no softener selected)
    {
        delay(tempoDelayEntreEtapas);
        encher(); // Fill using all solenoids
        bip();
        delay(tempoDelayEntreEtapas);

        bater(7, 300, 200); // Agitate 7 min
        bip();
        delay(tempoDelayEntreEtapas);

        drenar();
        // Spin is chained next in executar_programas()
    }

    if (usaAmaciante && duploEnxague) // Double rinse mode - First rinse
    {
        delay(tempoDelayEntreEtapas);
        encher(); // Fill without softener solenoid
        bip();
        delay(tempoDelayEntreEtapas);

        bater(5, 300, 200); // Agitate 5 min
        bip();
        delay(tempoDelayEntreEtapas);

        drenar();
        bip();
        delay(tempoDelayEntreEtapas);

        centrifugar(2, 5); // Spin intermediate rinse (2 min, 5 sprints)

        duploEnxague = false; // Flag ready for final rinse with softener
    }

    if (usaAmaciante && !duploEnxague) // Double rinse mode - Second/Final rinse with softener
    {
        delay(tempoDelayEntreEtapas);
        encher(); // Fill dispensing softener
        bip();
        delay(tempoDelayEntreEtapas);

        bater(2, 300, 300); // Agitate 2 min
        bip();

        molho(5); // Softener soak 5 min
        bip();

        bater(2, 300, 200); // Agitate 2 min
        bip();
        delay(tempoDelayEntreEtapas);

        drenar();
        // Spin is chained next in executar_programas()
    }
} // end enxaguar_programa()

void centrifugar_programa()
{
    // Activate Spin indicator LED
    led_centrifugar();

    // If machine was paused with water, drain first
    if (estadoChaveNivelB)
    {
        drenar();
    }
    centrifugar(4, 6); // Spin 4 min with 6 sprints
} // end centrifugar_programa()

void erros()
{
    // If an error occurs, halt all actuators and loop indefinitely
    // blinking the corresponding error code on panel LEDs.
    // Recovery requires power-cycling the machine.

    while (erro)
    {
        // Turn off all actuators
        digitalWrite(motorDir, LOW);
        digitalWrite(motorEsq, LOW);
        digitalWrite(atuadorBomba, LOW);
        digitalWrite(solenoide1_2, LOW);
        digitalWrite(solenoide3, LOW);

        if (tipoErro == 1) // Error Type 1: Water inlet / solenoid failure
        {
            // Beep and blink water level LEDs
            bip();
            digitalWrite(ledNivelB, HIGH);
            digitalWrite(ledNivelM, HIGH);
            delay(1000);

            digitalWrite(ledNivelB, LOW);
            digitalWrite(ledNivelM, LOW);
            delay(1000);
        }

        if (tipoErro == 2) // Error Type 2: Drain pump failure
        {
            // Beep and blink program LEDs
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
    } // end while
} // end erros()

void executar_programas()
{
    switch (seletorPrograma) // Check selected program
    {
    case 1:                // Wash Program
        while (botaoStart) // While running flag is true
        {
            lavar_programa(); // Run wash cycle
            bip(1, 300);
            enxaguar_programa(); // Follow with rinse cycle
            bip(1, 300);
            centrifugar_programa();   // Finish with spin cycle
            bip(3, 2000);             // Completion acoustic alert
            digitalWrite(ledOn, LOW); // Turn off running LED

            botaoStart = false; // Reset start flag to prevent re-triggering
        }
        break;

    case 2: // Rinse Program
        while (botaoStart) {
            enxaguar_programa(); // Start directly at rinse
            bip(1, 300);
            centrifugar_programa();
            bip(3, 2000);
            digitalWrite(ledOn, LOW);

            botaoStart = false;
        }
        break;

    case 3: // Spin Program
        while (botaoStart) {
            centrifugar_programa(); // Start directly at spin
            bip(3, 2000);
            digitalWrite(ledOn, LOW);

            botaoStart = false;
        }
        break;
    } // end switch
} // end executar_programas()

void loop()
{
    botao_sart();         // Check start button
    selecao_nivel();      // Check water level selection
    selecao_amaciante();  // Check softener selection
    selecao_programa();   // Check program selection
    chaves_niveis();      // Read pressure switch levels
    executar_programas(); // Execute active cycle if started

} // end loop