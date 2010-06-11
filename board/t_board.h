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

#define MAX_THREADS 16

#define MAGIC_LIMIT_POS 42
#define MAGIC_LIMIT_COLS 7
#define MAGIC_LIMIT_QUAD 70
#define MAGIC_LIMIT_QUADCODE 30
#define MAGIC_LIMIT_QUAD_PER_POS 14

// number of positions or squares on board
const int const_posLim = MAGIC_LIMIT_POS;

// this is an error code returned in place of a column number
const int const_colNil = -1;

typedef struct {
    int move_value;
    int max_depth;
} result_t;

typedef struct {
	char m_rgPosition[ MAGIC_LIMIT_POS ]; // stores the pieces on the board,
                                         // described in .cpp
	char m_rgQuad[ MAGIC_LIMIT_QUAD ];    // stores the quads of the board,
                                         // described in .cpp
	int m_sumStatEval;                   // stores the sum of quad[1..69]
	int m_cMoves;                        // stores the number of moves made so far
	int m_fIsComputerTurn;               // 1 if computer's turn to move, 0 if human's
} board_t;

typedef struct {
	int depth;
	int alpha;
	int beta;
	int	move;
	board_t board;
} work_t;

static int difficulty;	// from 0 to 9, increasing in difficulty
static int depthMax;	// ply, no. of moves to search ahea

// Our global board
static board_t board;

// thread work functions
void *t_calcMinMove(void *arg);
void *t_calcMaxMove(void *arg);

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
int  getNumMoves( void );
void getBoardState( char rgPosition[ MAGIC_LIMIT_POS ] );
int isGameOver( board_t *b );
void move( board_t *b, int colMove);
int calcMaxMove( void ); 
int calcMinMove( void ); 
//int calcMaxEval( int depth, int alpha, int beta);
//int calcMinEval( int depth, int alpha, int beta);
void updateQuad( board_t *b, int iQuad );
void removeNonMoves(board_t *b, int *moves, int *nummoves);
