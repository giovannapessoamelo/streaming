# Streaming de Vídeos - Arquivos Binários em C

**Entrega:** (individual / em dupla)

**Participantes:**
- Nome Completo - RA 000000
- (Nome do parceiro, se for dupla) - RA 000000

## Descrição
Programa em C que manipula dois arquivos binários relacionados:

- `videos.dat`: registros `Video` (id, título, gênero, duração, ano)
- `usuarios.dat`: registros `Usuario` (id, nome, e-mail, vetor com IDs dos vídeos favoritados)

## Funcionalidades
- Cadastro de vídeos e usuários
- Leitura de um registro (por ID) e de todos os registros
- Atualização de vídeos e usuários (inclui adicionar/remover favoritos)
- Remoção via arquivo temporário (copia tudo menos o removido, apaga o antigo e renomeia o novo)
- Relatórios: usuários com os títulos dos favoritos e vídeos com os nomes de quem favoritou

## Como compilar e executar
```
gcc -Wall -Wextra -o streaming main.c
./streaming
```
Veja também `comandos.txt`.

## Vídeo de demonstração
(link do YouTube)
