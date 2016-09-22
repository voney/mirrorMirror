#include <FastPatterns.h>

FastPatterns::FastPatterns(int Numleds){
	numLeds = Numleds;
	ledArray = new CRGB[numLeds];
	totalPatterns = MAXMEMBER;
	activePattern = RAINBOW_CYCLE;
	//direction = FORWARD;
	backwards = false;
}

void FastPatterns::DefaultOnComplete(){
	switch (activePattern){
		case RAINBOW_CYCLE:
			break;
		case THEATER_CHASE:
			break;
		case SCANNER:
			break;
		case FADE:
			colourA = colourB;
			colourB = CRGB(random8(),random8(),random8());
			break;
		case TWINKLE:
			break;
		default:
			break;
	}
}

void FastPatterns::Update(){
	interval = (baseSpeed*patternSpeed)/userSpeed;
	if ((currentMillis - lastUpdate) > (interval)){ 
		lastUpdate = currentMillis;
		switch (activePattern){
			case RAINBOW_CYCLE:
				RainbowCycleUpdate();
				break;
			case THEATER_CHASE:
				TheaterChaseUpdate();
				break;
			case SCANNER:
				ScannerUpdate();
				break;
			case FADE:
				FadeUpdate();
				break;
			case TWINKLE:
				TwinkleUpdate();
				break;
			default:
				break;
		}
	}
}

void FastPatterns::SwitchPattern(int pID){
	switch (pID){
		case RAINBOW_CYCLE:
			RainbowCycle();
			break;
		case THEATER_CHASE:
			TheaterChase();
			break;
		case SCANNER:
			Scanner();
			break;
		case FADE:
			Fade();
			break;
		case TWINKLE:
			Twinkle();
			break;
		default:
			break;
	}
}

void FastPatterns::Increment(){
	if (!backwards){
		currentStep++;
		if (currentStep >= totalSteps) {
			currentStep = 0;
			DefaultOnComplete();
		}
	} else {
		--currentStep;
		if (currentStep <= 0) {
			currentStep = totalSteps - 1;
			DefaultOnComplete();
		}
	}
}

void FastPatterns::Reverse(){
	if (!backwards){
		//direction = REVERSE;
		backwards = true;
		currentStep = totalSteps - 1;
	}
	else {
		//direction = FORWARD;
		backwards = false;
		currentStep = 0;
	}
}

void FastPatterns::RainbowCycle(){
	Serial.println("FastPatterns: Initialising Pattern: Rainbow Cycle");
	activePattern = RAINBOW_CYCLE;
	totalSteps = 255;
	patternSpeed = 1;
	currentStep = 0;
}

void FastPatterns::RainbowCycleUpdate(){
	fill_rainbow(ledArray, numLeds, currentStep, 255/numLeds );
	FastLED.show();
	Increment();
}

void FastPatterns::TheaterChase(){
	Serial.println("FastPatterns: Initialising Pattern: Theatre Chase");
	colourA = CRGB(random8(), random8(), random8());
	colourB = CRGB(random8(), random8(), random8());
	activePattern = THEATER_CHASE;
	totalSteps = numLeds;
	patternSpeed = 15;
	currentStep = 0;
}

void FastPatterns::TheaterChaseUpdate(){
	for (int i = 0; i < numLeds; i++){
		if ((i + currentStep) % 3 == 0){
			ledArray[i] = colourA;
		}
		else {
			ledArray[i] = colourB;
		}
	}
	FastLED.show();
	Increment();
}

void FastPatterns::Scanner(){
	Serial.println("FastPatterns: Initialising Pattern: Cylon Scanner");
	activePattern = SCANNER;
	patternSpeed = 2;
	totalSteps = (numLeds - 1) * 2;
	colourA = CRGB::Red;
	currentStep = 0;
}

void FastPatterns::ScannerUpdate(){
	for (int i = 0; i < numLeds; i++){
		if (i == currentStep){  // Scan Pixel to the right
			ledArray[i] = colourA;
		} else if (i == totalSteps - currentStep){ // Scan Pixel to the left
			ledArray[i] = colourA;
		} else { // Fading tail		
			ledArray[i].fadeToBlackBy(100);
		}
	}
	FastLED.show();
	Increment();
}

void FastPatterns::Fade(){
	Serial.println("FastPatterns: Initialising Pattern: Colour Fade");
	colourA = CRGB(random8(), random8(), random8());
	colourB = CRGB(random8(), random8(), random8());
	activePattern = FADE;
	patternSpeed = 1;
	totalSteps = 255;
	currentStep = 0;
}

void FastPatterns::FadeUpdate(){
	// Calculate linear interpolation between colourA and colourB
	uint8_t newRed = ((colourA.red * (totalSteps - currentStep)) + (colourB.red * currentStep)) / totalSteps;
	uint8_t newGreen = ((colourA.green * (totalSteps - currentStep)) + (colourB.green * currentStep)) / totalSteps;
	uint8_t newBlue = ((colourA.blue * (totalSteps - currentStep)) + (colourB.blue * currentStep)) / totalSteps;

	fill_solid(ledArray, numLeds, CRGB(newRed, newGreen, newBlue));
	FastLED.show();
	Increment();
}

void FastPatterns::Twinkle(){
	Serial.println("FastPatterns: Initialising Pattern: Twinkle");
	activePattern = TWINKLE;
	patternSpeed = 2;
	totalSteps = 32;
	colourA = CRGB::White;
	currentStep = 0;
}

void FastPatterns::TwinkleUpdate(){
	int chance = random(50);
		if (chance < 6) {
			ledArray[random(numLeds-1)] = colourA;
		}
		for (int i = 0; i < numLeds; i++) {
			ledArray[i].fadeToBlackBy(10);
		}
	FastLED.show();
	Increment();
}