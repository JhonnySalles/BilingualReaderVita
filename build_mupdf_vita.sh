#!/usr/bin/env bash
set -e

echo "=== Configurando variaveis para VitaSDK ==="
export VITASDK="/usr/local/vitasdk"
export PATH="$VITASDK/bin:$PATH"

export CC=arm-vita-eabi-gcc
export CXX=arm-vita-eabi-g++
export AR=arm-vita-eabi-ar
export LD=arm-vita-eabi-ld
export RANLIB=arm-vita-eabi-ranlib

cd libs/mupdf_src

echo "=== Compilando geradores host primeiro se necessario ==="
# O MuPDF precisa de ferramentas host (fontdump, cmapdump) compiladas para o sistema host
# Vamos compilar a biblioteca estatica do MuPDF desabilitando apps de desktop e tesseract
make -j4 HAVE_X11=no HAVE_GLUT=no HAVE_CURL=no HAVE_LEPTONICA=no HAVE_TESSERACT=no USE_SYSTEM_LIBS=no libs

echo "=== Copiando libs e headers para libs/mupdf ==="
mkdir -p ../mupdf/lib
mkdir -p ../mupdf/include

cp -rf include/* ../mupdf/include/
cp -f build/release/libmupdf.a ../mupdf/lib/
cp -f build/release/libmupdf-third.a ../mupdf/lib/ 2>/dev/null || true

echo "=== Concluido com sucesso! ==="
