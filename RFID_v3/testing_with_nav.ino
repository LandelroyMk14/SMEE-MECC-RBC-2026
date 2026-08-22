#include <SPI.h>
#include <MFRC522.h>
#include <stdlib.h>    // For Andrew's Nagivation

// --- Custom Pin Definitions ---
#define SS_PIN  A0
#define RST_PIN A1

// --- Andrew's Nagivation Definitions ---
    #define XPOS 0
    #define YPOS 1
    #define XNEG 2
    #define YNEG 3

    typedef struct {
        int direction;
        int position[2];
    } state;

    // Initial struct
    state robot_state = {XPOS, 0, 0};

// NAVIGATION FUNCTION
    char *navigate(state *state, int dest[2])
    {
        static char moves[100] = "";

        int orientation = state->direction;
        int *pos = state->position;

        int n = 0;

        int x_move = dest[0] - pos[0];
        int y_move = dest[1] - pos[1];

        // Move in X direction
        if (x_move != 0) {
            int required;

            if (x_move > 0)
                required = XPOS;
            else
                required = XNEG;

            int turn = (required - orientation + 4) % 4;

            if (turn == 1)
                moves[n++] = 'L';
            else if (turn == 2)
                moves[n++] = 'B';
            else if (turn == 3)
                moves[n++] = 'R';

            orientation = required;

            for (int i = 0; i < abs(x_move); i++)
                moves[n++] = 'F';
        }

        // Move in Y direction
        if (y_move != 0) {
            int required;

            if (y_move > 0)
                required = YPOS;
            else
                required = YNEG;

            int turn = (required - orientation + 4) % 4;

            if (turn == 1)
                moves[n++] = 'L';
            else if (turn == 2)
                moves[n++] = 'B';
            else if (turn == 3)
                moves[n++] = 'R';

            orientation = required;

            for (int i = 0; i < abs(y_move); i++)
                moves[n++] = 'F';
        }

        state->direction = orientation;
        moves[n] = '\0';
        return moves;
    }
//--------------------------------------------


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

  robot_state.position[0] = currentX;
  robot_state.position[1] = currentY;
  int anim1[2] = {0, 0};






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

       //------------------------------------------------------------------------
        anim1[0] = animX;
        anim1[1] = animY;
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


  char *moves = navigate(&robot_state, anim1);
  Serial.println(moves);


  
  Serial.println("----------------------------------------");

  // Halt card communication and stop encryption
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  delay(1000); // 1-second delay to prevent continuous re-reading
}