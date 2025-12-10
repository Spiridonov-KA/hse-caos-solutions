#!/usr/bin/env python3

import traceback
import sys
import os
import signal
import socket
from time import sleep


def parse_test_info(info_path):
    result = {}
    with open(info_path, 'rt') as inf:
        for line in inf:
            line = line.strip()
            if not line:
                continue
            key, _, val = line.partition('=')
            result[key.strip()] = val.strip()
    return result


RUN_OK = 0
RUN_WA = 1
RUN_PE = 2
RUN_CF = 6


def send_msg(src_port, dst_port, msg, protocol):
    try:
        s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        s.bind(('::1', src_port))
        s.sendto(msg, ('::1', int(dst_port)))
        protocol.buffer.write(sys.stdin.buffer.readline())
    except BrokenPipeError:
        print('no server', file=protocol)
        return RUN_WA
    return RUN_OK


def main():
    _, dat_path, out_path, corr_path, prog_pid, info_path = sys.argv
    info = parse_test_info(info_path)
    signal.signal(signal.SIGPIPE, signal.SIG_IGN)
    protocol = open(out_path, 'wt')

    cmd_argv = info.get('params', '').split()
    dest_port = int(cmd_argv[0])
    status = RUN_OK
    sleep(0.1)

    with open(dat_path, 'rt') as dat:
        for line in dat:
            src_port, msg = line.split(' ', 1)
            status = send_msg(int(src_port), dest_port, msg.rstrip('\n').encode(), protocol)
            if status != RUN_OK:
                break

    if prog_pid:
        os.kill(int(prog_pid), signal.SIGTERM)
    return status


if __name__ == '__main__':
    try:
        status = main()
    except Exception:
        print(traceback.format_exc(), file=sys.stderr)
        status = RUN_CF
    sys.exit(status)
