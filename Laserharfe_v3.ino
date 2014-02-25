#include <WaveHC.h>
#include <WaveUtil.h>

SdReader card;
FatVolume vol;
FatReader root;
FatReader file;
WaveHC wave;

short currentPlaying = -1;
boolean laserPin = 40;
int sample = 0;

int value[8];
int valueOn[8];
int valueOff[8];

int trackValues[30];
int trackDucklings[] = {
    7,6,5,4,3,3,2,2,2,2,3,2,2,2,2,3,4,4,4,4,5,5,6,6,6,6,7};
int trackSaufn[] = {
    6,4,7,3,7,6,7,6,3};
int trackDead[] = {
    7,7,5,3,5,7,7};
int trackTeacher[] = {
    5,3,0,5,7,5,3,0,5,7};
int trackCredits[] = {
    7,6,5,4,3,2,1,0,1,2,3,4,5,6,7};

///////////////////////////////////////////////////////////////
// SETUP - CALLED ONCE
///////////////////////////////////////////////////////////////

void setup() {
    Serial.begin(9600);
    debug("Arduino wurde gestartet.");

    pinMode(laserPin, OUTPUT);
    digitalWrite(laserPin, HIGH);

    if(!card.init()) error("Fehler beim Initialisieren (SD-Karte)!");
    debug("SD-Karte erfolgreich initialisiert.");

    card.partialBlockRead(true);

    if(!vol.init(card)) error("Fehler beim Initialisieren (Volume)!");
    debug("Volume erfolgreich initialisiert.");

    if(!root.openRoot(vol)) error("Wurzelverzeichnis konnte nicht geoffnet werden!");
    debug("Wurzelverzeichnis erfolgreich geoffnet.");

    if(!file.open(root, "intro.wav")) error("Datei konnte nicht geoffnet werden!");
    if(!wave.create(file)) error("Datei kann nicht abgespielt werden!");
    wave.play();

    calcThresholds();
    
    debug("Licht-Durchschnitt wurde bestimmt.");

    digitalWrite(laserPin, HIGH);
    debug("Setup abgeschlossen.");
    
    for(int i = 0; i < 30; i++)
        trackValues[i] = -1;

    while(wave.isplaying);
    debug("Intro beendet.");
}

///////////////////////////////////////////////////////////////
// MAIN LOOP - CALLED IN A LOOP
///////////////////////////////////////////////////////////////

void loop() {
    int newSound = -1;

    for(int i = 0; i < 8; i++) {
        value[i] = 1023 - analogRead(15 - i);
        //Serial.print(value[i], DEC);
        //Serial.print("\t");
    }
    //Serial.println();
    
    float maximum = -1;
    int maxLaser = -1;

    for(int i = 0; i < 8; i++) {
        int currDiff = valueOn[i] - value[i]; // 800 - 600
        int maxDiff = valueOn[i] - valueOff[i]; // 1000 - 600
        
        if(maxDiff == 0) {
            maxDiff = 1;
        }
        
        currDiff = min(currDiff, maxDiff);
        
        float val = (float) currDiff / (float) maxDiff;
        
        if(i < 3) {
        Serial.print(val, DEC);
        Serial.print("\t");
        
        Serial.print(currDiff, DEC);
        Serial.print("\t");
        
        Serial.print(maxDiff, DEC);
        Serial.print("\t");
        }
        
        if(val > maximum || maximum < 0) {
            maximum = val;
            maxLaser = i;  
        }
    }
    
    Serial.println();

    if(maxLaser == currentPlaying) {
        if(maximum >= .1f) {
            if(!wave.isplaying) {
                playFile(getWaveName(maxLaser), false);
            }
        } else {
            if(wave.isplaying)
                wave.stop();
            
            newSound = currentPlaying;
            currentPlaying = -1;
        }
    } else if(maximum >= .1f) {
        if(wave.isplaying)
            wave.stop();

        playFile(getWaveName(maxLaser), false);

        currentPlaying = maxLaser;
    } else {
        if(wave.isplaying)
           wave.stop();
           
        newSound = currentPlaying;
        currentPlaying = -1;
    }

    if(newSound != -1) {
        for(int i = 29; i > 0; i--)
            trackValues[i] = trackValues[i - 1];

        trackValues[0] = newSound;
    }
    
    checkTrack(trackDucklings, 27, "entn.wav");
    checkTrack(trackSaufn,     9,  "saufn.wav");
    checkTrack(trackDead,      7,  "dead.wav");
    checkTrack(trackCredits,   15, "credits.wav");
    
    for(int j = 0; j < sizeof(trackTeacher); j++) {
        if(trackTeacher[j] != trackValues[8 - j])
            break;

        if(j == 8) {
            sample = 3;
            //trackActivated = millis();
            
            for(int i = 0; i < sizeof(trackValues); i++)
                trackValues[i] = -1;
        }
    }

    sdErrorCheck();
}

///////////////////////////////////////////////////////////////
// PRINT A ERROR MESSAGE TO SERIAL AND EXIT
///////////////////////////////////////////////////////////////

void error(char *str) {
    Serial.print("[ERROR] ");
    Serial.println(str);
    while(1); // exit(1);
}

///////////////////////////////////////////////////////////////
// PRINT A DEBUG NOTICE TO SERIAL
///////////////////////////////////////////////////////////////

void debug(char *str) {
    Serial.print("[DEBUG] ");
    Serial.println(str);
}

///////////////////////////////////////////////////////////////
// CHECK SD CARD FOR ERRORS
///////////////////////////////////////////////////////////////

void sdErrorCheck() {
    if(!card.errorCode()) return;
    Serial.print("SD I/O-Error: ");
    Serial.print(card.errorCode(), HEX);
    Serial.print(", ");
    Serial.println(card.errorData(), HEX);
    while(1); // exit(1);
}

///////////////////////////////////////////////////////////////
// CREATE THE NAME OF THE WAVE FILE
///////////////////////////////////////////////////////////////

char* getWaveName(int i) {
    i = 7-i;
    char name[] = "xx.wav";
    name[0] = (char) (sample + 48); // 48 == 0
    name[1] = (char) (i      + 49); // 48 == 0
    return name;
}

///////////////////////////////////////////////////////////////
// PLAY A FILE
///////////////////////////////////////////////////////////////

void playFile(char *str, boolean blocking) {
    if(!file.open(root, str))
        error("Datei konnte nicht geoffnet werden!");
    
    if(!wave.create(file))
        error("Datei kann nicht abgespielt werden!");

    wave.play();
    
    if(blocking)
        while(wave.isplaying);
}

///////////////////////////////////////////////////////////////
// CHECK SPECIALS
///////////////////////////////////////////////////////////////

void checkTrack(int *in, int sizeOf, char *str) {    
    for(int j = 0; j < sizeOf; j++) {
        if(in[j] != trackValues[sizeOf - 1 - j])
            break;

        if(j == sizeOf - 1) {
            delay(1000);
            playFile(str, true);
            for(int i = 0; i < 30; i++)
                trackValues[i] = -1;
        }
    }
}

///////////////////////////////////////////////////////////////
// LASER SCHWELLWERT BESTIMMEN
///////////////////////////////////////////////////////////////

void calcThresholds() {
    digitalWrite(laserPin, LOW);
    delay(1000);
    
    for(int i = 0; i < 8; i++) {
        valueOff[i] = 1023 - analogRead(15 - i);
        Serial.print(valueOff[i], DEC);
        Serial.print("\t");
    }
    
    Serial.println();
    
    delay(1000);
    digitalWrite(laserPin, HIGH);
    delay(1000);
    
    for(int i = 0; i < 8; i++) {
        valueOn[i] = 1023 - analogRead(15 - i);
        Serial.print(valueOn[i], DEC);
        Serial.print("\t");
    }
    
    Serial.println();
    
    delay(1000);
    digitalWrite(laserPin, HIGH);
}
