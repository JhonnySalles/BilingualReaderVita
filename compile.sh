#!/usr/bin/env bash
set -e

# Obter diretorio raiz do projeto
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "======================================================="
echo "  BilingualReaderVita - Build Bash (MSYS2 / Linux)"
echo "======================================================="
echo ""

export VITASDK="/usr/local/vitasdk"
export PATH="$VITASDK/bin:$PATH"
echo "[*] VITASDK definido para: $VITASDK"

# Criar pastas necessarias
mkdir -p "$SCRIPT_DIR/build"
mkdir -p "$SCRIPT_DIR/apk"

# Entrar no diretorio de build
cd "$SCRIPT_DIR/build"

echo "[*] Executando CMake..."
cmake -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" -G "Unix Makefiles" ..

echo "[*] Compilando projeto..."
make -j"$(nproc 2>/dev/null || echo 4)"

# Copiar os binarios finais para a pasta apk
if [ -f "$SCRIPT_DIR/build/BilingualReaderVita.vpk" ]; then
    echo "[*] Copiando BilingualReaderVita.vpk para a pasta apk/..."
    cp -f "$SCRIPT_DIR/build/BilingualReaderVita.vpk" "$SCRIPT_DIR/apk/"
fi

if [ -f "$SCRIPT_DIR/build/eboot.bin" ]; then
    cp -f "$SCRIPT_DIR/build/eboot.bin" "$SCRIPT_DIR/apk/" 2>/dev/null || true
fi

echo ""
echo "======================================================="
if [ -f "$SCRIPT_DIR/apk/BilingualReaderVita.vpk" ]; then
    echo "[OK] Build concluido com sucesso!"
    echo "[OK] VPK gerado em: $SCRIPT_DIR/apk/BilingualReaderVita.vpk"
else
    echo "[!] O arquivo BilingualReaderVita.vpk nao foi gerado."
fi
echo "======================================================="
