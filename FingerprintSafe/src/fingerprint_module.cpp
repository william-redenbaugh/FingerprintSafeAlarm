#include "fingerprint_module.h"

/*!
*   @brief Instance of the adafruit fingerprint sensor module
*/
static Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);

/*!
*   @brief Function declarations
*/
void start_fingerprint_module(void); 
uint8_t get_fingerprint_enroll(uint8_t id);
static inline void setup_fingerprint_sensor(void); 
static void enroll_finger(int id_val); 
void fingerprint_thread(void *parameters); 

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);

/*!
*   @brief Starts the thread that handles all the fingerprint stuff. 
*/
void start_fingerprint_module(void){
    // Setup thread for dealing with fingerprint stuff. 
    os_add_thread(&fingerprint_thread, NULL, -1, NULL); 
}

/*!
*   @brief Fingerprint Enrollment Code
*   @return Finger ID number
*/
uint8_t get_fingerprint_enroll(uint8_t id){
    int p = -1;
    lcd.setCursor(0, 0);
    lcd.print("Tap Finger: ");

    while (p != FINGERPRINT_OK) {
        p = finger.getImage();
        // So that other activities get time to run
        os_thread_sleep_ms(35);
    }

    lcd.setCursor(0, 1);
    // OK success!
    p = finger.image2Tz(1);
    switch (p) {
    case FINGERPRINT_OK:
        lcd.print("Conv Success"); 
        break;
    case FINGERPRINT_IMAGEMESS:
        lcd.print("Image Messy");
        return p;
    case FINGERPRINT_PACKETRECIEVEERR:
        lcd.print("COM Error");
        return p;
    case FINGERPRINT_FEATUREFAIL:
        lcd.print("Features Fail");
        return p;
    case FINGERPRINT_INVALIDIMAGE:
        lcd.print("Invalid Image");
        return p;
    default:
        lcd.print("Unknown error");
        return p;
    }

    lcd.setCursor(0, 2);
    lcd.print("Remove Finger");
    os_thread_sleep_ms(2000);

    p = 0;
    while (p != FINGERPRINT_NOFINGER) {
        p = finger.getImage();
        os_thread_sleep_ms(35);
    }
    lcd.clear(); 

    lcd.setCursor(0, 0);
    p = -1;
    lcd.print("Tap Finger Again");

    while (p != FINGERPRINT_OK) {
        p = finger.getImage();
    }

    lcd.setCursor(0, 1);
    // OK success!
    p = finger.image2Tz(2);
    switch (p) {
    case FINGERPRINT_OK:
        lcd.print("Image converted");
        break;
    case FINGERPRINT_IMAGEMESS:
        lcd.print("Image messy");
        return p;
    case FINGERPRINT_PACKETRECIEVEERR:
        lcd.print("COM err");
        return p;
    case FINGERPRINT_FEATUREFAIL:
        lcd.print("Feature Fail");
        return p;
    case FINGERPRINT_INVALIDIMAGE:
        lcd.print("Invalid Image");
        return p;
    default:
        lcd.print("Unknown error");
        return p;
    }

    lcd.setCursor(0, 2);
    // OK converted!
    lcd.print("Saving Finger...");  lcd.print(id);

    os_thread_delay_s(2);
    lcd.clear(); 
    lcd.setCursor(0, 0);

    p = finger.createModel();
    if (p == FINGERPRINT_OK) {
        lcd.print("Finger Match!");
    } 
    else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        lcd.print("COM err");
        return p;
    } 
    else if (p == FINGERPRINT_ENROLLMISMATCH) {
        lcd.print("Finger NoMatch");
        return p;
    }
    else {
        lcd.print("Unknown error");
        return p;
    }

    lcd.setCursor(0, 1);
    lcd.print("ID: "); lcd.print(id);
    p = finger.storeModel(id);

    lcd.setCursor(0, 2);
    if (p == FINGERPRINT_OK){
        lcd.print("Stored!");
    }
    else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        lcd.print("COM err");
        return p;
    } 
    else if (p == FINGERPRINT_BADLOCATION) {
        lcd.print("Invalid Location");
        return p;
    } 
    else if (p == FINGERPRINT_FLASHERR) {
        lcd.print("Flash Err");
        return p;
    } 
    else {
        lcd.print("Unknown err");
        return p;
    }

    lcd.setCursor(0, 3);
    lcd.print("Scan Success!"); 

    os_thread_delay_s(1);
    return true;
}

/*!
*   @brief Starts up initializing the fingerprint sensor. 
*/
static inline void setup_fingerprint_sensor(void){
    // set the data rate for the sensor lcd port
    finger.begin(57600);

    lcd.setCursor(0, 0);
    lcd.print("Finger sensor setup:"); 
    lcd.setCursor(0, 1);
    if (finger.verifyPassword()) 
        lcd.print("Found Sensor!");
    else {
        lcd.print("Sensor Not Found");
        while (1)
            os_thread_sleep_ms(1000);
    }
    finger.getParameters();
}

/*!
*   @brief Calls the command to enroll a finger
*/
static void enroll_finger(int id_val){
    while (!get_fingerprint_enroll(id_val))
        os_thread_sleep_ms(100);
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK)  return -1;

    p = finger.image2Tz();
    if (p != FINGERPRINT_OK)  return -1;

    p = finger.fingerFastSearch();
    if (p != FINGERPRINT_OK)  return -1;

    // We need to have a reasonable amount of confidence in the match!
    if(finger.confidence <= 85)
        return -1; 
    // found a match!
    lcd.clear(); 
    lcd.setCursor(0, 0); 
    lcd.print("Found ID #"); lcd.print(finger.fingerID);
    lcd.setCursor(0, 1);
    lcd.print("Confidence: "); lcd.print(finger.confidence);
    return finger.fingerID;
}

/*!
*   @brief Thread that handles all of our fingerprint stuff.  
*/
void fingerprint_thread(void *parameters){
    // initialize the LCD
    lcd.begin();
    // Turn on the blacklight and print a message.
    lcd.backlight();

    setup_fingerprint_sensor(); 
    os_thread_sleep_ms(1000); 

    lcd.clear(); 
    
    pinMode(PB0, INPUT_PULLUP); 
    pinMode(PD2, INPUT_PULLUP); 
    
    for(;;){
        if((digitalRead(PD2) == 0)){
            lcd.setBacklight(true); 
            lcd.setCursor(0, 0); 
            lcd.print("System Scanning"); 
            // Wait for finger match
            while(getFingerprintIDez() == -1){}

            lcd.setCursor(0, 2); 
            digitalWrite(PB1, HIGH); 
            lcd.print("Safe Unlocked!"); 

            int n = 0; 
            while(digitalRead(PB0) == 0 && (n < 50)){
                os_thread_sleep_ms(100);
                n++; 
            }

            digitalWrite(PB1, LOW); 
            if(digitalRead(PB0) == 0){
                lcd.setCursor(0, 2); 
                lcd.print("Safe Timed Out"); 
                os_thread_sleep_ms(1000);
                lcd.clear(); 
                goto scanning; 
            }

            while(digitalRead(PB0) == 1){
                // If we want to enroll, we can 
                if(digitalRead(PD2) == 0)
                    get_fingerprint_enroll(16);
                
                os_thread_sleep_ms(100);
            }
            
            lcd.clear(); 
            lcd.setCursor(0, 0); 
            lcd.print("Safe re-locked");
            os_thread_sleep_ms(1500); 
            lcd.clear(); 
        
        }
        else{
            lcd.setBacklight(false); 
        }
        scanning:
        os_thread_sleep_ms(50);
    }
}