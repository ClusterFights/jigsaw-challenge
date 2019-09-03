/* makejigsaw.c
 * Bob Smith (bsmith@linuxtoys.org)
 *
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
 * The dogrid() routine finishes the grid by randomizing assigning the
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
 * Consider four valid solutions to the 3x3 puzzle above:
 *     123  369  987  741
 *     456  258  654  852
 *     789  147  987  471
 * To force a single solution we require that the lowest numbered valid
 * corner piece be in the top left position.  In the above case, the 123
 * arrangement would be the valid solution.
 *
 */



 * 

