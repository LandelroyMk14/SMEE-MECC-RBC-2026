#include <SPI.h>
#include <MFRC522.h>
#include <stdlib.h>


// ============================================================
// RFID PIN DEFINITIONS and DEFINITION
// ============================================================

#define SS_PIN  A0
#define RST_PIN A1

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

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
// ============================================================
// STORAGE
// ============================================================

// [0] = X coordinate
// [1] = Y coordinate

int current_location[2] = {-1, -1};

int anim1[2] = {-1, -1};
int anim2[2] = {-1, -1};
int anim3[2] = {-1, -1};

int animals[3][2];
int target_animal[2];

int first_pass = 1;
int a = 0;    // Flag for target animal


// ============================================================
// FUNCTIONS
// ============================================================

// Read a 16-byte MIFARE block
bool readBlockData(byte blockAddr, byte *buffer)
{
    MFRC522::StatusCode status;
    byte size = 18;

    status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        blockAddr,
        &key,
        &(mfrc522.uid)
    );

    if (status != MFRC522::STATUS_OK)
        return false;

    status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);

    return (status == MFRC522::STATUS_OK);
}


// Extract X and Y coordinates from RFID block
void parseCoordinates(byte *buffer, int *x, int *y)
{
    *x = buffer[3];
    *y = buffer[11];
}


// Read coordinates from a specific RFID block
bool readCoordinates(byte blockAddr, int location[2])
{
    byte buffer[18];

    if (!readBlockData(blockAddr, buffer))
        return false;

    parseCoordinates(buffer, &location[0], &location[1]);

    return true;
}

// Creating an instruction line for robot movement
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

// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(9600);

    SPI.begin();
    mfrc522.PCD_Init();

    // Default MIFARE key: FF FF FF FF FF FF
    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }

    Serial.println("RFID Scanner Initialized.");
    Serial.println("Ready to scan tags...");
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
    // Wait for a new RFID tag
    if (!mfrc522.PICC_IsNewCardPresent())
        return;

    if (!mfrc522.PICC_ReadCardSerial())
        return;


    // --------------------------------------------------------
    // 1. READ CURRENT LOCATION
    // --------------------------------------------------------

    if (readCoordinates(56, current_location))
    {
        Serial.print("Current Location: (");
        Serial.print(current_location[0]);
        Serial.print(", ");
        Serial.print(current_location[1]);
        Serial.println(")");

        robot_state.position[0] = current_location[0];
        robot_state.position[1] = current_location[1];

        if (has_target) 
        {
            char *moves = navigate(&robot_state, target_animal);
            Serial.print("Moves: ");
            Serial.println(moves);
        }
    }
    else
    {
        Serial.println("Failed to read current location.");
    }


    // --------------------------------------------------------
    // 2. IF THIS IS THE START TAG, READ ANIMAL LOCATIONS
    // --------------------------------------------------------

    if (current_location[0] == 0 &&
        current_location[1] == 0) 
    {
        if (first_pass == 1) 
        {
            first_pass = 0;
            Serial.println("Start tag detected.");

            if (readCoordinates(52, anim1))
            {
                Serial.print("Animal 1: (");
                Serial.print(anim1[0]);
                Serial.print(", ");
                Serial.print(anim1[1]);
                Serial.println(")");
            }

            if (readCoordinates(53, anim2))
            {
                Serial.print("Animal 2: (");
                Serial.print(anim2[0]);
                Serial.print(", ");
                Serial.print(anim2[1]);
                Serial.println(")");
            }

            if (readCoordinates(54, anim3))
            {
                Serial.print("Animal 3: (");
                Serial.print(anim3[0]);
                Serial.print(", ");
                Serial.print(anim3[1]);
                Serial.println(")");
            }

            // Assigning animals
            animals[0][0] = anim1[0];
            animals[0][1] = anim1[1];

            animals[1][0] = anim2[0];
            animals[1][1] = anim2[1];

            animals[2][0] = anim3[0];
            animals[2][1] = anim3[1];

            target_animal = animals[a++];
        }
            Serial.println("Animal %d Returned", a + 1);
            target_animal[a++]
    }


    // --------------------------------------------------------
    // 3. READ FRIENDLINESS
    // --------------------------------------------------------

    byte buffer[18];

    if (readBlockData(57, buffer))
    {
        byte friendliness = buffer[15];

        Serial.print("Friendliness: ");
        Serial.println(friendliness);
    }


    // --------------------------------------------------------
    // FINISH RFID COMMUNICATION
    // --------------------------------------------------------

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    delay(1000);
}