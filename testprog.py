#!/usr/bin/env python

import os, tempfile

PAGE_SIZE = 4096
TEST_PROG = './testprog'

def thread_name(i):
    return 'testprog' if i == 0 else 'thread-%d' % i

def parse_usage(lines):
    usage = {}

    for line in lines:
        l = line.split()
        tname = l[3]
        stack_usage = int(l[-1])
        usage[tname] = stack_usage

    return usage

def main():
    args = [ PAGE_SIZE*i for i in [0, 1, 10, 100, 1000] ]

    with tempfile.NamedTemporaryFile() as outfile:

        test_env = os.environ.copy()
        test_env['LD_PRELOAD'] = './stackreport.so'
        test_env['SR_OUTPUT_FILE'] = outfile.name

        cmd_line = [TEST_PROG] + map(str, args)
        #cmd_line = 'gdb --args'.split() + cmd_line
        print 'run: %s' % ' '.join(cmd_line)
        rc = os.spawnvpe(os.P_WAIT, cmd_line[0], cmd_line, test_env)
        assert rc == 0, 'unexpected rc: %d' % rc

        outfile.file.seek(0)
        usage = parse_usage(outfile.readlines())

    print usage
    for i in range(len(args)):
        tname = thread_name(i)
        assert tname in usage
        print '%s: %u' % (tname, usage[tname])
        assert usage[tname] >= args[i]
        assert usage[tname] - args[i] <= PAGE_SIZE * 4

    print 'all OK'

if __name__ == '__main__':
    main()
