#!/usr/bin/python3

import asyncio
import os
import signal
import socket
import subprocess
import sys
import traceback
from time import sleep, time


def parse_ti(info_path):
    result = {}
    with open(info_path, 'rt') as inf:
        for line in inf:
            line = line.strip()
            if not line:
                continue
            key, _, val = line.partition('=')
            result[key.strip()] = val.strip()
    return result


class PresentationError(Exception):
    pass


class Latch:
    def __init__(self, expected, id_=None):
        self.id = id_
        self.remaining = expected
        self.done = asyncio.Event()

    async def arrive_and_wait(self):
        if self.remaining == 0:
            raise RuntimeError('Unexpected arrive_and_wait() call')
        self.remaining -= 1
        if self.remaining == 0:
            self.done.set()
        else:
            await self.done.wait()

    async def wait(self):
        await self.done.wait()

    def __str__(self):
        return f'Latch {self.id}' if self.id is not None else 'Latch'


class Connection:
    def __init__(self, name, protocol):
        self.queue = asyncio.Queue()
        self.name = name
        self.protocol = protocol
        self.reader = None
        self.writer = None
        self.writer_closed = False
        self.reader_closed = False
        self.task = asyncio.create_task(self.run_connection())

    def log(self, message):
        print(f'{self.name}: {message}', file=self.protocol)

    async def init_connection(self):
        if self.reader is None:
            self.log('starting connection')
            await self._connect()
            self.log('started connection')

    async def _connect(self):
        raise NotImplementedError()

    async def send(self, data):
        if self.writer_closed:
            self.log('Cannot send(), writer is closed')
            raise RuntimeError()
        await self.init_connection()
        self.writer.write(data)
        await self.writer.drain()

    async def recv(self, expected):
        if self.reader_closed:
            self.log('Cannot recv(), reader is closed')
            raise RuntimeError()
        await self.init_connection()
        try:
            received = await self.reader.readexactly(len(expected))
        except asyncio.exceptions.IncompleteReadError as e:
            self.log(f'received: {e.partial}')
            assert False, (f'{self.name}: '
                           f'Unexpected EOF (expected {e.expected} bytes, got {len(e.partial)})')
        self.log(f'received: {received}')
        assert received == expected, (f'{self.name}: '
                                      f'Incorrect data (expected {expected}, received {received})')

    def close_writer(self):
        if not self.writer_closed:
            self.writer.write_eof()
            self.writer_closed = True

    def close_reader(self):
        if not self.reader_closed:
            # not sure if this is safe, but it works
            self.reader._transport._sock.shutdown(socket.SHUT_RD)
            self.reader_closed = True

    async def run_connection(self):
        try:
            while True:
                method, arg = await self.queue.get()
                self.log(f'method: {method}, arg: {arg}')
                if method == 'sleep':
                    await asyncio.sleep(arg)
                elif method == 'wait':
                    await arg.wait()
                elif method == 'arrive':
                    await arg.arrive_and_wait()
                elif method == 'conn':
                    await self.init_connection()
                elif method == 'send':
                    await self.send(arg)
                elif method == 'recv':
                    await self.recv(arg)
                elif method == 'stop':
                    self.close_writer()
                elif method == 'rstop':
                    self.close_reader()
                elif method == 'done':
                    self.close_writer()
                    if self.reader_closed:
                        # impossible to check
                        break
                    tail = await self.reader.read(10)
                    self.log(f'tail: {tail}')
                    assert tail == b'', f'{self.name}: Garbage data ({tail}) where EOF was expected'
                    break
                else:
                    self.log(f'unexpected method {method}')
                    raise RuntimeError()
        except (ConnectionError, FileNotFoundError, OSError) as e:
            print(traceback.format_exc(), file=sys.stderr)
            raise PresentationError(f'{self.name}: {e}')


class TCPConnection(Connection):
    def __init__(self, name, protocol, port):
        super().__init__(name, protocol)
        self.port = port

    async def _connect(self):
        self.reader, self.writer = await asyncio.open_connection('::1', self.port)


class UnixConnection(Connection):
    def __init__(self, name, protocol, socket_path):
        super().__init__(name, protocol)
        self.socket_path = socket_path

    async def _connect(self):
        self.reader, self.writer = await asyncio.open_unix_connection(self.socket_path)


async def process_input(dat_path, port, unix_path, protocol):
    connections = {}
    last_latch = None
    with open(dat_path, 'rt') as dat:
        for line in dat:
            if line.isspace() or line[0] == '#':
                continue
            if line[0] == '=' and set(line.strip()) == {'='}:
                last_id = last_latch.id if last_latch else -1
                last_latch = Latch(len(connections), last_id + 1)
                for conn in connections.values():
                    await conn.queue.put(('arrive', last_latch))
                continue
            conn_idx, *cmd = line.split(' ', 2)
            if not cmd:
                raise RuntimeError(f'empty cmd: "{line}"')
            elif len(cmd) == 1:
                method = cmd[0].rstrip('\n')
                arg = None
            else:
                method, arg = cmd

            if method == 'sleep':
                arg = float(arg)
            elif arg is not None:
                arg = arg.encode('ascii')
            try:
                conn = connections[conn_idx]
            except KeyError:
                if method != 'type':
                    raise RuntimeError(f'{conn_idx}: socket type is not specified')
                arg = arg.decode().rstrip('\n')
                if arg == 'tcp':
                    conn = TCPConnection(conn_idx, protocol, port)
                elif arg == 'unix':
                    conn = UnixConnection(conn_idx, protocol, unix_path)
                else:
                    raise RuntimeError(f'{conn_idx}: unknown socket type "{arg}"')

                connections[conn_idx] = conn
                if last_latch:
                    await conn.queue.put(('wait', last_latch))
            else:
                await conn.queue.put((method, arg))
    for conn in connections.values():
        await conn.queue.put(('done', None))
    await asyncio.gather(*[conn.task for conn in connections.values()])


RUN_OK = 0
RUN_WA = 1
RUN_PE = 2
RUN_CF = 6


def main():
    _, dat_path, out_path, corr_path, prog_pid, info_path = sys.argv
    info = parse_ti(info_path)
    signal.signal(signal.SIGPIPE, signal.SIG_IGN)
    protocol = open(out_path, 'wt')

    cmd_argv = info.get('params', '').split()
    port = int(cmd_argv[0])
    unix_path = cmd_argv[1]
    status = RUN_OK
    sleep(0.1)

    try:
        asyncio.run(process_input(dat_path, port, unix_path, protocol), debug=True)
    except AssertionError as e:
        print(str(e) or 'something is wrong (WA)', file=protocol)
        status = RUN_WA
    except PresentationError as e:
        print(str(e) or 'something is wrong (PE)', file=protocol)
        status = RUN_PE

    if prog_pid:
        try:
            os.kill(int(prog_pid), signal.SIGTERM)
        except:
            pass
    protocol.write(sys.stdin.read())
    return status


if __name__ == '__main__':
    try:
        status = main()
    except Exception:
        print(traceback.format_exc(), file=sys.stderr)
        status = RUN_CF
    sys.exit(status)
