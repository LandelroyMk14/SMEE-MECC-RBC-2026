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

// Is the robot currently returning a fetched animal to (0,0)?
bool returning_home = false;

// Has friendliness already been checked for the current animal?
bool friendliness_checked = false;

// Has the robot completed all animals?
bool finished = false;


// ============================================================
// ROBOT STATE
// ============================================================

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

void moveForward(state *robot)
{
    //code here

}

void turnLeft(state *robot)
{
    //code here

    robot->direction =
        (robot_state->direction + 3) % 4;
}

void turnRight(state *robot)
{
    //code here

    robot->direction =
        (robot_state->direction + 1) % 4;
}



void executeMovement(char *moves)
{
    if (moves[0] == '\0')
        return;

    char instruction = moves[0];

    switch (instruction)
    {
        case 'F':
            moveForward();
            break;

        case 'L':
            turnLeft();
            break;

        case 'R':
            turnRight();
            break;

        case 'B':
            turnRight();
            turnRight();
            break;
    }
}

char *navigate(state *robot, int dest[2])
{
    static char moves[100];

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

    // Position is NOT updated here.
    // RFID scans update the robot's actual position.

    executeMovement(&moves);
    return moves;
}

void turnTowards(state *robot, int target[2])
{
    int dx = target[0] - robot->position[0];
    int dy = target[1] - robot->position[1];

    int targetDirection;

    // Determine which direction the target is in
    if (dx > 0)
        targetDirection = XPOS;
    else if (dx < 0)
        targetDirection = XNEG;
    else if (dy > 0)
        targetDirection = YPOS;
    else if (dy < 0)
        targetDirection = YNEG;
    else
        return;  // Already at target

    // Calculate required turn
    int turn = (targetDirection - robot->direction + 4) % 4;

    switch (turn)
    {
        case 0:
            // Already facing target
            break;

        case 1:
            // Turn right 90°
            turnRight();
            break;

        case 2:
            // Turn 180°
            turnRight();
            turnRight();
            break;

        case 3:
            // Turn left 90°
            turnLeft();
            break;
    }

    // Update stored orientation
    robot->direction = targetDirection;
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
    returning_home = false;
    friendliness_checked = false;

    Serial.print("Target animal: ");
    Serial.println(a + 1);

    Serial.print("Target location: (");
    Serial.print(target_animal[0]);
    Serial.print(", ");
    Serial.print(target_animal[1]);
    Serial.println(")");
}


// ============================================================
// SET TARGET HOME
// ============================================================

void setTargetHome()
{
    target_animal[0] = 0;
    target_animal[1] = 0;

    returning_home = true;

    Serial.println("Returning animal to (0,0)...");

    char *moves = navigate(
        &robot_state,
        target_animal
    );

    Serial.print("Moves home: ");
    Serial.println(moves);
}


// ============================================================
// CHECK WHETHER ROBOT IS AT A LOCATION
// ============================================================

bool atLocation(int location[2])
{
    return (
        current_location[0] == location[0] &&
        current_location[1] == location[1]
    );
}


// ============================================================
// CHECK WHETHER ROBOT IS AT TARGET
// ============================================================

bool atTarget()
{
    return atLocation(target_animal);
}


// ============================================================
// CHECK WHETHER ROBOT IS ADJACENT TO ANIMAL
// ============================================================

// Returns true if the robot is exactly one grid square
// away from the current animal.
//
// Valid examples:
//
// Animal: (3,3)
// Robot:  (2,3) -> adjacent
// Robot:  (4,3) -> adjacent
// Robot:  (3,2) -> adjacent
// Robot:  (3,4) -> adjacent
//
// Robot:  (2,2) -> NOT adjacent

bool adjacentToTarget()
{
    int dx = abs(
        current_location[0] - target_animal[0]
    );

    int dy = abs(
        current_location[1] - target_animal[1]
    );


    return (dx + dy == 1);
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
    // If all animals have been considered/fetched,
    // stop processing.

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

            // Calculate route to animal 1
            char *moves = navigate(
                &robot_state,
                target_animal
            );

            Serial.print("Moves to animal 1: ");
            Serial.println(moves);
        }
    }
    else
    {
        Serial.println("Failed to read current location.");
    }


    // ========================================================
    // 3. READ FRIENDLINESS
    // ========================================================

    // Friendliness is ONLY read when:
    //
    // 1. Animal locations have been loaded
    // 2. We are NOT returning home
    // 3. We have NOT already checked this animal
    // 4. The robot is adjacent to the animal
    //
    // This prevents the friendliness block from being read
    // while travelling around unrelated map locations.

    byte buffer[18];

    byte friendliness = 0;

    if (animals_loaded &&
        !returning_home &&
        !friendliness_checked &&
        adjacentToTarget())
    {
        if (readBlockData(57, buffer))
        {
            friendliness = buffer[15];

            Serial.print("Friendliness: ");
            Serial.println(friendliness);

            friendliness_checked = true;


            // =================================================
            // 4. PROCESS FRIENDLINESS
            // =================================================

            // ASSUMPTION:
            // friendliness == 1 means friendly.
            // Any other value means not friendly.

            if (friendliness != 3)
            {
                // ---------------------------------------------
                // NOT FRIENDLY
                // ---------------------------------------------

                Serial.print("Animal ");
                Serial.print(a + 1);
                Serial.println(" is not friendly.");

                Serial.print("Skipping animal ");
                Serial.println(a + 1);

                a++;

                if (a >= 3)
                {
                    finished = true;

                    Serial.println();
                    Serial.println("================================");
                    Serial.println("All animals processed!");
                    Serial.println("================================");
                }
                else
                {
                    setTargetAnimal();

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


            // -----------------------------------------------
            // FRIENDLY
            // -----------------------------------------------

            else
            {
                Serial.print("Animal ");
                Serial.print(a + 1);
                Serial.println(" is friendly!");

                Serial.print("Fetching animal ");
                Serial.println(a + 1);


                // TODO:
                if (returning_home != 1)
                {
                turnTowards(&robot_state, target_animal);
                //claw
                }

                // Actual claw/fetching code goes here.
                //
                // For now, fetching is assumed to succeed
                // immediately.

                Serial.print("Animal ");
                Serial.print(a + 1);
                Serial.println(" fetched.");


                // -------------------------------------------
                // Mark animal as fetched
                // -------------------------------------------

                animal_reached = true;


                // -------------------------------------------
                // Return animal to (0,0)
                // -------------------------------------------

                setTargetHome();
            }
        }
        else
        {
            Serial.println("Failed to read friendliness.");
        }
    }


    // ========================================================
    // 5. RETURN HOME
    // ========================================================

    if (animals_loaded && !finished && returning_home)
    {
        // The animal is only considered delivered when
        // an RFID scan explicitly reports (0,0).

        if (current_location[0] == 0 &&
            current_location[1] == 0)
        {
            Serial.println();
            Serial.println("================================");

            Serial.print("Animal ");
            Serial.print(a + 1);
            Serial.println(" returned to (0,0).");

            Serial.println("================================");


            animal_reached = false;
            returning_home = false;


            // -----------------------------------------------
            // Move to next animal
            // -----------------------------------------------

            a++;

            if (a >= 3)
            {
                finished = true;

                Serial.println();
                Serial.println("================================");
                Serial.println("All animals processed!");
                Serial.println("================================");
            }
            else
            {
                setTargetAnimal();

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


    // ========================================================
    // 6. NAVIGATION
    // ========================================================

    if (animals_loaded && !finished)
    {
        // If returning home, continue navigating to (0,0)
        if (returning_home &&
            !atTarget())
        {
            char *moves = navigate(
                &robot_state,
                target_animal
            );

            Serial.print("Moves home: ");
            Serial.println(moves);
        }

        // If travelling to an animal, navigate until
        // adjacent to it.
        else if (!returning_home &&
                 !adjacentToTarget())
        {
            // ------------------------------------------------
            // Navigate to an adjacent square.
            //
            // IMPORTANT:
            // navigate() currently targets the animal itself.
            // This section therefore needs a separate adjacent
            // destination if the robot must physically stop
            // beside the animal rather than on it.
            // ------------------------------------------------

            char *moves = navigate(
                &robot_state,
                target_animal
            );

            Serial.print("Moves toward animal ");
            Serial.print(a + 1);
            Serial.print(": ");
            Serial.println(moves);
        }
    }


    // --------------------------------------------------------
    // FINISH RFID COMMUNICATION
    // --------------------------------------------------------

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    delay(1000);
}