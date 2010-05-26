/*
 * t_board.cpp: implements the logic of the "Drop Four" game
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

/*
 * Overview of A.I. Algorithm and Data
 *
 * The Board class contains an array of 6x7 (i.e. 42) integers, index 0-41.
 * The upper-left corner is 0, and the lower-right corner is 41. If the rows
 * are numbered 0-5, and the columns are numbered 0-6, then the location of
 * one square in the grid is 7 * row + col. The squares contain a 0 for a
 * blank, a -1 for Player One (human), and a 1 for Player Two (computer).
 * 
 * The Board class also contains an array of 69 integers.  The first 24 are
 * the horizontal rows of four squares (quads), four in each of the six
 * different columns.  The row from square 0 to 3 is quad 0, the row from
 * square 3 to 6 is quad 3, the row from 35 to 38 is quad 20, the row from
 * to 38 to 41 is quad 23. The next 21 are the vertical columns of four
 * squares (also called quads). The column from 0 to 21 is quad 24, the
 * column from 14 to 35 is quad 26, the column from 6 to 27 is quad 42, the
 * column from 20 to 41 is quad 44. The next 12 are the upperleft-lowerright
 * diagonals.  Starting from the upperleft, the diagonal starting at 0 is
 * quad 45, the diagonal starting at 3 is quad 48, the diagonal starting at
 * 14 is quad 53, the diagonal starting at 17 is quad 56. The last 12 are
 * the lowerleft-upperright diagonals.  Starting from the upperright, the
 * diagonal starting at 3 is quad 57, the diagonal starting at 6 is quad 60,
 * the diagonal starting at 17 is quad 65, the diagonal starting at 20 is
 * quad 68.
 *
 * Change: All quad numbers above are increased by 1, and the 0 is used as a
 * stop. The quads for each square (up to 14 with stop) are held in a const
 * integer array.
 *
 * Anyhow, these 69 'quads' represent all of the ways to win (or lose), so
 * owning a piece of these quads is a step towards winning.  However, if the
 * opponent also has a part of that quad, the quad is neutral because you
 * cannot win or lose on that quad (although your square may contribute
 * towards another quad).
 *
 * The quads are used to generate a static evaluation of the position.  If
 * the quad is empty or has both sides represented, it is set to zero.  If
 * only one side has a piece, the value of the quad is 1, 3, 8, or 1000 (the
 * first three values could be fiddled with, the fourth works as infinity in
 * practice), depending on whether one, two, three, or four squares are owned.
 * In keeping with the representation of human as negative and computer as
 * positive, the computer's quads add positively and the human's quads
 * subtract (add negatively). A higher static evaluation is better for the
 * computer, while a lower static evaluation is better for the human.  That
 * is, the computer is max and the human is mini in the minimaxing.  The use
 * of these quads means that the entire static evaluation doesn't have to be
 * recalculated with each move, only those quads that are associated with the
 * square that is changed.
 *
 * The Artificial Intelligence of the computer makes use of a minimax
 * algorithm with alpha-beta pruning.  The details of the logic behind this
 * algorithm, attributable to Knuth, can be found in many texts on A.I.  The
 * minimax part is relatively simple: generate a tree of all possible moves
 * to depth n, evaluate the "who's winning" question at depth n, and -
 * assuming that the computer and the human always pick the best move for
 * them - work backwards to figure out who's winning at the current position.
 * (That is, when several options are open to max, he picks the highest,
 * while when several options are open to mini, she picks the lowest.  The
 * value of max's or mini's game at that point is equal to the 'best' value
 * of the choices before them.)
 *
 * Alpha-beta pruning is a shortcut that works on top of minimaxing.  Suppose
 * that max can either make a move evaluated to be 10 or an unevaluated move.
 * When evaluating the other move, mini discovers a move with a value of 5.
 * Since mini always picks the lowest, the value of the 'unevaluated move' is
 * no greater than 5.  But since max already has the option of a move for 10,
 * further search on the other move is unnecessary, because it will never
 * happen. The converse is also true, with the roles of mini and max
 * transposed.
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include "t_board.h"

// set the default difficulty to level 4, and the max branch factor to 4 moves
const int const_defaultDifficulty = 4;

const int const_branchFactorMax   = 4;

// actually 69 quads, but 0 isn't used (so 1-69)
const int const_quadLim        = MAGIC_LIMIT_QUAD;
// number of 'quad codes'
const int const_quadcodeLim    = MAGIC_LIMIT_QUADCODE;
// number of quads affected (max) by a position (square)
const int const_quadsPerPosLim = MAGIC_LIMIT_QUAD_PER_POS;

const int const_worstEval = -10000; // unattainable numbers
const int const_bestEval  = 10000;

// points allotted to 0, 1, 2, 3, and 4 squares in a quad
const int const_dEvalP1 = 1,
          const_dEvalP2 = 3,
          const_dEvalP3 = 17,
          const_dEvalP4 = 2000,
          const_dEvalN1 = -1,
          const_dEvalN2 = -3,
          const_dEvalN3 = -18,
          const_dEvalN4 = -2000;

// the minimum number of points one will have if the winner
const int const_evalPositiveWinMin = 1000;
const int const_evalNegativeWinMin = -1000;

// posquad holds the quad numbers for each of the 42 squares, ended by a 0
// the data below was generated by a program commented out at the end of
// the file
const int
const_mpPosQuads[ MAGIC_LIMIT_POS ][ MAGIC_LIMIT_QUAD_PER_POS ] = {
	{1, 25, 46, 0},
	{1, 2, 28, 47, 0},
	{1, 2, 3, 31, 48, 0},
	{1, 2, 3, 4, 34, 49, 58, 0},
	{2, 3, 4, 37, 59, 0},
	{3, 4, 40, 60, 0},
	{4, 43, 61, 0},
	{5, 25, 26, 50, 0},
	{5, 6, 28, 29, 51, 46, 0},
	{5, 6, 7, 31, 32, 52, 47, 58, 0},
	{5, 6, 7, 8, 34, 35, 53, 48, 62, 59, 0},
	{6, 7, 8, 37, 38, 49, 63, 60, 0},
	{7, 8, 40, 41, 64, 61, 0},
	{8, 43, 44, 65, 0},
	{9, 25, 26, 27, 54, 0},
	{9, 10, 28, 29, 30, 55, 50, 58, 0},
	{9, 10, 11, 31, 32, 33, 56, 51, 46, 62, 59, 0},
	{9, 10, 11, 12, 34, 35, 36, 57, 52, 47, 66, 63, 60, 0},
	{10, 11, 12, 37, 38, 39, 53, 48, 67, 64, 61, 0},
	{11, 12, 40, 41, 42, 49, 68, 65, 0},
	{12, 43, 44, 45, 69, 0},
	{13, 25, 26, 27, 58, 0},
	{13, 14, 28, 29, 30, 54, 62, 59, 0},
	{13, 14, 15, 31, 32, 33, 55, 50, 66, 63, 60, 0},
	{13, 14, 15, 16, 34, 35, 36, 56, 51, 46, 67, 64, 61, 0},
	{14, 15, 16, 37, 38, 39, 57, 52, 47, 68, 65, 0},
	{15, 16, 40, 41, 42, 53, 48, 69, 0},
	{16, 43, 44, 45, 49, 0},
	{17, 26, 27, 62, 0},
	{17, 18, 29, 30, 66, 63, 0},
	{17, 18, 19, 32, 33, 54, 67, 64, 0},
	{17, 18, 19, 20, 35, 36, 55, 50, 68, 65, 0},
	{18, 19, 20, 38, 39, 56, 51, 69, 0},
	{19, 20, 41, 42, 57, 52, 0},
	{20, 44, 45, 53, 0},
	{21, 27, 66, 0},
	{21, 22, 30, 67, 0},
	{21, 22, 23, 33, 68, 0},
	{21, 22, 23, 24, 36, 54, 69, 0},
	{22, 23, 24, 39, 55, 0},
	{23, 24, 42, 56, 0},
	{24, 45, 57, 0}
};

// the quadcode system tells how many max's and min's are in it with the
// following code
// 0 - 0 max, 0 min    10 - 1 max, 0 min     20 - 2 max, 1 min
// 2 - 0 max, 1 min    12 - 1 max, 1 min     22 - 2 max, 2 min
// 4 - 0 max, 2 min    14 - 1 max, 2 min     24 - 3 max, 0 min
// 6 - 0 max, 3 min    16 - 1 max, 3 min     26 - 3 max, 1 min
// 8 - 0 max, 4 min    18 - 2 max, 0 min     28 - 4 max, 0 min
// the even number if min to be added, add 1 for odd if max to be added

// this will return the next (even) quadcode when a square is added
// -1 is error, already 4 squares for quad
const int const_rgUpQuadcode[] = {
	 2, 10,  4, 12,  6, 14,  8, 16, -1, -1,
	12, 18, 14, 20, 16, 22, -1, -1, 20, 24,
	22, 26, -1, -1, 26, 28, -1, -1, -1, -1
};

// this will return the previous (even) quadcode when a square is removed
const int const_rgDownQuadcode[] = {
	-1, -1,  0, -1,  2, -1,  4, -1,  6, -1,
	-1,  0, 10,  2, 12,  4, 14,  6, -1, 10,
	18, 12, 20, 14, -1, 18, 24, 20, -1, 24
};

// this will return the amount to add to stateval in light of change in quad
const int const_rgUpEval[] = {
	const_dEvalN1,
	const_dEvalP1,
	const_dEvalN2 - const_dEvalN1,
	-const_dEvalN1,
	const_dEvalN3 - const_dEvalN2,
	-const_dEvalN2,
	const_dEvalN4 - const_dEvalN3,
	-const_dEvalN3,
	0,
	0,
	-const_dEvalP1,
	const_dEvalP2 - const_dEvalP1,
	0,
	0,
	0,
	0,
	0,
	0,
	-const_dEvalP2,
	const_dEvalP3 - const_dEvalP2,
	0,
	0,
	0,
	0,
	-const_dEvalP3,
	const_dEvalP4 - const_dEvalP3,
	0,
	0,
	0,
	0
};

inline int t_isGameOver(cwork_t* data)
{
    return (    data->m_sumStatEval > const_evalPositiveWinMin
             || data->m_sumStatEval < const_evalNegativeWinMin
             || data->m_cMoves == 42 );
}

inline int isGameOver( void ) 
{
    return (    board.m_sumStatEval > const_evalPositiveWinMin
             || board.m_sumStatEval < const_evalNegativeWinMin
             || board.m_cMoves == 42 );
}

void boardInit()
{
	int iPosition;
	int iQuad;
    int iThread;

	// set it all to zero
	for (iPosition = 0; iPosition < const_posLim; iPosition++)
	{
		board.m_rgPosition[ iPosition ] = 0;
	}

	for (iQuad = 0; iQuad < const_quadLim; iQuad++)
    {
		board.m_rgQuad[ iQuad ] = 0;
    }

	board.m_sumStatEval = 0;
    board.m_cMoves = 0;

	// it is human's turn by default, and a given difficulty by default
	board.m_fIsComputerTurn = 0;
    // set the default difficulty
	setDifficulty( const_defaultDifficulty );

    // initialize our results struct
    pthread_mutex_init(&board.result.lock, NULL);
    pthread_cond_init(&board.result.cond, NULL);
    board.result.threads_finished = 0;
    board.result.best_move = const_colNil;
    board.result.second_best_move = const_colNil;

    // set up our threads.
    for (iThread = 0; iThread < MAGIC_LIMIT_COLS; iThread++) {
        // initialize our pthread library variables
        pthread_mutex_init(&(board.m_twork[iThread].lock), NULL);
        pthread_cond_init(&(board.m_twork[iThread].cond), NULL);
        // set our thread number
        board.m_twork[iThread].thread_num = iThread;
        // set our other values to zero
        board.m_twork[iThread].m_sumStatEval = 0;
        board.m_twork[iThread].m_cMoves = 0;
        board.m_twork[iThread].m_fIsComputerTurn = 0;
        for (iPosition = 0; iPosition < const_posLim; iPosition++)
        {
            board.m_twork[iThread].m_rgPosition[ iPosition ] = 0;
        }

        for (iQuad = 0; iQuad < const_quadLim; iQuad++)
        {
            board.m_twork[iThread].m_rgQuad[ iQuad ] = 0;
        }
        pthread_create(&board.m_threads[iThread], NULL, t_main, (void*) &board.m_twork[iThread]);
    }
}

// returns the number of moves that have been taken
int getNumMoves( void ) {
    return board.m_cMoves;
}

// returns the most recent taken move
int getLastMove( void ) {
    return board.m_rgHistory[board.m_cMoves - 1];
}

// returns the second to most recent taken move
int getSecondToLastMove( void ) {
    return board.m_rgHistory[board.m_cMoves - 2];
}

// returns 1 if computer won (max), 0 otherwise
int isComputerWin( void )
{
	return ( ( board.m_sumStatEval > const_evalPositiveWinMin ) ? 1 : 0 );
}

// returns 1 if human won (min), 0 otherwise
int isHumanWin( void )
{
	return ( ( board.m_sumStatEval < const_evalNegativeWinMin ) ? 1 : 0 );
}

int isComputerTurn( void )
{
	return board.m_fIsComputerTurn;
}

// if passed correctly, set the difficulty
// if not passed correctly, set to default difficulty
void setDifficulty( int difficulty )
{
    int iThreads;

	if (difficulty < 0 || difficulty > 9)
	{
		difficulty = const_defaultDifficulty;
	}

    // set the board difficulty
   	board.m_difficulty = difficulty;

	switch ( board.m_difficulty )
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
		board.m_depthMax = board.m_difficulty + 1;
		board.m_chancePickBest = 0.1 * (board.m_difficulty + 1);
		board.m_chancePickSecondBest = board.m_chancePickBest;
		break;
	case 5:
	case 6:
	case 7:
		board.m_depthMax = 5 + 2 * ( board.m_difficulty - 4 );
		board.m_chancePickBest = 0.1 * (board.m_difficulty + 1);
		board.m_chancePickSecondBest = 1.0 - board.m_chancePickBest;
		break;
	case 8:
	case 9:
		board.m_depthMax = 11 + 3 * ( board.m_difficulty - 7 );
		board.m_chancePickBest = 0.1 * (board.m_difficulty + 1);
		board.m_chancePickSecondBest = 1.0 - board.m_chancePickBest;
		break;
	}

    // seed our random function
	srand( (unsigned)time( NULL ) );
}

void setHumanFirst( void )
{
    int iThreads;
    // set the boards first turn
	board.m_fIsComputerTurn = 0;
    // set the threads' first turn as well
    for (iThreads = 0; iThreads < MAGIC_LIMIT_COLS; iThreads++) {
        pthread_mutex_lock(&board.m_twork[iThreads].lock);
        board.m_twork[iThreads].m_fIsComputerTurn = 0;
        pthread_mutex_unlock(&board.m_twork[iThreads].lock);
    }
}

void setComputerFirst( void )
{
    int iThreads;
    // set the boards first turn
	board.m_fIsComputerTurn = 1;
    // set the threads' first turn as well
    for (iThreads = 0; iThreads < MAGIC_LIMIT_COLS; iThreads++) {
        pthread_mutex_lock(&board.m_twork[iThreads].lock);
        board.m_twork[iThreads].m_fIsComputerTurn = 1;
        pthread_mutex_unlock(&board.m_twork[iThreads].lock);
    }
}

void getBoardState( int rgPosition[ const_posLim ] )
{
	for (int iPosition = 0; iPosition < const_posLim; iPosition++ )
	{
		rgPosition[ iPosition ] = board.m_rgPosition[ iPosition ];
	}
}

// returns mconst_colNil on error, the column where move was made on success
int takeHumanTurn( int colMove )
{
	if ( isGameOver()
	     || colMove < 0 || colMove > 6 || board.m_rgPosition[ colMove ] )
	{
		colMove = const_colNil;
	}
	else
	{
		move( colMove );
	}

	return colMove;
}

int takeComputerTurn( void )
{
	int colMove;

	if (isGameOver())
	{
		colMove = const_colNil;
	}
	else
	{
		if ( board.m_fIsComputerTurn )
	   	{
			colMove = t_calcMaxMove();
		}
		else
		{
			colMove = t_calcMinMove();
		}
		
		move( colMove );
	}
	
	return colMove;
}

// returns the column the piece was taken from
int takeBackMove( void )
{
	int colMove;

	if ( board.m_cMoves )
	{
		remove();
		colMove = board.m_rgHistory[ board.m_cMoves ];
	}
	else
	{
		colMove = const_colNil;
	}
	
	return colMove;
}

void move( int colMove )
{
	int quadTemp;
	const int* pQuads;
	int* colBase;
	int rowBySevens;
	int square;

	// add the latest move to history
	board.m_rgHistory[ board.m_cMoves++ ] = colMove;

	// find the lowest blank in column, thus setting the row
	colBase = board.m_rgPosition + 35 + colMove; // i.e., bottom row, same column
	rowBySevens = (*colBase ? (colBase[-7] ? (colBase[-14] ? (colBase[-21] ?
	               (colBase[-28] ? 0 : 7) : 14) : 21) : 28) : 35);
	square = rowBySevens + colMove;

	// set this position's value to -1 for human, 1 for computer
	// (whoever's turn it is)
	board.m_rgPosition[ square ] = ( board.m_fIsComputerTurn ? 1 : -1 );

	// update the quads for this position
	pQuads = const_mpPosQuads[ square ];
	updateQuad(*pQuads++);
	updateQuad(*pQuads++);
	updateQuad(*pQuads++);
	while ( quadTemp = *pQuads++ )
	{
		updateQuad( quadTemp );
	}

	// give the other guy a turn
	board.m_fIsComputerTurn = !board.m_fIsComputerTurn;
}

void t_move( int colMove, cwork_t* data )
{
	int quadTemp;
	const int* pQuads;
	int* colBase;
	int rowBySevens;
	int square;

	// add the latest move to history
	data->m_rgHistory[ (data->m_cMoves)++ ] = colMove;

	// find the lowest blank in column, thus setting the row
	colBase = data->m_rgPosition + 35 + colMove; // i.e., bottom row, same column
	rowBySevens = (*colBase ? (colBase[-7] ? (colBase[-14] ? (colBase[-21] ?
	               (colBase[-28] ? 0 : 7) : 14) : 21) : 28) : 35);
	square = rowBySevens + colMove;

	// set this position's value to -1 for human, 1 for computer
	// (whoever's turn it is)
	data->m_rgPosition[ square ] = ( data->m_fIsComputerTurn ? 1 : -1 );

	// update the quads for this position
	pQuads = const_mpPosQuads[ square ];
	t_updateQuad(*pQuads++, data);
	t_updateQuad(*pQuads++, data);
	t_updateQuad(*pQuads++, data);
	while ( quadTemp = *pQuads++ )
	{
		t_updateQuad( quadTemp, data );
	}

	// give the other guy a turn
	data->m_fIsComputerTurn = !data->m_fIsComputerTurn;
}

// The least significant bit has a 0 for human's turn, 1 for computer's.
// the next four bits hold a code 0-14 (i.e. 15 possibilities) for the
// number of max and min's squares.
// The quadcode is elaborated a bit more above, where the data arrays
// are declared. This function will update the quadcode and the stateval
// based on the addition of max (odd, i.e. +1) or min (even, i.e., +0)
inline void updateQuad( int iQuad )
{
	int quadcode = board.m_rgQuad[ iQuad ] + board.m_fIsComputerTurn;
	board.m_rgQuad[ iQuad ] = const_rgUpQuadcode[ quadcode ];
	board.m_sumStatEval += const_rgUpEval[ quadcode ];
}

inline void t_updateQuad( int iQuad, cwork_t* data )
{
	int quadcode = data->m_rgQuad[ iQuad ] + data->m_fIsComputerTurn;
	data->m_rgQuad[ iQuad ] = const_rgUpQuadcode[ quadcode ];
	data->m_sumStatEval += const_rgUpEval[ quadcode ];
}

// This function cannot be called if m_cMoves is 0. It does not check for
// that case, because it is a private member function optimized for speed.
void remove( void )
{
	int quadTemp;
	const int* pQuads;
	int* colBase;
	int rowBySevens;
	int square;

	// decrement movenum, retrieve last move
	int colMove = board.m_rgHistory[ --board.m_cMoves ];

	// find the highest occupied square
	colBase = board.m_rgPosition + colMove;
	rowBySevens = (*colBase) ? 0 : (colBase[7] ? 7 : (colBase[14] ? 14 :
	              (colBase[21] ? 21 : (colBase[28] ? 28 : 35))));
	square = rowBySevens + colMove;

	// if removing comp, now comp's turn; else human's if removing human
	board.m_fIsComputerTurn = !board.m_fIsComputerTurn;

	// set this position's value back to 0
	board.m_rgPosition[ square ] = 0;

	// reset the quads for this position
	pQuads = const_mpPosQuads[ square ];
	downdateQuad( *pQuads++ );
	downdateQuad( *pQuads++ );
	downdateQuad( *pQuads++ );
	while (quadTemp = *pQuads++)
	{
		downdateQuad( quadTemp );
	}
}

void t_remove( cwork_t* data )
{
	int quadTemp;
	const int* pQuads;
	int* colBase;
	int rowBySevens;
	int square;

	// decrement movenum, retrieve last move
	int colMove = data->m_rgHistory[ --(data->m_cMoves) ];

	// find the highest occupied square
	colBase = data->m_rgPosition + colMove;
	rowBySevens = (*colBase) ? 0 : (colBase[7] ? 7 : (colBase[14] ? 14 :
	              (colBase[21] ? 21 : (colBase[28] ? 28 : 35))));
	square = rowBySevens + colMove;

	// if removing comp, now comp's turn; else human's if removing human
	data->m_fIsComputerTurn = !data->m_fIsComputerTurn;

	// set this position's value back to 0
	data->m_rgPosition[ square ] = 0;

	// reset the quads for this position
	pQuads = const_mpPosQuads[ square ];
	t_downdateQuad( *pQuads++, data );
	t_downdateQuad( *pQuads++, data );
	t_downdateQuad( *pQuads++, data );
	while (quadTemp = *pQuads++)
	{
		t_downdateQuad( quadTemp, data );
	}
}

inline void downdateQuad( int iQuad )
{
	board.m_rgQuad[ iQuad ] = const_rgDownQuadcode[ board.m_rgQuad[ iQuad ] + board.m_fIsComputerTurn ];
	board.m_sumStatEval -= const_rgUpEval[ board.m_rgQuad[ iQuad ] + board.m_fIsComputerTurn ];
}

inline void t_downdateQuad( int iQuad, cwork_t* data )
{
	data->m_rgQuad[ iQuad ] = const_rgDownQuadcode[ data->m_rgQuad[ iQuad ] + data->m_fIsComputerTurn ];
	data->m_sumStatEval -= const_rgUpEval[ data->m_rgQuad[ iQuad ] + data->m_fIsComputerTurn ];
}

int calcMaxMove(void)
{
	// the root node is max, and so has an alpha value.
	int iMoves,iThreads;
	int temp;
	int alpha = const_worstEval;
	int best = const_worstEval - 1;
	int bestmove;
	int secondbestmove;
	double randomchance;

	// the list of valid moves, 'best' move first (descending static value)
	int rgMoves[] = {3, 2, 4, 1, 5, 0, 6};
	int movesLim = sizeof( rgMoves ) / sizeof( int );
	descendMoves( rgMoves, movesLim );

    // as a default set the best and second best to the statically best move
	bestmove = secondbestmove = rgMoves[ 0 ];

    printf("Main board has %d moves\n", board.m_cMoves);

    // for each move in the rgMoves array, evaluate the move
	for(iMoves = 0; iMoves < movesLim; iMoves++)
	{
		move( rgMoves[ iMoves ] );
		temp = isGameOver() ? board.m_sumStatEval
		                  : calcMinEval( board.m_depthMax, alpha, const_bestEval );
		remove();

		if (best < temp)
		{
			best = alpha = temp;
			secondbestmove = bestmove;
			bestmove = rgMoves[ iMoves ];
		}
	}
    // get results from threads here.

    // select randomly which move to return
	randomchance = rand() / (1.0 + (double)RAND_MAX);
	if ( randomchance < board.m_chancePickBest )
	{
		return bestmove;
	}
	else if ( randomchance < board.m_chancePickBest + board.m_chancePickSecondBest )
	{
		return secondbestmove;
	}
	else
	{
		randomchance = rand() / (1.0 + (double)RAND_MAX);
		return rgMoves[ (int) (randomchance * movesLim) ];
	}
}

int t_calcMaxMove(void)
{
	// the root node is max, and so has an alpha value.
	int iMoves,iThreads;
	int temp;
	int alpha = const_worstEval;
	int best = const_worstEval - 1;
	int bestmove;
	int secondbestmove;
	double randomchance;

	// the list of valid moves, 'best' move first (descending static value)
	int rgMoves[] = {3, 2, 4, 1, 5, 0, 6};
	int movesLim = sizeof( rgMoves ) / sizeof( int );
	descendMoves( rgMoves, movesLim );

    // as a default set the best and second best to the statically best move
	bestmove = secondbestmove = rgMoves[ 0 ];


    // Start threads doing a cycle of their work.
    for (iThreads = 0; iThreads < MAGIC_LIMIT_COLS; iThreads++) {
        pthread_mutex_lock(&board.m_twork[iThreads].lock);
        pthread_cond_signal(&board.m_twork[iThreads].cond);
        pthread_mutex_unlock(&board.m_twork[iThreads].lock);
    }

    printf("Main thread going to sleep\n");

    // Wait for threads to be done.
    pthread_mutex_lock(&board.result.lock);
    while (board.result.threads_finished != MAGIC_LIMIT_COLS) {
        pthread_cond_wait(&board.result.cond, &board.result.lock);
    }
    board.result.best_move = const_colNil;
    board.result.second_best_move = const_colNil;
    board.result.threads_finished = 0;
    pthread_mutex_unlock(&board.result.lock);

    printf("Main thread awoken.\n");

    // for each move in the rgMoves array, evaluate the move
	for(iMoves = 0; iMoves < movesLim; iMoves++)
	{
		move( rgMoves[ iMoves ] );
		temp = isGameOver() ? board.m_sumStatEval
		                  : calcMinEval( board.m_depthMax, alpha, const_bestEval );
		remove();

		if (best < temp)
		{
			best = alpha = temp;
			secondbestmove = bestmove;
			bestmove = rgMoves[ iMoves ];
		}
	}
    // get results from threads here.

    // select randomly which move to return
	randomchance = rand() / (1.0 + (double)RAND_MAX);
	if ( randomchance < board.m_chancePickBest )
	{
		return bestmove;
	}
	else if ( randomchance < board.m_chancePickBest + board.m_chancePickSecondBest )
	{
		return secondbestmove;
	}
	else
	{
		randomchance = rand() / (1.0 + (double)RAND_MAX);
		return rgMoves[ (int) (randomchance * movesLim) ];
	}
}

void t_calcMaxWork( cwork_t* data )
{
	// the root node is max, and so has an alpha value.
    int iMoves;
	int temp;
	int alpha = const_worstEval;
	int best = const_worstEval - 1;
	int bestmove;
	int secondbestmove;
	double randomchance;

	// the list of valid moves, 'best' move first (descending static value)
	int rgMoves[] = {3, 2, 4, 1, 5, 0, 6};
	int movesLim = sizeof( rgMoves ) / sizeof( int );
	t_descendMoves( rgMoves, movesLim, data );

    // as a default set the best and second best to the statically best move
	bestmove = secondbestmove = rgMoves[ 0 ];

    // if our column is in the moves array, evaluate it
	for(iMoves = 0; iMoves < movesLim; iMoves++)
	{
        if (rgMoves[iMoves] == data->thread_num) {
            t_move( rgMoves[ iMoves ], data );
            temp = t_isGameOver( data ) ? data->m_sumStatEval
                              : t_calcMinEval( board.m_depthMax, 
                                      alpha, const_bestEval, data );
            t_remove( data );

            if (best < temp)
            {
                best = alpha = temp;
                secondbestmove = bestmove;
                bestmove = rgMoves[ iMoves ];
            }
        }
	}
    printf("Thread %d: best was %d\n", data->thread_num, best);
    // put result in result
    pthread_mutex_lock(&board.result.lock);
    board.result.threads_finished++;
    if (board.result.threads_finished == MAGIC_LIMIT_COLS) {
        pthread_cond_signal(&board.result.cond);
    }
    pthread_mutex_unlock(&board.result.lock);
}

int calcMinMove(void)
{
	// the root is min, and therefore has a beta value
	int iMoves;
	int temp;
	int beta = const_bestEval;
	int best = const_bestEval + 1;
	int bestmove;
	int secondbestmove;
	double randomchance;

	// the list of valid moves, 'best' move first (descending static value)
	int rgMoves[] = {3, 2, 4, 1, 5, 0, 6};
	int movesLim = sizeof( rgMoves ) / sizeof( int );
	ascendMoves( rgMoves, movesLim );

    printf("Main board has %d moves\n", board.m_cMoves);

	bestmove = secondbestmove = rgMoves[ 0 ];

	for(iMoves = 0; iMoves < movesLim; iMoves++)
	{
		move( rgMoves[ iMoves ] );

		temp = isGameOver() ? board.m_sumStatEval
		                  : calcMaxEval( board.m_depthMax, const_worstEval, beta );

		remove();

		if (best > temp)
		{
			best = beta = temp;
			secondbestmove = bestmove;
			bestmove = rgMoves[ iMoves ];
		}
	}

	randomchance = rand() / (1.0 + (double)RAND_MAX);
	if ( randomchance < board.m_chancePickBest )
	{
		return bestmove;
	}
	else if ( randomchance < board.m_chancePickBest + board.m_chancePickSecondBest )
	{
		return secondbestmove;
	}
	else
	{
		randomchance = rand() / (1.0 + (double)RAND_MAX);
		return rgMoves[ (int) (randomchance * movesLim) ];
	}
}

int t_calcMinMove(void)
{
	// the root is min, and therefore has a beta value
	int iMoves, iThreads;
	int temp;
	int beta = const_bestEval;
	int best = const_bestEval + 1;
	int bestmove;
	int secondbestmove;
	double randomchance;

	// the list of valid moves, 'best' move first (descending static value)
	int rgMoves[] = {3, 2, 4, 1, 5, 0, 6};
	int movesLim = sizeof( rgMoves ) / sizeof( int );
	ascendMoves( rgMoves, movesLim );

	bestmove = secondbestmove = rgMoves[ 0 ];
    printf("Main board has %d moves\n", board.m_cMoves);

    // Start threads doing a cycle of their work.
    for (iThreads = 0; iThreads < MAGIC_LIMIT_COLS; iThreads++) {
        pthread_mutex_lock(&board.m_twork[iThreads].lock);
        pthread_cond_signal(&board.m_twork[iThreads].cond);
        pthread_mutex_unlock(&board.m_twork[iThreads].lock);
    }

    printf("Main thread going to sleep\n");

    // Wait for threads to be done.
    pthread_mutex_lock(&board.result.lock);
    while (board.result.threads_finished != MAGIC_LIMIT_COLS) {
        pthread_cond_wait(&board.result.cond, &board.result.lock);
    }
    board.result.best_move = const_colNil;
    board.result.second_best_move = const_colNil;
    board.result.threads_finished = 0;
    pthread_mutex_unlock(&board.result.lock);

    printf("Main thread awoken.\n");

	for(iMoves = 0; iMoves < movesLim; iMoves++)
	{
		move( rgMoves[ iMoves ] );

		temp = isGameOver() ? board.m_sumStatEval
		                  : calcMaxEval( board.m_depthMax, const_worstEval, beta );

		remove();

		if (best > temp)
		{
			best = beta = temp;
			secondbestmove = bestmove;
			bestmove = rgMoves[ iMoves ];
		}
	}

	randomchance = rand() / (1.0 + (double)RAND_MAX);
	if ( randomchance < board.m_chancePickBest )
	{
		return bestmove;
	}
	else if ( randomchance < board.m_chancePickBest + board.m_chancePickSecondBest )
	{
		return secondbestmove;
	}
	else
	{
		randomchance = rand() / (1.0 + (double)RAND_MAX);
		return rgMoves[ (int) (randomchance * movesLim) ];
	}
}
int calcMaxEval( int depth, int alpha, int beta )
{
	int iMoves;
	int temp;
	int best = const_worstEval;

	// the list of valid moves, 'best' move first (descending static value)
	int rgMoves[] = {3, 2, 4, 1, 5, 0, 6};
	int movesLim = sizeof( rgMoves ) / sizeof( int );

	// if this is the end of the tree (depth now 0)
	if (! (--depth))
	{                 
		for (int iMoves = 0; iMoves < movesLim; iMoves++)
        {
            if (!board.m_rgPosition[ iMoves ])
            {
                move( iMoves );
                
                if (board.m_sumStatEval > best)
                {
                    best = board.m_sumStatEval;
                }

                remove();
            }
        }
	}
	else
	{
		descendMoves(rgMoves, movesLim);

		// cut branching factor to mconst_branchFactorMax
		movesLim = (movesLim > const_branchFactorMax)
		           ? const_branchFactorMax : movesLim;

		// for every daughter
		for(iMoves = 0; iMoves < movesLim; iMoves++)
		{
			move( rgMoves[ iMoves ]);
			temp = isGameOver() ? board.m_sumStatEval
			                    : calcMinEval( depth, best, beta );
			remove();

			if (best < temp)
			{
				best = temp;
				
				// Check for an alphabeta "prune" of the tree. Early exit
				// because max has a position here that is better than another
				// position which min could choose, so min would never allow.
				if (temp >= beta)
				{
					break;
				}
			}
		}		  					 
	}

	return best;
}

int t_calcMaxEval( int depth, int alpha, int beta, cwork_t* data )
{
	int iMoves;
	int temp;
	int best = const_worstEval;

	// the list of valid moves, 'best' move first (descending static value)
	int rgMoves[] = {3, 2, 4, 1, 5, 0, 6};
	int movesLim = sizeof( rgMoves ) / sizeof( int );

	// if this is the end of the tree (depth now 0)
	if (! (--depth))
	{                 
		for (int iMoves = 0; iMoves < movesLim; iMoves++)
        {
            if (!data->m_rgPosition[ iMoves ])
            {
                t_move( iMoves, data );
                
                if (data->m_sumStatEval > best)
                {
                    best = data->m_sumStatEval;
                }

                t_remove( data );
            }
        }
	}
	else
	{
		t_descendMoves(rgMoves, movesLim, data);

		// cut branching factor to mconst_branchFactorMax
		movesLim = (movesLim > const_branchFactorMax)
		           ? const_branchFactorMax : movesLim;

		// for every daughter
		for(iMoves = 0; iMoves < movesLim; iMoves++)
		{
			t_move( rgMoves[ iMoves ], data);
			temp = t_isGameOver(data) ? data->m_sumStatEval
			                    : t_calcMinEval( depth, best, beta, data );
			t_remove( data );

			if (best < temp)
			{
				best = temp;
				
				// Check for an alphabeta "prune" of the tree. Early exit
				// because max has a position here that is better than another
				// position which min could choose, so min would never allow.
				if (temp >= beta)
				{
					break;
				}
			}
		}		  					 
	}

	return best;
}

int calcMinEval( int depth, int alpha, int beta )
{
	// start off assuming the worst for min
	int iMoves;
	int temp;
	int best = const_bestEval;
	
	// the list of valid moves, 'best' move first (descending static value)
	int rgMoves[] = {3, 2, 4, 1, 5, 0, 6};
	int movesLim = sizeof( rgMoves ) / sizeof( int );

	// if this is the end of the tree (depth now 0)
	if (! (--depth))
	{                 
		for (iMoves = 0; iMoves < movesLim; iMoves++)
		{
			if (!board.m_rgPosition[ iMoves ])
			{
				move( iMoves );

				if (board.m_sumStatEval < best)
				{
					best = board.m_sumStatEval;
	         	}

				remove();
			}
		}
	}
	else
	{
		ascendMoves( rgMoves, movesLim );

		// cut branching factor to mconst_branchFactorMax
		movesLim = (movesLim > const_branchFactorMax)
		           ? const_branchFactorMax : movesLim;

		// for every daughter
		for(iMoves = 0; iMoves < movesLim; iMoves++)
		{
			move( rgMoves[ iMoves ] );
			temp = isGameOver() ? board.m_sumStatEval
			                    : calcMaxEval( depth, alpha, best );
			remove();

			if (best > temp)
			{
				best = temp;
				
				// Check for an alphabeta "prune" of the tree. Early exit
				// because max has a position here that is better than another
				// position which min could choose, so min would never allow.
				if (temp <= alpha)
				{
					break;
				}
			}
		}
	}

	return best;
}

int t_calcMinEval( int depth, int alpha, int beta, cwork_t* data )
{
	// start off assuming the worst for min
	int iMoves;
	int temp;
	int best = const_bestEval;
	
	// the list of valid moves, 'best' move first (descending static value)
	int rgMoves[] = {3, 2, 4, 1, 5, 0, 6};
	int movesLim = sizeof( rgMoves ) / sizeof( int );

	// if this is the end of the tree (depth now 0)
	if (! (--depth))
	{                 
		for (iMoves = 0; iMoves < movesLim; iMoves++)
		{
			if (!data->m_rgPosition[ iMoves ])
			{
				t_move( iMoves, data );

				if (data->m_sumStatEval < best)
				{
					best = data->m_sumStatEval;
	         	}

				t_remove( data );
			}
		}
	}
	else
	{
		t_ascendMoves( rgMoves, movesLim, data );

		// cut branching factor to mconst_branchFactorMax
		movesLim = (movesLim > const_branchFactorMax)
		           ? const_branchFactorMax : movesLim;

		// for every daughter
		for(iMoves = 0; iMoves < movesLim; iMoves++)
		{
			t_move( rgMoves[ iMoves ], data );
			temp = t_isGameOver( data ) ? data->m_sumStatEval
			                    : t_calcMaxEval( depth, alpha, best, data );
			t_remove( data );

			if (best > temp)
			{
				best = temp;
				
				// Check for an alphabeta "prune" of the tree. Early exit
				// because max has a position here that is better than another
				// position which min could choose, so min would never allow.
				if (temp <= alpha)
				{
					break;
				}
			}
		}
	}

	return best;
}

void descendMoves( int* moves, int &movesLim )
{
	int i = 0;
	int j;
	int temp;
	int *statvals;
	int bigval, bigindex;

	statvals = (int *)calloc(movesLim,sizeof(int));

	while (i < movesLim)
	{
		// if the column of move i is full, take it off the list
		// by reducing size and copying the last element into its place
		if (board.m_rgPosition[ moves[ i ] ])  
		{
			moves[ i ] = moves[ movesLim - 1 ];
			movesLim--;
		}
		else
		{
			move( moves[ i ] );
			statvals[ moves[ i ] ] = board.m_sumStatEval;
			remove();
			i++;
		}
	}

	// sorting algorithm
	for (i = 0; i < movesLim - 1; i++)
	{
		bigval = statvals[ moves[ i ] ];
		bigindex = 0;

		for (j = i + 1; j < movesLim; j++)
		if (statvals[ moves[ j ] ] > bigval)
		{
			bigval = statvals[ moves[ j ] ];
			bigindex = j;
		}

		if (bigindex)
		{
			temp = moves[ i ];
			moves[ i ] = moves[ bigindex ];
			moves[ bigindex ] = temp;
		}
	}
	free(statvals);
}

void t_descendMoves( int* moves, int &movesLim, cwork_t* data)
{
	int i = 0;
	int j;
	int temp;
	int *statvals;
	int bigval, bigindex;

	statvals = (int *)calloc(movesLim,sizeof(int));

	while (i < movesLim)
	{
		// if the column of move i is full, take it off the list
		// by reducing size and copying the last element into its place
		if (data->m_rgPosition[ moves[ i ] ])  
		{
			moves[ i ] = moves[ movesLim - 1 ];
			movesLim--;
		}
		else
		{
			t_move( moves[ i ], data );
			statvals[ moves[ i ] ] = data->m_sumStatEval;
			t_remove( data );
			i++;
		}
	}

	// sorting algorithm
	for (i = 0; i < movesLim - 1; i++)
	{
		bigval = statvals[ moves[ i ] ];
		bigindex = 0;

		for (j = i + 1; j < movesLim; j++)
		if (statvals[ moves[ j ] ] > bigval)
		{
			bigval = statvals[ moves[ j ] ];
			bigindex = j;
		}

		if (bigindex)
		{
			temp = moves[ i ];
			moves[ i ] = moves[ bigindex ];
			moves[ bigindex ] = temp;
		}
	}
	free(statvals);
}

void ascendMoves( int* moves, int &movesLim )
{
	int i = 0;
	int j;
	int temp;
	int *statvals;
	int smallval, smallindex;

	statvals = (int *)calloc(movesLim,sizeof(int));

	while (i < movesLim)
	{
		// if the column of move i is full, take it off the list
		// by reducing size and copying the last element into its place
		if (board.m_rgPosition[ moves[ i ] ])
		{
			moves[ i ] = moves[ movesLim - 1 ];
			movesLim--;
		}
		else
		{
			move( moves[ i ] );
			statvals[ moves[ i++ ] ] = board.m_sumStatEval;
			remove();
		}
	}

	// sorting algorithm
	for (i = 0; i < movesLim - 1; i++)
	{
		smallval = statvals[ moves[ i ] ];
		smallindex = 0;

		for (j = i + 1; j < movesLim; ++j)
		if (statvals[ moves[ j ] ] < smallval)
		{
			smallval = statvals[ moves[ j ] ];
			smallindex = j;
		}

		if (smallindex)
		{
			temp = moves[ i ];
			moves[ i ] = moves[ smallindex ];
			moves[ smallindex ] = temp;
		}
	}
	free(statvals);
}

void t_ascendMoves( int* moves, int &movesLim, cwork_t* data )
{
	int i = 0;
	int j;
	int temp;
	int *statvals;
	int smallval, smallindex;

	statvals = (int *)calloc(movesLim,sizeof(int));

	while (i < movesLim)
	{
		// if the column of move i is full, take it off the list
		// by reducing size and copying the last element into its place
		if (data->m_rgPosition[ moves[ i ] ])
		{
			moves[ i ] = moves[ movesLim - 1 ];
			movesLim--;
		}
		else
		{
			t_move( moves[ i ], data );
			statvals[ moves[ i++ ] ] = data->m_sumStatEval;
			t_remove( data );
		}
	}

	// sorting algorithm
	for (i = 0; i < movesLim - 1; i++)
	{
		smallval = statvals[ moves[ i ] ];
		smallindex = 0;

		for (j = i + 1; j < movesLim; ++j)
		if (statvals[ moves[ j ] ] < smallval)
		{
			smallval = statvals[ moves[ j ] ];
			smallindex = j;
		}

		if (smallindex)
		{
			temp = moves[ i ];
			moves[ i ] = moves[ smallindex ];
			moves[ smallindex ] = temp;
		}
	}
	free(statvals);
}

void* t_main( void* args ) {
    // args holds our data.
    cwork_t* data = (cwork_t*) args;

    while(1) {
        // get a lock on our data space
        pthread_mutex_lock(&data->lock);
        printf("Thread %d: active!\n", data->thread_num);

        // while our board is in sync with the main board
        while (data->m_cMoves == board.m_cMoves) {
            printf("Thread %d: going to sleep!\n", data->thread_num);
            // wait until we are woken up and the board is different
            pthread_cond_wait(&data->cond, &data->lock);
            printf("Thread %d: awoken!\n", data->thread_num);
        }

        // The main board should never be more than 1 or 2 moves ahead of us
        assert(board.m_cMoves - data->m_cMoves == 1 || 
                board.m_cMoves - data->m_cMoves == 2);
        printf("Thread %d: Main board has %d moves\n", data->thread_num, board.m_cMoves);
        printf("Thread %d: Our board has %d moves\n", data->thread_num, data->m_cMoves);
        printf("Thread %d: updating its board!\n", data->thread_num);

        // Update our board with the latest move(s).
        if (board.m_cMoves - data->m_cMoves == 1) {
            t_move(getLastMove(), data);
        }
        else {
            t_move(getSecondToLastMove(), data);
            t_move(getLastMove(), data);
        }
        printf("Thread %d: Our new board has %d moves\n", data->thread_num, data->m_cMoves);
        // do our evaluation
        // If it's the computer's turn, do a Max.
		if ( board.m_fIsComputerTurn )
	   	{
            printf("Thread %d: Comp's turn.\n", data->thread_num);
            t_calcMaxWork( data );
		}
        // Otherwise, do a min.
        else {
            printf("Thread %d: Other's turn.\n", data->thread_num);
        }

        // unlock our data space
        pthread_mutex_unlock(&data->lock);

    }
    pthread_exit(NULL);
}


















































/****** The posquad data above was generated by this old code ****************

#include <iostream.h>

void updatequad(int quad);
void test(int row, int col);

void main(void)
{
	int row, col;
	for (row = 0; row < 6; ++row)
	{
		for (col = 0; col < 7; ++col)
		{
			cout << "{";
			test(row, col);
			cout << "0}, ";
		}
	}
}

void updatequad(int quad)
{
	cout << quad + 1 << ", ";
}

void test(int row, int col)
{
	// update horizontal quads for this position
	int basequad = 4 * row;
	if (col <= 3)
		updatequad(basequad);
	if (col && col <= 4)  // between 1 and 4
		updatequad(basequad + 1);
	if (col >= 2 && col <= 5)
		updatequad(basequad + 2);
	if (col >= 3)
		updatequad(basequad + 3);

	// update vertical quads for this position
	basequad = 24 + 3 * col;
	if (row <= 3)
		updatequad(basequad);
	if (row && row != 5)  // between 1 and 4
		updatequad(basequad + 1);
	if (row >= 2)
		updatequad(basequad + 2);

	// update upper-left/lower-right diagonals
	// find the quad by finding the upperleft starting points int the
	// 3x4 rectangle in the upperleft part of the connect4 board.
	// check the row,col and the three points diagonal from it up and left.
	// quadbase is hard-coded as 45
	int myrow = row;
	int mycol = col;
	if (myrow <= 2 && mycol <= 3)
		updatequad(45 + 4 * myrow + mycol);
	
	// if neither of them are 0, do it again at upperleft square
	// note that it decrements *after* evaluation
	if (myrow-- && mycol--)
	{
		if (myrow <= 2 && mycol <= 3)
			updatequad(45 + 4 * myrow + mycol);
		if (myrow-- && mycol--)
		{
			if (myrow <= 2 && mycol <= 3)
				updatequad(45 + 4 * myrow + mycol);
			if (myrow-- && mycol--)
				updatequad(45 + 4 * myrow + mycol);
		}
	}

	// update the upper-right / lower-left diagonal
	// find the quad by the upper-right starting point
	// quadbase is 57 really, but 54 to compensate for a col starting at 3.
	if (row <= 2 && col >= 3)
		updatequad(54 + 4 * row + col);
	if (row-- && col++ <= 5)
	{
		if (row <= 2 && col >= 3)
			updatequad(54 + 4 * row + col);
		if (row-- && col++ <= 5)
		{
			if (row <= 2 && col >= 3)
				updatequad(54 + 4 * row + col);
			if (row-- && col++ <= 5)
				updatequad(54 + 4 * row + col);
		}
	}
}

************************* end of unused old code **************************/

/************** the updatequad function used to be calculated this way *****

	if (max)
	{
		if (quadneg[quadnum])
		{
			if (!quadpos[quadnum]++)
				stateval -= NEGVAL[quadneg[quadnum]];
		}
		else
		{
			stateval -= POSVAL[quadpos[quadnum]++] - POSVAL[quadpos[quadnum]];
		}
	}
	else
	{
		if (quadpos[quadnum])
		{
			if (!quadneg[quadnum]++)
				stateval -= POSVAL[quadpos[quadnum]];
		}
		else
		{
			stateval -= NEGVAL[quadneg[quadnum]++] - NEGVAL[quadneg[quadnum]];
		}
	}
********************* end of unused old code *******************************/

/*************old version of eval functions********************

		//	This statement does it all. Recurse, propogating correct alpha
		//	and beta values (only one of which can change at any given node)
		//	unless of course the recursion has terminated, in which case we
		//	use the value of the node, as evaluated by worth when allsons
		//	created the node. Put the value thus obtained into temp, and
		//	compare it with the best value seen so far. The sense of
		//	comparison is reversed if curmax is true (bitwise xor is all
		//	right if both booleans are "pure"--0 or 1).


int Board::alphabeta(int move, int depth, int alpha, int beta)
{
	int i,
		temp,
		best,
		curmax; 	// contains the current value of the global "max",
					// negated (to minimize the number of negations)

	makemove(move);

	if (depth == MAXDEPTH || gameover())
	{
		temp = stateval;
		remove();
		return temp;
	}

	best = max ? WORST : BEST;	// start off assuming the worst
	curmax = max;			    // keep track of max through recursive calls

	// for every son
	for(i=0; i < 7 ; i++)
	{
	if (!pos[i])
	{
		if(curmax && (best < (temp = alphabeta(i,depth+1,best,beta)))
		   || !curmax && (best > (temp = alphabeta(i,depth+1,alpha,best))))
		{
			// check for an alphabeta "prune" of the tree
			if(curmax?(temp>=beta):(temp<=alpha))
			{
				remove();
				return(temp);
			}
			else
				best = temp;	// remember the best value seen so far
		}
	}
	}

	remove();
	return(best);
}

***************************/
