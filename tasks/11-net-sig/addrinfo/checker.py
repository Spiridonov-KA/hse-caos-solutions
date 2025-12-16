#!/usr/bin/env python3
from typing import Any
from typing import Optional
import sys
import socket
import ipaddress


def resolve(host: str, service: str) -> str:
    try:
        addresses = socket.getaddrinfo(host, service, family=socket.AF_INET, type=socket.SOCK_STREAM)
    except socket.gaierror as e:
        assert e.strerror is not None
        return e.strerror.strip()

    ans: Optional[tuple[Any, str, str]] = None
    for _, _, _, _, sockaddr in addresses:
        addr, port = sockaddr  # type: ignore
        ip = ipaddress.IPv4Address(addr)
        candidate = (ip, addr, port)
        if ans is None or candidate < ans:
            ans = candidate  # type: ignore

    assert ans is not None
    _, addr, port = ans
    return f'{addr}:{port}'


def main():
    if len(sys.argv) != 6:
        raise RuntimeError(f"Wrong number of arguments: {len(sys.argv) - 1} vs 5")

    _, _, inp, ouf, _, _ = sys.argv
    with open(inp, 'r') as inp, open(ouf, 'r') as out:
        inp_lines = inp.read().strip().split('\n')
        out_lines = out.read().strip().split('\n')
        if len(inp_lines) != len(out_lines):
            raise RuntimeError(f"Wrong number of lines in the output: {len(inp_lines)} expected, found: {len(out_lines)}")
        for i, (req, resp) in enumerate(zip(inp_lines, out_lines)):
            resp = resp.strip()
            correct_resp = resolve(*req.split())
            if correct_resp != resp:
                print(f"Wrong answer at line {i}: got \"{resp}\", expected \"{correct_resp}\"")
                sys.exit(1)


if __name__ == '__main__':
    main()
