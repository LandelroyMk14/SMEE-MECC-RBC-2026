#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN A1
#define SS_PIN  A0

// A coordinate contains an X and Y position
struct Coordinate {
  long x;
  long y;
};

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// Store the 3 animal coordinates
Coordinate animalTargets[3];


// ------------------------------------------------
// SETUP
// ------------------------------------------------

void setup() {

  Serial.begin(9600);
  while (!Serial);

  SPI.begin();
  mfrc522.PCD_Init();
  delay(4);

  // Factory default Key A
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("Tap the START tag"));
}


// ------------------------------------------------
// FIND THE SECTOR TRAILER FOR A BLOCK
// ------------------------------------------------

byte trailerBlockFor(byte block) {
  return (block / 4) * 4 + 3;
}


// ------------------------------------------------
// DECODE A COORDINATE BLOCK
//
// Bytes 0-3  = X coordinate
// Bytes 8-11 = Y coordinate
//
// Values are stored in big-endian order.
// ------------------------------------------------

Coordinate decodeCoordinateBlock(byte* block) {

  Coordinate c;

  c.x = ((long)block[0] << 24) |
        ((long)block[1] << 16) |
        ((long)block[2] << 8)  |
        block[3];

  c.y = ((long)block[8] << 24) |
        ((long)block[9] << 16) |
        ((long)block[10] << 8) |
        block[11];

  return c;
}


// ------------------------------------------------
// READ THE 3 ANIMAL LOCATIONS FROM THE START TAG
//
// Block 52 = Animal 1
// Block 53 = Animal 2
// Block 54 = Animal 3
// ------------------------------------------------

bool readAnimalLocations(MFRC522 &reader, MFRC522::MIFARE_Key &key) {

  const byte blocks[3] = {52, 53, 54};

  for (byte i = 0; i < 3; i++) {

    byte trailer = trailerBlockFor(blocks[i]);

    // Authenticate the sector using Key A
    MFRC522::StatusCode status = reader.PCD_Authenticate(
      MFRC522::PICC_CMD_MF_AUTH_KEY_A,
      trailer,
      &key,
      &(reader.uid)
    );

    if (status != MFRC522::STATUS_OK) {

      Serial.print(F("Authentication failed for block "));
      Serial.println(blocks[i]);

      return false;
    }

    // Read the block
    byte buffer[18];
    byte size = sizeof(buffer);

    status = reader.MIFARE_Read(blocks[i], buffer, &size);

    if (status != MFRC522::STATUS_OK) {

      Serial.print(F("Read failed for block "));
      Serial.println(blocks[i]);

      return false;
    }

    // Convert the raw bytes into a coordinate
    animalTargets[i] = decodeCoordinateBlock(buffer);
  }

  return true;
}


// ------------------------------------------------
// LOOP
// ------------------------------------------------

void loop() {

  // Check whether a new RFID tag is present
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println(F("START tag detected."));


  // ------------------------------------------------
  // READ THE 3 ANIMAL LOCATIONS
  // ------------------------------------------------

  if (readAnimalLocations(mfrc522, key)) {

    Serial.println(F("Animal locations:"));

    for (byte i = 0; i < 3; i++) {

      Serial.print(F("Animal "));
      Serial.print(i + 1);

      Serial.print(F(": X = "));
      Serial.print(animalTargets[i].x);

      Serial.print(F(", Y = "));
      Serial.println(animalTargets[i].y);
    }

  } else {

    Serial.println(F("Could not read animal locations."));
  }


  // ------------------------------------------------
  // FINISH RFID SESSION
  // ------------------------------------------------

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  Serial.println(F("--- scan complete ---"));
  Serial.println(F("Remove tag and re-tap to scan again."));

  delay(500);
}