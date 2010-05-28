/*
 * ioface.cpp: the text interface of Drop Four
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

#include <iostream>
using namespace std;
#include "ioface.h"
#include <stdlib.h>

// modify this to change general start-up routines
// this function is called when a Board is created
void init( void )
{
	cout << "\n\nWelcome to Drop Four!";
	cout << "\n\nA couple things to remember when playing:";
	cout << "\nType x or q and press enter to any prompt to exit/quit.";
	cout << "\nFollow the prompts and enjoy your game!";
	cout << endl;
}

// modify this to change how first mover is requested
// should return a 0 if computer is first and a 1 if human is first
int askfirst( void )
{
	// first is -1 by default; if still -1 after entry, user entry is
	// invalid, and the user should be promped again.
	char input;
	int first = -1;

	do {
		cout << "\nWould you like to go first (y/n)? ";
		input = prompt();

		if (input == 'Y' || input == 'y')
		{
			first = 1;
		}
		if (input == 'N' || input == 'n')
		{
			first = 0;
		}
	} while (first == -1);

	return first;
}

// modify this to change how difficulty is asked
// should return a number from 0 to 9 (9 being most difficult)
int askdifficulty( void )
{
	char input;
	int difficulty = -1;  // by default, allows looping

	do {
		cout << "\nPlease enter level of difficulty (0-9): ";
		input = prompt();

		if (input >= '0' && input <= '9')
		{
			difficulty = (int)input - (int)'0';
		}
	} while (difficulty == -1);

	return difficulty;
}

// modify this to change how a move is requested
// should return a number from 0 to 6 (for the columns, left to right)
int askmove( void )
{
	char input;
	int col = -1;  // by default

	do {
		cout << "\nPlease enter column to drop piece (0-6): ";
		input = prompt();

		if (input >= '0' && input <= '6')
		{
			col = (int)input - (int)'0';
		}
	} while (col == -1);

	return col;
}

// called by the ask* functions
char prompt( void )
{
	char trash;
	char line[ 80 ];
	char input;

	cin >> line;
	//cin.get( line, 80 ); // get a line of input from user
	//cin.get( trash );    // remove the '\n' from the buffer
	input = line[ 0 ];   // we only want the first character entered
	//printf("%d\n",(int)input);

	if (input == 'q' || input == 'Q' || input == 'x' || input == 'X')
	{
		// ask for confirmation and, if confirm, abort the program
		quit();
	}

	return input;   // return the first character entered
}

// modify this to change how board is shown
// col contains a number 0 to 6 for last move
// humanmove contains 0 if computer's last move, 1 if human's last move
// boardpos contains 42 integers which are 0 if blank, 1 if computer, -1 if human.
// the characters displayed are either 'X' for the human,
// 'O' for the computer, or '*' for a blank.  the characters are from left
// to right horizontally on board, row by row, starting at top row.
void display( int* boardpos, int col, int humanmove )
{
	char output[ 128 ];
	int x, y;
	int i = 0;

	output[ i++ ] = '\n';
	for (y = 0; y < 7; y++)
	{
		output[ i++ ] = '0' + y;
		output[ i++ ] = ' ';
	}

	for (x = 0; x < 6; x++)
	{
		output[ i++ ] = '\n';

		for (y = 0; y < 7; y++)
		{
			switch ( boardpos[7 * x + y] )
			{
			case 0:
				output[ i++ ] = '*';
				break;
			case 1:
				output[ i++ ] = 'O';
				break;
			case -1:
				output[ i++ ] = 'X';
				break;
			}

			output[ i++ ] = ' ';
		}
	}

	output[i++] = '\n';
	output[i] = '\0';

	cout << output;
}

// modify or delete this function at will
// called by prompt if user presses x, X, q, or Q
void quit( void )
{
	char input;
	cout << "\nAre you sure you want to quit (y/n)? ";
	input = prompt();

	if (input == 'y' || input == 'Y')
	{
		 // call early exit function from stdlib.h
		exit(0);
	}
}

// modify this function to change the display of the result of the end of game
// passed winner which is 0 for draw, 1 for comp win, -1 for human win
void endgame( int winner )
{
	switch (winner)
	{
	case 0:
		cout << "\n\nIt was a draw!";
		break;
	case 1:
		cout << "\n\nSorry, you lost.";
		break;
        case -1:
		cout << "\n\nCongratulations, you won!";
		break;
	}

	cout << "\n\nPress enter to quit. ";
	cin.get();
}
