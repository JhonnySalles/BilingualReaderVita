# Guia Completo de Instalação, Configuração e Compilação: BilingualReaderVita

Este guia detalha passo a passo todos os pré-requisitos, a ordem correta de instalação das dependências, a configuração do ambiente de desenvolvimento VitaSDK, os processos de compilação automatizada e a execução/instalação do pacote `.vpk` gerado (tanto no emulador Vita3K quanto no console PS Vita real).

---

## 1. Visão Geral do Projeto e Dependências

O **BilingualReaderVita** é um leitor de livros digitais, mangás e quadrinhos desenvolvido em **C++17** com suporte a formatos `CBZ`, `CBR (RAR)`, `EPUB`, `PDF`, `TXT` e `TAR`.

### Dependências Utilizadas:
1. **Bibliotecas do Sistema PS Vita** (inclusas no VitaSDK stubs): `SceDisplay`, `SceGxm`, `SceCtrl`, `SceTouch`, `ScePower`, `SceSysmodule`, `SceAppMgr`, `SceCommonDialog`, `ScePgf`, `ScePvf`, `SceIme`.
2. **Bibliotecas Portadas via VDPM**:
   - `vita2d` (Renderização 2D acelerada via GPU GXM)
   - `freetype` (Renderização de fontes TTF/OTF)
   - `libpng` / `png` (Decodificação de imagens PNG)
   - `libjpeg-turbo` / `jpeg` (Decodificação de imagens JPEG)
   - `zlib` / `z` (Descompressão e suporte a ZIP/CBZ)
3. **Bibliotecas Embutidas no Repositório (`libs/`)**:
   - `libs/unrar` (Código C++ compilado estaticamente pelo CMake para suporte a arquivos RAR / CBR)
   - `libs/mupdf` (Binários estáticos e headers do MuPDF para renderização de PDF e EPUB)

---

## 2. Pré-requisitos de Sistema

Para compilar projetos para o PlayStation Vita no Windows, é necessário o ambiente Unix **MSYS2**, que fornece as ferramentas de compilação (`gcc`, `make`, `cmake`, `tar`, etc.) e gerencia os pacotes do VitaSDK.

### Download do MSYS2:
- Baixe o instalador oficial do **MSYS2** em: [https://www.msys2.org/](https://www.msys2.org/)
- Instale no diretório padrão (ex: `C:\msys64` ou `D:\msys64`).

---

## 3. Passo a Passo de Instalação e Configuração

### Passo 1: Atualizar o MSYS2 e Instalar Ferramentas Base
1. Abra o terminal **MSYS2 MINGW64** ou **MSYS2 MSYS**.
2. Atualize a base de pacotes:
   ```bash
   pacman -Syu
   ```
   *(Caso o terminal solicite o fechamento da janela após a atualização do núcleo, feche-a e abra novamente).*
3. Atualize os pacotes restantes:
   ```bash
   pacman -Su
   ```
4. Instale as ferramentas de compilação essenciais:
   ```bash
   pacman -S git cmake make gcc tar wget unzip patch diffutils
   ```

---

### Passo 2: Instalação do VitaSDK (Toolchain ARM)
O **VDPM** (Vita Development Package Manager) baixa e configura a cadeia de ferramentas de cross-compilação ARM oficial (`arm-vita-eabi-gcc`, etc.):

1. No terminal MSYS2, clone e execute o bootstrap do VitaSDK:
   ```bash
   cd ~
   git clone https://github.com/vitasdk/vdpm
   cd vdpm
   ./bootstrap-vitasdk.sh
   ```
2. O toolchain será instalado em `/usr/local/vitasdk`.

---

### Passo 3: Configurar Variáveis de Ambiente

#### No ambiente MSYS2 (Bash):
Adicione as variáveis ao seu arquivo de perfil `~/.bashrc`:
```bash
echo 'export VITASDK=/usr/local/vitasdk' >> ~/.bashrc
echo 'export PATH=$VITASDK/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

#### No Windows (Variáveis do Sistema / Usuário):
Para que ferramentas externas e scripts no Windows reconheçam o SDK:
1. Pressione `Win + R`, digite `sysdm.cpl` e vá na aba **Avançado** -> **Variáveis de Ambiente**.
2. Crie a variável de usuário/sistema:
   - **Nome**: `VITASDK`
   - **Valor**: `C:\msys64\usr\local\vitasdk` *(ou `D:\msys64\usr\local\vitasdk` se instalado no disco D)*.
3. Edite a variável `Path` e adicione:
   - `%VITASDK%\bin`

---

### Passo 4: Instalar as Bibliotecas do Vita via VDPM
Ainda no terminal MSYS2, dentro da pasta `vdpm`:

1. Instale as bibliotecas necessárias para o BilingualReaderVita:
   ```bash
   cd ~/vdpm
   ./install-packages.sh zlib libpng libjpeg-turbo freetype vita2d
   ```
   *(Opcional: Você pode rodar `./install-all.sh` para instalar todas as bibliotecas homebrew disponíveis para o Vita).*

---

### Passo 5: Dependências Internas (UnRAR e MuPDF)

- **UnRAR (`libs/unrar`)**: O código fonte já está incluso no repositório. O `CMakeLists.txt` compila automaticamente a biblioteca estática durante o processo de build do projeto.
- **MuPDF (`libs/mupdf`)**: As bibliotecas compiladas (`libmupdf.a`, `libmupdf-third.a`) e os headers já estão pré-configurados no repositório.
  - *Caso precise recompilar o MuPDF a partir do código fonte em `libs/mupdf_src`*:
    - **No Windows**: Execute o script [rebuild_mupdf.bat](file:///f:/Projetos/BilingualReaderVita/rebuild_mupdf.bat).
    - **No MSYS2**: Execute `bash build_mupdf_vita.sh`.

---

## 4. Compilação do Aplicativo

O projeto possui scripts automatizados para compilar e gerar o pacote de instalação final `.vpk`.

### Método 1: Compilação Automática no Windows (Recomendado)
Basta executar o script [compile.bat](file:///f:/Projetos/BilingualReaderVita/compile.bat) dando duplo clique ou via terminal (PowerShell / CMD):
```bat
.\compile.bat
```
**O que o script faz:**
1. Detecta automaticamente a instalação do MSYS2 em `D:\msys64`, `C:\msys64` ou `%MSYS2_PATH%`.
2. Cria os diretórios `build/` e `apk/`.
3. Invoca o [compile.sh](file:///f:/Projetos/BilingualReaderVita/compile.sh) dentro do ambiente MSYS2.
4. Executa o `cmake` com o toolchain do VitaSDK e compila com `make -j4`.
5. Gera o executável `eboot.bin` e empacota o instalador `BilingualReaderVita.vpk`.
6. Copia o arquivo final gerado para a pasta [apk/BilingualReaderVita.vpk](file:///f:/Projetos/BilingualReaderVita/apk/BilingualReaderVita.vpk).

---

### Método 2: Compilação Manual via Terminal MSYS2
1. Abra o terminal **MSYS2 MINGW64**.
2. Navegue até a pasta do projeto:
   ```bash
   cd /f/Projetos/BilingualReaderVita
   ```
3. Execute o script de compilação:
   ```bash
   bash compile.sh
   ```
4. Ou execute os passos do CMake manualmente:
   ```bash
   mkdir -p build && cd build
   cmake -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" -G "Unix Makefiles" ..
   make -j$(nproc)
   cp BilingualReaderVita.vpk ../apk/
   ```

---

## 5. Instalação e Testes

O arquivo final para instalação é gerado no caminho:
`apk/BilingualReaderVita.vpk` (Title ID: `BREADPSV`, Nome: `Bilingual Reader`).

### 5.1. Execução no Emulador Vita3K (PC)
1. Abra o **Vita3K** (ex: localizado em `AppData\Roaming\Vita3K\Vita3K\Vita3K.exe`).
2. No menu superior, clique em:
   **File (Arquivo)** -> **Install .pkg / .vpk** *(ou simplesmente arraste e solte o arquivo `BilingualReaderVita.vpk` na janela do Vita3K)*.
3. Selecione o arquivo `f:\Projetos\BilingualReaderVita\apk\BilingualReaderVita.vpk`.
4. Confirme a instalação. O ícone do aplicativo aparecerá na LiveArea do Vita3K.
5. Clique no ícone para abrir e testar o leitor.

### 5.2. Instalação no Console PlayStation Vita Real
1. Conecte o PS Vita ao computador via **VitaShell** (modo USB ou servidor FTP).
2. Transfira o arquivo `BilingualReaderVita.vpk` para qualquer diretório do cartão de memória (por exemplo: `ux0:data/` ou `ux0:vpk/`).
3. No VitaShell do console, navegue até o arquivo transferido e pressione **X** para instalar o `.vpk`.
4. Confirme as permissões de instalação estendida.
5. Após concluir, feche o VitaShell e abra a bolha do **Bilingual Reader** na LiveArea do console.

---

## 6. Estrutura de Arquivos de Build Relevantes

| Arquivo / Diretório | Descrição |
| :--- | :--- |
| [compile.bat](file:///f:/Projetos/BilingualReaderVita/compile.bat) | Script em lote do Windows para compilação com 1 clique |
| [compile.sh](file:///f:/Projetos/BilingualReaderVita/compile.sh) | Script Bash do CMake e Make executado no MSYS2 |
| [CMakeLists.txt](file:///f:/Projetos/BilingualReaderVita/CMakeLists.txt) | Configuração principal de compilação, flags C++17, inclusões e empacotamento VPK |
| [rebuild_mupdf.bat](file:///f:/Projetos/BilingualReaderVita/rebuild_mupdf.bat) | Script auxiliar para recompilar o MuPDF caso modificado |
| [build_mupdf_vita.sh](file:///f:/Projetos/BilingualReaderVita/build_mupdf_vita.sh) | Script Bash para compilar `libmupdf.a` e `libmupdf-third.a` para ARM Vita |
| `libs/unrar/` | Código-fonte C++ do UnRAR embutido |
| `libs/mupdf/` | Headers e bibliotecas estáticas do MuPDF para PS Vita |
| `apk/` | Pasta onde o binário final `BilingualReaderVita.vpk` é disponibilizado |
| `sce_sys/` | Ícones e metadados da LiveArea do PS Vita (`icon0.png`, `livearea/`) |
