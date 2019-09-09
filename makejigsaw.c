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
 * The initgrid() routine fills in the grid with all of the known values
 * so that, if printed, the grid would appear similar to the one above.
 *
 * The dogrid() routine finishes the grid by randomly assigning the
 * interlocking pieces to an adjoining piece.  
 *
 * The outputpbm() routine saves the completed grid as a set of .pbm
 * files. Outputgrid() saves the pieces but gives random numbers and 
 * rotations to each pieces as it is saved.  The solution to the jigsaw
 * is saved to the "solution.txt" file.  As shown above, solutions have
 * the piece number and its clockwise rotation.  
 *
 * Consider four valid solutions to the 3x3 puzzle above:
 *     123  369  987  741
 *     456  258  654  852
 *     789  147  321  963
 * A rotated solution is still a valid solution and a puzzle with many
 * pieces may have duplicate pieces.  This makes it difficult to have
 * "one good solution".  To work around this we use a program to check
 * for a valid solution by putting all the pieces on a grid.  If there
 * are no gaps or overlaps the solution is valid.
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
        // Nominal finger width in millimeters
#define MMPF         10
        // Use a macro to make printing an svg line easier to read
#define SVGLINE(x1,y1,x2,y2) { \
        fprintf(fs, "<line x1='%d' y1='%d' x2='%d' y2='%d'/>\n", x1, y1, x2, y2);\
        };
        


/**************************************************************
 *  - Globals, function prototypes, and forward references
 **************************************************************/
void initgrid(int *, int, int, int);
void dogrid(int *, int, int, int);
void outputpbm(int *, int, int, int);
void outputsvg(int *, int, int, int);




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


    // Compute the random interlocking bits
    dogrid(grid, width, height, edge);


    // Output the grid
    outputpbm(grid, width, height, edge);


    // Output the SVG image
    outputsvg(grid, width, height, edge);

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
    int   is,js;            // Start location in i.j 
    int   gw;               // grid width

    gw = 1 + (width * (edge -1));

    // Walk the array computing the piece number at each location
    for (j = 0; j < height; j++) {
        js = j * (edge -1);
        for (i = 0; i < width; i++) {
            is = i * (edge -1);
            for (jk = 0; jk < edge; jk++) {
                for (ik = 0; ik < edge; ik++) {
                    // Do not set the areas where pieces interlock
                    // (not really needed but make debugging easier)
                    if ((ik == 0) && (i != 0))
                        continue;
                    if ((jk == 0) && (j != 0))
                        continue;
                    if ((ik == edge -1) && (i != width -1))
                        continue;
                    if ((jk == edge -1) && (j != height -1))
                        continue;

                    grid[(ik + is) + ((js + jk) * gw)] = i + (j * width);
                }
            }
        }
    }
}


/**************************************************************
 * dogrid(): - Compute the random bits in the grid
 *
 **************************************************************/
void dogrid(int *grid, int width, int height, int edge)
{
    int   ik,jk;            // Increment over edge in i/j dimension
    int   step;             // Distance between columns/rows to change
    int   randdir;          // One of four random directions
    int   rw;               // RowWidth
    int   roff, coff;       // Row and column offsets
    int   gw, gh;           // grid width, height
    int   x;                // Linear address of cell in grid 

    gw = 1 + (width * (edge -1));
    gh = 1 + (height * (edge -1));
    step = edge -1;

    // Do top/bottom random selection
    for (ik = step; ik < (gw -1); ik += step) {
        for (jk = 0; jk < gh; jk++) {
            x = ik + (jk * gw);
            if ((rand() % 2) == 0)
                grid[x] = grid[x-1];
            else
                grid[x] = grid[x+1];
        }
    }

    // Do left/right random selection
    for (jk = step; jk < (gh -1); jk += step) {
        for (ik = 0; ik < gw; ik++) {
            x = ik + (jk * gw);
            if ((rand() % 2) == 0)
                grid[x] = grid[x - gw];
            else
                grid[x] = grid[x + gw];
        }
    }

    // Compute the random attachment for pieces at the intersections
    for (jk = step; jk < (gh -1); jk += step) {
        for (ik = step; ik < (gw -1); ik += step) {
            x = ik + (jk * gw);
            // Do four way random selection
            randdir = rand() % 4;
            if (randdir == 0) {
                grid[x] = grid[x-1];
            }
            else if (randdir == 1) {
                grid[x] = grid[x+1];
            }
            else if (randdir == 2) {
                grid[x] = grid[x + gw];
            }
            else {
                grid[x] = grid[x - gw];
            }
        }
    }
}


/**************************************************************
 * outputpbm(): - output the puzzle as a set of .pbm files
 *
 **************************************************************/
void outputpbm(int *grid, int width, int height, int edge)
{
    int   gw;               // Grid width
    int   gh;               // Grid height
    int   n;                // Do the n'th piece
    int   i,j;              // Increment over width and height
    int   ik,jk;            // Increment over edge in i/j dimension
    int   is,js;            // Starting i/j for loops
    int   x = 0;            // location in the grid array
    int  *piece;            // list of pieces
    int   npiece;           // number of pieces
    int   newloc;           // random location to put piece
    FILE *fp;               // File pointer to .pbm file
    FILE *fs;               // File pointer to solution file
    int   ret;              // system call return value
    char  fname[PBMNAMELEN]; // .pbm file name
    int   randrot;          // random rotation 0=0, 1=90, 2=180, 3=270


    // Build a list of pieces, number them, then rearrange them.
    npiece = height * width;
    piece = (int *) malloc(sizeof(int) * npiece);
    if (piece == 0) {
        printf("malloc failure\n");
        exit(1);
    }
    // number the pieces
    for (n = 0; n < npiece; n++) {
        piece[n] = n;
    }
    // rearrange them
    for (n =0; n < npiece; n++) {
        newloc = rand() % npiece;
        j = piece[newloc];
        piece[newloc] = piece[n];
        piece[n] = j;
    }

    // Open the solution file
    fs = fopen("solution.txt", "w");
    if (fs == 0)
        exit(1);


    // Output each piece as a .pbm file.  Rotate the piece before
    // writing to the file.
    gw = (width * (edge - 1)) + 1;
    for (n = 0; n < npiece; n++) {
        // open the randomized file name
        ret = snprintf(fname, PBMNAMELEN, "p%04d.pbm", piece[n]);
        if (ret < 0)
            exit(1);
        fp = fopen(fname, "w");
        if (fp == 0)
            exit(1);
        fprintf(fp, "P1\n");
        fprintf(fp, "# %s\n", fname);
        fprintf(fp, "%d %d\n", edge, edge);

        // get a random rotation for the piece
        randrot = rand() % 4;

        // Save the correct piece and rotation to the solution file
        fprintf(fs, "%s %d\n", fname, randrot * 90);

        // get to the target piece
        i = n % width;    // as piece number
        j = n / height;
        is = i * (edge -1);      // as grid index
        js = j * (edge -1);

        // walk the perimeter of the piece to get the edges
        for (jk = 0; jk < edge; jk++) {
            for (ik = 0; ik < edge; ik++) {
                // select bit to print based on rotation
                if (randrot == 0)
                    x = ik + is + ((jk + js) * gw);              // 0 rotation
                else if (randrot == 1)
                    x = jk + is + (((edge -1 -ik) + js) * gw);           // 90
                else if (randrot == 2)
                    x = edge -1 - ik + is + (((edge -1 -jk) + js) * gw); // 180
                else
                    x = edge -1 - jk + is + ((ik + js) * gw);            // 270
                if (grid[x] == n)
                    fprintf(fp, "1");
                else
                    fprintf(fp, "0");
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
    }
    fclose(fs);


    // Walk the array printing the piece number at each location
    gw = (width * (edge - 1)) + 1;
    gh = (height * (edge - 1)) + 1;
    for (j = 0; j < gh; j++) {
        for (i = 0; i < gw; i++) {
            printf("%2d ", grid[i + (j * gw)]);
        }
        printf("\n");
    } 
}


/**************************************************************
 * outputsvg(): - save the puzzle as a scalable vector graphics
 * file.
 *
 **************************************************************/
void outputsvg(int *grid, int width, int height, int edge)
{
    int   gw;               // Grid width
    int   gh;               // Grid height
    int   i,j;              // Increment over edge in i/j dimension
    int   x = 0;            // location in the grid array
    FILE *fs;               // File pointer to solution file
    int   bw = 20;          // width of border in mm around the whole puzzle


    // Open the solution file
    fs = fopen("solution.svg", "w");
    if (fs == 0)
        exit(1);

    // Print header of svg file.  We let each bit in the solution be
    // 10mm (MMPF) wide.  
    gw = ((edge -1) * width) + 1;
    gh = ((edge -1) * height) + 1;
    fprintf(fs, "<svg width='%dmm' height='%dmm'\n", ((gw * MMPF) + (2 * bw)),
                 ((gh * MMPF) + (2 * bw)));
    fprintf(fs, "viewBox='0 0 %d %d'\n", ((gw * MMPF) + (2 * bw)),
                 ((gh * MMPF) + (2 * bw)));
    fprintf(fs, "stroke-width='1' stroke='rgb(0,0,0)'>\n\n");

    // print the outline of the puzzle
    SVGLINE(bw, bw, bw+(gw*MMPF), bw);                        // top
    SVGLINE(bw, bw+(gh*MMPF), bw+(gw*MMPF), bw+(gh*MMPF));    // bottom
    SVGLINE(bw, bw, bw, bw+(gh*MMPF));                        // left
    SVGLINE(bw+(gw*MMPF), bw, bw+(gw*MMPF), bw+(gh*MMPF));    // right
    fprintf(fs, "\n");

    // Scan from left to right and top to bottom printing vertical lines
    // where there is a transition from one piece to the next
    for (j = 0; j < gh; j++) {
        for (i = 0; i < gw-1; i++) {
            x = i + (gw * j);
            if (grid[x] != grid[x+1]) {
                SVGLINE(MMPF+bw+(i*MMPF), bw+(j*MMPF), MMPF+bw+(i*MMPF), bw+(j*MMPF)+MMPF);
            }
        }
    }

    // Scan from top to bottom and left to right printing horizontal lines
    // where there is a transition from one piece to the next
    for (i = 0; i < gw; i++) {
        for (j = 0; j < gh-1; j++) {
            x = i + (gw * j);
            if (grid[x] != grid[x+gw]) {
                SVGLINE(bw+(i*MMPF), MMPF+bw+(j*MMPF), bw+(i*MMPF)+MMPF, MMPF+bw+(j*MMPF));
            }
        }
    }


    fprintf(fs, "</svg>\n");
    fclose(fs);
}

