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


void setup() {
  Serial.begin(9600);
  while (!Serial);

  SPI.begin();
  mfrc522.PCD_Init();
  delay(4);

  // Factory default Key A = FF FF FF FF FF FF
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("Tap an RFID tag"));
}


// Decode the information stored in block 56.
//
// Bytes 0-3  = X coordinate
// Bytes 8-11 = Y coordinate
//
// The values are stored in big-endian order.
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


// Try to authenticate and read block 56.
bool readTagCoordinate(MFRC522 &reader, Coordinate &out) {

  const byte BLOCK = 56;

  // Block 59 is the sector trailer for sector 14.
  const byte TRAILER = 59;

  // Authenticate sector 14 using Key A
  MFRC522::StatusCode status = reader.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A,
    TRAILER,
    &key,
    &(reader.uid)
  );

  // Stop if authentication failed
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(reader.GetStatusCodeName(status));
    return false;
  }

  byte buffer[18];
  byte size = sizeof(buffer);

  // Read block 56
  status = reader.MIFARE_Read(BLOCK, buffer, &size);

  // Stop if the read failed
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Read failed: "));
    Serial.println(reader.GetStatusCodeName(status));
    return false;
  }

  // Convert the raw bytes into X and Y coordinates
  out = decodeCoordinateBlock(buffer);

  return true;
}


void loop() {

  // Check whether a new RFID tag is present
  if (!mfrc522.PICC_IsNewCardPresent() ||
      !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println(F("Tag detected."));


  // ------------------------------------------------
  // READ AND DECODE THE COORDINATE FROM BLOCK 56
  // ------------------------------------------------

  Coordinate coordinate;

  if (readTagCoordinate(mfrc522, coordinate)) {

    Serial.println(F("Coordinate detected:"));

    Serial.print(F("X = "));
    Serial.println(coordinate.x);

    Serial.print(F("Y = "));
    Serial.println(coordinate.y);

  } else {

    Serial.println(F("Could not read block 56."));
  }


  // Tell the tag that we are finished
  mfrc522.PICC_HaltA();

  // End any active encryption/authentication session
  mfrc522.PCD_StopCrypto1();

  Serial.println(F("--- scan complete ---"));

  delay(500);
}