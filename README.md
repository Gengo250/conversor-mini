# conversor-mini

Conversor **CLI** (linha de comando) bem simples para transformar arquivos **Word/Writer** (ex.: `.docx`, `.odt`) e **texto** (`.txt`) em **PDF**, usando o **LibreOffice** em modo *headless* (sem interface gráfica).  
Feito para **uso pessoal** e gratuito: você roda localmente, no seu Linux, sem enviar arquivo pra nenhum serviço online.

## O que ele faz

- Recebe um arquivo de entrada (ex.: `arquivo.docx`)
- Chama o `soffice` (LibreOffice) em modo headless
- Gera um `arquivo.pdf` no diretório de saída (`./out` por padrão)
- Usa um **perfil isolado** por execução (evita conflitos quando você roda várias conversões em paralelo)

---

## Requisitos

### Sistema
- Linux (o código usa `<unistd.h>` e redireciona pra `/dev/null`)

### Dependências
- **LibreOffice** (precisa existir o comando `soffice` no PATH)
- Compilador C++ com suporte a **C++17** (por causa de `std::filesystem`)

No Arch Linux (recomendado):

```bash
sudo pacman -S --needed base-devel gcc make
# escolha UM:
sudo pacman -S libreoffice-fresh
# ou (mais conservador/estável):
sudo pacman -S libreoffice-still
```

Verifique se o `soffice` está disponível:

```bash
command -v soffice
soffice --version
```

---

## Estrutura do projeto (exemplo)

Sua árvore atual (resumo):

```text
.
└── test
    ├── arquivos_conversao_test
    │   ├── *.docx
    │   └── teste.txt
    ├── converter_cli
    ├── converter_cli.cpp
    └── out
        ├── *.pdf
        └── teste.txt
```

> Observação: o `out/` pode conter outros arquivos (como `.txt`) porque você está usando a pasta como “saída geral”. O conversor **só cria PDF**; ele não mexe nos outros arquivos.

---

## Compilando

Dentro da pasta onde está o `converter_cli.cpp`:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic converter_cli.cpp -o converter_cli
```

Dica: se quiser instalar localmente no seu PATH (opcional):

```bash
sudo install -m 755 converter_cli /usr/local/bin/converter_cli
```

---

## Como usar

### Uso básico

```bash
./converter_cli <input.(docx|odt|txt|...)> [outdir]
```

- `input`: caminho do arquivo de entrada
- `outdir` (opcional): diretório de saída. Se não passar, usa `./out`

Exemplos:

```bash
# 1) Converte um docx e joga o PDF em ./out
./converter_cli "test/arquivos_conversao_test/teste.docx"

# 2) Converte e escolhe pasta de saída
./converter_cli "test/arquivos_conversao_test/relatorio_financeiro_europa_teste.docx" "test/out"

# 3) Convertendo um .txt (LibreOffice importa e exporta como PDF)
./converter_cli "test/arquivos_conversao_test/teste.txt" "test/out"
```

Saída esperada:

```text
OK ✅ Gerou: test/out/teste.pdf
```

---

## Conversão em lote (batch)

Converter todos os `.docx` de uma pasta:

```bash
mkdir -p test/out
for f in test/arquivos_conversao_test/*.docx; do
  ./converter_cli "$f" test/out
done
```

Com `find` (pega subpastas também):

```bash
find test/arquivos_conversao_test -type f -name "*.docx" -print0 \
  | xargs -0 -I{} ./converter_cli "{}" test/out
```

---

## Como funciona por dentro (o “porquê” do código)

O programa monta e executa um comando parecido com:

```bash
soffice --headless --nologo --nodefault --nofirststartwizard \
  --nolockcheck --norestore --invisible \
  "-env:UserInstallation=file:///tmp/lo_profile_<PID>" \
  --convert-to "pdf:writer_pdf_Export" \
  --outdir "<PASTA_DE_SAIDA>" "<ARQUIVO_ENTRADA>"
```

Pontos importantes do código:

- **`std::filesystem`** cria a pasta de saída e valida o arquivo de entrada.
- **Perfil isolado**: cria uma pasta temporária (`/tmp/lo_profile_<pid>`) e aponta o LibreOffice pra ela com `-env:UserInstallation=...`.
  - Isso evita erros do tipo “LibreOffice já está usando perfil” quando você roda várias conversões.
- **Limpeza**: ao final, remove o perfil temporário.
- **Verificação**: espera encontrar `<nome_do_arquivo>.pdf` em `outdir`.

---

## Códigos de retorno (exit codes)

Útil pra scripts:

- `0`: sucesso
- `2`: uso errado (faltou argumento)
- `3`: arquivo inválido (não existe ou não é arquivo)
- `4`: LibreOffice retornou erro na conversão
- `5`: conversão rodou, mas o PDF esperado não apareceu no destino

---

## Limitações e observações

- **Segurança**: o programa usa `std::system()` e uma função de escape bem simples (`shellEscape`).
  - Pra uso pessoal/local é ok, mas não é ideal para cenários “multiusuário” ou recebendo caminhos de entrada não confiáveis.
  - Se um dia você quiser deixar isso “blindado”, o próximo passo é trocar por `fork/exec` passando argv (sem shell).
- **Compatibilidade**: pensado para Linux. No Windows/macOS dá pra adaptar, mas exige ajustes.
- **Entrada**: o LibreOffice converte muitos formatos, mas o resultado pode variar conforme fonte/compatibilidade do documento.

---

## Troubleshooting (quando dá ruim 😅)

### `soffice: command not found`
Instale LibreOffice e garanta que `soffice` está no PATH:

```bash
sudo pacman -S libreoffice-fresh
command -v soffice
```

### PDF não apareceu (`exit code 5`)
- Verifique permissões do `outdir`
- Confirme se o LibreOffice realmente suporta o formato daquele arquivo
- Rode manualmente o comando do LibreOffice (sem redirecionar pra `/dev/null`) para ver logs:
  - No código atual, a saída do `soffice` é descartada. Para depurar, remova `>/dev/null 2>&1`.

### Documento com nome “estranho” (acentos/espaços)
O escape atual lida com espaços e aspas. Em geral funciona bem em Linux modernos (UTF-8), mas se houver caracteres muito incomuns, teste.

---


