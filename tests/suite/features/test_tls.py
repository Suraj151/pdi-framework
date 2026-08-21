#!/usr/bin/env python3

"""
The tls command, which provisions the device's own HTTPS/mDNS server certificate.

Generating a certificate is deliberately not exercised here: the command writes
to one fixed path, the live server certificate, so a success run mid-suite would
swap the cert the HTTPS portal and the mDNS advertisement are already serving.
What is asserted is the argument guard that runs before anything is written, so
the command is proven registered and parsing its options without disturbing the
cert on disk.
"""

from .registry import test, expect_in, expect_not_in


@test("tls certgen refuses without a subject and generates nothing",
      needs=("tls",))
def tls_certgen_needs_subject(t):
    """
    `tls q=1` with neither a name nor an ip cannot name the certificate, and the
    command must say so and stop before it writes, not emit a half-formed cert or
    overwrite the live server key. The guard runs ahead of the directory and key
    creation, so the refusal must be reported and no success line must appear.
    """
    out = t.run("tls q=1", timeout=max(t.timeout, 30))
    expect_in("Provide", out, "the guard names what the certgen is missing")
    expect_not_in("successfully", out.lower(),
                  "a refused certgen does not report a generated cert")


@test("tls rejects an unknown query", needs=("tls",))
def tls_unknown_query(t):
    """
    A query outside the command's range is an argument error, not a silent
    success: the command has exactly one action and anything else must be
    reported rather than treated as certgen.
    """
    out = t.run("tls q=9", timeout=max(t.timeout, 20)).lower()
    expect_not_in("successfully", out, "an out-of-range query does not generate a cert")
