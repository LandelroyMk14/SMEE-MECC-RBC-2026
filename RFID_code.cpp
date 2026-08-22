/* INFO
RFID CODE DUMP - copy into arduino to test
Abandoned the vector idea to focus on reading info first
BLOCK 56 (COORDINATE INFO) v1 - straight from Gemini
BLOCK 57 (ANIMAL INFO) - yet to be implemented
*/

// CODE FOR COORDINATES (ONLY BLOCK 56)
    struct Block56Data {
    bool success;
    byte val3;   // Byte index 3 (0x03)
    byte val11;  // Byte index 11 (0x01)
    };

    /**
    * Reads Block 56 and extracts only the two highlighted byte values.
    */
    Block56Data readBlock56() {
    Block56Data data = {false, 0, 0};

    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        return data;
    }

    byte trailerBlock = 59; // Hardcoded trailer block for Sector 14
    byte rawBuffer[18];
    byte bufferSize = sizeof(rawBuffer);

    // Authenticate sector access
    if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid)) != MFRC522::STATUS_OK) {
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return data;
    }

    // Read Block 56
    MFRC522::StatusCode status = mfrc522.MIFARE_Read(56, rawBuffer, &bufferSize);

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    if (status != MFRC522::STATUS_OK) {
        return data;
    }

    // Extract only the highlighted byte positions
    data.val3  = rawBuffer[3];   // Pulls 0x03 (3)
    data.val11 = rawBuffer[11];  // Pulls 0x01 (1)
    data.success = true;

    return data;
    }

// HOW TO USE (ONLY BLOCK56)
    Block56Data result = readBlock56();

    if (result.success) {
    byte byte3Value  = result.val3;   // Holds 3
    byte byte11Value = result.val11;  // Holds 1
    }

// CODE IN IDE (ONLY BLOCK56)
    #include <SPI.h>
    #include <MFRC522.h>

    #define SS_PIN  10
    #define RST_PIN 9

    MFRC522 mfrc522(SS_PIN, RST_PIN);
    MFRC522::MIFARE_Key key;

    // Data structure to store the extracted values
    struct Block56Data {
    bool success;
    byte val3;   // Byte index 3  (0x03)
    byte val11;  // Byte index 11 (0x01)
    };

    void setup() {
    Serial.begin(9600);
    SPI.begin();
    mfrc522.PCD_Init();

    // Set standard factory Key A (FF FF FF FF FF FF)
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    }

    /**
    * Reads Block 56 and extracts bytes 3 and 11 directly into variables.
    */
    Block56Data readBlock56() {
    Block56Data data = {false, 0, 0};

    // Check for card presence
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        return data;
    }

    byte trailerBlock = 59; // Sector 14 authentication block
    byte rawBuffer[18];
    byte bufferSize = sizeof(rawBuffer);

    // Authenticate sector 14
    if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid)) != MFRC522::STATUS_OK) {
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return data;
    }

    // Read raw 16 bytes from Block 56
    MFRC522::StatusCode status = mfrc522.MIFARE_Read(56, rawBuffer, &bufferSize);

    // Halt card and stop encryption engine
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    if (status != MFRC522::STATUS_OK) {
        return data;
    }

    // Save specific byte positions directly into the struct
    data.val3 = rawBuffer[3];    // Extracts 0x03
    data.val11 = rawBuffer[11];  // Extracts 0x01
    data.success = true;

    return data;
    }

    void loop() {
    Block56Data result = readBlock56();

    if (result.success) {
        // Stores byte values directly into local variables for your logic
        byte firstHighlight  = result.val3;   // Variable contains 3
        byte secondHighlight = result.val11;  // Variable contains 1

        // Example logic using the extracted variables:
        if (firstHighlight == 3 && secondHighlight == 1) {
        // Do something when values match
        }

        delay(1000); // Prevent repeated readings
    }
    }