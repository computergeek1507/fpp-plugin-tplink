#!/bin/bash
# Builds the native Tapo client tests and runs them against fake_tapo.py.
# Needs a C++17 compiler, OpenSSL headers and python3 with python-kasa
# (pip install python-kasa), whose KLAP code is the reference.
set -e
cd "$(dirname "$0")"
PYTHON=${PYTHON:-python3}
PORT=${PORT:-18080}

c++ -std=c++17 -Wall -Wextra -O1 -o tapo_tests tapo_tests.cpp \
    ../src/core/Klap.cpp ../src/core/TapoClient.cpp ../src/core/MiniJson.cpp \
    $(pkg-config --cflags --libs libcrypto 2>/dev/null || echo -lcrypto) -lpthread

# Known answers from python-kasa for the hash and cipher checks.
eval "$("$PYTHON" - <<'PY'
from kasa.credentials import Credentials
from kasa.transports.klaptransport import KlapTransport, KlapTransportV2, KlapEncryptionSession
c = Credentials("a@b.c", "pw")
print("export EXPECT_V2_AUTH=" + KlapTransportV2.generate_auth_hash(c).hex())
print("export EXPECT_V1_AUTH=" + KlapTransport.generate_auth_hash(c).hex())
import base64
print("export EXPECT_V1_B64=" + base64.b64encode(KlapTransport.generate_auth_hash(c)).decode())
print("export EXPECT_V2_B64=" + base64.b64encode(KlapTransportV2.generate_auth_hash(c)).decode())
s = KlapEncryptionSession(b"\x01" * 16, b"\x02" * 16, KlapTransportV2.generate_auth_hash(c))
enc, seq = s.encrypt(b'{"method":"get_device_info"}')
print("export EXPECT_SEQ=%d" % seq)
print("export EXPECT_ENC=" + enc.hex())
PY
)"

"$PYTHON" fake_tapo.py "$PORT" &
SERVER=$!
trap 'kill $SERVER 2>/dev/null' EXIT
for _ in $(seq 50); do
    curl -s "http://127.0.0.1:$PORT/test/state" >/dev/null 2>&1 && break
    sleep 0.1
done

./tapo_tests "$PORT"

# The settings page's PHP must produce the same hashes as python-kasa.
if command -v php >/dev/null; then
    php test_tapo_account.php "$EXPECT_V1_B64" "$EXPECT_V2_B64"
    "$PYTHON" fake_discovery.py 19999 20012 &
    DISCO=$!
    trap 'kill $SERVER $DISCO 2>/dev/null' EXIT
    sleep 0.5
    php test_discovery.php 19999 20012
fi
