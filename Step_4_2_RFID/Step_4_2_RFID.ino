#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN A1
#define SS_PIN  A0

// ------------------------------------------------
// START TAG COORDINATES
// ------------------------------------------------

#define START_X 0
#define START_Y 0


// ------------------------------------------------
// DATA STRUCTURES
// ------------------------------------------------

struct Coordinate {
  long x;
  long y;
};


// ------------------------------------------------
// GLOBAL ROBOT SUBSYSTEM VARIABLES
// Use these variables to access data across subsystems.
// ------------------------------------------------

Coordinate currentPinCoordinate;  // Current X and Y position of the robot's scanned pin
int pinFriendlinessRaw;           // Raw friendliness integer value (3 = friendly, 2 = not friendly)
bool isCurrentAnimalFriendly;     // True if friendly, false if dangerous or unknown

Coordinate animalTargets[3];      // The 3 global target coordinates of the animals


// ------------------------------------------------
// HARDWARE INSTANCES
// ------------------------------------------------

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;


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

  Serial.println(F("Tap an RFID tag"));
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
// DECODE FRIENDLINESS
//
// 3 = friendly
// 2 = not friendly
// Anything else = unreadable/unknown
//
// Value is stored in first 4 bytes,
// big-endian.
// ------------------------------------------------

int decodeFriendliness(byte* block) {

  long value =
    ((long)block[0] << 24) |
    ((long)block[1] << 16) |
    ((long)block[2] << 8) |
    block[3];

  return (int)value;
}


// ------------------------------------------------
// READ COORDINATE (block 56) AND FRIENDLINESS (block 57)
// FROM THE CURRENT TAG.
//
// Blocks 56 and 57 live in the SAME sector (trailer 59),
// so we authenticate that sector ONCE and read both blocks
// from the same session. Authenticating twice for two blocks
// in the same sector desyncs the crypto1 stream on this library
// and silently returns zeroed buffers instead of an error.
// ------------------------------------------------

bool readTagCoordinateAndFriendliness(MFRC522 &reader,
                                       MFRC522::MIFARE_Key &key,
                                       Coordinate &coordOut,
                                       int &friendlinessOut) {

  const byte TRAILER = trailerBlockFor(56); // == trailerBlockFor(57)

  MFRC522::StatusCode status = reader.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A,
    TRAILER,
    &key,
    &(reader.uid)
  );

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed for sector: "));
    Serial.println(reader.GetStatusCodeName(status));
    return false;
  }

  byte buffer[18];
  byte size = sizeof(buffer);

  // Block 56 — coordinate
  status = reader.MIFARE_Read(56, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Could not read block 56: "));
    Serial.println(reader.GetStatusCodeName(status));
    return false;
  }
  coordOut = decodeCoordinateBlock(buffer);

  // Block 57 — friendliness (same authenticated session)
  size = sizeof(buffer);
  status = reader.MIFARE_Read(57, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Could not read block 57: "));
    Serial.println(reader.GetStatusCodeName(status));
    return false;
  }
  friendlinessOut = decodeFriendliness(buffer);

  return true;
}


// ------------------------------------------------
// CHECK IF ANIMAL IS VALID
//
// 3 = friendly
// ------------------------------------------------

bool isAnimalValid(int friendlinessValue) {

  return friendlinessValue == 3;
}


// ------------------------------------------------
// CHECK WHETHER THIS IS THE START TAG
// ------------------------------------------------

bool isStartTag(Coordinate coordinate) {

  return coordinate.x == START_X &&
         coordinate.y == START_Y;
}


// ------------------------------------------------
// READ THE 3 ANIMAL LOCATIONS FROM THE START TAG
//
// Block 52 = Animal 1
// Block 53 = Animal 2
// Block 54 = Animal 3
// ------------------------------------------------

bool readAnimalLocations(MFRC522 &reader,
                         MFRC522::MIFARE_Key &key) {

  const byte blocks[3] = {52, 53, 54};

  for (byte i = 0; i < 3; i++) {

    byte trailer = trailerBlockFor(blocks[i]);

    // Authenticate the sector
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

    // Decode coordinate
    animalTargets[i] = decodeCoordinateBlock(buffer);
  }

  return true;
}


// ------------------------------------------------
// LOOP
// ------------------------------------------------

void loop() {

  // Check whether a new RFID tag is present
  if (!mfrc522.PICC_IsNewCardPresent() ||
      !mfrc522.PICC_ReadCardSerial()) {

    return;
  }

  Serial.println();
  Serial.println(F("Tag detected."));


  // ------------------------------------------------
  // READ COORDINATE + FRIENDLINESS IN ONE AUTHENTICATED SESSION
  // ------------------------------------------------

  if (readTagCoordinateAndFriendliness(mfrc522, key, currentPinCoordinate, pinFriendlinessRaw)) {

    Serial.println(F("Current tag coordinate saved:"));
    Serial.print(F("X = "));
    Serial.println(currentPinCoordinate.x);
    Serial.print(F("Y = "));
    Serial.println(currentPinCoordinate.y);

    Serial.print(F("Friendliness value saved = "));
    Serial.println(pinFriendlinessRaw);

    // Evaluate system safety flags
    isCurrentAnimalFriendly = isAnimalValid(pinFriendlinessRaw);

    if (isCurrentAnimalFriendly) {
      Serial.println(F("Status: FRIENDLY"));
    } else if (pinFriendlinessRaw == 2) {
      Serial.println(F("Status: NOT FRIENDLY"));
    } else {
      Serial.println(F("Status: UNKNOWN - invalid value"));
    }


    // ------------------------------------------------
    // CHECK IF THIS IS THE START TAG
    // ------------------------------------------------

    if (isStartTag(currentPinCoordinate)) {

      Serial.println(F("Start tag verified."));

      // ------------------------------------------------
      // READ AND STORE THE 3 ANIMAL LOCATIONS
      // ------------------------------------------------

      if (readAnimalLocations(mfrc522, key)) {

        Serial.println(F("Animal target coordinates updated:"));

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
    }

  } else {
    Serial.println(F("Failed to read tag coordinate/friendliness."));
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