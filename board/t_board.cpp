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
#include <string.h>
#include "t_board.h"

//#define debug

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

inline int isGameOver( board_t *b )
{
    return (    b->m_sumStatEval > const_evalPositiveWinMin
             || b->m_sumStatEval < const_evalNegativeWinMin
             || b->m_cMoves == 42 );
}

void boardInit()
{
	// set it all to zero
	memset(&board, 0, sizeof(board));

    // set the default difficulty
	setDifficulty( const_defaultDifficulty );
}

// returns the number of moves that have been taken
int getNumMoves( void ) {
    return board.m_cMoves;
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
void setDifficulty( int diff )
{
	if (diff < 0 || diff > 9)
	{
		diff = const_defaultDifficulty;
	}

    // set the board difficulty
   	difficulty = diff;

	switch ( difficulty )
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
		depthMax = difficulty + 1;
//		board.m_chancePickBest = 0.1 * (board.m_difficulty + 1);
//		board.m_chancePickSecondBest = board.m_chancePickBest;
		break;
	case 5:
	case 6:
	case 7:
		depthMax = 5 + 2 * ( difficulty - 4 );
//		board.m_chancePickBest = 0.1 * (board.m_difficulty + 1);
//		board.m_chancePickSecondBest = 1.0 - board.m_chancePickBest;
		break;
	case 8:
	case 9:
		depthMax = 11 + 3 * ( difficulty - 7 );
//		board.m_chancePickBest = 0.1 * (board.m_difficulty + 1);
//		board.m_chancePickSecondBest = 1.0 - board.m_chancePickBest;
		break;
	}

//    // seed our random function
//	srand( (unsigned)time( NULL ) );
}

void setHumanFirst( void )
{
    // set the boards first turn
	board.m_fIsComputerTurn = 0;
}

void setComputerFirst( void )
{
    // set the boards first turn
	board.m_fIsComputerTurn = 1;
}

void getBoardState( char rgPosition[ const_posLim ] )
{
	memcpy(rgPosition, board.m_rgPosition, sizeof(char) * const_posLim);
}

// returns mconst_colNil on error, the column where move was made on success
int takeHumanTurn( int colMove )
{
	if ( isGameOver(&board)
	     || colMove < 0 || colMove > 6 || board.m_rgPosition[ colMove ] )
	{
		colMove = const_colNil;
	}
	else
	{
		move( &board, colMove );
	}

	return colMove;
}

int takeComputerTurn( void )
{
	int colMove;

	if (isGameOver(&board))
	{
		colMove = const_colNil;
	}
	else
	{
		if ( board.m_fIsComputerTurn )
	   	{
			colMove = calcMaxMove();
		}
		else
		{
			colMove = calcMinMove();
		}
		
		move( &board, colMove );
	}
	
	return colMove;
}

void move(board_t *b, int colMove )
{
	int quadTemp;
	const int* pQuads;
	char* colBase;
	int rowBySevens;
	int square;

	b->m_cMoves++;

	// find the lowest blank in column, thus setting the row
	colBase = b->m_rgPosition + 35 + colMove; // i.e., bottom row, same column
	rowBySevens = (*colBase ? (colBase[-7] ? (colBase[-14] ? (colBase[-21] ?
	               (colBase[-28] ? 0 : 7) : 14) : 21) : 28) : 35);
	square = rowBySevens + colMove;

	// set this position's value to -1 for human, 1 for computer
	// (whoever's turn it is)
	b->m_rgPosition[ square ] = ( b->m_fIsComputerTurn ? 1 : -1 );

	// update the quads for this position
	pQuads = const_mpPosQuads[ square ];
	updateQuad(b, *pQuads++);
	updateQuad(b, *pQuads++);
	updateQuad(b, *pQuads++);
	while ( quadTemp = *pQuads++ )
	{
		updateQuad(b, quadTemp );
	}

	// give the other guy a turn
	b->m_fIsComputerTurn = !b->m_fIsComputerTurn;
}

// The least significant bit has a 0 for human's turn, 1 for computer's.
// the next four bits hold a code 0-14 (i.e. 15 possibilities) for the
// number of max and min's squares.
// The quadcode is elaborated a bit more above, where the data arrays
// are declared. This function will update the quadcode and the stateval
// based on the addition of max (odd, i.e. +1) or min (even, i.e., +0)
inline void updateQuad( board_t *b, int iQuad )
{
	int quadcode = b->m_rgQuad[ iQuad ] + b->m_fIsComputerTurn;
	b->m_rgQuad[ iQuad ] = const_rgUpQuadcode[ quadcode ];
	b->m_sumStatEval += const_rgUpEval[ quadcode ];
}

int calcMaxMove(void)
{
	// the root node is max, and so has an alpha value.
	int iMoves;
	int temp;
	int alpha = const_worstEval;
	int best = const_worstEval - 1;
	int bestmove;

	// some initial pthread stuff
	pthread_t threads[7];
	pthread_attr_t attr;
	pthread_attr_init (&attr);
	work_t childWork[7];

	// the list of valid moves, 'best' move first (descending static value)
	int rgMoves[] = {3, 2, 4, 1, 5, 0, 6};
	int movesLim = sizeof( rgMoves ) / sizeof( int );

	// Remove unavailable moves
	removeNonMoves(&board, rgMoves, &movesLim);

    // as a default set the best and second best to the statically best move
	bestmove = rgMoves[ 0 ];

#ifdef debug
    printf("Main board has %d moves\n", board.m_cMoves);
#endif

    for(iMoves = 0; iMoves < movesLim; iMoves++)
    {
    	childWork[iMoves].move = rgMoves[iMoves];
    	childWork[iMoves].alpha = alpha;
    	childWork[iMoves].beta = const_bestEval;
    	childWork[iMoves].depth = depthMax;
    	childWork[iMoves].board = board;

    	pthread_create(&threads[iMoves], &attr, t_calcMaxMove, (void *)&childWork[iMoves]);
    }

    // for each move in the rgMoves array, evaluate the move
	for(iMoves = 0; iMoves < movesLim; iMoves++)
	{
		result_t * result;
		pthread_join(threads[iMoves], (void **)&result);
		temp = result->move_value;

		if (best < temp)
		{
#ifdef debug
			printf("New best move: %d with value %d\n", temp, rgMoves[iMoves]);
#endif
			best = alpha = temp;
			bestmove = rgMoves[iMoves];
		}

		free(result);
	}

    return bestmove;
}

int calcMinMove(void)
{
	// the root is min, and therefore has a beta value
	int iMoves;
	int temp;
	int beta = const_bestEval;
	int best = const_bestEval + 1;
	int bestmove;

	// some initial pthread stuff
	pthread_t threads[7];
	pthread_attr_t attr;
	pthread_attr_init (&attr);
	work_t childWork[7];

	// the list of valid moves, 'best' move first (descending static value)
	int rgMoves[] = {3, 2, 4, 1, 5, 0, 6};
	int movesLim = sizeof( rgMoves ) / sizeof( int );

	// Remove unavailable moves
	removeNonMoves(&board, rgMoves, &movesLim);

#ifdef debug
    printf("Main board has %d moves\n", board.m_cMoves);
#endif

	bestmove = rgMoves[ 0 ];

	for(iMoves = 0; iMoves < movesLim; iMoves++)
	{
		childWork[iMoves].move = rgMoves[iMoves];
		childWork[iMoves].alpha = const_worstEval;
		childWork[iMoves].beta = beta;
		childWork[iMoves].depth = depthMax;
		childWork[iMoves].board = board;

		pthread_create(&threads[iMoves], &attr, t_calcMinMove, (void *)&childWork[iMoves]);
	}

	// for each move in the rgMoves array, evaluate the move
	for(iMoves = 0; iMoves < movesLim; iMoves++)
	{
		result_t * result;
		pthread_join(threads[iMoves], (void **)&result);
		temp = result->move_value;

		if (best > temp)
		{
#ifdef debug
			printf("New best move: %d with value %d\n", temp, rgMoves[iMoves]);
#endif
			best = beta = temp;
			bestmove = rgMoves[iMoves];
		}

		free(result);
	}

	return bestmove;
}

void *t_calcMaxMove(void *arg) {
	int iMoves;
	int temp;
	int best = const_bestEval;
	int maxDepth;
	result_t *cresult;

	// the list of valid moves, 'best' move first (descending static value)
	int rgMoves[] = {3, 2, 4, 1, 5, 0, 6};
	int movesLim = sizeof( rgMoves ) / sizeof( int );

	// cut branching factor to mconst_branchFactorMax
	movesLim = (movesLim > const_branchFactorMax)
			   ? const_branchFactorMax : movesLim;

	work_t *work = (work_t *)arg;
	int depth = work->depth;

#ifdef debug
	printf("%d: Test Move: %d at depth %d\n", pthread_self(), work->move, depth);
#endif

	result_t *result = (result_t *)malloc(sizeof(result_t));
	if(result == NULL)
		exit(1);

#ifdef debug
	printf("%d: Before Move\n", pthread_self());
#endif
	move(&work->board, work->move);
#ifdef debug
	printf("%d: After Move\n", pthread_self());
#endif

	// if the game is over or this is the end of the tree (depth now 0)
	if(isGameOver(&work->board) || !(depth--))
	{
#ifdef debug
	printf("%d: Game over detected or end of tree: %d\n", pthread_self(), depth);
#endif

		result->move_value = work->board.m_sumStatEval;
		result->max_depth = work->depth;
	}
	else
	{
		// some initial pthread stuff
		pthread_t threads[7];
		pthread_attr_t attr;
		pthread_attr_init (&attr);

		work_t childWork[7];

#ifdef debug
	printf("%d: Removing non-moves\n", pthread_self());
#endif
		// Remove unavailable moves
		removeNonMoves(&work->board, rgMoves, &movesLim);

		// for every daughter
		for(iMoves = 0; iMoves < movesLim; iMoves++)
		{
#ifdef debug
	printf("%d: Trying child move %d\n", pthread_self(), rgMoves[iMoves]);
#endif
			childWork[iMoves].move = rgMoves[iMoves];
			childWork[iMoves].alpha = work->alpha;
			childWork[iMoves].beta = best;
			childWork[iMoves].depth = depth;
			childWork[iMoves].board = work->board;

#ifdef debug
	printf("%d: Creating thread\n", pthread_self());
#endif
			int status = pthread_create(&threads[iMoves], &attr, t_calcMinMove, (void *)&childWork[iMoves]);
#ifdef debug
	printf("%d: Thread created: %d\n", pthread_self(), status);
#endif
		}

		for(iMoves = 0; iMoves < movesLim; iMoves++)
		{
#ifdef debug
	printf("%d: Joining thread %d\n", pthread_self(), threads[iMoves]);
#endif
			pthread_join(threads[iMoves], (void **)&cresult);
			if (cresult->move_value < best)
			{
				best = cresult->move_value;
				result->move_value = best;
				result->max_depth = cresult->max_depth;

				// Check for an alphabeta "prune" of the tree. Early exit
				// because max has a position here that is better than another
				// position which min could choose, so min would never allow.
				if (best <= work->alpha)
				{
					break;
				}
			}
			free(cresult);
		}
	}

#ifdef debug
	printf("%d: Returning result: %d\n", pthread_self(), result->move_value);
#endif
	pthread_exit((void *)result);
}

void *t_calcMinMove(void *arg) {
	int iMoves;
	int temp;
	int best = const_worstEval;
	int maxDepth;
	result_t *cresult;

	// the list of valid moves, 'best' move first (descending static value)
	int rgMoves[] = {3, 2, 4, 1, 5, 0, 6};
	int movesLim = sizeof( rgMoves ) / sizeof( int );

	// cut branching factor to mconst_branchFactorMax
	movesLim = (movesLim > const_branchFactorMax)
			   ? const_branchFactorMax : movesLim;

	work_t *work = (work_t *)arg;
	int depth = work->depth;

#ifdef debug
	printf("%d: Test Move: %d at depth %d\n", pthread_self(), work->move, depth);
#endif

	result_t *result = (result_t *)malloc(sizeof(result_t));
	if(result == NULL)
		exit(1);

#ifdef debug
	printf("%d: Before Move\n", pthread_self());
#endif

	move(&work->board, work->move);

#ifdef debug
	printf("%d: After Move\n", pthread_self());
#endif

	// if the game is over or this is the end of the tree (depth now 0)
	if(isGameOver(&work->board) || !(depth--))
	{
#ifdef debug
	printf("%d: Game over detected or end of tree: %d\n", pthread_self(), depth);
#endif

		result->move_value = work->board.m_sumStatEval;
		result->max_depth = work->depth;
	}
	else
	{
		// some initial pthread stuff
		pthread_t threads[7];
		pthread_attr_t attr;
		pthread_attr_init (&attr);

		work_t childWork[7];

		// Remove unavailable moves
		removeNonMoves(&work->board, rgMoves, &movesLim);

		// for every daughter
		for(iMoves = 0; iMoves < movesLim; iMoves++)
		{
#ifdef debug
	printf("%d: Trying child move %d\n", pthread_self(), rgMoves[iMoves]);
#endif

			childWork[iMoves].move = rgMoves[iMoves];
			childWork[iMoves].alpha = work->alpha;
			childWork[iMoves].beta = best;
			childWork[iMoves].depth = depth;
			childWork[iMoves].board = work->board;

#ifdef debug
	printf("%d: Creating thread\n", pthread_self());
#endif
			int status = pthread_create(&threads[iMoves], &attr, t_calcMinMove, (void *)&childWork[iMoves]);
#ifdef debug
	printf("%d: Thread created: %d\n", pthread_self(), status);
#endif
		}

		for(iMoves = 0; iMoves < movesLim; iMoves++)
		{
#ifdef debug
	printf("%d: Joining thread %d\n", pthread_self(), threads[iMoves]);
#endif
			pthread_join(threads[iMoves], (void **)&cresult);
			if (cresult->move_value > best)
			{
				best = cresult->move_value;
				result->move_value = best;
				result->max_depth = cresult->max_depth;

				// Check for an alphabeta "prune" of the tree. Early exit
				// because max has a position here that is better than another
				// position which min could choose, so min would never allow.
				if (best >= work->beta)
				{
					break;
				}
			}
			free(cresult);
		}
	}

#ifdef debug
	printf("%d: Returning result: %d\n", pthread_self(), result->move_value);
#endif
	pthread_exit((void *)result);
}

void removeNonMoves(board_t *b, int *moves, int *nummoves) {
	int i = 0;
	while(i < *nummoves) {
		if (b->m_rgPosition[ moves[ i ] ])
		{
			moves[ i ] = moves [ *nummoves - 1 ];
			(*nummoves)--;
		}
		else { i++; }
	}
}
