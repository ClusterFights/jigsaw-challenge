/* Name:        validatejigsaw.c
 *
 * Description: This program creates a jigsaw puzzle.  The input is a set
 *              of portable bitmap files (.pbm) and a solution.txt file.
 *              Each .pbm file describes one jigsaw puzzle piece.
 *
 * Copyright:   Copyright (C) 2019 by Bob Smith (bsmith@linuxtoys.org)
 *              All rights reserved.
 *
 * License:     This program is free software; you can redistribute it and/or
 *              modify it under the terms of the Version 2 of the GNU General
 *              Public License as published by the Free Software Foundation.
 *              GPL2.txt in the top level directory is a copy of this license.
 *              This program is distributed in the hope that it will be useful,
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *              GNU General Public License for more details.
 *
 * Build:       gcc -o validatejigsaw validatejigsaw.c
 *
 */

/* 
 * INTRODUCTION
 * This program validates a solution to a jigsaw puzzle created with
 * makejigsaw.  The solution input file, solution.txt, is in exactly
 * the same format as the makejigsaw output file.
 * The output of this file is either the work 'valid' or the word 
 * 'invalid'.  The program exits with a 0 if valid and 1 if invalid.
 *
 *
 * OVERVIEW
 * Command line parameters to this program specify the height and width of
 * the puzzle as well as how many "fingers" to have on each piece edge.  The
 * input .pbm files describing each piece looks something like the following:
 *     P1
 *     # 44.pbm
 *     7 7
 *     1 1 0 0 1 1 0
 *     1 1 1 1 1 1 1
 *     0 1 1 1 1 1 0
 *     0 1 1 1 1 1 1
 *     1 1 1 1 1 1 1
 *     0 1 1 1 1 1 0
 *     0 1 0 1 0 1 0
 *
 * Each piece is given a random number and a random orientation.  The goal
 * of a solver program is to list the pieces and their clockwize orientation
 * starting from the top left corner and going from left to right.  A
 * solution gives the file name of the piece and how much the piece should
 * be rotated in the counterclockwise direction before placement.  The
 * output of a solver program might look like
 *     p0044.pbm 270
 *     p0197.pbm 0
 *     p0073.pbm 180
 *     ....
 *
 *
 * PROGRAM DESIGN
 * The primary data structure is a grid of cells where each cell tells
 * what piece is occupying it.  The pieces interlock so a 7x7 piece next
 * to an interlocking 7x7 piece would have a size of 13x7.  For example,
 * a jigsaw with a width of 10, a height of 8, and a piece size of 7x7
 * would have a grid size of:
 *     gwidth  = ((p-size -1) * width) + 1 = ((6 * 10) + 1 = 61
 *     gheight = ((p-size -1) * height) + 1 = ((6 * 8) + 1 = 49
 *
 * If you think about it you'll see that all the action of a piece is in
 * the top row, the last column, the bottom row, and the first column.  This
 * suggests that we could save memory by storing only these values.  The full
 * gwidth * gheight grid wastes memory but makes computing the interlocking
 * pieces easier to understand and program.
 *     
 * Pieces are numbered starting with 1 and going from left to right, and
 * top to bottom.  Pieces are renumbered and rotated when the piece .pbm
 * files are created.
 *
 * Using single digit cells, a 3 by 3 puzzle with a piece size of 5 might
 * have a starting grid that looks something like:
 *
 *     1 1 1 1 ? 2 2 2 2 ? 3 3 3 3
 *     1 1 1 1 ? 2 2 2 2 ? 3 3 3 3
 *     1 1 1 1 ? 2 2 2 2 ? 3 3 3 3
 *     1 1 1 1 ? 2 2 2 2 ? 3 3 3 3
 *     ? ? ? ? ? ? ? ? ? ? ? ? ? ?
 *     4 4 4 4 ? 5 5 5 5 ? 6 6 6 6
 *     4 4 4 4 ? 5 5 5 5 ? 6 6 6 6
 *     4 4 4 4 ? 5 5 5 5 ? 6 6 6 6
 *     4 4 4 4 ? 5 5 5 5 ? 6 6 6 6
 *     ? ? ? ? ? ? ? ? ? ? ? ? ? ?
 *     7 7 7 7 ? 8 8 8 8 ? 9 9 9 9
 *     7 7 7 7 ? 8 8 8 8 ? 9 9 9 9
 *     7 7 7 7 ? 8 8 8 8 ? 9 9 9 9
 *     7 7 7 7 ? 8 8 8 8 ? 9 9 9 9
 *
 * The main() routine gets the input parameters, computes the size of the
 * grid, and allocates memory for it.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>



/**************************************************************
 *  - Limits and defines
 **************************************************************/
        // Maximum width and height of a puzzle
#define MAX_WIDTH    500
#define MAX_HEIGHT   500
        // Maximum resolution of the fingers on a piece
#define MAX_EDGE     8
        // .pbm file name length
#define PBMNAMELEN   40


/**************************************************************
 *  - Globals, function prototypes, and forward references
 **************************************************************/
void initgrid(int *, int, int, int);
void getgrid(int *, int, int, int);
void testgrid(int *, int, int, int);




/**************************************************************
 * main(): - Collect the width, height, and finger size of the
 *           desired puzzle and allocate memory for the grid.
 *
 * Input:        argc, argv
 * Output:       0 on normal exit, -1 on error exit with errno set
 **************************************************************/
int main(int argc, char **argv)
{
    int  *grid;             // The grid of points
    int   width;            // Width of the puzzle in pieces
    int   height;           // Height of the puzzle in pieces
    int   edge;             // Resolution of a piece edge
    int   gw;               // Grid width
    int   gh;               // Grid height
    int   i,j;              // Generic loop counters


    // Get the width, height, and edge resolution from the user
    if (! ((argc == 4) &&
           (sscanf(argv[1], "%d", &width) == 1) &&
           (sscanf(argv[2], "%d", &height) == 1) &&
           (sscanf(argv[3], "%d", &edge) == 1) &&
           (width >= 2) &&
           (height >= 2) &&
           (edge >= 2) &&
           (width <= MAX_WIDTH) &&
           (height <= MAX_HEIGHT) &&
           (edge <= MAX_EDGE)))
    {
        // Could not get puzzle parameters
        printf("Usage: %s <width> <height> <size>\n", argv[0]);
        exit(1);
    }

    // Allocate memory for grid
    gw = (width * (edge - 1)) + 1;
    gh = (height * (edge - 1)) + 1;
    grid = (int *) malloc(sizeof(int) * gw * gh);
    for (i = 0; i < gw; i++) {
        for (j = 0; j < gh; j++) {
            grid[i + (j * gw)] = -1;
        }
    } 

    // Init the grid with the known values
    initgrid(grid, width, height, edge);


    // Read each piece from solution.txt and place it.
    getgrid(grid, width, height, edge);

    // No collisions but are all grid locations filled?
    // Program exit is in testgrid().
    testgrid(grid, width, height, edge);
}


/**************************************************************
 * initgrid(): - Fill the grid with -1 to indicate a bit is
 * not occupied.
 *
 **************************************************************/
void initgrid(int *grid, int width, int height, int edge)
{
    int   i,j;              // loop indices
    int   gw;               // grid width
    int   gh;               // grid height
    int   x;                // grid location

    gw = 1 + (width * (edge -1));
    gh = 1 + (height * (edge -1));

    // Walk the array marking each location as unused
    for (j = 0; j < gh; j++) {
        for (i = 0; i < gw; i++) {
             x = (j * width) + i;
             grid[x] = -1;
        }
    }
}


/**************************************************************
 * getgrid(): - read .pbm files and place pieces on grid
 *
 **************************************************************/
void getgrid(int *grid, int width, int height, int edge)
{
    int   gw;               // Grid width
    int   gh;               // Grid height
    int   i,j;              // Increment over width and height
    int   ik,jk;            // Increment over edge in i/j dimension
    int   is,js;            // Starting i/j for loops
    int   piece = 0;        // Which piece we're working on
    int   x = 0;            // location in the grid array
    FILE *fp;               // File pointer to .pbm file
    FILE *fs;               // File pointer to solution file
    int   ret;              // system call return value
    char  fname[PBMNAMELEN]; // .pbm file name
    int   discard;          // discard lines at top of .pbm file
    int   angle;
    char  inchar;

    // get grid width and height
    gw = (width * (edge - 1)) + 1;
    gh = (height * (edge - 1)) + 1;

    // Open the solution file
    fs = fopen("solution.txt", "r");
    if (fs == 0)
        exit(1);

    while (1) {             // loop reading file names from solution.txt
        // printf("%d %s %d\n", piece, fname, angle);

        ret = fscanf(fs, "%s %d", fname, &angle);
        // return when we hit the end of file
        if (ret <= 0) {
            fclose(fs);
            return;
        }
        fp = fopen(fname, "r");
        if (fp == 0) {
            printf("No piece file for %s\n", fname);
            exit(1);
        }


        // read then discard the first 3 lines in .pbm file
        discard = 3;
        while (discard) {
            ret = fread(&inchar, sizeof(char), 1, fp);
            if (ret != 1) {
                printf("Error processing file %s\n", fname);
                exit(1);
            }
            if (inchar == '\n')
                discard--;
        }

        // get to the target piece
        i = piece % width;    // as piece number
        j = piece / height;
        is = i * (edge -1);      // as grid index
        js = j * (edge -1);

        // Claim grid location for each '1' in the .pbm file
        for (jk = 0; jk < edge; jk++) {
            for (ik = 0; ik < edge; ik++) {
                // Compute location of bit based on rotation of piece
                if (angle == 0)
                    x = ik + is + ((jk + js) * gw);              // 0 rotation
                else if (angle == 90)
                    x = jk + is + (((edge -1 -ik) + js) * gw);           // 90
                else if (angle == 180)
                    x = edge -1 - ik + is + (((edge -1 -jk) + js) * gw); // 180
                else
                    x = edge -1 - jk + is + ((ik + js) * gw);            // 270

                ret = fread(&inchar, sizeof(char), 1, fp);
                if (ret != 1) {
                    printf("Error processing file %s\n", fname);
                    exit(1);
                }
                if (inchar == '0') {
                    continue;
                }
                if (inchar == '1') {
                    // is another piece on this spot?
                    if (grid[x] != -1) {
                        printf("invalid -- Collision between pieces %d and %d\n", grid[x], piece);
                        exit(1);
                    }
                    // if not, we claim it
                    grid[x] = piece;
                }
                else {
                    // expected a 1 or 0, error out
                    printf("Error processing file %s\n", fname);
                    exit(1);
                }
            }
            // skip to the next line
            ret = fread(&inchar, sizeof(char), 1, fp);
            if ((ret != 1) || (inchar != '\n')) { 
                printf("Error processing file %s\n", fname);
                exit(1);
            }
        }

        // loop back up to get the next piece
        fclose(fp);
        piece++;
    }
}


/**************************************************************
 * testgrid(): - verify that all grid locations are filled.
 * Output 'valid' or 'invalid' and exit
 *
 **************************************************************/
void testgrid(int *grid, int width, int height, int edge)
{
    int   i,j;              // loop indices
    int   gw;               // grid width
    int   gh;               // grid height
    int   x;                // grid location

    gw = 1 + (width * (edge -1));
    gh = 1 + (height * (edge -1));

    // Walk the array marking each location as unused
    for (j = 0; j < gh; j++) {
        for (i = 0; i < gw; i++) {
            x = (j * gw) + i;
            if (grid[x] == -1) {
                printf("invalid -- missing bit at grid location j=%d i=%d\n", j, i);
                exit(1);
            }
        }
    }

    // To get here means there were no collisions and every grid location is filled
    printf("valid\n");
    exit(0);
}

