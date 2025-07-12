#include "mbed.h"
#include "arm_book_lib.h"

// Inputs
DigitalIn enterButton(BUTTON1);
DigitalIn gasDetector(D2);
DigitalIn overTempDetector(D3);
DigitalIn aButton(D4);
DigitalIn bButton(D5);
DigitalIn cButton(D6);
DigitalIn dButton(D7);  // reserved

// Outputs
DigitalOut alarmLed(LED1);
DigitalOut systemBlockedLed(LED2);
DigitalOut incorrectCodeLed(LED3);

// State & counters
bool emergencyMode          = false;
bool alarmState             = OFF;
int  numberOfIncorrectCodes = 0;

// Timing
Timer flashTimer;

int main() {
    // Configure pull-downs
    enterButton.mode(PullDown);
    gasDetector.mode(PullDown);
    overTempDetector.mode(PullDown);
    aButton.mode(PullDown);
    bButton.mode(PullDown);
    cButton.mode(PullDown);
    dButton.mode(PullDown);

    // LEDs off initially
    alarmLed = OFF;
    systemBlockedLed = OFF;
    incorrectCodeLed = OFF;

    // Start flash timer
    flashTimer.start();

    while (true) {
        bool gas  = gasDetector.read();
        bool temp = overTempDetector.read();

        // 1) Normal alarm when gas OR temp
        if (!emergencyMode && (gas || temp)) {
            alarmState = ON;
        }
        alarmLed = alarmState;

        // 2) Enter emergency if BOTH sensors active
        if (!emergencyMode && gas && temp) {
            emergencyMode = true;
            flashTimer.reset();
        }

        // 3) Emergency mode handling
        if (emergencyMode) {
            // 3a) Simple flash every 100 ms
            if (flashTimer.elapsed_time().count() >= 500000) {  // 100 000 µs = 100 ms
                alarmState = !alarmState;
                flashTimer.reset();
            }

            // 3b) Code entry while not yet locked
            if (numberOfIncorrectCodes < 4) {
                // Use provided code-entry snippet
                if (enterButton.read() && !incorrectCodeLed.read() && alarmState) {
                    if (aButton && bButton && !cButton && !dButton) {
                        alarmState = OFF;
                        emergencyMode = false;
                        alarmLed = OFF;
                        numberOfIncorrectCodes = 0;
                    } else {
                        incorrectCodeLed = ON;
                        numberOfIncorrectCodes++;
                        thread_sleep_for(500);   // brief indication
                        incorrectCodeLed = OFF;
                    }
                }
            }
            // 3c) Lockout after 4 wrong attempts
            else {
                systemBlockedLed = ON;
                thread_sleep_for(20000);    // 20 seconds lock delay
                systemBlockedLed = OFF;
                numberOfIncorrectCodes = 0; // reset counter
            }
        }
    }
}
