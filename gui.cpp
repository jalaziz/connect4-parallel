/*
 * gui.cpp: the gui interface of Drop Four
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
#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <GL/glut.h>
#include "board/t_board.h"

// global variables
int screen_height = 720;
int screen_width = 720;
game_state state = prompt_column;
char game_board[const_posLim];
int col_preview = 3;
bool game_over;
bool draw_player_piece_first = false;

void guiDisplay() {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // light blue "sky" background
    glClearColor(0.0, 0.6, 1.0, 0.0);

    // get the board state
    getBoardState(game_board);

    // draw the board
    glColor3ub(255, 190, 0);
    glLineWidth(3);
    // horizonals
    for (int i = 0; i < 7; i++) {
        glBegin(GL_LINES);
            glVertex3i(0, i, 0);
            glVertex3i(7, i, 0);
        glEnd();
    }
    // verticals
    for (int i = 0; i < 8; i++) {
        glBegin(GL_LINES);
            glVertex3i(i, 0, 0);
            glVertex3i(i, 6, 0);
        glEnd();
    }
    // draw game pieces
    for (int y = 0; y < 6; y++) {
        for (int x = 0; x < 7; x++) {
            // human is blue
            if (game_board[y*7 + x] == -1) glColor3ub(0, 10, 200);
            // computer is red
            else if (game_board[y*7 + x] == 1) glColor3ub(200, 0, 20);

            // only draw is there is a piece in this square
            if (game_board[y*7 + x] != 0) {
                glPushMatrix();
                glTranslatef(x + 0.5, 5.0 - y + 0.5, 0.0);
                glutSolidSphere(0.475, 50, 50);
                glPopMatrix();
            }
        }
    }
    // draw prospective piece
    glPushMatrix();
    glColor4ub(0, 10, 200, 80);
    glTranslatef(col_preview + 0.5, 6.5, 0.0);
    glutSolidSphere(0.475, 50, 50);
    glPopMatrix();

    // game loop
    if (state != waiting_for_input && !game_over) game_loop();

    // refresh the window
    glutSwapBuffers();
    glutPostRedisplay();

    return;
}

void reshape(int w, int h) {
    // prevent a divide by zero error
    if (h == 0) h = 1;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //gluPerspective(FOVY, (float)w / (float)h, 0.01, 40000.0);
    double s = 5.0;
    double ratio = (float)h / (float)w;
    glOrtho(-s, s, -s*ratio, s*ratio, 0.01, 9001.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(3.5, 3.0, 5.0,
            3.5, 3.0, 0.0,
            0.0, 1.0, 0.0);

    screen_width = w;
    screen_height = h;

    return;
}

void mouse(int button, int state, int x, int y) {
    return;
}

void key(unsigned char key, int x, int y) {
    // close the program when escape is hit
    if (key == 27) exit(0);
    printf("%c\n", key);
    state = prompt_column;

    switch (key) {
        case 13:
            move(&board, col_preview);
            col_preview = 3;
            draw_player_piece_first = true;
            break;
        default:
            break;
    }

    return;
}

void specialKey(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP:
            break;
        case GLUT_KEY_DOWN:
            break;
        case GLUT_KEY_LEFT:
            if (col_preview > 0) col_preview--;
            break;
        case GLUT_KEY_RIGHT:
            if (col_preview < 6) col_preview++;
            break;
    }

    return;
}

void game_loop() {
    if (isGameOver(&board)) {
        endgame( isComputerWin() ? 1 : ( isHumanWin() ? -1 : 0 ) );
        game_over = true;
        return;
    }

    if (draw_player_piece_first) {
        draw_player_piece_first = false;
        return;
    }

    if ( isComputerTurn() ) {
        timeval t1, t2;
        double elapsedTime;
        gettimeofday(&t1, NULL);
        takeComputerTurn();
        gettimeofday(&t2, NULL);
        elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
        elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
        cout << endl << "The computer took ";
        cout << elapsedTime / 1000.0;
        cout << " seconds to make its decision." << endl << endl;
        state = prompt_column;
    }
    else {
		printf("Please choose a column to drop piece into.\n");
        printf("(Left and Right arrow to change column. Enter to drop.)\n");
        state = waiting_for_input;
        /*
        while ( takeHumanTurn( askmove() ) == const_colNil )
            ; // loop until a valid move is entered
        // even though askmove() already validates input
        // */
    }

    return;
}

// modify this to change general start-up routines
// this function is called when a Board is created
void init( void )
{
    printf("\n\n");
	cout << "Welcome to Drop Four!" << endl << endl;
	cout << "A couple things to remember when playing:" << endl;
	cout << "Type x or q and press enter to any prompt to exit/quit." << endl;
	cout << "Follow the prompts and enjoy your game!" << endl;
	cout << "Protip: For the GUI version, you need to have focus" << endl;
    cout << "on the game window to place game pieces." << endl;
	cout << endl;
}

// modify this to change how first mover is requested
// should return a 0 if computer is first and a 1 if human is first
int askfirst( void ) {
	// first is -1 by default; if still -1 after entry, user entry is
	// invalid, and the user should be promped again.
	char input;
	int first = -1; 

	do {
		cout << "\nWould you like to go first (y/n)? ";
		input = prompt();

		if (input == 'Y' || input == 'y') {
			first = 1;
		}
		if (input == 'N' || input == 'n') {
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
void display( char* boardpos, int col, int humanmove )
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
		cout << "\n\nIt was a draw!" << endl;
		break;
	case 1:
		cout << "\n\nSorry, you lost." << endl;
		break;
    case -1:
		cout << "\n\nCongratulations, you won!" << endl;
		break;
	}

    printf("\nPress Escape to exit\n");

    return;
}
