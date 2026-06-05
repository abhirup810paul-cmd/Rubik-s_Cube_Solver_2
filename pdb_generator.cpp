#include <fstream> // Make sure this is at the very top of your file!
#include <iostream>
#include <vector>
#include <queue>
#include <fstream>
#include <chrono>
#include <cstdint>

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

    // (In a full implementation, you would add f(), r(), b(), d() here as well)

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

    virtual bool isSolved() const = 0;
    virtual void print() const = 0;
};

// =========================================================================
// PASTE YOUR ENTIRE "RubiksCubeBitboard" CLASS HERE
// (Make sure it includes the publicGetCorner() and applyMove() functions)
// =========================================================================


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
    uint8_t publicGetCorner(int index) const {
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


    // RubiksCube& l() override { return *this; }
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
    // RubiksCube& l_prime() override { return *this; }
    // RubiksCube& l2() override { return *this; }

    // RubiksCube& f() override { return *this; }
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



    // RubiksCube& f_prime() override { return *this; }
    // RubiksCube& f_prime() override {
    //     // --- CORNERS ---
    //     uint8_t c0 = getCorner(0); 
    //     uint8_t c1 = getCorner(1); 
    //     uint8_t c4 = getCorner(4); 
    //     uint8_t c5 = getCorner(5); 

    //     // Reverse the cycle and reverse the twists
    //     setCorner(1, (c0 & 7) | (((c0 >> 3) + 2) % 3 << 3));
    //     setCorner(5, (c1 & 7) | (((c1 >> 3) + 1) % 3 << 3));
    //     setCorner(4, (c5 & 7) | (((c5 >> 3) + 2) % 3 << 3));
    //     setCorner(0, (c4 & 7) | (((c4 >> 3) + 1) % 3 << 3));

    //     // --- EDGES ---
    //     uint8_t e0 = getEdge(0); 
    //     uint8_t e4 = getEdge(4); 
    //     uint8_t e5 = getEdge(5); 
    //     uint8_t e8 = getEdge(8); 

    //     // Reverse the cycle, but keep the XOR flip (^ 1)
    //     setEdge(5, (e0 & 15) | (((e0 >> 4) ^ 1) << 4));
    //     setEdge(0, (e4 & 15) | (((e4 >> 4) ^ 1) << 4));
    //     setEdge(4, (e8 & 15) | (((e8 >> 4) ^ 1) << 4));
    //     setEdge(8, (e5 & 15) | (((e5 >> 4) ^ 1) << 4));

    //     return *this;
    // }

    // RubiksCube& f2() override { return *this; }
    // RubiksCube& f2() override {
    //     // --- CORNERS ---
    //     uint8_t c0 = getCorner(0); 
    //     uint8_t c1 = getCorner(1); 
    //     uint8_t c4 = getCorner(4); 
    //     uint8_t c5 = getCorner(5); 

    //     // 180 degree diagonal swap, NO orientation math needed!
    //     setCorner(0, c5);
    //     setCorner(5, c0);
    //     setCorner(1, c4);
    //     setCorner(4, c1);

    //     // --- EDGES ---
    //     uint8_t e0 = getEdge(0); 
    //     uint8_t e4 = getEdge(4); 
    //     uint8_t e5 = getEdge(5); 
    //     uint8_t e8 = getEdge(8); 

    //     // 180 degree direct cross swap, NO orientation math needed!
    //     setEdge(0, e8);
    //     setEdge(8, e0);
    //     setEdge(4, e5);
    //     setEdge(5, e4);

    //     return *this;
    // }


    // RubiksCube& r() override { return *this; }
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
    // RubiksCube& r_prime() override { return *this; }
    // RubiksCube& r2() override { return *this; }

    // RubiksCube& b() override { return *this; }
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
    // RubiksCube& b_prime() override { return *this; }
    // RubiksCube& b2() override { return *this; }


    // RubiksCube& d() override { return *this; }
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
    // RubiksCube& d_prime() override { return *this; }
    // RubiksCube& d2() override { return *this; }
    
    // bool isSolved() const override {
    //     return true; // Placeholder
    // }




    // Applies a move based on its index (0 to 17)
    void applyMove(int moveIndex) {
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
    int getInverseMove(int moveIndex) {
        // If it's a half-turn (like U2, L2), it is its own inverse
        if (moveIndex % 3 == 2) return moveIndex;
        // If it's a clockwise turn (like U), the inverse is prime (U')
        if (moveIndex % 3 == 0) return moveIndex + 1;
        // If it's a prime turn (like U'), the inverse is clockwise (U)
        return moveIndex - 1;
    }

    // Helper to print human-readable move notation
    string getMoveName(int moveIndex) {
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



// // Global database array
// vector<uint8_t> corner_pdb;

// // The indexing function
// int getCornerIndex(const RubiksCubeBitboard& cube) {
//     int pos[8];
//     int ori[8];
//     for(int i = 0; i < 8; ++i) {
//         uint8_t c = cube.publicGetCorner(i);
//         pos[i] = c & 7;      
//         ori[i] = c >> 3;     
//     }
    
//     int pos_rank = 0;
//     int factorials[] = {5040, 720, 120, 24, 6, 2, 1}; 
//     for (int i = 0; i < 7; i++) {
//         int less = 0;
//         for (int j = i + 1; j < 8; j++) {
//             if (pos[j] < pos[i]) less++;
//         }
//         pos_rank += less * factorials[i];
//     }

//     int ori_rank = 0;
//     for (int i = 0; i < 7; i++) {
//         ori_rank = ori_rank * 3 + ori[i];
//     }
//     return pos_rank * 2187 + ori_rank;
// }


int getCornerHeuristic(const RubiksCubeBitboard& cube) {
    int total_twists = 0;
    
    for (int i = 0; i < 8; i++) {
        uint8_t cornerData = cube.publicGetCorner(i);
        uint8_t orientation = cornerData >> 3; // Isolate the top 2 orientation bits
        
        if (orientation != 0) {
            total_twists += 1; // This corner is twisted!
        }
    }
    
    // Ceil division: (total_twists + 3) / 4
    // If 1 corner is twisted, we need at least 1 move ( (1+3)/4 = 1 )
    // If 5 corners are twisted, we need at least 2 moves ( (5+3)/4 = 2 )
    return (total_twists + 3) / 4;
}


// int getCornerHeuristic(const RubiksCubeBitboard& cube) {
//     return corner_pdb[getCornerIndex(cube)];
// }


// 8! permutations of corners * 3^7 orientations = 88,179,840 total corner states
const int PDB_SIZE = 88179840; 

// The database: A massive array where the index is the cube state, 
// and the value is the minimum number of moves to solve it.
// Initialized to 255 (which represents "unvisited").
vector<uint8_t> corner_pdb(PDB_SIZE, 255); 

/**
 * MATHEMATICAL INDEXING:
 * Compresses the physical layout of the 8 corners into a single unique integer 
 * between 0 and 88,179,839 so we can look it up in an array instantly.
 */
int getCornerIndex(const RubiksCubeBitboard& cube) {
    int pos[8];
    int ori[8];
    for(int i = 0; i < 8; ++i) {
        uint8_t c = cube.publicGetCorner(i);
        pos[i] = c & 7;      // Bottom 3 bits are position
        ori[i] = c >> 3;     // Top 2 bits are orientation
    }
    
    // 1. Calculate Permutation Rank (Lexicographic order of the 8 pieces)
    int pos_rank = 0;
    int factorials[] = {5040, 720, 120, 24, 6, 2, 1}; // 7! down to 1!
    for (int i = 0; i < 7; i++) {
        int less = 0;
        for (int j = i + 1; j < 8; j++) {
            if (pos[j] < pos[i]) less++;
        }
        pos_rank += less * factorials[i];
    }

    // 2. Calculate Orientation Rank (Base-3 integer)
    int ori_rank = 0;
    for (int i = 0; i < 7; i++) {
        ori_rank = ori_rank * 3 + ori[i];
    }

    // Combine them into a single unique ID
    return pos_rank * 2187 + ori_rank;
}

void generatePDB() {
    cout << "[!] Allocating 88MB RAM for Pattern Database...\n";
    
    RubiksCubeBitboard solved_cube;
    int start_index = getCornerIndex(solved_cube);
    
    // The solved state takes 0 moves to solve
    corner_pdb[start_index] = 0; 
    
    // Setup the BFS queue
    queue<RubiksCubeBitboard> q;
    q.push(solved_cube);
    
    int visited = 1;
    int current_depth = 0;
    
    cout << "[+] Starting Breadth-First Search (This will take a few minutes)...\n";
    cout << "--------------------------------------------------\n";
    
    while (!q.empty()) {
        int level_size = q.size();
        cout << "Depth " << current_depth << " | Branches to explore: " << level_size << "\n";
        
        for (int i = 0; i < level_size; i++) {
            RubiksCubeBitboard curr = q.front();
            q.pop();
            
            int curr_idx = getCornerIndex(curr);
            uint8_t curr_dist = corner_pdb[curr_idx];
            
            // Branch out to all 18 possible moves
            for (int m = 0; m < 18; m++) {
                RubiksCubeBitboard next_cube = curr;
                next_cube.applyMove(m);
                
                int next_idx = getCornerIndex(next_cube);
                
                // If we have never seen this specific state before, save it!
                if (corner_pdb[next_idx] == 255) {
                    corner_pdb[next_idx] = curr_dist + 1;
                    q.push(next_cube);
                    visited++;
                }
            }
        }
        current_depth++;
        
        // If we found all 88 million states, stop searching!
        if (visited == PDB_SIZE) break; 
    }
    
    cout << "--------------------------------------------------\n";
    cout << "[+] BFS Complete! Unique states discovered: " << visited << " / " << PDB_SIZE << "\n";
    
    cout << "[!] Saving structural data to 'corner_pdb.dat'...\n";
    ofstream outfile("corner_pdb.dat", ios::binary);
    outfile.write(reinterpret_cast<char*>(corner_pdb.data()), PDB_SIZE);
    outfile.close();
    cout << "[+] File saved successfully directly to disk!\n";
}

int main() {
    auto start = chrono::high_resolution_clock::now();
    
    generatePDB();
    
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;
    cout << "\n[+] Execution completed in: " << elapsed.count() << " seconds.\n";
    
    return 0;
}



