/*
 * t_board.h: header file to the implementation of the "Drop Four" game logic
 * in parallel
 *
 * Copyright (C) 2005 Peter Kirby.
 * E-mail Peter Kirby at gmail (peterkirby) or at www.peterkirby.com.
 *
 * "Drop Four" is a clone of the "Connect Four" (tm) of Milton Bradley.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 */

#define MAGIC_LIMIT_POS 42
#define MAGIC_LIMIT_COLS 7
#define MAGIC_LIMIT_QUAD 70
#define MAGIC_LIMIT_QUADCODE 30
#define MAGIC_LIMIT_QUAD_PER_POS 14

// number of positions or squares on board
const int const_posLim = MAGIC_LIMIT_POS;

// this is an error code returned in place of a column number
const int const_colNil = -1;

// This structure holds local copies of the board for each thread and 
// is what they modify for their calculations.
typedef struct cwork_ {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int thread_num;
	int m_rgPosition[ MAGIC_LIMIT_POS ]; // stores the pieces on the board, described in .cpp
	int m_rgQuad[ MAGIC_LIMIT_QUAD ];    // stores the quads of the board, described in .cpp
	int m_sumStatEval;                   // stores the sum of quad[1..69]
	int m_rgHistory[ MAGIC_LIMIT_POS ];  // contain col's of previous moves
	int m_cMoves;                        // stores the number of moves made so far
	int m_fIsComputerTurn;               // 1 if computer's turn to move, 0 if human's
} cwork_t;

typedef struct board_ {
    cwork_t m_twork[ MAGIC_LIMIT_COLS ]; // stores each threads work spaces
    pthread_t m_threads[ MAGIC_LIMIT_COLS ];    // stores the threads
	int m_rgPosition[ MAGIC_LIMIT_POS ]; // stores the pieces on the board, 
                                         // described in .cpp
	int m_rgQuad[ MAGIC_LIMIT_QUAD ];    // stores the quads of the board, 
                                         // described in .cpp
	int m_sumStatEval;                   // stores the sum of quad[1..69]
	int m_rgHistory[ MAGIC_LIMIT_POS ];  // contain col's of previous moves
	int m_cMoves;                        // stores the number of moves made so far
	int m_fIsComputerTurn;               // 1 if computer's turn to move, 0 if human's
	int m_difficulty;                    // from 0 to 9, increasing in difficulty
	int m_depthMax;                      // ply, no. of moves to search ahead
	double m_chancePickBest;       // the chance the computer will pick the best move
	double m_chancePickSecondBest; // the chance the computer will pick second best move
} board_t;

// Our global board
static board_t board;

// the board work functions.
void boardInit();
void setDifficulty( int difficulty );
void setHumanFirst( void );
void setComputerFirst( void );
int  isComputerWin( void );
int  isHumanWin( void );
int  isComputerTurn( void );
int  takeHumanTurn( int colMove );
int  takeComputerTurn( void );
int  takeBackMove( void );
int  getNumMoves( void );
int  getLastMove( void );
void getBoardState( int rgPosition[ MAGIC_LIMIT_POS ] );
int isGameOver( void );
void move( int colMove);
void remove( void );
int calcMaxMove( void ); 
int calcMinMove( void ); 
int calcMaxEval( int depth, int alpha, int beta);
int calcMinEval( int depth, int alpha, int beta);
void updateQuad( int iQuad );
void downdateQuad( int iQuad );
void descendMoves( int* moves, int &movesLim );
void ascendMoves( int* moves, int &movesLim );

// threads' main function
void *t_main( void* args );

// threads' work functions
int  t_calcMaxEval( int depth, int alpha, int beta, cwork_t* data );
int  t_calcMinEval( int depth, int alpha, int beta, cwork_t* data );
void t_descendMoves( int* moves, int &nummoves, cwork_t* data );
void t_ascendMoves( int* moves, int &nummoves, cwork_t* data );
void t_move( int colMove, cwork_t* data );
void t_remove( cwork_t* data );
void t_updateQuad( int iQuad, cwork_t* data );
void t_downdateQuad( int iQuad, cwork_t* data );
inline int t_isGameOver(cwork_t* data);
