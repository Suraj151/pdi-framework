#!/usr/bin/env python3

"""
Turning a --device string into a shell.

    serial:/dev/ttyUSB0            a board on a cable, at the default baud
    serial:/dev/ttyUSB0@115200     with the baud named
    ssh:user@192.168.1.50          the board's own ssh server
    ssh:user@192.168.1.50:2222     on another port
    telnet:192.168.1.50            the board's telnet service
    telnet:127.0.0.1:10023         a host process, on the port it fell back to
    host:/path/to/pdid             the stack as a host process

Only the transport a target names is imported, so a machine without pyserial
can still run the ssh tier and the other way round.

A serial target is attached to as it is found, not reset. Pulsing EN would give
a predictable banner but also drops the board's network association, and the
tests that need one would skip for minutes afterwards.
"""

from .shell import ShellError


def parse(spec):
    """(scheme, remainder) from a --device string, defaulting to serial."""
    if ":" not in spec:
        return "serial", spec

    scheme, _, rest = spec.partition(":")
    if scheme not in ("serial", "ssh", "telnet", "host"):
        # a bare path with a colon in it is still a serial port
        return "serial", spec

    return scheme, rest


def connect(spec, username=None, password=None, key_filename=None, reset=False):
    """
    Open the target and return (shell, description). The caller does the boot
    wait and the login, because what those mean differs per transport.
    """
    scheme, rest = parse(spec)

    if scheme == "serial":
        from .serial_shell import SerialShell, DEFAULT_BAUD

        port, _, baud = rest.partition("@")
        baud = int(baud) if baud else DEFAULT_BAUD
        return SerialShell(port, baud=baud, reset=reset), "%s at %d" % (port, baud)

    if scheme == "ssh":
        from .ssh_shell import SshShell, DEFAULT_PORT

        userpart, _, hostpart = rest.rpartition("@")
        if not hostpart:
            raise ShellError("ssh target needs a host, e.g. ssh:user@10.0.0.5")

        host, _, port = hostpart.partition(":")
        port = int(port) if port else DEFAULT_PORT
        user = userpart or username

        shell = SshShell(host, port=port, username=user, password=password,
                         key_filename=key_filename)
        return shell, "%s@%s:%d" % (user, host, port)

    if scheme == "telnet":
        from .telnet_shell import TelnetShell, DEFAULT_PORT as TELNET_PORT

        host, _, port = rest.partition(":")
        port = int(port) if port else TELNET_PORT
        shell = TelnetShell(host, port=port)
        return shell, "telnet %s:%d" % (host, shell.port)

    from .host_shell import HostShell

    return HostShell(rest), "pdid at %s" % rest
