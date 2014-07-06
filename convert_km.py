#!/usr/bin/env python
#-*- coding: utf-8 -*-

"""This script convert xrdp's km-xxxx.ini keymap file to vnc2rdp code"""

import re
import sys

def main():
    if len(sys.argv) != 2:
        print 'Usage: %s FILE' % sys.argv[0]

    km_filename = sys.argv[1]
    km_file = open(km_filename, 'r')

    km = {
        'noshift': [0, 0, 0, 0, 0, 0, 0, 0],
        'shift': [0, 0, 0, 0, 0, 0, 0, 0],
        'altgr': [0, 0, 0, 0, 0, 0, 0, 0],
        'capslock': [0, 0, 0, 0, 0, 0, 0, 0],
        'shiftcapslock': [0, 0, 0, 0, 0, 0, 0, 0]
    }
    lines = km_file.readlines()
    keysym_rx = re.compile(r'^Key.*=(?P<keysym>.*):.*$')
    for line in lines:
        if line.startswith('['):
            section = line[1:len(line) - 2]
            continue
        if not keysym_rx.match(line):
            continue
        keysym = int(keysym_rx.sub(r'\g<keysym>', line))
        km[section].append(keysym)

    for i, key in enumerate(km.keys()):
        print '.%s = {' % key
        for j, keysym in enumerate(km[key]):
            if j % 8 == 0:
                prefix = '\t'
            else:
                prefix = ''
            if j != len(km[key]) - 1:
                suffix = ','
                if (j + 1) % 8 != 0:
                    suffix += ' '
                else:
                    suffix += '\n'
            else:
                suffix = '\n'
            sys.stdout.write('%s0x%04x%s' % (prefix, keysym, suffix))
        if i == len(km.keys()) - 1:
            print '}'
        else:
            print '},'

if __name__ == '__main__':
    main()
