#!/bin/bash
set -e

# =============================
# 🔧 Konfigurasi Subjek Umum
# =============================
COUNTRY="ID"
STATE="Bali"
CITY="Denpasar"
O="AriasaProp"
OU="Fermity"
CN="localhost"
SUBJ="/C=$COUNTRY/ST=$STATE/L=$CITY/O=$O/OU=$OU/CN=$CN"
LIFE=3650
KEY_LENGTH=2048

# clean old certificate
ls -1 *.pem *.csr | xargs rm -f

SAN_CONF=server-san.cnf
cat > $SAN_CONF <<EOF
[req]
distinguished_name=req
req_extensions=san
[ san ]
subjectAltName=DNS:localhost,IP:127.0.0.1
EOF

echo "Generate private key"
openssl genrsa -out key.pem $KEY_LENGTH

echo "Generate certificate signing request"
openssl req -new -key key.pem -out crt-req.csr -subj $SUBJ -config $SAN_CONF -reqexts san

echo "Generate self-signed certificate for 10 years"
openssl x509 -req -days $LIFE -in crt-req.csr -signkey key.pem -out crt.pem -subj $SUBJ

rm -f $SAN_CONF

echo "✅ Certificates done: "
ls -1 *.pem *.csr
