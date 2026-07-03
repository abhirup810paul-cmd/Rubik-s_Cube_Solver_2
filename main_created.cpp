#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <fstream> 

using namespace std;

/**
 * 1. THE ABSTRACT BASE CLASS (The Blueprint)
 * This defines what EVERY Rubik's Cube representation must be able to do.
 * We use 'virtual ... = 0' to enforce that any class inheriting from this MUST implement these functions.
 */
class RubiksCube {
public:
    // Standard faces: Up, Left, Front, Right, Back, Down
    enum class FACE { UP, LEFT, FRONT, RIGHT, BACK, DOWN };
    // Standard colors
    enum class COLOR { WHITE, GREEN, RED, BLUE, ORANGE, YELLOW };

    // Pure virtual functions for moves (Returning reference to allow move chaining e.g. cube.u().r().f())
    virtual RubiksCube& u() = 0;  // Up clockwise
    virtual RubiksCube& u_prime() = 0; // Up prime (counter-clockwise)
    virtual RubiksCube& u2() = 0; // Up 180 degrees

    virtual RubiksCube& l() = 0;  // Left clockwise
    virtual RubiksCube& l_prime() = 0; // Left prime
    virtual RubiksCube& l2() = 0; // Left 180

    // Front clockwise, prime, 180
    virtual RubiksCube& f() = 0;
    virtual RubiksCube& f_prime() = 0;
    virtual RubiksCube& f2() = 0;

    // Right clockwise, prime, 180
    virtual RubiksCube& r() = 0;
    virtual RubiksCube& r_prime() = 0;
    virtual RubiksCube& r2() = 0;

    // Back clockwise, prime, 180
    virtual RubiksCube& b() = 0;
    virtual RubiksCube& b_prime() = 0;
    virtual RubiksCube& b2() = 0;

    // Down clockwise, prime, 180
    virtual RubiksCube& d() = 0;
    virtual RubiksCube& d_prime() = 0;
    virtual RubiksCube& d2() = 0;

    // The new bridge function for the database
    virtual uint8_t publicGetCorner(int index) const = 0;

    virtual void applyMove(int move) = 0;
    virtual int getInverseMove(int move) const = 0;
    virtual string getMoveName(int move) const = 0;

    virtual bool isSolved() const = 0;
    virtual void print() const = 0;
};

/**
 * 2. THE 3D ARRAY IMPLEMENTATION
 * This represents the cube as a 6x3x3 grid of characters.
 */
class RubiksCube3dArray : public RubiksCube {
private:
    // cube[face][row][column]
    char cube[6][3][3];

    // Helper function to rotate a 2D matrix (a single face) 90 degrees clockwise
    void rotateFaceClockwise(int face) {
        char temp[3][3];
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                temp[j][2 - i] = cube[face][i][j];
            }
        }
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                cube[face][i][j] = temp[i][j];
            }
        }
    }

    // Helper function to check if 3 chars match 3 specific colors (in any order)
    bool hasColors(char c1, char c2, char c3, char t1, char t2, char t3) const {
        int matchCount = 0;
        if (c1 == t1 || c1 == t2 || c1 == t3) matchCount++;
        if (c2 == t1 || c2 == t2 || c2 == t3) matchCount++;
        if (c3 == t1 || c3 == t2 || c3 == t3) matchCount++;
        return matchCount == 3;
    }

    int getCornerPositionID(char c1, char c2, char c3) const {
        // Assuming Standard Western Colors: 
        // W=Up, Y=Down, G=Front, B=Back, R=Right, O=Left
        
        // 0: URF (Up-Right-Front)
        if (hasColors(c1, c2, c3, 'W', 'R', 'G')) return 0;
        // 1: UFL (Up-Front-Left)
        if (hasColors(c1, c2, c3, 'W', 'G', 'O')) return 1;
        // 2: ULB (Up-Left-Back)
        if (hasColors(c1, c2, c3, 'W', 'O', 'B')) return 2;
        // 3: UBR (Up-Back-Right)
        if (hasColors(c1, c2, c3, 'W', 'B', 'R')) return 3;
        // 4: DFR (Down-Front-Right)
        if (hasColors(c1, c2, c3, 'Y', 'G', 'R')) return 4;
        // 5: DLF (Down-Left-Front)
        if (hasColors(c1, c2, c3, 'Y', 'O', 'G')) return 5;
        // 6: DBL (Down-Back-Left)
        if (hasColors(c1, c2, c3, 'Y', 'B', 'O')) return 6;
        // 7: DRB (Down-Right-Back)
        if (hasColors(c1, c2, c3, 'Y', 'R', 'B')) return 7;

        return -1; // Error fallback
    }

public:
    // Constructor: Initialize a solved cube
    RubiksCube3dArray() {
        char colors[] = {'W', 'G', 'R', 'B', 'O', 'Y'};
        for (int f = 0; f < 6; f++) {
            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 3; c++) {
                    cube[f][r][c] = colors[f];
                }
            }
        }
    }

    // --- IMPLEMENTING THE MOVES ---

    // Turn the UP face clockwise (U)
    RubiksCube& u() override {
        // 1. Rotate the Up face itself 90 degrees clockwise
        rotateFaceClockwise((int)FACE::UP);

        // 2. Shift the adjacent top rows of Left, Front, Right, and Back faces
        char temp[3];
        for (int i = 0; i < 3; i++) temp[i] = cube[(int)FACE::BACK][0][i];

        for (int i = 0; i < 3; i++) cube[(int)FACE::BACK][0][i] = cube[(int)FACE::LEFT][0][i];
        for (int i = 0; i < 3; i++) cube[(int)FACE::LEFT][0][i] = cube[(int)FACE::FRONT][0][i];
        for (int i = 0; i < 3; i++) cube[(int)FACE::FRONT][0][i] = cube[(int)FACE::RIGHT][0][i];
        for (int i = 0; i < 3; i++) cube[(int)FACE::RIGHT][0][i] = temp[i];

        return *this;
    }

    // Turn the UP face counter-clockwise (U')
    RubiksCube& u_prime() override {
        this->u();
        this->u();
        this->u(); // Doing 3 clockwise turns is equal to 1 counter-clockwise turn
        return *this;
    }

    // Turn the UP face 180 degrees (U2)
    RubiksCube& u2() override {
        this->u();
        this->u();
        return *this;
    }

    // Dummy implementations for L moves to satisfy the compiler
     
    RubiksCube& l() override {
        rotateFaceClockwise((int)FACE::LEFT);

        char temp[3];
        for (int i = 0; i < 3; i++) temp[i] = cube[(int)FACE::UP][i][0]; // Left col of UP

        for (int i = 0; i < 3; i++) cube[(int)FACE::UP][i][0] = cube[(int)FACE::BACK][2 - i][2]; // Right col of BACK (inverted)
        for (int i = 0; i < 3; i++) cube[(int)FACE::BACK][i][2] = cube[(int)FACE::DOWN][2 - i][0]; // Left col of DOWN (inverted)
        for (int i = 0; i < 3; i++) cube[(int)FACE::DOWN][i][0] = cube[(int)FACE::FRONT][i][0]; // Left col of FRONT
        for (int i = 0; i < 3; i++) cube[(int)FACE::FRONT][i][0] = temp[i];

        return *this;
    }
         

    RubiksCube& l_prime() override { 
        this->l();
        this->l();
        this->l(); // Doing 3 clockwise turns is equal to 1 counter-clockwise
        return *this; 
    }

    RubiksCube& l2() override { 
        this->l();
        this->l();
        return *this; 
    }

    // Dummy implementations for F moves to satisfy the compiler
    RubiksCube& f() override {
        // 1. Rotate Front face itself 90 deg clockwise
        rotateFaceClockwise((int)FACE::FRONT);

        // 2. Shift adjacent tracks
        char temp[3];
        for (int i = 0; i < 3; i++) temp[i] = cube[(int)FACE::UP][2][i]; // Bottom row of UP

        for (int i = 0; i < 3; i++) cube[(int)FACE::UP][2][i] = cube[(int)FACE::LEFT][2 - i][2]; // Right col of LEFT (bottom-to-top)
        for (int i = 0; i < 3; i++) cube[(int)FACE::LEFT][i][2] = cube[(int)FACE::DOWN][0][i]; // Top row of DOWN
        for (int i = 0; i < 3; i++) cube[(int)FACE::DOWN][0][i] = cube[(int)FACE::RIGHT][2 - i][0]; // Left col of RIGHT (bottom-to-top)
        for (int i = 0; i < 3; i++) cube[(int)FACE::RIGHT][i][0] = temp[i];

        return *this;
    }

    RubiksCube& f_prime() override { 
        this->f();
        this->f();
        this->f();
        return *this; 
    }

    RubiksCube& f2() override { 
        this->f();
        this->f();
        return *this; 
    }

    // Dummy implementations for R moves to satisfy the compiler
    RubiksCube& r() override {
        rotateFaceClockwise((int)FACE::RIGHT);

        char temp[3];
        for (int i = 0; i < 3; i++) temp[i] = cube[(int)FACE::UP][i][2]; // Right col of UP

        for (int i = 0; i < 3; i++) cube[(int)FACE::UP][i][2] = cube[(int)FACE::FRONT][i][2]; // Right col of FRONT
        for (int i = 0; i < 3; i++) cube[(int)FACE::FRONT][i][2] = cube[(int)FACE::DOWN][i][2]; // Right col of DOWN
        for (int i = 0; i < 3; i++) cube[(int)FACE::DOWN][i][2] = cube[(int)FACE::BACK][2 - i][0]; // Left col of BACK (inverted)
        for (int i = 0; i < 3; i++) cube[(int)FACE::BACK][i][0] = temp[2 - i];

        return *this;
    }
    
    RubiksCube& r_prime() override { 
        this->r();
        this->r();
        this->r();
        return *this; 
    }
    
    RubiksCube& r2() override { 
        this->r();
        this->r();
        return *this; 
    }

    // Dummy implementations for B moves to satisfy the compiler
    RubiksCube& b() override {
        rotateFaceClockwise((int)FACE::BACK);

        char temp[3];
        for (int i = 0; i < 3; i++) temp[i] = cube[(int)FACE::UP][0][i]; // Top row of UP

        for (int i = 0; i < 3; i++) cube[(int)FACE::UP][0][i] = cube[(int)FACE::RIGHT][i][2]; // Right col of RIGHT
        for (int i = 0; i < 3; i++) cube[(int)FACE::RIGHT][i][2] = cube[(int)FACE::DOWN][2][2 - i]; // Bottom row of DOWN (inverted)
        for (int i = 0; i < 3; i++) cube[(int)FACE::DOWN][2][i] = cube[(int)FACE::LEFT][i][0]; // Left col of LEFT
        for (int i = 0; i < 3; i++) cube[(int)FACE::LEFT][i][0] = temp[2 - i];

        return *this;
    }

    RubiksCube& b_prime() override { 
        this->b();
        this->b();
        this->b();
        return *this; 
    }

    RubiksCube& b2() override { 
        this->b();
        this->b();
        return *this; 
    }

    // Dummy implementations for D moves to satisfy the compiler
    RubiksCube& d() override {
        rotateFaceClockwise((int)FACE::DOWN);

        char temp[3];
        for (int i = 0; i < 3; i++) temp[i] = cube[(int)FACE::FRONT][2][i]; // Bottom row of FRONT

        for (int i = 0; i < 3; i++) cube[(int)FACE::FRONT][2][i] = cube[(int)FACE::LEFT][2][i];
        for (int i = 0; i < 3; i++) cube[(int)FACE::LEFT][2][i] = cube[(int)FACE::BACK][2][i];
        for (int i = 0; i < 3; i++) cube[(int)FACE::BACK][2][i] = cube[(int)FACE::RIGHT][2][i];
        for (int i = 0; i < 3; i++) cube[(int)FACE::RIGHT][2][i] = temp[i];

        return *this;
    }
    
    RubiksCube& d_prime() override { 
        this->d();
        this->d();
        this->d();
        return *this; 
    }

    RubiksCube& d2() override { 
        this->d();
        this->d();
        return *this; 
    }


    uint8_t publicGetCorner(int index) const override {
        char top_bottom_sticker;
        char right_left_sticker;
        char front_back_sticker;

        switch (index) {
            case 0: // URF
                top_bottom_sticker = cube[(int)FACE::UP][2][2];
                right_left_sticker = cube[(int)FACE::RIGHT][0][0];
                front_back_sticker = cube[(int)FACE::FRONT][0][2];
                break;
            case 1: // UFL
                top_bottom_sticker = cube[(int)FACE::UP][2][0];
                front_back_sticker = cube[(int)FACE::FRONT][0][0];
                right_left_sticker = cube[(int)FACE::LEFT][0][2];
                break;
            case 2: // ULB
                top_bottom_sticker = cube[(int)FACE::UP][0][0];
                right_left_sticker = cube[(int)FACE::LEFT][0][0];
                front_back_sticker = cube[(int)FACE::BACK][0][2];
                break;
            case 3: // UBR
                top_bottom_sticker = cube[(int)FACE::UP][0][2];
                front_back_sticker = cube[(int)FACE::BACK][0][0];
                right_left_sticker = cube[(int)FACE::RIGHT][0][2];
                break;
            case 4: // DFR
                top_bottom_sticker = cube[(int)FACE::DOWN][0][2];
                front_back_sticker = cube[(int)FACE::FRONT][2][2];
                right_left_sticker = cube[(int)FACE::RIGHT][2][0];
                break;
            case 5: // DLF
                top_bottom_sticker = cube[(int)FACE::DOWN][0][0];
                right_left_sticker = cube[(int)FACE::LEFT][2][2];
                front_back_sticker = cube[(int)FACE::FRONT][2][0];
                break;
            case 6: // DBL
                top_bottom_sticker = cube[(int)FACE::DOWN][2][0];
                front_back_sticker = cube[(int)FACE::BACK][2][2];
                right_left_sticker = cube[(int)FACE::LEFT][2][0];
                break;
            case 7: // DRB
                top_bottom_sticker = cube[(int)FACE::DOWN][2][2];
                right_left_sticker = cube[(int)FACE::RIGHT][2][2];
                front_back_sticker = cube[(int)FACE::BACK][2][0];
                break;
            default:
                return 0;
        }

        // 1. Identify which physical piece is sitting here
        int position = getCornerPositionID(top_bottom_sticker, right_left_sticker, front_back_sticker);

        // 2. Calculate Orientation (Standard Kociemba rules)
        // 0 = White/Yellow is on the Top/Bottom face
        // 1 = White/Yellow is twisted clockwise (onto the R/L face)
        // 2 = White/Yellow is twisted counter-clockwise (onto the F/B face)
        int orientation = 0;
        if (top_bottom_sticker == 'W' || top_bottom_sticker == 'Y') {
            orientation = 0;
        } else if (right_left_sticker == 'W' || right_left_sticker == 'Y') {
            orientation = 1; 
        } else if (front_back_sticker == 'W' || front_back_sticker == 'Y') {
            orientation = 2; 
        }

        // 3. Pack into the 8-bit format expected by the heuristic database
        return (uint8_t)((orientation << 3) | position);
    }


    void applyMove(int move) override {
        switch (move) {
            case 0: u(); break;
            case 1: u_prime(); break;
            case 2: u2(); break;
            case 3: l(); break;
            case 4: l_prime(); break;
            case 5: l2(); break;
            case 6: f(); break;
            case 7: f_prime(); break;
            case 8: f2(); break;
            case 9: r(); break;
            case 10: r_prime(); break;
            case 11: r2(); break;
            case 12: b(); break;
            case 13: b_prime(); break;
            case 14: b2(); break;
            case 15: d(); break;
            case 16: d_prime(); break;
            case 17: d2(); break;
        }
    }


    int getInverseMove(int move) const override {
        int face = move / 3;  // Determines which face (0 to 5)
        int turn = move % 3;  // Determines which turn type (0, 1, or 2)

        if (turn == 0) return face * 3 + 1; // Clockwise -> Prime
        if (turn == 1) return face * 3;     // Prime -> Clockwise
        return move;                        // 180 -> 180
    }


    string getMoveName(int move) const override {
        switch (move) {
            case 0: return "U";
            case 1: return "U'";
            case 2: return "U2";
            case 3: return "L";
            case 4: return "L'";
            case 5: return "L2";
            case 6: return "F";
            case 7: return "F'";
            case 8: return "F2";
            case 9: return "R";
            case 10: return "R'";
            case 11: return "R2";
            case 12: return "B";
            case 13: return "B'";
            case 14: return "B2";
            case 15: return "D";
            case 16: return "D'";
            case 17: return "D2";
            default: return "";
        }
    }


    // Check if the cube is solved
    bool isSolved() const override {
        for (int f = 0; f < 6; f++) {
            char firstColor = cube[f][0][0];
            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 3; c++) {
                    if (cube[f][r][c] != firstColor) return false;
                }
            }
        }
        return true;
    }

    // Print the top row of the cube to console for debugging
    void print() const override {
        cout << "UP FACE:\n";
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) cout << cube[(int)FACE::UP][r][c] << " ";
            cout << "\n";
        }
        cout << "FRONT FACE (Top Row): " << cube[(int)FACE::FRONT][0][0] << " " << cube[(int)FACE::FRONT][0][1] << " " << cube[(int)FACE::FRONT][0][2] << "\n\n";
    }
};






/**
 * 2. THE BITBOARD IMPLEMENTATION
 * This represents the cube as two 64 bit integers (One for the 8 corners, another for 12 edges).
 */
class RubiksCubeBitboard : public RubiksCube {
private:
    uint64_t corners;
    uint64_t edges;

    // --- HELPER FUNCTIONS (The "Clear and Set" Logic) ---

    // Reads the 5-bit block for a specific corner (0 to 7)
    uint8_t getCorner(int index) const {
        return (corners >> (5 * index)) & 31ULL; // 31 is 11111 in binary
    }

    // Writes a new 5-bit block to a specific corner slot
    void setCorner(int index, uint8_t value) {
        corners = (corners & ~(31ULL << (5 * index))) | ((uint64_t)value << (5 * index));
    }

    // Reads the 5-bit block for a specific edge (0 to 11)
    uint8_t getEdge(int index) const {
        return (edges >> (5 * index)) & 31ULL; 
    }

    // Writes a new 5-bit block to a specific edge slot
    void setEdge(int index, uint8_t value) {
        edges = (edges & ~(31ULL << (5 * index))) | ((uint64_t)value << (5 * index));
    }

public:
    // --- CONSTRUCTOR ---
    // This builds a perfectly solved cube from scratch
    RubiksCubeBitboard() {
        corners = 0;
        edges = 0;
        
        // A solved corner's 5 bits are just its position number (Orientation is 00)
        for (int i = 0; i < 8; i++) setCorner(i, i); 
        for (int i = 0; i < 12; i++) setEdge(i, i);
    }

    // Public function to get corner data for heuristic calculation (since getCorner is private)
    uint8_t publicGetCorner(int index) const override {
        return getCorner(index);
    }

    // --- MOVE IMPLEMENTATIONS ---
    
    // Turn the UP face clockwise (U)
    RubiksCube& u() override {
        // 1. Read the 4 corners on the top face
        uint8_t c0 = getCorner(0); // Up-Right-Front
        uint8_t c1 = getCorner(1); // Up-Left-Front
        uint8_t c2 = getCorner(2); // Up-Left-Back
        uint8_t c3 = getCorner(3); // Up-Right-Back

        // 2. Shift them clockwise (U move doesn't twist Up/Down orientation!)
        setCorner(0, c3);
        setCorner(1, c0);
        setCorner(2, c1);
        setCorner(3, c2);

        // 3. Read the 4 edges on the top face
        uint8_t e0 = getEdge(0);
        uint8_t e1 = getEdge(1);
        uint8_t e2 = getEdge(2);
        uint8_t e3 = getEdge(3);

        // 4. Shift them clockwise
        setEdge(0, e3);
        setEdge(1, e0);
        setEdge(2, e1);
        setEdge(3, e2);

        return *this;
    }

    // Dummy implementations for the remaining 17 moves to satisfy the compiler
    RubiksCube& u_prime() override { return u().u().u(); }
    RubiksCube& u2() override { return u().u(); }


    RubiksCube& l() override {
        // --- CORNERS (ULB->DLB->DLF->ULF) ---
        uint8_t c1 = getCorner(1); // ULF
        uint8_t c2 = getCorner(2); // ULB
        uint8_t c5 = getCorner(5); // DLF
        uint8_t c6 = getCorner(6); // DLB

        setCorner(6, (c2 & 7) | (((c2 >> 3) + 1) % 3 << 3));
        setCorner(5, (c6 & 7) | (((c6 >> 3) + 2) % 3 << 3));
        setCorner(1, (c5 & 7) | (((c5 >> 3) + 1) % 3 << 3));
        setCorner(2, (c1 & 7) | (((c1 >> 3) + 2) % 3 << 3));

        // --- EDGES (UL->FL->DL->BL) ---
        uint8_t e1 = getEdge(1); // UL
        uint8_t e5 = getEdge(5); // FL
        uint8_t e6 = getEdge(6); // BL
        uint8_t e9 = getEdge(9); // DL

        setEdge(5, e1);
        setEdge(9, e5);
        setEdge(6, e9);
        setEdge(1, e6);

        return *this;
    }
    RubiksCube& l_prime() override { return l().l().l(); }
    RubiksCube& l2() override { return l().l(); }

    RubiksCube& f() override {
        // --- CORNERS ---
        // 1. Read the current state of the 4 Front corners
        uint8_t c0 = getCorner(0); // Up-Right-Front
        uint8_t c1 = getCorner(1); // Up-Left-Front
        uint8_t c4 = getCorner(4); // Down-Right-Front
        uint8_t c5 = getCorner(5); // Down-Left-Front

        // 2. Shift and Twist
        // c1 moves to slot 0 (Twist + 1)
        // c0 moves to slot 4 (Twist + 2)
        // c4 moves to slot 5 (Twist + 1)
        // c5 moves to slot 1 (Twist + 2)
        // Note: & 7 keeps the 3 position bits. >> 3 isolates orientation. % 3 keeps it within 0-2.
        setCorner(0, (c1 & 7) | (((c1 >> 3) + 1) % 3 << 3)); 
        setCorner(4, (c0 & 7) | (((c0 >> 3) + 2) % 3 << 3)); 
        setCorner(5, (c4 & 7) | (((c4 >> 3) + 1) % 3 << 3)); 
        setCorner(1, (c5 & 7) | (((c5 >> 3) + 2) % 3 << 3)); 

        // --- EDGES ---
        // 1. Read the 4 Front edges
        uint8_t e0 = getEdge(0); // Up-Front
        uint8_t e4 = getEdge(4); // Front-Right
        uint8_t e5 = getEdge(5); // Front-Left
        uint8_t e8 = getEdge(8); // Down-Front

        // 2. Shift and Flip
        // Front and Back moves physically flip the edges relative to the Up/Down poles.
        // The ^ 1 (XOR) flips the top orientation bit from 0 to 1, or 1 to 0.
        setEdge(0, (e5 & 15) | (((e5 >> 4) ^ 1) << 4));
        setEdge(4, (e0 & 15) | (((e0 >> 4) ^ 1) << 4));
        setEdge(8, (e4 & 15) | (((e4 >> 4) ^ 1) << 4));
        setEdge(5, (e8 & 15) | (((e8 >> 4) ^ 1) << 4));

        return *this;
    }
    RubiksCube& f_prime() override { return f().f().f(); }
    RubiksCube& f2() override { return f().f(); }

    RubiksCube& r() override {
        // --- CORNERS (URB->URF->DRF->DRB) ---
        uint8_t c0 = getCorner(0); // URF
        uint8_t c3 = getCorner(3); // URB
        uint8_t c4 = getCorner(4); // DRF
        uint8_t c7 = getCorner(7); // DRB

        setCorner(0, (c3 & 7) | (((c3 >> 3) + 1) % 3 << 3));
        setCorner(4, (c0 & 7) | (((c0 >> 3) + 2) % 3 << 3));
        setCorner(7, (c4 & 7) | (((c4 >> 3) + 1) % 3 << 3));
        setCorner(3, (c7 & 7) | (((c7 >> 3) + 2) % 3 << 3));

        // --- EDGES (UR->BR->DR->FR) ---
        uint8_t e3 = getEdge(3);  // UR
        uint8_t e4 = getEdge(4);  // FR
        uint8_t e7 = getEdge(7);  // BR
        uint8_t e11 = getEdge(11); // DR

        setEdge(7, e3);
        setEdge(11, e7);
        setEdge(4, e11);
        setEdge(3, e4);

        return *this;
    }
    RubiksCube& r_prime() override { return r().r().r(); }
    RubiksCube& r2() override { return r().r(); }

    RubiksCube& b() override {
        // --- CORNERS (URB->ULB->DLB->DRB) ---
        uint8_t c2 = getCorner(2); // ULB
        uint8_t c3 = getCorner(3); // URB
        uint8_t c6 = getCorner(6); // DLB
        uint8_t c7 = getCorner(7); // DRB

        setCorner(2, (c3 & 7) | (((c3 >> 3) + 1) % 3 << 3));
        setCorner(6, (c2 & 7) | (((c2 >> 3) + 2) % 3 << 3));
        setCorner(7, (c6 & 7) | (((c6 >> 3) + 1) % 3 << 3));
        setCorner(3, (c7 & 7) | (((c7 >> 3) + 2) % 3 << 3));

        // --- EDGES (UB->BL->DB->BR) ---
        uint8_t e2 = getEdge(2);  // UB
        uint8_t e6 = getEdge(6);  // BL
        uint8_t e7 = getEdge(7);  // BR
        uint8_t e10 = getEdge(10); // DB

        setEdge(6, (e2 & 15) | (((e2 >> 4) ^ 1) << 4));
        setEdge(10, (e6 & 15) | (((e6 >> 4) ^ 1) << 4));
        setEdge(7, (e10 & 15) | (((e10 >> 4) ^ 1) << 4));
        setEdge(2, (e7 & 15) | (((e7 >> 4) ^ 1) << 4));

        return *this;
    }
    RubiksCube& b_prime() override { return b().b().b(); }
    RubiksCube& b2() override { return b().b(); }

    RubiksCube& d() override {
        // --- CORNERS (DRF->DLF->DLB->DRB) ---
        uint8_t c4 = getCorner(4); // DRF
        uint8_t c5 = getCorner(5); // DLF
        uint8_t c6 = getCorner(6); // DLB
        uint8_t c7 = getCorner(7); // DRB

        setCorner(5, c4);
        setCorner(6, c5);
        setCorner(7, c6);
        setCorner(4, c7);

        // --- EDGES (DF->DL->DB->DR) ---
        uint8_t e8 = getEdge(8);   // DF
        uint8_t e9 = getEdge(9);   // DL
        uint8_t e10 = getEdge(10); // DB
        uint8_t e11 = getEdge(11); // DR

        setEdge(9, e8);
        setEdge(10, e9);
        setEdge(11, e10);
        setEdge(8, e11);

        return *this;
    }
    RubiksCube& d_prime() override { return d().d().d(); }
    RubiksCube& d2() override { return d().d(); }


    // Applies a move based on its index (0 to 17)
    void applyMove(int moveIndex) override {
        switch(moveIndex) {
            case 0: u(); break;        case 1: u_prime(); break;        case 2: u2(); break;
            case 3: l(); break;        case 4: l_prime(); break;        case 5: l2(); break;
            case 6: f(); break;        case 7: f_prime(); break;        case 8: f2(); break;
            case 9: r(); break;        case 10: r_prime(); break;       case 11: r2(); break;
            case 12: b(); break;       case 13: b_prime(); break;       case 14: b2(); break;
            case 15: d(); break;       case 16: d_prime(); break;       case 17: d2(); break;
        }
    }

    // Returns the inverse move index to cleanly backtrack
    int getInverseMove(int moveIndex) const override {
        // If it's a half-turn (like U2, L2), it is its own inverse
        if (moveIndex % 3 == 2) return moveIndex;
        // If it's a clockwise turn (like U), the inverse is prime (U')
        if (moveIndex % 3 == 0) return moveIndex + 1;
        // If it's a prime turn (like U'), the inverse is clockwise (U)
        return moveIndex - 1;
    }

    // Helper to print human-readable move notation
    string getMoveName(int moveIndex) const override {
        string names[] = {
            "U", "U'", "U2", "L", "L'", "L2", 
            "F", "F'", "F2", "R", "R'", "R2", 
            "B", "B'", "B2", "D", "D'", "D2"
        };
        return names[moveIndex];
    }




    bool isSolved() const override {
        // In a perfectly solved cube, every corner 'i' contains value 'i', 
        // and every edge 'i' contains value 'i'.
        for (int i = 0; i < 8; i++) {
            if (getCorner(i) != i) return false;
        }
        for (int i = 0; i < 12; i++) {
            if (getEdge(i) != i) return false;
        }
            return true;
    }
    
    void print() const override {
        // We will write a function to convert the bits to text later!
        cout << "Bitboard corners: " << corners << "\n";
        cout << "Bitboard edges: " << edges << "\n";
    }
};



// Global database array
vector<uint8_t> corner_pdb;

// The indexing function
int getCornerIndex(const RubiksCube& cube) {
    int pos[8];
    int ori[8];
    for(int i = 0; i < 8; ++i) {
        uint8_t c = cube.publicGetCorner(i);
        pos[i] = c & 7;      
        ori[i] = c >> 3;     
    }
    
    int pos_rank = 0;
    int factorials[] = {5040, 720, 120, 24, 6, 2, 1}; 
    for (int i = 0; i < 7; i++) {
        int less = 0;
        for (int j = i + 1; j < 8; j++) {
            if (pos[j] < pos[i]) less++;
        }
        pos_rank += less * factorials[i];
    }

    int ori_rank = 0;
    for (int i = 0; i < 7; i++) {
        ori_rank = ori_rank * 3 + ori[i];
    }
    return pos_rank * 2187 + ori_rank;
}


int getCornerHeuristic(const RubiksCube& cube) {
    return corner_pdb[getCornerIndex(cube)];
}


// 3. SOLVING ALGORITHM: NORMAL DFS 
vector<int> current_path;

bool dfs(RubiksCube& cube, int current_depth, int depth_limit) {
    if (cube.isSolved()) return true;
    if (current_depth >= depth_limit) return false;

    for (int i = 0; i < 18; i++) {
        // Optimization: Don't turn the same face twice in a row (e.g., U followed by U')
        if (!current_path.empty() && (current_path.back() / 3 == i / 3)) continue;

        // 1. Walk through the door
        cube.applyMove(i);
        current_path.push_back(i);

        // 2. Explore deeper down this path
        if (dfs(cube, current_depth + 1, depth_limit)) return true;

        // 3. Backtrack (Undo the move)
        current_path.pop_back();
        cube.applyMove(cube.getInverseMove(i));
    }
    return false;
}



// UPDATED DFS WITH CORNER HEURISTIC PRUNING
bool updated_dfs(RubiksCube& cube, int current_depth, int depth_limit) {
    if (cube.isSolved()) return true;

    // --- THE IDA* SNIPER LINE ---
    if (current_depth + getCornerHeuristic(cube) > depth_limit) {
        return false; // Prune this entire branch of billions of states!
    }

    for (int i = 0; i < 18; i++) {
        if (!current_path.empty() && (current_path.back() / 3 == i / 3)) continue;

        cube.applyMove(i);
        current_path.push_back(i);

        if (updated_dfs(cube, current_depth + 1, depth_limit)) return true;

        current_path.pop_back();
        cube.applyMove(cube.getInverseMove(i));
    }
    return false;
}


// 4. SOLVING ALGORITHM: ITERATIVE DEEPENING DFS (IDDFS)
void iddfsSolve(RubiksCube& cube) {
    // We will cap this standard IDDFS at a safe depth of 5 moves for testing 
    // because without a heuristic, searching depth 6+ takes too long!
    int max_depth = 5; // You can increase this for deeper searches, but be warned of exponential time!

    for (int limit = 0; limit <= max_depth; limit++) {
        cout << "Searching depth limit: " << limit << "...\n";
        current_path.clear();
        
        if (dfs(cube, 0, limit)) {
            cout << "\nCUBE SOLVED in " << limit << " moves!\n";
            cout << "Solution sequence: ";
            for (int move : current_path) {
                cout << cube.getMoveName(move) << " ";
            }
            cout << "\n";
            return;
        }
    }
    cout << "Could not solve the cube within the depth limit.\n";
}



// 5. SOLVING ALGORITHM: IDA* (Iterative Deepening A* with Corner Heuristic)
void idaStarSolve(RubiksCube& cube) {
    int max_depth = 22; // Change this to 22 for more challenging scrambles (but be patient!)

    for (int limit = 0; limit <= max_depth; limit++) {
        current_path.clear();

        if (updated_dfs(cube, 0, limit)) {
            for (int move : current_path) {
                cout << cube.getMoveName(move) << " ";
            }

            cout << endl;
            return;
        }
    }
    cout << "Could not solve the cube within the depth limit.\n";
}


int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // --- NEW: LOAD THE PATTERN DATABASE INTO RAM ---
    corner_pdb.resize(88179840);
    ifstream infile("corner_pdb.dat", ios::binary);
    if (!infile) {
        cout << "[ERROR] Could not find corner_pdb.dat! Please run generator first.\n";
        return 1;
    }
    infile.read(reinterpret_cast<char*>(corner_pdb.data()), 88179840);
    infile.close();
    // -----------------------------------------------

    string inputLine;
    while (getline(cin, inputLine)) {
        if (inputLine.empty() || inputLine == "exit") break;

        // RubiksCube3dArray cube;
        RubiksCubeBitboard cube;
        // RubiksCube cube; 
        
        // Parse the input string separated by spaces
        string currentMove = "";
        bool isValid = true;
        
        for (char c : inputLine) {
            if (c == ' ') {
                if (!currentMove.empty()) {
                    // Convert move name to index and apply it
                    int idx = -1;
                    for(int m = 0; m < 18; m++) {
                        if(cube.getMoveName(m) == currentMove) { idx = m; break; }
                    }
                    if (idx != -1) cube.applyMove(idx);
                    currentMove = "";
                }
            } else {
                currentMove += c;
            }
        }
        // Catch the last move if there wasn't a trailing space
        if (!currentMove.empty()) {
            int idx = -1;
            for(int m = 0; m < 18; m++) {
                if(cube.getMoveName(m) == currentMove) { idx = m; break; }
            }
            if (idx != -1) cube.applyMove(idx);
        }

        // Run our lightning fast IDA* search
        // (Make sure your idaStarSolve prints ONLY the solution sequence for cleaner parsing)
        idaStarSolve(cube);
    }

    return 0;
}