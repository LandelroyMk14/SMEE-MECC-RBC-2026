#include <SPI.h>
#include <MFRC522.h>
#include <stdlib.h>


// ============================================================
// RFID PIN DEFINITIONS
// ============================================================

#define SS_PIN  A0
#define RST_PIN A1

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;


// ============================================================
// NAVIGATION DEFINITIONS
// ============================================================

#define XPOS 0
#define YPOS 1
#define XNEG 2
#define YNEG 3

typedef struct {
    int direction;
    int position[2];
} state;


// ============================================================
// STORAGE
// ============================================================

// [0] = X coordinate
// [1] = Y coordinate

int current_location[2] = {-1, -1};

int anim1[2] = {-1, -1};
int anim2[2] = {-1, -1};
int anim3[2] = {-1, -1};

// All animal locations
int animals[3][2];

// Current animal being targeted
int target_animal[2] = {-1, -1};

// Which animal are we currently targeting?
// 0 = animal 1
// 1 = animal 2
// 2 = animal 3
int a = 0;

// Has the robot loaded the animal locations?
bool animals_loaded = false;

// Has the current animal been fetched?
bool animal_reached = false;

// Has the robot completed all animals?
bool finished = false;


// ============================================================
// ROBOT STATE
// ============================================================

// Global so both setup() and loop() can access it
//
// Initial state:
// - Facing XPOS
// - Position (0, 0)

state robot_state = {XPOS, {0, 0}};


// ============================================================
// RFID FUNCTIONS
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


// ============================================================
// NAVIGATION FUNCTION
// ============================================================

// Calculates a route from the robot's current state
// to the destination.
//
// Does NOT move the robot or update its position.
//
// Returns a string containing:
// L = turn left
// R = turn right
// B = turn 180 degrees
// F = move forward

char *navigate(state *robot, int dest[2])
{
    static char moves[100];

    // Clear previous route
    moves[0] = '\0';

    int orientation = robot->direction;

    int x_move = dest[0] - robot->position[0];
    int y_move = dest[1] - robot->position[1];

    int n = 0;


    // --------------------------------------------------------
    // Move in X direction
    // --------------------------------------------------------

    if (x_move != 0)
    {
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
        {
            moves[n++] = 'F';
        }
    }


    // --------------------------------------------------------
    // Move in Y direction
    // --------------------------------------------------------

    if (y_move != 0)
    {
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
        {
            moves[n++] = 'F';
        }
    }

    moves[n] = '\0';

    // IMPORTANT:
    // We do NOT update robot->position here.
    //
    // The robot's actual position is updated when an RFID
    // tag is scanned.

    return moves;
}


// ============================================================
// SET TARGET ANIMAL
// ============================================================

void setTargetAnimal()
{
    if (a >= 3)
    {
        finished = true;
        return;
    }

    target_animal[0] = animals[a][0];
    target_animal[1] = animals[a][1];

    animal_reached = false;

    Serial.print("Target animal: ");
    Serial.println(a + 1);

    Serial.print("Target location: (");
    Serial.print(target_animal[0]);
    Serial.print(", ");
    Serial.print(target_animal[1]);
    Serial.println(")");
}


// ============================================================
// CHECK WHETHER ROBOT HAS REACHED TARGET
// ============================================================

bool atTarget()
{
    return (
        current_location[0] == target_animal[0] &&
        current_location[1] == target_animal[1]
    );
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(9600);

    SPI.begin();
    mfrc522.PCD_Init();

    // Default MIFARE key:
    // FF FF FF FF FF FF

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
    // If all animals have been fetched, stop processing
    if (finished)
    {
        return;
    }


    // --------------------------------------------------------
    // WAIT FOR A NEW RFID TAG
    // --------------------------------------------------------

    if (!mfrc522.PICC_IsNewCardPresent())
        return;

    if (!mfrc522.PICC_ReadCardSerial())
        return;


    // --------------------------------------------------------
    // 1. READ CURRENT LOCATION
    // --------------------------------------------------------

    if (readCoordinates(56, current_location))
    {
        Serial.println();
        Serial.println("----------------------------");

        Serial.print("Current Location: (");
        Serial.print(current_location[0]);
        Serial.print(", ");
        Serial.print(current_location[1]);
        Serial.println(")");


        // ----------------------------------------------------
        // Update robot's actual position
        // ----------------------------------------------------

        robot_state.position[0] = current_location[0];
        robot_state.position[1] = current_location[1];


        // ----------------------------------------------------
        // 2. FIRST TIME AT START
        // ----------------------------------------------------

        if (!animals_loaded &&
            current_location[0] == 0 &&
            current_location[1] == 0)
        {
            Serial.println("Start tag detected.");
            Serial.println("Reading animal locations...");


            // -----------------------------------------------
            // Animal 1
            // -----------------------------------------------

            if (readCoordinates(52, anim1))
            {
                Serial.print("Animal 1: (");
                Serial.print(anim1[0]);
                Serial.print(", ");
                Serial.print(anim1[1]);
                Serial.println(")");

                animals[0][0] = anim1[0];
                animals[0][1] = anim1[1];
            }


            // -----------------------------------------------
            // Animal 2
            // -----------------------------------------------

            if (readCoordinates(53, anim2))
            {
                Serial.print("Animal 2: (");
                Serial.print(anim2[0]);
                Serial.print(", ");
                Serial.print(anim2[1]);
                Serial.println(")");

                animals[1][0] = anim2[0];
                animals[1][1] = anim2[1];
            }


            // -----------------------------------------------
            // Animal 3
            // -----------------------------------------------

            if (readCoordinates(54, anim3))
            {
                Serial.print("Animal 3: (");
                Serial.print(anim3[0]);
                Serial.print(", ");
                Serial.print(anim3[1]);
                Serial.println(")");

                animals[2][0] = anim3[0];
                animals[2][1] = anim3[1];
            }


            // -----------------------------------------------
            // Animal locations successfully loaded
            // -----------------------------------------------

            animals_loaded = true;

            Serial.println("Animal locations loaded.");

            // Select animal 1
            setTargetAnimal();
        }


        // ----------------------------------------------------
        // 3. CHECK WHETHER TARGET HAS BEEN REACHED
        // ----------------------------------------------------

        if (animals_loaded && !finished)
        {
            if (atTarget() && !animal_reached)
            {
                animal_reached = true;

                Serial.print("Animal ");
                Serial.print(a + 1);
                Serial.println(" reached!");

                // -------------------------------------------
                // THIS IS WHERE YOUR FETCHING CODE GOES
                // -------------------------------------------

                Serial.print("Fetching animal ");
                Serial.println(a + 1);

                // For now we assume fetching succeeds
                // immediately.
                //
                // If your claw takes time, you can replace
                // this with a fetchAnimal() function.

                Serial.print("Animal ");
                Serial.print(a + 1);
                Serial.println(" fetched.");


                // -------------------------------------------
                // Move to next animal
                // -------------------------------------------

                a++;

                if (a >= 3)
                {
                    finished = true;

                    Serial.println();
                    Serial.println("================================");
                    Serial.println("All animals fetched!");
                    Serial.println("================================");
                }
                else
                {
                    setTargetAnimal();

                    // Calculate route to next animal
                    char *moves = navigate(
                        &robot_state,
                        target_animal
                    );

                    Serial.print("Moves to animal ");
                    Serial.print(a + 1);
                    Serial.print(": ");
                    Serial.println(moves);
                }
            }
            else if (!atTarget())
            {
                // -------------------------------------------
                // Calculate route to current target
                // -------------------------------------------

                char *moves = navigate(
                    &robot_state,
                    target_animal
                );

                Serial.print("Moves to animal ");
                Serial.print(a + 1);
                Serial.print(": ");
                Serial.println(moves);
            }
        }
    }
    else
    {
        Serial.println("Failed to read current location.");
    }


    // --------------------------------------------------------
    // 4. READ FRIENDLINESS
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