/* Name:        makejigsaw.c
 *
 * Description: This program creates a jigsaw puzzle.  The output is a set
 *              of portable bitmap files (.pbm).  Each file describes one
 *              jigsaw puzzle piece.
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
 * Build:       gcc -o makejigsaw makejigsaw.c
 *
 */

/* 
 * INTRODUCTION
 * This program creates a jigsaw puzzle.  The output is a set of portable
 * bitmap files (.pbm).  Each file describes one jigsaw puzzle piece.
 *
 *
 * OVERVIEW
 * Command line parameters to this program specify the height and width of
 * the puzzle as well as how many "fingers" to have on each piece edge.  The
 * output file describing each piece looks something like the following:
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
 * starting from the top left corner and going from left to right.  The
 * output of a solver program might look like
 *     44-270
 *     197-0
 *     73-180
 *     ....
 *
 *
 * PROGRAM DESIGN
 * The primary data structure is a grid of cells where each cell tells
 * what piece is occupying it.  The pieces interlock so a 7x7 piece next
 * to an interlocking 7x7 piece would have a size of 13x7.  For example,
 * a jigsaw with a width of 10, a height of 8, and a piece size of 7x7
 * would have a grid size of:
 *     gwidth  = (p-size * width) - 1 = ((7 * 10) - 1 = 69
 *     gheight = (p-size * height) -1 = ((7 * 8) - 1 = 55
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
 * The initgrid() routine fills in the grid with all of the known values
 * so that, if printed, the grid would appear similar to the one above.
 *
 * The dogrid() routine finishes the grid by randomly assigning the
 * interlocking pieces to an adjoining piece.  
 *
 * The outputgrid() routine saves the completed grid as a set of .pbm
 * files.  Note that sometimes identical pieces are generated.  The
 * outputgrid() routine checks for this and renames the file to have
 * the same name as the original but with an extension that gives the
 * duplicate number.  So if piece 44 had a duplicate, the duplicate would
 * be named 44.1.  Duplicate detection looks for rotated duplicates as
 * well.  The solution should give original piece number, not the number
 * with * its duplicate extension.  Thus when the piece 44.1 appears in a
 * solution it is listed as just 44.
 *
 * Outputgrid() saves the pieces but gives random numbers and rotations to
 * each pieces as it is saved.  The solution to the jigsaw is saved to the
 * "solution.txt" file.  As shown above, solutions have the piece number
 * and its clockwise rotation.  
 *
 * Consider four valid solutions to the 3x3 puzzle above:
 *     123  369  987  741
 *     456  258  654  852
 *     789  147  321  963
 * To force a single solution we require that the lowest numbered valid
 * corner piece be in the top left position.  In the above case, the 123
 * arrangement would be the valid solution.
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


/**************************************************************
 *  - Globals, function prototypes, and forward references
 **************************************************************/
void initgrid(int *, int, int, int);
void dogrid(int *, int, int, int);
void outputgrid(int *, int, int, int);




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
    int   gw;               // Grid width (width * size) -1
    int   gh;               // Grid height (height * size) -1
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
    gw = (width * edge) - 1;
    gh = (height * edge) - 1;
    grid = (int *) malloc(sizeof(int) * gw * gh * edge);
    for (i = 0; i < gw; i++) {
        for (j = 0; j < gh; j++) {
            grid[i + (j * gw)] = -1;
        }
    } 


    // Init the grid with the known values
    initgrid(grid, width, height, edge);


    // Compute the random interlocking bits
    dogrid(grid, width, height, edge);


    // Output the grid
    outputgrid(grid, width, height, edge);

    exit(0);
}


/**************************************************************
 * initgrid(): - Fill in the easy to compute bits
 *
 **************************************************************/
void initgrid(int *grid, int width, int height, int edge)
{
    int   i,j;              // Increment over width and height
    int   ik,jk;            // Increment over edge in i/j dimension
    int   x = 0;            // location in the grid array


    // Walk the array computing the piece number at each location
    for (j = 0; j < height; j++) {
        for (jk = 0; jk < edge; jk++) {
            for (i = 0; i < width; i++) {
                for (ik = 0; ik < edge; ik++) {
                    // Last column is (width * edge) -1
                    if ((i == width -1) && (ik == edge -1))
                        continue;
                    // Last column is (width * edge) -1
                    if ((j == height -1) && (jk == edge -1))
                        continue;
                    if ((ik == edge -1) || (jk == edge -1))
                        grid[x] = -1;
                    else
                        grid[x] = i + (j * width);
                    x = x + 1;
                }
            }
        }
    }
}


/**************************************************************
 * dogrid(): - Compute the random bits of the grid
 *
 **************************************************************/
void dogrid(int *grid, int width, int height, int edge)
{
    int   i,j;              // Increment over width and height
    int   ik,jk;            // Increment over edge in i/j dimension
    int   x = 0;            // location in the grid array
    int   randdir;          // One of four random directions
    int   rw;               // RowWidth
    int   roff, coff;       // Row and column offsets
int y;


    // Walk the array computing the piece number at each location
    for (j = 0; j < height; j++) {
        for (jk = 0; jk < edge; jk++) {
            for (i = 0; i < width; i++) {
                for (ik = 0; ik < edge; ik++) {
                    // Last column is (width * edge) -1
                    if ((i == width -1) && (ik == edge -1))
                        continue;
                    // Last column is (width * edge) -1
                    if ((j == height -1) && (jk == edge -1))
                        continue;

                    // 
                    if ((ik == (edge -1)) && (jk != (edge -1))) {
                        // Do left/right random selection
                        if ((rand() % 2) == 0)
                            grid[x] = grid[x-1];
                        else
                            grid[x] = grid[x+1];
                    }
                    if ((ik != (edge -1)) && (jk == (edge -1))) {
                        // Do top/bottom random selection
                        if ((rand() % 2) == 0)
                            grid[x] = grid[x - ((edge * width) -1)];
                        else
                            grid[x] = grid[x + ((edge * width) -1)];
                    }
                    x = x + 1;
                }
            }
        }
    }

    // Compute the random attachment for pieces at the intersections
    rw = (width * edge) - 1;                 // row width
    for (j = 0; j < height; j++) {
        roff = (j * edge) + (edge - 1);      // num full rows in offset
        for (i = 0; i < width; i++) {
            coff = (i * edge) + (edge - 1);  // num full cols in offset
            x = (roff * rw) + coff;
            // Do four way random selection
            randdir = rand() % 4;
            if (randdir == 0) {
                grid[x] = grid[x-1];
            }
            else if (randdir == 1) {
                grid[x] = grid[x+1];
            }
            else if (randdir == 2) {
                grid[x] = grid[x + ((edge * width) -1)];
            }
            else {
                grid[x] = grid[x - ((edge * width) -1)];
            }
        }
    }
}



/**************************************************************
 * outputgrid(): - Fill in the easy to compute bits
 *
 **************************************************************/
void outputgrid(int *grid, int width, int height, int edge)
{
    int   i,j;              // Increment over width and height
    int   ik,jk;            // Increment over edge in i/j dimension
    int   x = 0;            // location in the grid array


    // Walk the array printing the piece number at each location
    for (j = 0; j < height; j++) {
        for (jk = 0; jk < edge; jk++) {
            for (i = 0; i < width; i++) {
                for (ik = 0; ik < edge; ik++) {
                    // Last column is (width * edge) -1
                    if ((i == width -1) && (ik == edge -1))
                        continue;
                    // Last column is (width * edge) -1
                    if ((j == height -1) && (jk == edge -1))
                        continue;
                    printf("%2d ",grid[x]);
                    x = x + 1;
                }
            }
            printf("\n");
        }
    }
}

