
import argparse, base64, hashlib
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import encode_dss_signature
from cryptography.hazmat.backends import default_backend

def genkeys():
    priv = ec.generate_private_key(ec.SECP256R1(), default_backend())
    pub = priv.public_key()
    with open("priv.pem", "wb") as f:
        f.write(priv.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption()
        ))
    with open("pub.pem", "wb") as f:
        f.write(pub.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        ))

def sign(fname, device_id):
    with open(fname, "rb") as f:
        data = f.read()
    digest = hashlib.sha256(data).digest() + device_id.encode()
    priv = serialization.load_pem_private_key(open("priv.pem","rb").read(), password=None, backend=default_backend())
    sig = priv.sign(digest, ec.ECDSA(hashes.SHA256()))
    print(base64.b64encode(sig).decode())

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("cmd", choices=["genkeys","sign"])
    p.add_argument("file", nargs="?")
    p.add_argument("device_id", nargs="?")
    args = p.parse_args()

    if args.cmd=="genkeys":
        genkeys()
    elif args.cmd=="sign":
        sign(args.file, args.device_id)
