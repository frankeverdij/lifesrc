#!/usr/bin/env python3

from os import path, makedirs
import copy
import argparse
import sys

"""
what does this program do?
1) Load partials and determine period.
2  Make mirror images.
3) Determine dimensions and position of unknown cells for each partial:
3a)  Determine if the search strip is left or right.
3b)  Determine start- and stoppoint of search strip relative to top.
4) Make combinations of gen 0 on the left and gen [0..6] on the right.
4a)  Start with working out the range of translations up and down.
4b)  Determine width as the width of each partial plus the number
       of extra columns between the two.
5) Finish pattern by adding dead and unknown cell:
5a)  Start with background of all dead cells, width and height to fit
       the pattern.
5b)  Add rows at the bottom to create extra search space.
5c)  Find the topmost row of unknown cells of either pattern.
5d)  From this row downward the background will be filled in
       with unknown cells.
5e)  Then place partials left and right, overwriting background.
6) Write pattern output in lif format in directory
     ncols/left-pattern-generation.
7) Write script in directory.
"""

def dimension_and_handedness(p):
    start = 0
    stop = 0
    leftright = ''
    for j,r in enumerate(p):
        if r[0] == '?':
            leftright = 'r'
            start = j
            break
        if r[-1] == '?':
            leftright = 'l'
            start = j
            break
    for j,r in reversed(list(enumerate(p))):
        if r[0] == '?':
           stop = j
           break
        if r[-1] == '?':
           stop = j
           break
    return list([len(p[0]), len(p), leftright, start, stop])

def partial_mirrors(partial):
    mp = []
    for p in partial:
        rr = []
        for r in p:
            rr.append(list(reversed(list(r))))
        mp.append(rr)
    return mp

def read_lif(lif_filename):
    with open(lif_filename, 'r') as fh:
        a = []
        for line_str in fh:
            a.append(list(line_str.rstrip('\n')))
        b = [x for x in a if x]
    return b

class Partials:
    def __init__(self, rows = 0, wantPeriod = 0, unkCols = 1, symmetric = False):
        self.period = 0
        self.wantPeriod = wantPeriod
        self.basePartials = []
        self.basePartialInfo = []
        self.partials = []
        self.partialInfo = []
        self.patterns = []
        self.patternInfo = []
        self.symmetry = False
        self.rows = rows
        self.unkCols = unkCols
        self.symmetric = symmetric

    def read_partials(self):
        while (path.exists(str(self.period)+".lif")):
            p = read_lif(str(self.period)+".lif")
            self.partials.append(p)
            self.period += 1

        if (path.exists("base.lif")):
            p = read_lif("base.lif")
            self.basePartials.append(p)
        else:
            self.symmetry = True
            self.basePartials.append(copy.deepcopy(self.partials[0]))

    def create_mirrors(self):
        mp = partial_mirrors(self.partials)
        self.partials.extend(mp)
        mq = partial_mirrors(self.basePartials)
        self.basePartials.extend(mq)

    def get_dimensions(self):
        for i,p in enumerate(self.partials):
            self.partialInfo.append(dimension_and_handedness(p))
        for i,p in enumerate(self.basePartials):
            self.basePartialInfo.append(dimension_and_handedness(p))

    def create_patterns(self, ncol = 0):
        self.patterns = []
        self.patternInfo = []
        baseHanded = (0 if self.basePartialInfo[0][2] == 'l' else 1)
        partialHanded = (0 if self.partialInfo[0][2] == 'r' else self.period)

        # loop over all right patterns, including its own mirror
        for i in range(self.period):
            j = partialHanded + i
            minOffset = self.basePartialInfo[baseHanded][3] \
                        - self.partialInfo[j][4]
            maxOffset = self.basePartialInfo[baseHanded][4] \
                        - self.partialInfo[j][3]

            # loop over all possible right pattern offsets having at least
            #   one shared row with unknown cells from the left pattern
            for k in range(minOffset, maxOffset + 1):
                width = self.basePartialInfo[baseHanded][0] + ncol + \
                    self.partialInfo[j][0]
                offset = ([-k, 0] if k < 0 else [0, k])
                # discard the negative translations of mirror images,
                #   since they will be the mirror image of the positive
                #   translations.
                if (self.symmetry and (j % self.period == 0) and (k < 0)):
                    continue
                height = max(self.basePartialInfo[baseHanded][1] + offset[0],
                    self.partialInfo[j][1] + offset[1]) + self.rows
                unkRow = min(self.basePartialInfo[baseHanded][3] + offset[0],
                    self.partialInfo[j][3] + offset[1])
                # draw background
                bg = []
                for _ in range(height):
                    bg.append(['.'] * width)

                # add in the unknown background field starting from row unkRow
                for m in range(unkRow, height):
                    for n in range(ncol + 2 * self.unkCols):
                        bg[m][self.basePartialInfo[baseHanded][0] \
                            - self.unkCols + n] = '?'

                # add left partial on top of the background
                for m in range(self.basePartialInfo[baseHanded][1]):
                    for n in range(self.basePartialInfo[baseHanded][0]):
                        bg[m + offset[0]][n] \
                            = self.basePartials[baseHanded][m][n]

                # add right partial on top of the background, right aligned
                for m in range(self.partialInfo[j][1]):
                    for n in range(self.partialInfo[j][0]):
                        bg[m + offset[1]][n + width - self.partialInfo[j][0]] \
                            = self.partials[j][m][n]

                # save pattern and pattern info
                self.patternInfo.append([ncol, j % self.period, offset[0],
                                        offset[1]])
                self.patterns.append(bg)

    def write_patterns(self):
        for i,j in enumerate(self.patternInfo):
            dirpath = str(j[0]) + '/' + str(j[1])
            futureGen = str(j[2]) + '_' + str(j[3])
            if (not path.exists(dirpath)):
                makedirs(dirpath, exist_ok=True)

            # write the lif
            with open(dirpath + '/' + futureGen + ".lif", 'w') as fh:
                for p in self.patterns[i]:
                    row = ''.join(list(p)) + '\n'
                    fh.write(row)
                fh.close()

            # write shell script for this pattern
            if not self.symmetric:
                self.write_script(dirpath, futureGen, i,
                                str(i) + "_" + futureGen, '')
            if (self.symmetry and (j[1] % self.period == 0) and (j[2] == j[3])):
                # create an extra script for the symmetric case
                self.write_script(dirpath, futureGen, i, 's', " -sc")
            
    def write_script(self, dirpath, lif, i, base, sym):
        with open(dirpath + '/' + base + ".sh", 'w') as fh:
            script = "#!/bin/sh\nset -x\ncd " + dirpath + "\n" \
                + "if [ \! $( grep -sqE 'object|Inconsistent' " \
                + base + ".log ; echo $? ) -eq 0 ] ; then\n" \
                + "  if [ -f " + base + ".dmp ] ; then\n" \
                + "    $LIFESRCDUMB " + base + ".dmp -l " + base + ".dmp" \
                + " -o " + base + ".out >> " + base + ".log 2>&1\n" \
                + "  else\n" \
                + "    $LIFESRCDUMB " + base + ".dmp" \
                + " -r" + str(len(self.patterns[i])) \
                + " -c" + str(len(self.patterns[i][0])) \
                + " -g" + str(self.wantPeriod if self.wantPeriod > 0 \
                            else self.period) + " -i " + lif + ".lif" \
                + sym \
                + " -o " + base + ".out >> " \
                + base + ".log 2>&1\n  fi\nfi\n"
            fh.write(script)

def main():
    parser = argparse.ArgumentParser(
        description = 'Create LIF output files of left- and right- collection '
                      'of partials arranged by middle column spacing and '
                      'right partial pattern phase ',
        formatter_class = argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-cr', '--columnrange', type = int, default = 5,
        help = 'range of columns added to the center of the pattern')
    parser.add_argument('-uk', '--unknowncolumns', type = int, default = 1,
        help = 'extra columns added to the center of the background')
    parser.add_argument('-r', '--rows', type = int, default = 0,
        help = 'number of extra rows with unknown cells appended')
    parser.add_argument('-p', '--period', type = int, default = 0,
        help = 'override period of ship in script creation')
    parser.add_argument('-s', '--symmetric', action = 'store_true',
        help = 'only output symmetric patterns')
    args = parser.parse_args(sys.argv[1:])

    p = Partials(args.rows, args.period, args.unknowncolumns, args.symmetric)
    p.read_partials()
    p.create_mirrors()
    p.get_dimensions()
    for i in range(args.columnrange):
        p.create_patterns(i)
        p.write_patterns()
    with open("genlifs.command", 'w') as fh:
        fh.write(" ".join(sys.argv)+'\n')

if __name__ == "__main__":
    main()

