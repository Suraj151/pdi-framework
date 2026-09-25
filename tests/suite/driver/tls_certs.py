#!/usr/bin/env python3

"""
A self signed certificate for a test listener, made fresh per run.

The board reaches the test machine by address rather than by name, and a client
that verifies its peer checks the address against the certificate — so the
certificate has to carry the address as an IP SAN, and the address is only known
once the fixture has worked out which of this machine's interfaces faces the
board. Nothing here is reusable between runs for that reason.

Returns paths rather than bytes because both the broker (load_cert_chain) and
the device upload want a file.
"""

import datetime
import ipaddress
import os

try:
    from cryptography import x509
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import rsa
    from cryptography.x509.oid import NameOID
    AVAILABLE = True
except ImportError:
    AVAILABLE = False


def self_signed(address, directory, name="pdi-test-broker"):
    """
    Write a certificate and key naming this address, and return both paths.

    The key is RSA because the framework's cipher list leads with
    ECDHE_RSA, and 2048 bits keeps the handshake inside what a small
    device will spend on it.
    """
    if not AVAILABLE:
        raise RuntimeError("python cryptography is not installed")

    key = rsa.generate_private_key(public_exponent=65537, key_size=2048)

    subject = x509.Name([
        x509.NameAttribute(NameOID.COMMON_NAME, name),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, "PDI Framework Tests"),
    ])

    now = datetime.datetime.utcnow()
    cert = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(subject)
        .public_key(key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(now - datetime.timedelta(days=1))
        .not_valid_after(now + datetime.timedelta(days=30))
        .add_extension(
            x509.SubjectAlternativeName([
                x509.IPAddress(ipaddress.ip_address(address)),
            ]),
            critical=False,
        )
        .add_extension(
            x509.BasicConstraints(ca=True, path_length=None),
            critical=True,
        )
        .sign(key, hashes.SHA256())
    )

    certpath = os.path.join(directory, "%s.crt" % name)
    keypath = os.path.join(directory, "%s.key" % name)

    with open(certpath, "wb") as handle:
        handle.write(cert.public_bytes(serialization.Encoding.PEM))

    with open(keypath, "wb") as handle:
        handle.write(key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        ))

    return certpath, keypath
