// chess_engine.cpp
#include <bits/stdc++.h>
#include <cstdint>
#include <chrono>

// Constants
constexpr int BOARD_SIZE = 64;
constexpr int MAX_MOVES = 218; // Maximum possible moves in a position
constexpr int INF = 1000000;

// Piece definitions
enum Piece {
    WP, WN, WB, WR, WQ, WK,
    BP, BN, BB, BR, BQ, BK,
    EMPTY
};

// Directions for sliding pieces
// Define separate direction arrays
const int bishop_directions[4] = {-9, -7, 7, 9};  // Diagonal directions
const int rook_directions[4] = {-8, -1, 1, 8};    // Vertical and horizontal directions
const int queen_directions[8] = {-9, -8, -7, -1, 1, 7, 8, 9}; // All directions

const int piece_value[2][5] = {
        {124, 781, 825, 1276, 2538}, // middle game
        {206, 854, 915, 1380, 2682}  // end game
};

const int psqt[2][5][8][4] = {
        {
                {
                    {-175, -92, -74, -73},
                    {-77, -41, -27, -15},
                    {-61, -17, 6, 12},
                    {-35, 8, 40, 49},
                    {-34, 13, 44, 51},
                    {-9, 22, 58, 53},
                    {-67, -27, 4, 37},
                    {-201, -83, -56, -26}
                    },
                {{-53, -5, -8, -23}, {-15, 8, 19, 4}, {-7, 21, -5, 17}, {-5, 11, 25, 39}, {-12, 29, 22, 31},
                 {-16, 6, 1, 11}, {-17, -14, 5, 0}, {-48, 1, -14, -23}},
                {{-31, -20, -14, -5}, {-21, -13, -8, 6}, {-25, -11, -1, 3}, {-13, -5, -4, -6}, {-27, -15, -4, 3},
                 {-22, -2, 6, 12}, {-2, 12, 16, 18}, {-17, -19, -1, 9}},
                {{3, -5, -5, 4}, {-3, 5, 8, 12}, {-3, 6, 13, 7}, {4, 5, 9, 8}, {0, 14, 12, 5}, {-4, 10, 6, 8},
                 {-5, 6, 10, 8}, {-2, -2, 1, -2}},
                {{271, 327, 271, 198}, {278, 303, 234, 179}, {195, 258, 169, 120}, {164, 190, 138, 98},
                 {154, 179, 105, 70}, {123, 145, 81, 31}, {88, 120, 65, 33}, {59, 89, 45, -1}}
        },
        {
                {{-96,-65,-49,-21},{-67,-54,-18,8},{-40,-27,-8,29},{-35,-2,13,28},{-45,-16,9,39},{-51,-44,-16,17},{-69,-50,-51,12},{-100,-88,-56,-17}},
                {{-57,-30,-37,-12},{-37,-13,-17,1},{-16,-1,-2,10},{-20,-6,0,17},{-17,-1,-14,15},{-30,6,4,6},{-31,-20,-1,1},{-46,-42,-37,-24}},
                {{-9,-13,-10,-9},{-12,-9,-1,-2},{6,-8,-2,-6},{-6,1,-9,7},{-5,8,7,-6},{6,1,-7,10},{4,5,20,-5},{18,0,19,13}},
                {{-69,-57,-47,-26},{-55,-31,-22,-4},{-39,-18,-9,3},{-23,-3,13,24},{-29,-6,9,21},{-38,-18,-12,1},{-50,-27,-24,-8},{-75,-52,-43,-36}},
                {{1,45,85,76},{53,100,133,135},{88,130,169,175},{103,156,172,172},{96,166,199,199},{92,172,184,191},{47,121,116,131},{11,59,73,78}}
        }
};

const int pawn_psqt[2][8][8] = {
        {{0,0,0,0,0,0,0,0},{3,3,10,19,16,19,7,-5},{-9,-15,11,15,32,22,5,-22},{-4,-23,6,20,40,17,4,-8},{13,0,-13,1,11,-2,-13,5},
         {5,-12,-7,22,-8,-5,-15,-8},{-7,7,-3,-13,5,-16,10,-8},{0,0,0,0,0,0,0,0}},
        {{0,0,0,0,0,0,0,0},{-10,-6,10,0,14,7,-5,-19},{-10,-10,-10,4,4,3,-6,-4},{6,-2,-8,-4,-13,-12,-10,-9},{10,5,4,-5,-5,-5,14,9},
         {28,20,21,28,30,7,6,13},{0,-11,12,21,25,19,4,7},{0,0,0,0,0,0,0,0}}
};

// Bitboard typedef
typedef uint64_t Bitboard;

// Board structure
struct Board {
    Bitboard pieces[12] = {0}; // WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK
    Bitboard occupancy[3] = {0}; // White, Black
    bool white_to_move = true;
    bool castling_rights[4] = {true, true, true, true}; // WK, WQ, BK, BQ
    int en_passant = -1; // Square index for en passant
    int ply = 0; // Half-move count
    int fullmove_number = 1; // Full move number

    // Converts algebraic square notation to square index
    static int algebraic_to_index(const std::string &square) {
        if (square.length() != 2) return -1; // Invalid format
        char file = square[0];
        char rank = square[1];
        if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return -1; // Invalid characters
        int file_idx = file - 'a';
        int rank_idx = rank - '1';
        return rank_idx * 8 + file_idx;
    }

    // Imports board position from FEN string
    void import_fen(const std::string &fen) {
        // Reset all bitboards
        for (int p = 0; p < 12; ++p) pieces[p] = 0;
        occupancy[0] = 0;
        occupancy[1] = 0;
        occupancy[2] = 0;
        white_to_move = true;
        for (int i = 0; i < 4; ++i) castling_rights[i] = false;
        en_passant = -1;
        ply = 0;
        fullmove_number = 1;

        // Split the FEN string into its six fields
        std::stringstream ss(fen);
        std::string piece_placement, active_color, castling, en_passant_str, halfmove_str, fullmove_str;
        ss >> piece_placement >> active_color >> castling >> en_passant_str >> halfmove_str >> fullmove_str;

        // Process piece placement
        int square = 56; // Start from a8 (56) to h8 (63)
        for (char c: piece_placement) {
            if (c == '/') {
                square -= 16; // Move to the next rank down (a7)
                continue;
            }
            if (isdigit(c)) {
                square += (c - '0'); // Skip empty squares
            } else {
                int piece = -1;
                switch (c) {
                    case 'P':
                        piece = 0;
                        break; // WP
                    case 'N':
                        piece = 1;
                        break; // WN
                    case 'B':
                        piece = 2;
                        break; // WB
                    case 'R':
                        piece = 3;
                        break; // WR
                    case 'Q':
                        piece = 4;
                        break; // WQ
                    case 'K':
                        piece = 5;
                        break; // WK
                    case 'p':
                        piece = 6;
                        break; // BP
                    case 'n':
                        piece = 7;
                        break; // BN
                    case 'b':
                        piece = 8;
                        break; // BB
                    case 'r':
                        piece = 9;
                        break; // BR
                    case 'q':
                        piece = 10;
                        break; // BQ
                    case 'k':
                        piece = 11;
                        break; // BK
                    default:
                        piece = -1;
                        break; // Invalid piece
                }
                if (piece != -1) {
                    pieces[piece] |= (1ULL << square);
                    if (piece < 6) {
                        occupancy[0] |= (1ULL << square); // White
                    } else {
                        occupancy[1] |= (1ULL << square); // Black
                    }
                    occupancy[2] |= (1ULL << square); // All
                }
                square++;
            }
        }

        // Active color
        white_to_move = (active_color == "w");

        // Castling rights
        if (castling != "-") {
            for (char c: castling) {
                switch (c) {
                    case 'K':
                        castling_rights[0] = true;
                        break; // White Kingside
                    case 'Q':
                        castling_rights[1] = true;
                        break; // White Queenside
                    case 'k':
                        castling_rights[2] = true;
                        break; // Black Kingside
                    case 'q':
                        castling_rights[3] = true;
                        break; // Black Queenside
                    default:
                        break; // Ignore invalid characters
                }
            }
        }

        // En passant
        if (en_passant_str != "-") {
            en_passant = algebraic_to_index(en_passant_str);
            // Ensure it's a valid square (optional)
            if (en_passant < 0 || en_passant > 63) {
                en_passant = -1; // Invalid en passant square
            }
        }

        // Halfmove clock
        if (!halfmove_str.empty()) {
            ply = std::stoi(halfmove_str);
        }

        // Fullmove number
        if (!fullmove_str.empty()) {
            fullmove_number = std::stoi(fullmove_str);
        }
    }

    // Initialize to starting position
    void initialize() {
        // White pieces
        pieces[WP] = 0x000000000000FF00;
        pieces[WN] = 0x0000000000000042;
        pieces[WB] = 0x0000000000000024;
        pieces[WR] = 0x0000000000000081;
        pieces[WQ] = 0x0000000000000008;
        pieces[WK] = 0x0000000000000010;
        // Black pieces
        pieces[BP] = 0x00FF000000000000;
        pieces[BN] = 0x4200000000000000;
        pieces[BB] = 0x2400000000000000;
        pieces[BR] = 0x8100000000000000;
        pieces[BQ] = 0x0800000000000000;
        pieces[BK] = 0x1000000000000000;
        // Occupancy
        occupancy[0] = pieces[WP] | pieces[WN] | pieces[WB] | pieces[WR] | pieces[WQ] | pieces[WK];
        occupancy[1] = pieces[BP] | pieces[BN] | pieces[BB] | pieces[BR] | pieces[BQ] | pieces[BK];
        occupancy[2] = occupancy[0] | occupancy[1];
    }

    // Get piece at square
    int get_piece(int square) const {
        for (int p = 0; p < 12; p++) {
            if (pieces[p] & (1ULL << square)) return p;
        }
        return EMPTY;
    }

    // Set piece at square
    void set_piece(int square, int piece) {
        for (unsigned long long &p: pieces) {
            p &= ~(1ULL << square);
        }
        if (piece != EMPTY) {
            pieces[piece] |= (1ULL << square);
            occupancy[piece < 6 ? 0 : 1] |= (1ULL << square);
            occupancy[2] |= (1ULL << square);
        } else {
            occupancy[0] &= ~(1ULL << square);
            occupancy[1] &= ~(1ULL << square);
            occupancy[2] &= ~(1ULL << square);
        }
    }
};

// Move structure
struct Move {
    int from;
    int to;
    int promotion; // EMPTY if not a promotion

    explicit Move(int f = 0, int t = 0, int p = EMPTY) : from(f), to(t), promotion(p) {}
};

// Utility functions
inline int pop_lsb(Bitboard &bb) {
    int sq = __builtin_ctzll(bb);
    bb &= bb - 1;
    return sq;
}

int pinned_direction(const Board& board, int square) {
    // Check if the square has a piece
    int piece = board.get_piece(square);
    if(piece == EMPTY) return 0;

    // Determine color: 1 for White, -1 for Black
    int color = (piece < 6) ? 1 : -1;

    // Define direction vectors and their corresponding direction codes
    struct Direction {
        int dx;
        int dy;
        int code;
    };

    Direction directions[8] = {
            {1, 0, 1},   // East - horizontal
            {-1, 0, 1},  // West - horizontal
            {0, 1, 3},   // North - vertical
            {0, -1, 3},  // South - vertical
            {1, 1, 2},   // Northeast - topleft to bottomright
            {-1, 1, 4},  // Northwest - topright to bottomleft
            {-1, -1, 4}, // Southwest - topright to bottomleft
            {1, -1, 2}    // Southeast - topleft to bottomright
    };

    // Helper functions to get file and rank from square
    auto get_file = [](int sq) -> int { return sq %8; };
    auto get_rank = [](int sq) -> int { return sq /8; };

    int x = get_file(square);
    int y = get_rank(square);

    for(int i = 0; i < 8; i++){
        int dx = directions[i].dx;
        int dy = directions[i].dy;
        int code = directions[i].code;

        int current_x = x + dx;
        int current_y = y + dy;

        bool king_found = false;

        // Traverse in the direction until a blocker or the edge of the board
        while(current_x >=0 && current_x <8 && current_y >=0 && current_y <8){
            int current_square = current_y *8 + current_x;
            int current_piece = board.get_piece(current_square);

            if(current_piece == EMPTY){
                current_x += dx;
                current_y += dy;
                continue;
            }

            if( (current_piece == (color ==1 ? WK : BK)) ){
                king_found = true;
                break;
            }
            else{
                // A piece is blocking, not the king
                break;
            }
        }

        if(king_found){
            // Now, check in the opposite direction for an enemy sliding piece that can attack along this direction.
            int opp_x = x - dx;
            int opp_y = y - dy;

            while(opp_x >=0 && opp_x <8 && opp_y >=0 && opp_y <8){
                int opp_square = opp_y *8 + opp_x;
                int opp_piece = board.get_piece(opp_square);

                if(opp_piece == EMPTY){
                    opp_x -= dx;
                    opp_y -= dy;
                    continue;
                }

                // Check if the piece is an enemy sliding piece that can attack along this direction.
                bool is_enemy = (color ==1 && opp_piece >= BP && opp_piece <= BK) ||
                                (color ==-1 && opp_piece >= WP && opp_piece <= WK);

                if(is_enemy){
                    bool is_attacking_piece = false;
                    // Determine if the enemy piece can attack in this direction
                    if(code ==1){ // Horizontal
                        if(opp_piece == WR || opp_piece == WQ || opp_piece == BR || opp_piece == BQ){
                            is_attacking_piece = true;
                        }
                    }
                    else if(code ==2 || code ==4){ // Diagonal
                        if(opp_piece == WB || opp_piece == WQ || opp_piece == BB || opp_piece == BQ){
                            is_attacking_piece = true;
                        }
                    }
                    else if(code ==3){ // Vertical
                        if(opp_piece == WR || opp_piece == WQ || opp_piece == BR || opp_piece == BQ){
                            is_attacking_piece = true;
                        }
                    }

                    if(is_attacking_piece){
                        // Direction code is 1,2,3,4. Multiply by color.
                        return code * color;
                    }
                }

                // If enemy piece is not a sliding attacker, no pin.
                break;
            }
        }
    }

    // No pin found
    return 0;
}

int pinned(const Board& board, int square) {
    return (pinned_direction(board, square) > 0) ? 1 : 0;
}

// Helper function: Converts (x, y) to square index
inline int xy_to_square(int x, int y) {
    if (x < 0 || x > 7 || y < 0 || y > 7) return -1;
    return y * 8 + x;
}

// Counts the number of attacks on 'square' by enemy knights
int knight_attack(const Board& board, int square, int s2 = -1) {
    if (square < 0 || square >= 64) return 0; // Invalid square

    int attack_count = 0;
    int x = square % 8;
    int y = square / 8;

    // Define all 8 possible knight move offsets
    const int knight_offsets[8][2] = {
            {2, 1}, {1, 2}, {-1, 2}, {-2, 1},
            {-2, -1}, {-1, -2}, {1, -2}, {2, -1}
    };

    for (int i = 0; i < 8; ++i) {
        int target_x = x + knight_offsets[i][0];
        int target_y = y + knight_offsets[i][1];
        int target_square = xy_to_square(target_x, target_y);
        if (target_square == -1) continue; // Out of bounds

        // If 's2' is specified, skip other squares
        if (s2 != -1 && target_square != s2) continue;

        int piece = board.get_piece(target_square);
        bool is_enemy_knight = board.white_to_move ? (piece == BN) : (piece == WN);
        if (is_enemy_knight && !pinned(board, target_square)) {
            attack_count++;
        }
    }

    return attack_count;
}

// Counts the number of attacks on 'square' by enemy bishops (including x-ray through queens)
int bishop_xray_attack(const Board& board, int square, int s2 = -1) {
    if (square < 0 || square >= 64) return 0; // Invalid square

    int attack_count = 0;
    int x = square % 8;
    int y = square / 8;

    // Define the four diagonal directions
    const int bishop_directions[4][2] = {
            {1, 1}, {-1, 1}, {-1, -1}, {1, -1}
    };

    for (int i = 0; i < 4; ++i) {
        int dx = bishop_directions[i][0];
        int dy = bishop_directions[i][1];
        int target_x = x + dx;
        int target_y = y + dy;

        while (target_x >= 0 && target_x < 8 && target_y >= 0 && target_y < 8) {
            int target_square = xy_to_square(target_x, target_y);
            if (target_square == -1) break;

            int piece = board.get_piece(target_square);

            if (piece == EMPTY) {
                // Continue scanning in this direction
                target_x += dx;
                target_y += dy;
                continue;
            }

            // If 's2' is specified and doesn't match, skip
            if (s2 != -1 && target_square != s2) break;

            bool is_enemy_bishop_or_queen = board.white_to_move ?
                                            (piece == BB || piece == BQ) :
                                            (piece == WB || piece == WQ);

            if (is_enemy_bishop_or_queen && !pinned(board, target_square)) {
                attack_count++;
            }

            break; // Blocked by any piece
        }
    }

    return attack_count;
}

// Counts the number of attacks on 'square' by enemy rooks (including x-ray through queens)
int rook_xray_attack(const Board& board, int square, int s2 = -1) {
    if (square < 0 || square >= 64) return 0; // Invalid square

    int attack_count = 0;
    int x = square % 8;
    int y = square / 8;

    // Define the four straight directions
    const int rook_directions[4][2] = {
            {1, 0}, {-1, 0}, {0, 1}, {0, -1}
    };

    for (int i = 0; i < 4; ++i) {
        int dx = rook_directions[i][0];
        int dy = rook_directions[i][1];
        int target_x = x + dx;
        int target_y = y + dy;

        while (target_x >= 0 && target_x < 8 && target_y >= 0 && target_y < 8) {
            int target_square = xy_to_square(target_x, target_y);
            if (target_square == -1) break;

            int piece = board.get_piece(target_square);

            if (piece == EMPTY) {
                // Continue scanning in this direction
                target_x += dx;
                target_y += dy;
                continue;
            }

            // If 's2' is specified and doesn't match, skip
            if (s2 != -1 && target_square != s2) break;

            bool is_enemy_rook_or_queen = board.white_to_move ?
                                          (piece == BR || piece == BQ) :
                                          (piece == WR || piece == WQ);

            if (is_enemy_rook_or_queen && !pinned(board, target_square)) {
                attack_count++;
            }

            break; // Blocked by any piece
        }
    }

    return attack_count;
}

// Counts the number of attacks on 'square' by enemy queens
int queen_attack(const Board& board, int square, int s2 = -1) {
    if (square < 0 || square >= 64) return 0; // Invalid square

    int attack_count = 0;
    int x = square % 8;
    int y = square / 8;

    // Define all eight directions (diagonal and straight)
    const int queen_directions[8][2] = {
            {1, 1}, {-1, 1}, {-1, -1}, {1, -1},
            {1, 0}, {-1, 0}, {0, 1}, {0, -1}
    };

    for (int i = 0; i < 8; ++i) {
        int dx = queen_directions[i][0];
        int dy = queen_directions[i][1];
        int target_x = x + dx;
        int target_y = y + dy;

        while (target_x >= 0 && target_x < 8 && target_y >= 0 && target_y < 8) {
            int target_square = xy_to_square(target_x, target_y);
            if (target_square == -1) break;

            int piece = board.get_piece(target_square);

            if (piece == EMPTY) {
                // Continue scanning in this direction
                target_x += dx;
                target_y += dy;
                continue;
            }

            // If 's2' is specified and doesn't match, skip
            if (s2 != -1 && target_square != s2) break;

            bool is_enemy_queen = board.white_to_move ?
                                  (piece == BQ) :
                                  (piece == WQ);

            if (is_enemy_queen && !pinned(board, target_square)) {
                attack_count++;
            }

            break; // Blocked by any piece
        }
    }

    return attack_count;
}

// Counts the rank of a given square or sums ranks for all squares if square == -1
int rank(const Board& board, int square = -1) {
    if (square == -1) {
        int total_rank = 0;
        for(int sq = 0; sq < 64; ++sq){
            int y = sq / 8;
            total_rank += (8 - y);
        }
        return total_rank;
    }

    if (square < 0 || square >= 64) return 0; // Invalid square

    int y = square / 8;
    return 8 - y;
}

// Counts blockers for the king at a given square or sums for all squares if square == -1
int blockers_for_king(const Board& board, int square = -1) {
    if (square == -1) {
        int total_blockers = 0;
        for(int sq = 0; sq < 64; ++sq){
            total_blockers += blockers_for_king(board, sq);
        }
        return total_blockers;
    }

    if (square < 0 || square >= 64) return 0; // Invalid square

    // Get (x, y) coordinates
    int x = square % 8;
    int y = square / 8;

    // Color-flipped board to analyze from the opponent's perspective
    Board flipped = colorflip(board);

    // Convert to flipped coordinates
    int flipped_square = xy_to_square(x, 7 - y);

    // Check if the piece at flipped_square is pinned
    if (flipped.pinned(flipped, flipped_square)) {
        return 1;
    }

    return 0;
}

// Determines if a square is within the mobility area
// Returns 1 if in mobility area, 0 otherwise
int mobility_area(const Board& board, int square = -1) {
    if (square == -1) {
        int total_mobility = 0;
        for(int sq = 0; sq < 64; ++sq){
            total_mobility += mobility_area(board, sq);
        }
        return total_mobility;
    }

    if (square < 0 || square >= 64) return 0; // Invalid square

    // Get (x, y) coordinates
    int x = square % 8;
    int y = square / 8;

    // Piece at the square
    int piece = board.get_piece(square);

    // If the piece is King or Queen, exclude from mobility area
    if (piece == WK || piece == BK || piece == WQ || piece == BQ) {
        return 0;
    }

    // Check for enemy pawns protecting the square
    // Assuming white_to_move indicates the side to move, so enemy pawns are black pawns if white_to_move is true
    bool is_white_to_move = board.white_to_move;
    // Enemy pawns attack diagonally forward
    int enemy_pawn1 = is_white_to_move ? BP : WP;
    int enemy_pawn2 = is_white_to_move ? BP : WP;

    // Positions of enemy pawns that can attack this square
    int pawn_attack1 = xy_to_square(x - 1, y - 1);
    int pawn_attack2 = xy_to_square(x + 1, y - 1);

    if (pawn_attack1 != -1 && board.get_piece(pawn_attack1) == enemy_pawn1) return 0;
    if (pawn_attack2 != -1 && board.get_piece(pawn_attack2) == enemy_pawn2) return 0;

    // Check if the piece is a Pawn and handle mobility area exclusions
    if (piece == WP || piece == BP) {
        // Get rank of the pawn
        int pawn_rank = rank(board, square);
        // Pawns on ranks 2 and 3 (for white pawns, or 7 and 6 for black pawns)
        if (is_white_to_move) {
            if (pawn_rank < 4) return 0; // White pawns on ranks 1-3
            // Check if the pawn is blocked by another piece
            int behind_square = xy_to_square(x, y - 1);
            if (behind_square != -1 && board.get_piece(behind_square) != EMPTY) return 0;
        }
        else {
            if (pawn_rank < 4) return 0; // Black pawns on ranks 1-3
            // Check if the pawn is blocked by another piece
            int behind_square = xy_to_square(x, y + 1);
            if (behind_square != -1 && board.get_piece(behind_square) != EMPTY) return 0;
        }
    }

    // Check for blockers for the king
    // Assuming the square corresponds to a piece that could block the king
    // Use colorflip to analyze from opponent's perspective
    Board flipped = colorflip(board);
    int flipped_square = xy_to_square(x, 7 - (y));
    if (blockers_for_king(flipped, flipped_square)) {
        return 0;
    }

    // All conditions passed; the square is within the mobility area
    return 1;
}

// Counts the mobility of a piece at a given square or sums for all squares if square == -1
int mobility(const Board& board, int square = -1) {
    if (square == -1) {
        int total_mobility = 0;
        for(int sq = 0; sq < 64; ++sq){
            total_mobility += mobility(board, sq);
        }
        return total_mobility;
    }

    if (square < 0 || square >= 64) return 0; // Invalid square

    // Get the piece at the square
    int piece = board.get_piece(square);

    // Check if the piece is one of N, B, R, Q
    if (piece != WN && piece != BN && piece != WB && piece != BB &&
        piece != WR && piece != BR && piece != WQ && piece != BQ) {
        return 0;
    }

    int mobility_count = 0;

    // Iterate through all squares on the board
    for(int x = 0; x < 8; ++x){
        for(int y = 0; y < 8; ++y){
            int target_square = xy_to_square(x, y);
            if(target_square == -1) continue;

            // Check if the target square is within the mobility area
            if(!mobility_area(board, target_square)) continue;

            // Depending on the piece type, check attacks
            if(piece == WN || piece == BN){
                // Knight attack
                if(knight_attack(board, target_square, square) && board.get_piece(target_square) != WQ && board.get_piece(target_square) != BQ){
                    mobility_count++;
                }
            }
            else if(piece == WB || piece == BB){
                // Bishop x-ray attack
                if(bishop_xray_attack(board, target_square, square) && board.get_piece(target_square) != WQ && board.get_piece(target_square) != BQ){
                    mobility_count++;
                }
            }
            else if(piece == WR || piece == BR){
                // Rook x-ray attack
                if(rook_xray_attack(board, target_square, square)){
                    mobility_count++;
                }
            }
            else if(piece == WQ || piece == BQ){
                // Queen attack
                if(queen_attack(board, target_square, square)){
                    mobility_count++;
                }
            }
        }
    }

    return mobility_count;
}

// Assigns mobility bonuses based on piece type and mobility count
int mobility_bonus(const Board& board, int square = -1, bool mg = true) {
    if (square == -1) {
        int total_bonus = 0;
        for(int sq = 0; sq < 64; ++sq){
            total_bonus += mobility_bonus(board, sq, mg);
        }
        return total_bonus;
    }

    if (square < 0 || square >= 64) return 0; // Invalid square

    // Get the piece at the square
    int piece = board.get_piece(square);

    // Check if the piece is one of N, B, R, Q
    if (piece != WN && piece != BN && piece != WB && piece != BB &&
        piece != WR && piece != BR && piece != WQ && piece != BQ) {
        return 0;
    }

    // Define the bonus tables for middlegame and endgame
    const std::vector<std::vector<int>> mg_bonus = {
            {-62, -53, -12, -4, 3, 13, 22, 28, 33},                            // Knight
            {-48, -20, 16, 26, 38, 51, 55, 63, 63, 68, 81, 81, 91, 98},      // Bishop
            {-60, -20, 2, 3, 3, 11, 22, 31, 40, 40, 41, 48, 57, 57, 62},     // Rook
            {-30, -12, -8, -9, 20, 23, 23, 35, 38, 53, 64, 65, 65, 66, 67, 67, 72, 72, 77, 79, 93, 108, 108, 108, 110, 114, 114, 116} // Queen
    };

    const std::vector<std::vector<int>> eg_bonus = {
            {-81, -56, -31, -16, 5, 11, 17, 20, 25},                                 // Knight
            {-59, -23, -3, 13, 24, 42, 54, 57, 65, 73, 78, 86, 88, 97},              // Bishop
            {-78, -17, 23, 39, 70, 99, 103, 121, 134, 139, 158, 164, 168, 169, 172}, // Rook
            {-48, -30, -7, 19, 40, 55, 59, 75, 78, 96, 96, 100, 121, 127, 131, 133, 136, 141, 147, 150, 151, 168, 168, 171, 182, 182, 192, 219} // Queen
    };

    // Determine the index for the bonus table based on piece type
    int bonus_index = -1;
    if(piece == WN || piece == BN){
        bonus_index = 0; // Knight
    }
    else if(piece == WB || piece == BB){
        bonus_index = 1; // Bishop
    }
    else if(piece == WR || piece == BR){
        bonus_index = 2; // Rook
    }
    else if(piece == WQ || piece == BQ){
        bonus_index = 3; // Queen
    }

    if(bonus_index == -1) return 0; // Should not happen

    // Get mobility count for the piece
    int mobility_count = mobility(board, square);

    // Select the appropriate bonus table
    const std::vector<std::vector<int>>* bonus_table = mg ? &mg_bonus : &eg_bonus;

    // Ensure mobility_count is within the bonus table range
    if(mobility_count < 0){
        mobility_count = 0;
    }
    else if(mobility_count >= (*bonus_table)[bonus_index].size()){
        mobility_count = (*bonus_table)[bonus_index].size() - 1;
    }

    // Retrieve and return the bonus
    return (*bonus_table)[bonus_index][mobility_count];
}



int evaluate(const Board &board) {
    int score = 0;

    // Evaluates MATERIAL and PIECE LOCATION
    for (int p = 0; p < 12; p++) {
        Bitboard bb = board.pieces[p];
        while (bb) {
            int sq = pop_lsb(bb);
            if (p == 5 || p == 11) continue;
            if (p < 6) {
                score += piece_value[0][p];
                if (p == 0) score += pawn_psqt[0][sq / 8][sq % 8];
                else score += psqt[0][p - 1][sq / 8][std::min(sq % 8, 7 - sq % 8)];
            } else {
                sq = 63 - sq;
                score -= piece_value[0][p - 6];
                if (p == 6) score -= pawn_psqt[0][sq / 8][sq % 8];
                else score -= psqt[0][p - 7][sq / 8][std::min(sq % 8, 7 - sq % 8)];
            }
        }
    }

    return board.white_to_move ? score : -score;
}

// Move generation
struct MoveGenerator {
    static void generate_moves(const Board &board, std::vector<Move> &moves) {
        if (board.white_to_move) {
            generate_pawn_moves(board, moves, true);
            generate_knight_moves(board, moves, true);
            generate_bishop_moves(board, moves, true);
            generate_rook_moves(board, moves, true);
            generate_queen_moves(board, moves, true);
            generate_king_moves(board, moves, true);
        } else {
            generate_pawn_moves(board, moves, false);
            generate_knight_moves(board, moves, false);
            generate_bishop_moves(board, moves, false);
            generate_rook_moves(board, moves, false);
            generate_queen_moves(board, moves, false);
            generate_king_moves(board, moves, false);
        }
        generate_castling_moves(board, moves);
    }

    static void generate_pawn_moves(const Board &board, std::vector<Move> &moves, bool white) {
        Bitboard pawns = white ? board.pieces[WP] : board.pieces[BP];
        Bitboard empty = ~(board.occupancy[0] | board.occupancy[1]);
        while (pawns) {
            int from = pop_lsb(pawns);
            if (white) {
                if (from < 56 && empty & (1ULL << (from + 8))) {
                    // Move forward
                    if ((from + 8) / 8 == 7) {
                        // Promotions
                        moves.emplace_back(from, from + 8, WQ);
                        moves.emplace_back(from, from + 8, WR);
                        moves.emplace_back(from, from + 8, WB);
                        moves.emplace_back(from, from + 8, WN);
                    } else {
                        moves.emplace_back(from, from + 8);
                        // Double move
                        if (from >= 8 && from <= 15 && (empty & (1ULL << (from + 16)))) {
                            moves.emplace_back(from, from + 16);
                        }
                    }
                }
                // Captures
                if ((from % 8) != 0 && (board.occupancy[1] & (1ULL << (from + 7)))) {
                    if (from + 7 < 64) {
                        if ((from + 7) / 8 == 7) {
                            moves.emplace_back(from, from + 7, WQ);
                            moves.emplace_back(from, from + 7, WR);
                            moves.emplace_back(from, from + 7, WB);
                            moves.emplace_back(from, from + 7, WN);
                        } else {
                            moves.emplace_back(from, from + 7);
                        }
                    }
                }
                if ((from % 8) != 7 && (board.occupancy[1] & (1ULL << (from + 9)))) {
                    if (from + 9 < 64) {
                        if ((from + 9) / 8 == 7) {
                            moves.emplace_back(from, from + 9, WQ);
                            moves.emplace_back(from, from + 9, WR);
                            moves.emplace_back(from, from + 9, WB);
                            moves.emplace_back(from, from + 9, WN);
                        } else {
                            moves.emplace_back(from, from + 9);
                        }
                    }
                }
                // En passant
                if (board.en_passant != -1) {
                    if (abs(board.en_passant - from) == 7 || abs(board.en_passant - from) == 9) {
                        moves.emplace_back(from, board.en_passant);
                    }
                }
            } else {
                if (from > 7 && empty & (1ULL << (from - 8))) {
                    // Move forward
                    if ((from - 8) / 8 == 0) {
                        // Promotions
                        moves.emplace_back(from, from - 8, BQ);
                        moves.emplace_back(from, from - 8, BR);
                        moves.emplace_back(from, from - 8, BB);
                        moves.emplace_back(from, from - 8, BN);
                    } else {
                        moves.emplace_back(from, from - 8);
                        // Double move
                        if (from >= 48 && from <= 55 && (empty & (1ULL << (from - 16)))) {
                            moves.emplace_back(from, from - 16);
                        }
                    }
                }
                // Captures
                if ((from % 8) != 0 && (board.occupancy[0] & (1ULL << (from - 9)))) {
                    if (from - 9 >= 0) {
                        if ((from - 9) / 8 == 0) {
                            moves.emplace_back(from, from - 9, BQ);
                            moves.emplace_back(from, from - 9, BR);
                            moves.emplace_back(from, from - 9, BB);
                            moves.emplace_back(from, from - 9, BN);
                        } else {
                            moves.emplace_back(from, from - 9);
                        }
                    }
                }
                if ((from % 8) != 7 && (board.occupancy[0] & (1ULL << (from - 7)))) {
                    if (from - 7 >= 0) {
                        if ((from - 7) / 8 == 0) {
                            moves.emplace_back(from, from - 7, BQ);
                            moves.emplace_back(from, from - 7, BR);
                            moves.emplace_back(from, from - 7, BB);
                            moves.emplace_back(from, from - 7, BN);
                        } else {
                            moves.emplace_back(from, from - 7);
                        }
                    }
                }
                // En passant
                if (board.en_passant != -1) {
                    if (abs(board.en_passant - from) == 7 || abs(board.en_passant - from) == 9) {
                        moves.emplace_back(from, board.en_passant);
                    }
                }
            }
        }
    }

    static void generate_knight_moves(const Board &board, std::vector<Move> &moves, bool white) {
        Bitboard knights = white ? board.pieces[WN] : board.pieces[BN];
        Bitboard own = white ? board.occupancy[0] : board.occupancy[1];
        while (knights) {
            int from = pop_lsb(knights);
            const int knight_offsets[8] = {17, 15, 10, 6, -17, -15, -10, -6};
            for (auto offset: knight_offsets) {
                int to = from + offset;
                if (to < 0 || to >= 64) continue;
                // Handle wrapping
                if (abs(to % 8 - from % 8) > 2) continue;
                if (!(own & (1ULL << to))) {
                    moves.emplace_back(from, to);
                }
            }
        }
    }

// Modified generate_slider_moves
    static void
    generate_slider_moves(const Board &board, std::vector<Move> &moves, bool white, int piece, const int *dirs,
                          int dir_count) {
        Bitboard sliders = white ? board.pieces[piece] : board.pieces[piece + 6];
        Bitboard own = white ? board.occupancy[0] : board.occupancy[1];

        while (sliders) {
            int from = pop_lsb(sliders);
            for (int d = 0; d < dir_count; ++d) {
                int to = from;
                while (true) {
                    to += dirs[d];
                    if (to < 0 || to >= 64) break; // Off-board

                    // Handle wrapping for diagonal moves
                    if (dirs[d] == -9 || dirs[d] == -7 || dirs[d] == 7 || dirs[d] == 9) {
                        if (to % 8 == 0 || to % 8 == 7) break; // Prevent wrapping
                    }

                    if (own & (1ULL << to)) break;
                    moves.emplace_back(from, to);
                    if ((board.occupancy[0] | board.occupancy[1]) & (1ULL << to)) break; // Stop at first obstruction
                }
            }
        }
    }

// Updated move generation functions
    static void generate_bishop_moves(const Board &board, std::vector<Move> &moves, bool white) {
        generate_slider_moves(board, moves, white, white ? WB : BB, bishop_directions, 4);
    }

    static void generate_rook_moves(const Board &board, std::vector<Move> &moves, bool white) {
        generate_slider_moves(board, moves, white, white ? WR : BR, rook_directions, 4);
    }

    static void generate_queen_moves(const Board &board, std::vector<Move> &moves, bool white) {
        generate_slider_moves(board, moves, white, white ? WQ : BQ, queen_directions, 8);
    }


    static void generate_king_moves(const Board &board, std::vector<Move> &moves, bool white) {
        Bitboard king = white ? board.pieces[WK] : board.pieces[BK];
        Bitboard own = white ? board.occupancy[0] : board.occupancy[1];
        while (king) {
            int from = pop_lsb(king);
            for (int d = 0; d < 8; ++d) {
                int to = from + queen_directions[d];
                if (to < 0 || to >= 64) continue;
                // Handle wrapping
                if (abs(to % 8 - from % 8) > 1) continue;
                if (!(own & (1ULL << to))) {
                    moves.emplace_back(from, to);
                }
            }
        }
    }

    static void generate_castling_moves(const Board &board, std::vector<Move> &moves) {
        // Simplified castling: only if king and rook have not moved and squares between are empty
        if (board.white_to_move) {
            // Kingside
            if (board.castling_rights[0]) {
                if (!(board.occupancy[0] | board.occupancy[1]) & 0x60) {
                    moves.emplace_back(4, 6); // e1 to g1
                }
            }
            // Queenside
            if (board.castling_rights[1]) {
                if (!(board.occupancy[0] | board.occupancy[1]) & 0x1C) {
                    moves.emplace_back(4, 2); // e1 to c1
                }
            }
        } else {
            // Kingside
            if (board.castling_rights[2]) {
                if (!(board.occupancy[0] | board.occupancy[1]) & 0x6000000000000000) {
                    moves.emplace_back(60, 62); // e8 to g8
                }
            }
            // Queenside
            if (board.castling_rights[3]) {
                if (!(board.occupancy[0] | board.occupancy[1]) & 0x1C00000000000000) {
                    moves.emplace_back(60, 58); // e8 to c8
                }
            }
        }
    }
};


// Negamax with Alpha-Beta Pruning
int negamax(Board board, int depth, int alpha, int beta, Move &best_move) {
    if (depth == 0) {
        return evaluate(board);
    }
    std::vector<Move> moves;
    MoveGenerator::generate_moves(board, moves);
    if (moves.empty()) {
        // Checkmate or stalemate
        return evaluate(board);
    }
    int max_eval = -INF;
    for (auto &move: moves) {
        // Make move
        Board new_board = board;
        int captured = new_board.get_piece(move.to);
        new_board.set_piece(move.to, move.promotion != EMPTY ? move.promotion : new_board.get_piece(move.from));
        new_board.set_piece(move.from, EMPTY);
        // Toggle side
        new_board.white_to_move = !new_board.white_to_move;
        // Recursive call
        int eval = -negamax(new_board, depth - 1, -beta, -alpha, best_move);
//        if (depth == 1 && move.from == 26 && move.to == 20) {
//            std::cout << eval << '\n';
//        }
        if (eval > max_eval) {
            max_eval = eval;
            if (depth == 1) {
                best_move = move;
            }
        }
        alpha = std::max(alpha, eval);
        if (alpha >= beta) {
            break; // Beta cutoff
        }
    }
    return max_eval;
}

// Main function
int main() {
    Board board;
    board.initialize();
//    board.import_fen("r4b2/2pk4/p2p4/1P1b4/3P4/8/1Pn2PPP/2B3K1 w - - 1");
    board.import_fen("rnbqkbnr/pppp1ppp/8/4pQ2/4P1B1/2N1B3/PPPP1PPP/R3K1NR b KQkq - 1 2");
    int tmp = 0;
    for (int i = 0; i < 64; i++) {
        tmp += knight_attack(board, i);
    }
    std::cout << tmp << '\n';
    std::cout << evaluate(board) << '\n';

    while (true) {
        std::vector<Move> moves;
        MoveGenerator::generate_moves(board, moves);
//        for (const auto selected_move : moves) {
//            std::cout << "Move: " << char('a' + selected_move.from%8) << (selected_move.from/8 +1)
//                      << char('a' + selected_move.to%8) << (selected_move.to/8 +1) << std::endl;
//        }
        if (moves.empty()) {
            std::cout << "Game Over." << std::endl;
            break;
        }

        // Find best move using Negamax
        Move selected_move;
        int score = negamax(board, 3, -INF, INF, selected_move); // Depth 3

        // Apply best move
        // For simplicity, find the move with 'to' == best_move
//        Move selected_move;
//        for(auto &m: moves){
//            if(m.to == best_move){
//                selected_move = m;
//                break;
//            }
//        }

        if (selected_move.to == -1) {
            std::cout << "No valid moves." << std::endl;
            break;
        }

        // Make the move
        int captured = board.get_piece(selected_move.to);
        board.set_piece(selected_move.to, selected_move.promotion != EMPTY ? selected_move.promotion : board.get_piece(
                selected_move.from));
        board.set_piece(selected_move.from, EMPTY);
        board.white_to_move = !board.white_to_move;

        // Output the move
        std::cout << "Move: " << char('a' + selected_move.from % 8) << (selected_move.from / 8 + 1)
                  << char('a' + selected_move.to % 8) << (selected_move.to / 8 + 1) << std::endl;
        std::cout << score;
        return 0;
    }
    return 0;
}
