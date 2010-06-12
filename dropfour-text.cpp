/*
 * dropfour-text.cpp: the text version of Drop Four
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
 * Drop Four: general design by Peter Kirby
 *
 * This is written in C++.  I wrote a previous version in QBasic, but it was
 * on the slow side at higher difficulty levels.  This is an attempt to
 * optimize the Artificial Intelligence of the program.  The graphics are
 * non-existent at this point and could certainly be added.  The interface
 * functions can be changed (in ioface.cpp) without any change to board.cpp.
 *
 * Well, I did create a graphical GUI using the Windows API functions;
 * however, I am not satisfied with having a Windows-only program.  Therefore
 * this program will be designed to use wxWidgets.  The text interface is
 * primarily for those wishing to test out the AI while the wxWidgets GUI
 * is being developed.  The files board.cpp and board.h *must* remain exactly
 * the same in both the text version and the graphical version.
 *
 * To avoid long lines, use tabs width 4 or less; however, the tabbing should
 * be consistent at any width. A sort of Hungarian is used to indicate what
 * the variables do (whether they are arrays, or indexes, and so on).
 *
 * Further comments are dispersed throughout the source code.
 */

#include <iostream>
using namespace std;
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include "ioface.h"
#include "board/t_board.h"

int main()
{
	int rgBoardPos[ const_posLim ];
	clock_t clkBefore, clkAfter;

    boardInit();
	init();

	if ( askfirst() )
	{
		setHumanFirst();
	}
	else
	{
		setComputerFirst();
	}

	setDifficulty( askdifficulty() );

	getBoardState( rgBoardPos );
	display( rgBoardPos );

	while (!isGameOver())
	{
		if ( isComputerTurn() )
		{
            timeval t1, t2;
            double elapsedTime;
            gettimeofday(&t1, NULL);
			takeComputerTurn();
            gettimeofday(&t2, NULL);
            elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
            elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
			cout << endl << "The computer took ";
			cout << elapsedTime / 1000.0;
			cout << " seconds to make its decision." << endl;
		}
		else
		{
			while ( takeHumanTurn( askmove() ) == const_colNil )
				; // loop until a valid move is entered
				  // even though askmove() already validates input
		}

		getBoardState( rgBoardPos );
		display( rgBoardPos );
	}

	endgame( isComputerWin() ? 1 : ( isHumanWin() ? -1 : 0 ) );

	return EXIT_SUCCESS;
}
