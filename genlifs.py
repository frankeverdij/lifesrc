#!/usr/bin/env python3

from os import path, makedirs

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

class Partials:
    def __init__(self):
        self.period = 0
        self.partials = []
        self.partialInfo = []
        self.patterns = []
        self.patternInfo = []

    def read_partials(self):
        while (path.exists(str(self.period)+".lif")):
            file_lif = open(str(self.period)+".lif", 'r')
            a = []
            for line_str in file_lif:
                a.append(list(line_str.rstrip('\n')))
            b = [x for x in a if x]
            self.partials.append(b)
            self.period += 1

    def create_mirrors(self):
        mp = []
        for p in self.partials:
            rr = []
            for r in p:
                rr.append(list(reversed(list(r))))
            mp.append(rr)
        self.partials.extend(mp)

    def get_dimensions(self):
        for i,p in enumerate(self.partials):
            a = [len(p[0]), len(p)]
            start = 0
            stop = 0
            leftright = ''
            for j,r in enumerate(p):
                if r[0] == '?':
                    leftright = 'l'
                    start = j
                    break
                if r[-1] == '?':
                    leftright = 'r'
                    start = j
                    break
            for j,r in reversed(list(enumerate(p))):
                if r[0] == '?':
                    stop = j
                    break
                if r[-1] == '?':
                    stop = j
                    break
            self.partialInfo.append(a + [leftright, start, stop])

    def create_patterns(self, ncol = 0):
        self.patterns = []
        self.patternInfo = []
        base = (0 if self.partialInfo[0][2] == 'r' else self.period)

        # loop over all left patterns, including its own mirror
        for i in range(self.period):
            j = (base + i + self.period) % (2 * self.period)
            minOffset = self.partialInfo[base][3] - self.partialInfo[j][4]
            maxOffset = self.partialInfo[base][4] - self.partialInfo[j][3]

            # loop over all possible left pattern offsets having at least
            #   one row with unknown cells from both patterns
            for k in range(minOffset, maxOffset + 1):
                width = self.partialInfo[base][0] + ncol + \
                    self.partialInfo[j][0]
                offset = ([-k, 0] if k < 0 else [0, k])
                # discard the negative translations of mirror images,
                #   since they will be the mirror image of the positive
                #   translations.
                if ((j % self.period == 0) and (k < 0)):
                    continue
                height = max(self.partialInfo[base][1] + offset[0],
                    self.partialInfo[j][1] + offset[1]) + self.period
                unkRow = min(self.partialInfo[base][3] + offset[0],
                    self.partialInfo[j][3] + offset[1])
                # draw background
                bg = []
                for _ in range(height):
                    bg.append(['.'] * width)

                # add in the unknown background field starting from row unkRow
                for m in range(unkRow, height):
                    for n in range(ncol + 2):
                        bg[m][self.partialInfo[base][0] - 1 + n] = '?'

                # add left partial on top of the background
                for m in range(self.partialInfo[base][1]):
                    for n in range(self.partialInfo[base][0]):
                        bg[m + offset[0]][n] = self.partials[base][m][n]

                # add right partial on top of the background, right aligned
                for m in range(self.partialInfo[j][1]):
                    for n in range(self.partialInfo[j][0]):
                        bg[m + offset[1]][n + width - self.partialInfo[j][0]] \
                            = self.partials[j][m][n]

                # save pattern and pattern info
                self.patternInfo.append([ncol, j, offset[0], offset[1]])
                self.patterns.append(bg)

    def print_partialInfo(self, period = 0):
        print(self.partialInfo[period])

    def print_partials(self, period = 0):
        print(self.partials[period])

    def write_patterns(self):
        for i,j in enumerate(self.patternInfo):
            dirpath = str(j[0]) + '/' + str(j[1])
            futureGen = str(j[2]) + '_' + str(j[3])
            if (not path.exists(dirpath)):
                makedirs(dirpath)

            # write the lif
            with open(dirpath + '/' + futureGen + ".lif", 'w') as fh:
                for p in self.patterns[i]:
                    row = ''.join(list(p)) + '\n'
                    fh.write(row)
                fh.close()

            # write shell script for this pattern
            with open(dirpath + '/' + str(i) + ".sh", 'w') as fh:
                script = "#!/bin/sh\n$LIFESRCDUMB" \
                    + " -r" + str(len(self.patterns[i])) \
                    + " -c" + str(len(self.patterns[i][0])) \
                    + " -g" + str(self.period) \
                    + " -i " + futureGen + ".lif" \
                    + " -d100000 " + futureGen + ".dmp" \
                    + " -o " + futureGen + ".out\n"
                fh.write(script)
                fh.close()

            # create an extra script for the symmetric case
            if ((j[1] % self.period == 0) and (j[2] == j[3])):
                with open(dirpath + "/s.sh", 'w') as fh:
                    script = "#!/bin/sh\n$LIFESRCDUMB" \
                        + " -r" + str(len(self.patterns[i])) \
                        + " -c" + str(len(self.patterns[i][0])) \
                        + " -g" + str(self.period) \
                        + " -sc" \
                        + " -i " + futureGen + ".lif" \
                        + " -d100000 s.dmp" \
                        + " -o s.out\n"
                    fh.write(script)
                    fh.close()

def main():
    p = Partials()
    p.read_partials()
    p.create_mirrors()
    p.get_dimensions()
    for i in range(5):
        p.create_patterns(i)
        p.write_patterns()

if __name__ == "__main__":
    main()

