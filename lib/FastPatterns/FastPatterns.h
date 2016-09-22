#pragma once
#include <FastLED.h>

extern unsigned long currentMillis;

class FastPatterns{
    private:
        int baseSpeed = 100;                // Base Speed for patterns in milliseconds.
        int patternSpeed;                   // Speed multiplier for the pattern, set this in your pattern initialiser.
        enum patternIDs {                   // The pattern IDs if you add another add it before TWINKLE
            RAINBOW_CYCLE, 
            THEATER_CHASE, 
            SCANNER, 
            FADE, 
            TWINKLE, 
            MAXMEMBER
        };
    
    public:
        bool backwards;
        String patternList[MAXMEMBER] = {   // List the names of the available patterns, make sure they stay in the same order as the IDs above
            "Rainbow Cycle", 
            "Theatre Chase", 
            "Cylon Scanner", 
            "Colour Fade", 
            "White Twinkles"
        }; 

        uint8_t activePattern;              // Index of the active pattern within the pattern table
        uint8_t totalPatterns;              // The total number of patterns available
        int numLeds;                        // The number of LEDs
        int interval;                       // Calculated interval
        uint8_t userSpeed = 5;              // Divider for the pattern speed, higher is faster.
                
        unsigned long lastUpdate;           // When the last Pattern update occurred in millis()
        

        CRGB colourA, colourB;              // Which colours are in use for two colour patterns
        uint16_t totalSteps;                // total number of steps in the pattern
        uint16_t currentStep;               // current step within the pattern                    
        CRGB* ledArray;                     // The array to define the LEDs
        CRGBPalette16 currentPalette;       // Current colour palette in use
        
        // typedef void(*completionCallBack)(void);
        // completionCallBack OnComplete;

        FastPatterns(int NumLeds);

        // A default completion function to use if you don't write your own.
        void DefaultOnComplete();

        // Update the pattern
        void Update();

        // change to a different pattern by it's number.
        void SwitchPattern(int pID);

        // Increment currentStep, reset at the end and call the completion function, counts either up or down depending on the value of direction.
        void Increment();

        // Reverse Pattern direction
        void Reverse();

        // Initializer and updater for Rainbow Cycle
        void RainbowCycle();
        void RainbowCycleUpdate();

        // Initializer and updater for Theatre Chase
        void TheaterChase();
        void TheaterChaseUpdate();

        // Initializer and updater for Scanner
        void Scanner();
        void ScannerUpdate();

        // Initializer and updater for Fade
        void Fade();
        void FadeUpdate();

        // Initializer and updater for Twinkle
        void Twinkle();
        void TwinkleUpdate();


};