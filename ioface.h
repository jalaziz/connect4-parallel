/*
 * ioface.h: header file to the text interface of Drop Four
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

// These functions are called by dropfour-text.cpp and must be present in some
// form for a text interface, but may be modified to suit user environment.

// this function is called when a Board is created
void init( void );

// should return a 0 if computer is first and a 1 if human is first
int askfirst( void );

// should return a number from 0 to 9 (9 being most difficult)
int askdifficulty( void );

// should return a number from 0 to 6 (for the columns, left to right)
int askmove( void );

// called by the ask* functions, handles the 'q'/'x' command
char prompt( void );

// col contains a number 0 to 6 for last move
// humanmove contains 0 if computer's last move, 1 if human's last move
// boardpos contains 42 integers which are 0 if blank, 1 if computer, -1 if human.
// the characters displayed are either 'X' for the human,
// 'O' for the computer, or '*' for a blank.  the characters are from left
// to right horizontally on board, row by row, starting at top row.
void display( int* boardpos, int col = -1, int humanmove = -1 );

// called by prompt if user presses x, X, q, or Q
void quit( void );

// shows a message about who won
// passed winner which is 0 for draw, 1 for comp win, -1 for human win
void endgame( int winner );
