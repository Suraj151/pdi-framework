#!/usr/bin/env python3

"""
Generate example self-signed TLS certificate + key for the pdi-framework
HTTPS server on a device (ESP8266, etc.). Output PEM files match the
default paths declared in src/config/TlsConfig.h, so the generated
artefacts can be uploaded directly to the device filesystem.

Examples:

    # Server cert for a device reachable as esp-device.local on 192.168.1.50
    ./GenTlsCerts.py --dns esp-device.local --ip 192.168.1.50

    # Multiple SAN entries
    ./GenTlsCerts.py --dns esp.local --dns dev.local --ip 10.0.0.7 --ip 192.168.1.50

    # Use RSA 2048 instead of EC P-256
    ./GenTlsCerts.py --dns esp.local --key-type rsa

    # Also produce a CA + client cert pair for mTLS testing
    ./GenTlsCerts.py --dns esp.local --ip 192.168.1.50 --with-client-ca

    # Step 1: generate a reusable dev CA (run once). Upload ca.crt
    # to /etc/ssl/ca-bundle.crt on every device that should trust it.
    ./GenTlsCerts.py --gen-ca

    # Step 2: mint a server cert for any test target, signed by that CA.
    # Repeat for each new domain/IP; device trusts them all via the CA.
    ./GenTlsCerts.py --ca-cert certs/ca.crt --ca-key certs/ca.key \
                     --dns api.test.local --ip 192.168.1.60

Requires the `openssl` command-line tool on PATH.
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_OUTPUT_DIR = os.path.join(SCRIPT_DIR, os.pardir, "certs")


def run(cmd, **kwargs):
    """Run a shell command, propagating output and raising on failure."""
    proc = subprocess.run(cmd, **kwargs)
    if proc.returncode != 0:
        raise SystemExit(
            f"Command failed (exit {proc.returncode}): {' '.join(cmd)}"
        )
    return proc


def ensure_openssl():
    if shutil.which("openssl") is None:
        sys.exit("Error: openssl not found on PATH. Install openssl and retry.")


def write_san_config(dns_list, ip_list, cn, basic_ca=False):
    """
    Write a temporary openssl config file that drives 'req' with the
    desired SubjectAltName / basicConstraints. Returns the path; the
    caller is responsible for unlinking it.
    """
    lines = [
        "[req]",
        "distinguished_name = req_dn",
        "req_extensions = v3_req",
        "prompt = no",
        "",
        "[req_dn]",
        f"CN = {cn}",
        "",
        "[v3_req]",
    ]
    if basic_ca:
        lines.append("basicConstraints = critical, CA:TRUE")
        lines.append("keyUsage = critical, digitalSignature, keyCertSign, cRLSign")
    else:
        lines.append("basicConstraints = critical, CA:FALSE")
        lines.append("keyUsage = critical, digitalSignature, keyEncipherment")
        lines.append("extendedKeyUsage = serverAuth, clientAuth")

    san_entries = []
    for i, dns in enumerate(dns_list, start=1):
        san_entries.append(f"DNS.{i} = {dns}")
    for i, ip in enumerate(ip_list, start=1):
        san_entries.append(f"IP.{i} = {ip}")
    if san_entries:
        lines.append("subjectAltName = @alt_names")
        lines.append("")
        lines.append("[alt_names]")
        lines.extend(san_entries)

    fd, path = tempfile.mkstemp(suffix=".cnf", text=True)
    with os.fdopen(fd, "w") as f:
        f.write("\n".join(lines) + "\n")
    return path


def generate_key(key_type, out_path):
    if key_type == "ec":
        run(["openssl", "ecparam", "-genkey", "-name", "prime256v1",
             "-noout", "-out", out_path])
    elif key_type == "rsa":
        run(["openssl", "genrsa", "-out", out_path, "2048"])
    else:
        sys.exit(f"Unknown key type: {key_type}")


def self_signed_cert(key_path, cert_path, conf_path, days):
    run([
        "openssl", "req", "-new", "-x509",
        "-key", key_path,
        "-out", cert_path,
        "-days", str(days),
        "-config", conf_path,
        "-extensions", "v3_req",
    ])


def issue_cert_from_ca(ca_key, ca_cert, key_path, cert_path, conf_path, days):
    csr_fd, csr_path = tempfile.mkstemp(suffix=".csr")
    os.close(csr_fd)
    try:
        run([
            "openssl", "req", "-new",
            "-key", key_path,
            "-out", csr_path,
            "-config", conf_path,
        ])
        run([
            "openssl", "x509", "-req",
            "-in", csr_path,
            "-CA", ca_cert,
            "-CAkey", ca_key,
            "-CAcreateserial",
            "-out", cert_path,
            "-days", str(days),
            "-extfile", conf_path,
            "-extensions", "v3_req",
        ])
    finally:
        try:
            os.unlink(csr_path)
        except OSError:
            pass


def main():
    parser = argparse.ArgumentParser(
        description="Generate example TLS certificates for pdi-framework HTTPS server.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Upload the generated server.crt and server.key to /etc/http/ on the device.\n"
            "If --with-client-ca was used, also upload client-ca.crt to /etc/http/ for mTLS\n"
            "and import client.crt+client.key into your browser or curl invocation."
        ),
    )
    parser.add_argument("--dns", action="append", default=[],
                        help="DNS name to include in SubjectAltName (repeatable).")
    parser.add_argument("--ip", action="append", default=[],
                        help="IP address to include in SubjectAltName (repeatable).")
    parser.add_argument("--cn", default=None,
                        help="Subject Common Name. Defaults to the first --dns "
                             "or, if none, the first --ip, or 'esp-device'.")
    parser.add_argument("--days", type=int, default=365,
                        help="Certificate validity period (default: 365).")
    parser.add_argument("--key-type", choices=("ec", "rsa"), default="ec",
                        help="Key type for server cert. ec = P-256 (recommended), "
                             "rsa = 2048-bit. Default: ec.")
    parser.add_argument("--output-dir", default=DEFAULT_OUTPUT_DIR,
                        help=f"Output directory. Default: {DEFAULT_OUTPUT_DIR}")
    parser.add_argument("--with-client-ca", action="store_true",
                        help="Also generate a CA + client cert/key pair "
                             "for mTLS testing. Server cert will be issued by "
                             "this CA so the server can also be validated by clients.")
    parser.add_argument("--gen-ca", action="store_true",
                        help="Generate a reusable dev CA only (ca.key + ca.crt) "
                             "and exit. Use the resulting ca.crt as the trust "
                             "bundle on devices, then sign per-domain server "
                             "certs with --ca-cert/--ca-key in a follow-up run.")
    parser.add_argument("--ca-cert", default=None,
                        help="Path to an existing CA certificate (PEM) to issue "
                             "the server cert from. Must be paired with --ca-key.")
    parser.add_argument("--ca-key", default=None,
                        help="Path to the matching CA private key (PEM). Must "
                             "be paired with --ca-cert.")
    args = parser.parse_args()

    ensure_openssl()

    if args.gen_ca and (args.with_client_ca or args.ca_cert or args.ca_key
                       or args.dns or args.ip):
        sys.exit("Error: --gen-ca is exclusive; do not combine with "
                 "--with-client-ca, --ca-cert/--ca-key, --dns, or --ip.")

    if bool(args.ca_cert) != bool(args.ca_key):
        sys.exit("Error: --ca-cert and --ca-key must be provided together.")

    if args.ca_cert and args.with_client_ca:
        sys.exit("Error: --with-client-ca generates a fresh CA; cannot also "
                 "consume one via --ca-cert/--ca-key.")

    if not args.gen_ca and not args.dns and not args.ip:
        sys.exit("Error: provide at least one --dns or --ip for SubjectAltName.")

    out_dir = os.path.abspath(args.output_dir)
    os.makedirs(out_dir, exist_ok=True)

    if args.gen_ca:
        ca_key = os.path.join(out_dir, "ca.key")
        ca_crt = os.path.join(out_dir, "ca.crt")
        cn = args.cn or "pdi-dev-ca"

        print(f"[*] Output directory : {out_dir}")
        print(f"[*] CA CN            : {cn}")
        print(f"[*] CA key type      : {args.key_type}")
        print(f"[*] Validity         : {args.days} days")
        print()

        ca_conf = write_san_config([], [], cn, basic_ca=True)
        try:
            print("[*] Generating CA private key ...")
            generate_key(args.key_type, ca_key)
            print("[*] Generating self-signed CA cert ...")
            self_signed_cert(ca_key, ca_crt, ca_conf, args.days)
        finally:
            try:
                os.unlink(ca_conf)
            except OSError:
                pass

        print()
        print("[+] Done.")
        print()
        print("Keep ca.key SECRET (only this machine needs it to sign new certs).")
        print(f"Upload this to every device's LittleFS:")
        print(f"  {ca_crt}  -> /etc/ssl/ca-bundle.crt")
        print()
        print("Then mint per-domain server certs with:")
        print(f"  ./GenTlsCerts.py --ca-cert {ca_crt} --ca-key {ca_key} "
              f"--dns <hostname> --ip <ip>")
        return

    if args.cn is None:
        args.cn = args.dns[0] if args.dns else (args.ip[0] if args.ip else "esp-device")

    server_key = os.path.join(out_dir, "server.key")
    server_crt = os.path.join(out_dir, "server.crt")

    print(f"[*] Output directory : {out_dir}")
    print(f"[*] Subject CN       : {args.cn}")
    print(f"[*] SAN DNS          : {args.dns or '<none>'}")
    print(f"[*] SAN IP           : {args.ip or '<none>'}")
    print(f"[*] Server key type  : {args.key_type}")
    print(f"[*] Validity         : {args.days} days")
    if args.ca_cert:
        print(f"[*] Issuer CA cert   : {args.ca_cert}")
        print(f"[*] Issuer CA key    : {args.ca_key}")
    print()

    server_conf = write_san_config(args.dns, args.ip, args.cn, basic_ca=False)
    try:
        print("[*] Generating server private key ...")
        generate_key(args.key_type, server_key)

        if args.ca_cert:
            print("[*] Issuing server cert from provided CA ...")
            issue_cert_from_ca(args.ca_key, args.ca_cert, server_key, server_crt,
                               server_conf, args.days)
        elif args.with_client_ca:
            ca_dir = out_dir
            ca_key = os.path.join(ca_dir, "client-ca.key")
            ca_crt = os.path.join(ca_dir, "client-ca.crt")
            client_key = os.path.join(ca_dir, "client.key")
            client_crt = os.path.join(ca_dir, "client.crt")

            ca_conf = write_san_config([], [], f"{args.cn}-ca", basic_ca=True)
            client_conf = write_san_config(["client"], [], "client", basic_ca=False)
            try:
                print("[*] Generating client CA key + cert ...")
                generate_key(args.key_type, ca_key)
                self_signed_cert(ca_key, ca_crt, ca_conf, args.days)

                print("[*] Issuing server cert from CA ...")
                issue_cert_from_ca(ca_key, ca_crt, server_key, server_crt,
                                   server_conf, args.days)

                print("[*] Generating client cert (for mTLS testing) ...")
                generate_key(args.key_type, client_key)
                issue_cert_from_ca(ca_key, ca_crt, client_key, client_crt,
                                   client_conf, args.days)
            finally:
                for p in (ca_conf, client_conf):
                    try:
                        os.unlink(p)
                    except OSError:
                        pass
        else:
            print("[*] Generating self-signed server cert ...")
            self_signed_cert(server_key, server_crt, server_conf, args.days)
    finally:
        try:
            os.unlink(server_conf)
        except OSError:
            pass

    print()
    print("[+] Done.")
    print()
    print("Upload these to the device's LittleFS:")
    print(f"  {server_crt}  -> /etc/http/server.crt")
    print(f"  {server_key}  -> /etc/http/server.key")
    if args.with_client_ca:
        print(f"  {os.path.join(out_dir, 'client-ca.crt')}  -> /etc/http/client-ca.crt")
        print()
        print("Test mTLS with curl:")
        print(f"  curl --cacert {server_crt} \\")
        print(f"       --cert   {os.path.join(out_dir, 'client.crt')} \\")
        print(f"       --key    {os.path.join(out_dir, 'client.key')} \\")
        print(f"       https://{args.dns[0] if args.dns else args.ip[0]}/")
    elif args.ca_cert:
        print()
        print(f"Devices that already trust {args.ca_cert} (as /etc/ssl/ca-bundle.crt)")
        print("will accept this server cert with no further setup.")
        print()
        print("Test with curl:")
        print(f"  curl --cacert {args.ca_cert} https://{args.dns[0] if args.dns else args.ip[0]}/")
    else:
        print()
        print("Test with curl:")
        print(f"  curl --cacert {server_crt} https://{args.dns[0] if args.dns else args.ip[0]}/")


if __name__ == "__main__":
    main()
