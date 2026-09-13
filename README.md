# BilingualReaderVita

Leitor moderno e leve de Livros e Mangás para o PlayStation Vita (PS Vita), desenvolvido em C/C++ utilizando VitaSDK e renderização acelerada por GPU via Vita2D.

## Recursos
- Suporte a múltiplos formatos: `.txt`, `.cbz` e `.epub`.
- Interface moderna, rápida e escura, com cards visuais estilizados e badges coloridos.
- Aceleração por GPU para renderização de texto e imagens.
- Leitura direta da biblioteca interna em `ux0:data/BilingualReaderVita/`.
- Preparado para recursos bilíngues e sincronização futura.

## Estrutura do Projeto
- `src/main.cpp`: Ponto de entrada, loop de renderização (60 FPS) e máquina de estados.
- `src/ui_components.cpp/h`: Componentes visuais (cards, cabeçalho, rodapé, badges, cores).
- `src/file_browser.cpp/h`: Leitura e gerenciamento de arquivos em `ux0:data/BilingualReaderVita/`.
- `src/reader_txt.cpp/h`: Motor de leitura e paginação para arquivos `.txt`.
- `src/reader_cbz.cpp/h`: Motor de leitura de mangá para arquivos `.cbz`.
- `src/reader_epub.cpp/h`: Motor de leitura de livros para arquivos `.epub`.

## Como Compilar
Certifique-se de ter o [VitaSDK](https://vitasdk.org/) instalado e configurado no seu sistema:

```bash
mkdir build
cd build
cmake ..
make
```

O arquivo `BilingualReaderVita.vpk` será gerado e poderá ser instalado no console via VitaShell ou executado no Vita3K.
