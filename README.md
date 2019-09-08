# jigsaw-challenge
Solve a digitally generated jigsaw puzzle.


INTRODUCTION
This program creates a jigsaw puzzle.  The output is a set of portable
bitmap files (.pbm).  Each file describes one jigsaw puzzle piece.


OVERVIEW
Command line parameters to this program specify the height and width of
the puzzle as well as how many "fingers" to have on each piece edge.  The
output file describing each piece looks something like the following:
```
    P1
    # 44.pbm
    7 7
    1 1 0 0 1 1 0
    1 1 1 1 1 1 1
    0 1 1 1 1 1 0
    0 1 1 1 1 1 1
    1 1 1 1 1 1 1
    0 1 1 1 1 1 0
    0 1 0 1 0 1 0

```
Each piece is given a random number and a random orientation.  The goal
of a solver program is to list the pieces and their clockwize orientation
starting from the top left corner and going from left to right.  A
solution gives the file name of the piece and how much the piece should
be rotated in the counterclockwise direction before placement.  The
output of a solver program might look like
```
      p0044.pbm 270
      p0197.pbm 0
      p0073.pbm 180
      ....
```

Consider four valid, but rotated, solutions to a 3x3 puzzle:
```
    123  369  987  741
    456  258  654  852
    789  147  321  963
```
A rotated solution is still a valid solution and a puzzle with many
pieces may have duplicate pieces.  This makes it difficult to have
"one good solution".  To work around this we use a program to check
for a valid solution by putting all the pieces on a grid.  If there
are no gaps or overlaps the solution is valid.


