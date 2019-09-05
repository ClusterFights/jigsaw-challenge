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
starting from the top left corner and going from left to right.  The
output of a solver program might look like
```
    44-270
    197-0
    73-180
    ....

```
Note that sometimes identical pieces are generated.  These pieces are
given the same name as the original but with an extension that gives the
duplicate number.  So if piece 44 had a duplicate, the duplicate would
be named 44.1.  Duplicate detection looks for pieces that become a
duplicate when rotated.

Your solution should give original piece number, not the number with its
duplicate extension.  Thus when the piece 44.1 appears in a solution it
is listed as just 44.


Consider four valid, but rotated, solutions to a 3x3 puzzle:
```
    123  369  987  741
    456  258  654  852
    789  147  321  963
```
To force a single solution we require that the lowest numbered valid
corner piece be in the top left position.  In the above case, the 123
arrangement would be the valid solution.


