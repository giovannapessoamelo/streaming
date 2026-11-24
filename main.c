/*
 * Streaming de videos - manipulacao de arquivos binarios em C
 *
 * Arquivos:
 *   videos.dat   -> registros do tipo Video
 *   usuarios.dat -> registros do tipo Usuario (possui vetor de IDs de videos favoritos)
 *
 * Relacionamento: Usuario.favoritos[] guarda IDs de Video.
 * Importante: o campo "id" e o PRIMEIRO campo das duas structs
 * (a funcao generica de remocao depende disso).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARQ_VIDEOS   "videos.dat"
#define ARQ_USUARIOS "usuarios.dat"
#define ARQ_TEMP     "temp.dat"
#define MAX_FAV 20

typedef struct {
    int  id;               /* sempre o primeiro campo */
    char titulo[60];
    char genero[30];
    int  duracao;          /* em minutos */
    int  ano;
} Video;

typedef struct {
    int  id;               /* sempre o primeiro campo */
    char nome[50];
    char email[50];
    int  qtdFavoritos;
    int  favoritos[MAX_FAV]; /* IDs dos videos favoritados */
} Usuario;

/* ------------------------------------------------------------------ */
/* Utilitarios de entrada                                              */
/* ------------------------------------------------------------------ */

static void lerTexto(const char *msg, char *dest, int tam) {
    printf("%s", msg);
    if (fgets(dest, tam, stdin) == NULL) {
        dest[0] = '\0';
        return;
    }
    dest[strcspn(dest, "\n")] = '\0';
}

static int lerInt(const char *msg) {
    char buf[32];
    printf("%s", msg);
    if (fgets(buf, sizeof(buf), stdin) == NULL) return 0;
    return (int)strtol(buf, NULL, 10);
}

/* Se o usuario so apertar ENTER, mantem o valor atual */
static void lerTextoPadrao(const char *msg, char *dest, int tam) {
    char buf[128];
    printf("%s [%s]: ", msg, dest);
    if (fgets(buf, sizeof(buf), stdin) == NULL) return;
    buf[strcspn(buf, "\n")] = '\0';
    if (buf[0] != '\0') {
        strncpy(dest, buf, tam - 1);
        dest[tam - 1] = '\0';
    }
}

static int lerIntPadrao(const char *msg, int atual) {
    char buf[32];
    printf("%s [%d]: ", msg, atual);
    if (fgets(buf, sizeof(buf), stdin) == NULL) return atual;
    if (buf[0] == '\n') return atual;
    return (int)strtol(buf, NULL, 10);
}

/* ------------------------------------------------------------------ */
/* Funcoes genericas de arquivo                                        */
/* ------------------------------------------------------------------ */

/* Proximo ID = maior ID existente + 1 */
static int proximoId(const char *arq, size_t tam) {
    FILE *f = fopen(arq, "rb");
    int maior = 0, id;
    void *buf;

    if (f == NULL) return 1;
    buf = malloc(tam);
    while (fread(buf, tam, 1, f) == 1) {
        memcpy(&id, buf, sizeof(int));
        if (id > maior) maior = id;
    }
    free(buf);
    fclose(f);
    return maior + 1;
}

/*
 * Remocao: cria um arquivo novo com todos os registros, menos o removido.
 * Depois fecha os dois, apaga o antigo e renomeia o novo.
 * Retorna 1 se removeu, 0 caso contrario.
 */
static int removerRegistro(const char *arq, size_t tam, int idRemover) {
    FILE *orig = fopen(arq, "rb");
    FILE *novo;
    void *buf;
    int id, achou = 0;

    if (orig == NULL) return 0;
    novo = fopen(ARQ_TEMP, "wb");
    if (novo == NULL) {
        fclose(orig);
        return 0;
    }

    buf = malloc(tam);
    while (fread(buf, tam, 1, orig) == 1) {
        memcpy(&id, buf, sizeof(int));
        if (id == idRemover) {
            achou = 1;      /* nao copia este registro */
            continue;
        }
        fwrite(buf, tam, 1, novo);
    }
    free(buf);

    fclose(orig);
    fclose(novo);

    if (!achou) {
        remove(ARQ_TEMP);
        return 0;
    }
    remove(arq);
    rename(ARQ_TEMP, arq);
    return 1;
}

/* ------------------------------------------------------------------ */
/* Busca (retorna a posicao do registro no arquivo, ou -1)             */
/* ------------------------------------------------------------------ */

static int buscarVideo(int id, Video *out) {
    FILE *f = fopen(ARQ_VIDEOS, "rb");
    Video v;
    int pos = 0;

    if (f == NULL) return -1;
    while (fread(&v, sizeof(Video), 1, f) == 1) {
        if (v.id == id) {
            if (out) *out = v;
            fclose(f);
            return pos;
        }
        pos++;
    }
    fclose(f);
    return -1;
}

static int buscarUsuario(int id, Usuario *out) {
    FILE *f = fopen(ARQ_USUARIOS, "rb");
    Usuario u;
    int pos = 0;

    if (f == NULL) return -1;
    while (fread(&u, sizeof(Usuario), 1, f) == 1) {
        if (u.id == id) {
            if (out) *out = u;
            fclose(f);
            return pos;
        }
        pos++;
    }
    fclose(f);
    return -1;
}

/* ------------------------------------------------------------------ */
/* Impressao de um registro                                            */
/* ------------------------------------------------------------------ */

static void imprimirVideo(const Video *v) {
    printf("ID: %d | Titulo: %s | Genero: %s | Duracao: %d min | Ano: %d\n",
           v->id, v->titulo, v->genero, v->duracao, v->ano);
}

static void imprimirUsuario(const Usuario *u) {
    int i;
    printf("ID: %d | Nome: %s | E-mail: %s | Favoritos (IDs):",
           u->id, u->nome, u->email);
    if (u->qtdFavoritos == 0) printf(" nenhum");
    for (i = 0; i < u->qtdFavoritos; i++) printf(" %d", u->favoritos[i]);
    printf("\n");
}

/* ------------------------------------------------------------------ */
/* VIDEOS: cadastrar, ler, listar, atualizar, remover                  */
/* ------------------------------------------------------------------ */

static void cadastrarVideo(void) {
    Video v;
    FILE *f;

    memset(&v, 0, sizeof(Video));
    v.id = proximoId(ARQ_VIDEOS, sizeof(Video));
    lerTexto("Titulo: ", v.titulo, sizeof(v.titulo));
    lerTexto("Genero: ", v.genero, sizeof(v.genero));
    v.duracao = lerInt("Duracao (minutos): ");
    v.ano = lerInt("Ano de lancamento: ");

    f = fopen(ARQ_VIDEOS, "ab");
    if (f == NULL) {
        printf("Erro ao abrir %s\n", ARQ_VIDEOS);
        return;
    }
    fwrite(&v, sizeof(Video), 1, f);
    fclose(f);
    printf("Video cadastrado com ID %d.\n", v.id);
}

static void lerUmVideo(void) {
    Video v;
    int id = lerInt("ID do video: ");

    if (buscarVideo(id, &v) < 0) printf("Video nao encontrado.\n");
    else imprimirVideo(&v);
}

static void listarVideos(void) {
    FILE *f = fopen(ARQ_VIDEOS, "rb");
    Video v;
    int cont = 0;

    if (f == NULL) {
        printf("Nenhum video cadastrado.\n");
        return;
    }
    while (fread(&v, sizeof(Video), 1, f) == 1) {
        imprimirVideo(&v);
        cont++;
    }
    fclose(f);
    if (cont == 0) printf("Nenhum video cadastrado.\n");
}

static void atualizarVideo(void) {
    Video v;
    FILE *f;
    int id = lerInt("ID do video a atualizar: ");
    int pos = buscarVideo(id, &v);

    if (pos < 0) {
        printf("Video nao encontrado.\n");
        return;
    }
    printf("(ENTER mantem o valor atual)\n");
    lerTextoPadrao("Titulo", v.titulo, sizeof(v.titulo));
    lerTextoPadrao("Genero", v.genero, sizeof(v.genero));
    v.duracao = lerIntPadrao("Duracao", v.duracao);
    v.ano = lerIntPadrao("Ano", v.ano);

    f = fopen(ARQ_VIDEOS, "r+b");
    if (f == NULL) return;
    fseek(f, (long)pos * (long)sizeof(Video), SEEK_SET);
    fwrite(&v, sizeof(Video), 1, f);
    fclose(f);
    printf("Video atualizado.\n");
}

/* Tira o ID removido da lista de favoritos de todos os usuarios */
static void limparFavoritosDoVideo(int idVideo) {
    FILE *f = fopen(ARQ_USUARIOS, "r+b");
    Usuario u;
    int i, j;

    if (f == NULL) return;
    while (fread(&u, sizeof(Usuario), 1, f) == 1) {
        int mudou = 0;
        for (i = 0; i < u.qtdFavoritos; i++) {
            if (u.favoritos[i] == idVideo) {
                for (j = i; j < u.qtdFavoritos - 1; j++)
                    u.favoritos[j] = u.favoritos[j + 1];
                u.qtdFavoritos--;
                mudou = 1;
                i--;
            }
        }
        if (mudou) {
            fseek(f, -(long)sizeof(Usuario), SEEK_CUR);
            fwrite(&u, sizeof(Usuario), 1, f);
            fseek(f, 0, SEEK_CUR); /* necessario ao alternar escrita/leitura */
        }
    }
    fclose(f);
}

static void removerVideo(void) {
    int id = lerInt("ID do video a remover: ");

    if (removerRegistro(ARQ_VIDEOS, sizeof(Video), id)) {
        limparFavoritosDoVideo(id);
        printf("Video removido (e retirado dos favoritos dos usuarios).\n");
    } else {
        printf("Video nao encontrado.\n");
    }
}

/* ------------------------------------------------------------------ */
/* USUARIOS: cadastrar, ler, listar, atualizar, remover                */
/* ------------------------------------------------------------------ */

static int adicionarFavorito(Usuario *u, int idVideo) {
    int i;
    if (buscarVideo(idVideo, NULL) < 0) {
        printf("Video %d nao existe.\n", idVideo);
        return 0;
    }
    for (i = 0; i < u->qtdFavoritos; i++) {
        if (u->favoritos[i] == idVideo) {
            printf("Video ja esta nos favoritos.\n");
            return 0;
        }
    }
    if (u->qtdFavoritos >= MAX_FAV) {
        printf("Limite de %d favoritos atingido.\n", MAX_FAV);
        return 0;
    }
    u->favoritos[u->qtdFavoritos++] = idVideo;
    return 1;
}

static int removerFavorito(Usuario *u, int idVideo) {
    int i, j;
    for (i = 0; i < u->qtdFavoritos; i++) {
        if (u->favoritos[i] == idVideo) {
            for (j = i; j < u->qtdFavoritos - 1; j++)
                u->favoritos[j] = u->favoritos[j + 1];
            u->qtdFavoritos--;
            return 1;
        }
    }
    printf("Esse video nao esta nos favoritos do usuario.\n");
    return 0;
}

static void cadastrarUsuario(void) {
    Usuario u;
    FILE *f;
    int idv;

    memset(&u, 0, sizeof(Usuario));
    u.id = proximoId(ARQ_USUARIOS, sizeof(Usuario));
    lerTexto("Nome: ", u.nome, sizeof(u.nome));
    lerTexto("E-mail: ", u.email, sizeof(u.email));

    printf("Informe os IDs dos videos favoritos (0 para terminar):\n");
    while (u.qtdFavoritos < MAX_FAV) {
        idv = lerInt("  ID do video: ");
        if (idv == 0) break;
        adicionarFavorito(&u, idv);
    }

    f = fopen(ARQ_USUARIOS, "ab");
    if (f == NULL) {
        printf("Erro ao abrir %s\n", ARQ_USUARIOS);
        return;
    }
    fwrite(&u, sizeof(Usuario), 1, f);
    fclose(f);
    printf("Usuario cadastrado com ID %d.\n", u.id);
}

static void lerUmUsuario(void) {
    Usuario u;
    int id = lerInt("ID do usuario: ");

    if (buscarUsuario(id, &u) < 0) printf("Usuario nao encontrado.\n");
    else imprimirUsuario(&u);
}

static void listarUsuarios(void) {
    FILE *f = fopen(ARQ_USUARIOS, "rb");
    Usuario u;
    int cont = 0;

    if (f == NULL) {
        printf("Nenhum usuario cadastrado.\n");
        return;
    }
    while (fread(&u, sizeof(Usuario), 1, f) == 1) {
        imprimirUsuario(&u);
        cont++;
    }
    fclose(f);
    if (cont == 0) printf("Nenhum usuario cadastrado.\n");
}

static void atualizarUsuario(void) {
    Usuario u;
    FILE *f;
    int id = lerInt("ID do usuario a atualizar: ");
    int pos = buscarUsuario(id, &u);
    int op;

    if (pos < 0) {
        printf("Usuario nao encontrado.\n");
        return;
    }

    do {
        printf("\n--- Atualizar usuario ---\n");
        imprimirUsuario(&u);
        printf("1 - Alterar nome e e-mail\n");
        printf("2 - Adicionar video aos favoritos\n");
        printf("3 - Remover video dos favoritos\n");
        printf("0 - Salvar e voltar\n");
        op = lerInt("Opcao: ");

        if (op == 1) {
            printf("(ENTER mantem o valor atual)\n");
            lerTextoPadrao("Nome", u.nome, sizeof(u.nome));
            lerTextoPadrao("E-mail", u.email, sizeof(u.email));
        } else if (op == 2) {
            adicionarFavorito(&u, lerInt("ID do video: "));
        } else if (op == 3) {
            removerFavorito(&u, lerInt("ID do video: "));
        }
    } while (op != 0);

    f = fopen(ARQ_USUARIOS, "r+b");
    if (f == NULL) return;
    fseek(f, (long)pos * (long)sizeof(Usuario), SEEK_SET);
    fwrite(&u, sizeof(Usuario), 1, f);
    fclose(f);
    printf("Usuario atualizado.\n");
}

static void removerUsuario(void) {
    int id = lerInt("ID do usuario a remover: ");

    if (removerRegistro(ARQ_USUARIOS, sizeof(Usuario), id))
        printf("Usuario removido.\n");
    else
        printf("Usuario nao encontrado.\n");
}

/* ------------------------------------------------------------------ */
/* RELATORIOS                                                          */
/* ------------------------------------------------------------------ */

/* Todos os usuarios com os TITULOS dos videos favoritados */
static void relatorioUsuariosFavoritos(void) {
    FILE *f = fopen(ARQ_USUARIOS, "rb");
    Usuario u;
    Video v;
    int i, cont = 0;

    if (f == NULL) {
        printf("Nenhum usuario cadastrado.\n");
        return;
    }
    printf("\n===== USUARIOS E SEUS VIDEOS FAVORITOS =====\n");
    while (fread(&u, sizeof(Usuario), 1, f) == 1) {
        printf("\nUsuario #%d: %s (%s)\n", u.id, u.nome, u.email);
        if (u.qtdFavoritos == 0) printf("   (sem favoritos)\n");
        for (i = 0; i < u.qtdFavoritos; i++) {
            if (buscarVideo(u.favoritos[i], &v) >= 0)
                printf("   - %s [%s, %d]\n", v.titulo, v.genero, v.ano);
            else
                printf("   - (video %d nao encontrado)\n", u.favoritos[i]);
        }
        cont++;
    }
    fclose(f);
    if (cont == 0) printf("Nenhum usuario cadastrado.\n");
}

/* Todos os videos com os NOMES dos usuarios que os favoritaram */
static void relatorioVideosFavoritadosPor(void) {
    FILE *fv = fopen(ARQ_VIDEOS, "rb");
    FILE *fu;
    Video v;
    Usuario u;
    int i, achou, cont = 0;

    if (fv == NULL) {
        printf("Nenhum video cadastrado.\n");
        return;
    }
    printf("\n===== VIDEOS E QUEM OS FAVORITOU =====\n");
    while (fread(&v, sizeof(Video), 1, fv) == 1) {
        printf("\nVideo #%d: %s [%s, %d]\n", v.id, v.titulo, v.genero, v.ano);
        achou = 0;
        fu = fopen(ARQ_USUARIOS, "rb");
        if (fu != NULL) {
            while (fread(&u, sizeof(Usuario), 1, fu) == 1) {
                for (i = 0; i < u.qtdFavoritos; i++) {
                    if (u.favoritos[i] == v.id) {
                        printf("   - %s\n", u.nome);
                        achou = 1;
                        break;
                    }
                }
            }
            fclose(fu);
        }
        if (!achou) printf("   (ninguem favoritou)\n");
        cont++;
    }
    fclose(fv);
    if (cont == 0) printf("Nenhum video cadastrado.\n");
}

/* ------------------------------------------------------------------ */
/* MENUS                                                               */
/* ------------------------------------------------------------------ */

static void menuVideos(void) {
    int op;
    do {
        printf("\n--- VIDEOS ---\n");
        printf("1 - Cadastrar\n2 - Ler um video (por ID)\n3 - Listar todos\n");
        printf("4 - Atualizar\n5 - Remover\n0 - Voltar\n");
        op = lerInt("Opcao: ");
        switch (op) {
            case 1: cadastrarVideo(); break;
            case 2: lerUmVideo();     break;
            case 3: listarVideos();   break;
            case 4: atualizarVideo(); break;
            case 5: removerVideo();   break;
        }
    } while (op != 0);
}

static void menuUsuarios(void) {
    int op;
    do {
        printf("\n--- USUARIOS ---\n");
        printf("1 - Cadastrar\n2 - Ler um usuario (por ID)\n3 - Listar todos\n");
        printf("4 - Atualizar\n5 - Remover\n0 - Voltar\n");
        op = lerInt("Opcao: ");
        switch (op) {
            case 1: cadastrarUsuario(); break;
            case 2: lerUmUsuario();     break;
            case 3: listarUsuarios();   break;
            case 4: atualizarUsuario(); break;
            case 5: removerUsuario();   break;
        }
    } while (op != 0);
}

static void menuRelatorios(void) {
    int op;
    do {
        printf("\n--- RELATORIOS ---\n");
        printf("1 - Usuarios e seus videos favoritos\n");
        printf("2 - Videos e quem os favoritou\n");
        printf("0 - Voltar\n");
        op = lerInt("Opcao: ");
        switch (op) {
            case 1: relatorioUsuariosFavoritos();   break;
            case 2: relatorioVideosFavoritadosPor(); break;
        }
    } while (op != 0);
}

int main(void) {
    int op;
    do {
        printf("\n===== STREAMING DE VIDEOS =====\n");
        printf("1 - Videos\n2 - Usuarios\n3 - Relatorios\n0 - Sair\n");
        op = lerInt("Opcao: ");
        switch (op) {
            case 1: menuVideos();     break;
            case 2: menuUsuarios();   break;
            case 3: menuRelatorios(); break;
        }
    } while (op != 0);

    printf("Ate logo!\n");
    return 0;
}
