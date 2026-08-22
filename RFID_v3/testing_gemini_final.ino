#include <SPI.h>
#include <MFRC522.h>

// --- Custom Pin Definitions ---
#define SS_PIN  A0
#define RST_PIN A1

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// Helper function to authenticate and read a 16-byte block
bool readBlockData(byte blockAddr, byte *buffer) {
  MFRC522::StatusCode status;
  byte size = 18;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) return false;

  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  return (status == MFRC522::STATUS_OK);
}

// Parses X (bytes 0-3) and Y (bytes 8-11) from block data
void parseCoordinates(byte *buffer, int &x, int &y) {
  x = buffer[3];  // Byte 3 (4th byte of first set)
  y = buffer[11]; // Byte 11 (4th byte of third set)
}

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  // Set default MIFARE key (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println("RFID Scanner Initialized. Ready to scan tags...");
}

void loop() {
  // Check if a new card is present and can be read
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  byte buffer[18];
  int currentX = -1;
  int currentY = -1;

  // 1. Read current location from Block 56
  if (readBlockData(56, buffer)) {
    parseCoordinates(buffer, currentX, currentY);
    Serial.print("Current Location: (");
    Serial.print(currentX);
    Serial.print(", ");
    Serial.print(currentY);
    Serial.println(")");
  } else {
    Serial.println("Failed to read location from Block 56");
  }

  // 2. If tag is at (0,0), read animal coordinates from Blocks 52, 53, 54
  if (currentX == 0 && currentY == 0) {
    Serial.println("=== START TAG (0,0) DETECTED ===");
    int animX, animY;

    if (readBlockData(52, buffer)) {
      parseCoordinates(buffer, animX, animY);
      Serial.print("Animal 1 Location: ("); Serial.print(animX); Serial.print(", "); Serial.print(animY); Serial.println(")");
    }
    if (readBlockData(53, buffer)) {
      parseCoordinates(buffer, animX, animY);
      Serial.print("Animal 2 Location: ("); Serial.print(animX); Serial.print(", "); Serial.print(animY); Serial.println(")");
    }
    if (readBlockData(54, buffer)) {
      parseCoordinates(buffer, animX, animY);
      Serial.print("Animal 3 Location: ("); Serial.print(animX); Serial.print(", "); Serial.print(animY); Serial.println(")");
    }
  }

  // 3. Read Friendliness value from Block 57 (Column/Byte 15)
  if (readBlockData(57, buffer)) {
    byte friendliness = buffer[15];
    Serial.print("Friendliness Value: ");
    Serial.print(friendliness);
    
    if (friendliness == 3) {
      Serial.println(" (Friendly Animal)");
    } else if (friendliness == 2) {
      Serial.println(" (Unfriendly Animal)");
    } else {
      Serial.println(" (No nearby animal data)");
    }
  }

  Serial.println("----------------------------------------");

  // Halt card communication and stop encryption
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  delay(1000); // 1-second delay to prevent continuous re-reading
}