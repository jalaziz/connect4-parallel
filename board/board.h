/*
 * board.h: header file to the implementation of the "Drop Four" game logic
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

class Board
{
public:
	Board();
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
	
	static const int mconst_colNil;
	// number of positions or squares on board
	static const int mconst_posLim         = MAGIC_LIMIT_POS;

	void getBoardState( int rgPosition[ MAGIC_LIMIT_POS ] );
	
	inline int isGameOver( void )
	{
		return (    m_sumStatEval > mconst_evalPositiveWinMin
		         || m_sumStatEval < mconst_evalNegativeWinMin
		         || m_cMoves == 42 );
	}

private:
	int  calcMaxMove( void );
	int  calcMinMove( void );
	int  calcMaxEval( int depth, int alpha, int beta );
	int  calcMinEval( int depth, int alpha, int beta );
	void descendMoves( int* moves, int &nummoves );
	void ascendMoves( int* moves, int &nummoves );
	void move( int colMove );
	void remove( void );
	void updateQuad( int iQuad );
	void downdateQuad( int iQuad );

	static const int mconst_defaultDifficulty;
	static const int mconst_branchFactorMax;
	static const int mconst_worstEval;
	static const int mconst_bestEval;
	static const int mconst_quadLim;
	static const int mconst_dEvalP1, mconst_dEvalP2, mconst_dEvalP3, mconst_dEvalP4;
	static const int mconst_dEvalN1, mconst_dEvalN2, mconst_dEvalN3, mconst_dEvalN4;
	static const int mconst_evalPositiveWinMin, mconst_evalNegativeWinMin;
	static const int mconst_quadsPerPosLim;
	static const int mconst_mpPosQuads[ MAGIC_LIMIT_POS ][ MAGIC_LIMIT_QUAD_PER_POS ];
	static const int mconst_quadcodeLim;
	static const int mconst_rgUpQuadcode[ MAGIC_LIMIT_QUADCODE ];
	static const int mconst_rgDownQuadcode[ MAGIC_LIMIT_QUADCODE ];
	static const int mconst_rgUpEval[ MAGIC_LIMIT_QUADCODE ];

	int m_rgPosition[ MAGIC_LIMIT_POS ]; // stores the pieces on the board, described in .cpp
	int m_rgQuad[ MAGIC_LIMIT_QUAD ];    // stores the quads of the board, described in .cpp
	int m_sumStatEval;                   // stores the sum of quad[1..69]
	int m_rgHistory[ MAGIC_LIMIT_POS ];  // contain col's of previous moves
	int m_cMoves;                        // stores the number of moves made so far
	int m_fIsComputerTurn;               // 1 if computer's turn to move, 0 if human's
	int m_difficulty;                    // from 0 to 9, increasing in difficulty
	int m_depthMax;                      // ply, no. of moves to search ahead
	double m_chancePickBest;             // the chance the computer will pick the best move
	double m_chancePickSecondBest;       // the chance the computer will pick second best move
};
