#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>

#define __RESULT__
#ifdef  __RESULT__
#define DFA0_TRANS "./result/DFA1_TRANS.data"
#define DFA0_MATCH "./result/DFA1_MATCH.data"
#define DFA1_TRANS "./result/DFA2_TRANS.data"
#define DFA1_MATCH "./result/DFA2_MATCH.data"
#define DFA2_TRANS "./result/DFA3_TRANS.data"
#define DFA2_MATCH "./result/DFA3_MATCH.data"
#define MRMT_TABLE "./result/TSI.data"
#else
#define DFA0_TRANS "/tmp/.5472617368/DFA1_TRANS.data"
#define DFA0_MATCH "/tmp/.5472617368/DFA1_MATCH.data"
#define DFA1_TRANS "/tmp/.5472617368/DFA2_TRANS.data"
#define DFA1_MATCH "/tmp/.5472617368/DFA2_MATCH.data"
#define DFA2_TRANS "/tmp/.5472617368/DFA3_TRANS.data"
#define DFA2_MATCH "/tmp/.5472617368/DFA3_MATCH.data"
#define MRMT_TABLE "/tmp/.5472617368/TSI.data"
#endif

#define __word_t uint32_t
//#define BL ( sizeof(__word_t) * 8 - 1 )
//#define BH ({ __word_t x = 0; __word_t y = BL; while (y) { y = y >> 1; x++; } x; })
//#define WQ ( ( (__bit_num) - 1 ) / ( sizeof(__word_t) * 8 ) + 1 )
#define BL 0x1F
#define BH 5
#define WQ 16
#define QUEUE_SIZE 64
#define QUEUE_FAST 8

#define F 2.4
#define N 2
#define C 256
#define G ~0

#define idxadd(x,y) y->word[(x)>>BH]|=(1<<((x)&BL))
#define idxcmp(x,y) (1&((y->word[(x)>>BH])>>((x)&BL)))==1
#define idxcpy(x,y) \
    memcpy(y, x, sizeof(index_unit_t)); \
    memset(x, 0, sizeof(index_unit_t));
#define idxcat(x,y) \
    label_t l; \
    for (l=0; l<WQ; l++) { \
        y->word[l] |= x->word[l]; \
    } \
    memset(x, 0, sizeof(index_unit_t));

typedef uint16_t regex_t; // ruleset scale: 1~65536
typedef uint8_t  table_t; // dfa number: 0~3
typedef uint32_t state_t; // state identifier: 0~4,294,967,294 (4G)
typedef uint8_t  input_t; // input character range: 0~255
typedef uint32_t count_t; // input character count: 0~4,294,967,295 (4G)
typedef uint8_t  label_t; // judgement variable: 0/1

typedef struct mtrans_s {
    state_t state;
    input_t input;
} mtrans_t;

typedef struct mstate_s {
    state_t magic;
    mtrans_t *mtrans;
    input_t num;
} mstate_t;

struct index_unit {
    __word_t word[WQ];
};
typedef struct index_unit index_unit_t;

struct state_unit {
    struct state_unit *next;
    state_t state;
    regex_t index;
    table_t table;
};
typedef struct state_unit state_unit_t;

struct match_unit {
    struct match_unit *next;
    regex_t regex;
};
typedef struct match_unit match_unit_t;

struct datum_unit {
    count_t ni;
    state_t ns[N];
    regex_t nr;
    regex_t np;
    regex_t nn;
    table_t nt;
};
typedef struct datum_unit datum_unit_t;

static state_t **DFA[N] = {NULL}, END[N] = {G};
static match_unit_t **TIE[N] = {NULL};
static regex_t **MRT = NULL;
static input_t *POOL = NULL;

static size_t QUEUE_WARN = QUEUE_FAST;

static int load_regex_data(datum_unit_t *du)
{
    FILE *fp = NULL, *fq = NULL;
    match_unit_t *mu = NULL;
    double avg_t_num = 0;
    count_t max_t_num = 0, mtrans_num = 0, trans_num = 0;
    table_t t = 0, n = 0;
    state_t s = 0, d = 0, m = 0, magic[C] = {0}, count[C] = {0};
    input_t c = 0;
    regex_t r = 0, x = 0, z = 0;
    label_t l = 0, w = 0;
    int i, j, k, num = 0, rv;
    printf("DEBUG: %s() start\n", __FUNCTION__);
    for (n=0; n<N; n++) {
        switch (n) {
            case 0:
                fp = fopen(DFA0_TRANS, "r");
                fq = fopen(DFA0_MATCH, "r");
                break;
            case 1:
                fp = fopen(DFA1_TRANS, "r");
                fq = fopen(DFA1_MATCH, "r");
                break;
            case 2:
                fp = fopen(DFA2_TRANS, "r");
                fq = fopen(DFA2_MATCH, "r");
                break;
            case 3:
                continue;
                break;
            default:
                break;
        }
        if (fp == NULL || fq == NULL) {
            rv = -11;
            goto err;
        }
        rv = fscanf(fp, "%u\n", &m);
        if (m <= 1) {
            fclose(fp);
            fclose(fq);
            continue;
        }
        du->ns[t] = m;
        DFA[t] = (state_t **)malloc(m*sizeof(state_t *));
        if (DFA[t] == NULL) {
            rv = -12;
            goto err;
        }
        memset(DFA[t], 0, m*sizeof(state_t *));
        for (s=0; s<m; s++) {
            DFA[t][s] = (state_t *)malloc(C*sizeof(state_t));
            if (DFA[t][s] == NULL) {
                rv = -13;
                goto err;
            }
            memset(DFA[t][s], 0, C*sizeof(state_t));
            for (c=0; c<C-1; c++)
            {
                rv = fscanf(fp, "%u\t", &DFA[t][s][c]);
            }
            rv = fscanf(fp, "%u\n", &DFA[t][s][C-1]);
        }
        TIE[t] = (match_unit_t **)malloc(m*sizeof(match_unit_t *));
        if (TIE[t] == NULL) {
            rv = -14;
            goto err;
        }
        memset(TIE[t], 0, m*sizeof(match_unit_t *));
        for (s=0; s<m; s++) {
            rv = fscanf(fq, "%u", &d);
            if (d != 0) {
                END[t] = s;
            }
            rv = fscanf(fq, "\t%hu", &z);
            if (z > 0) {
                TIE[t][s] = (match_unit_t *)malloc(z*sizeof(match_unit_t));
                if (TIE[t][s] == NULL) {
                    rv = -15;
                    goto err;
                }
                memset(TIE[t][s], 0, z*sizeof(match_unit_t));
                rv = fscanf(fq, "\t@");
                mu = TIE[t][s];
                for (r=0; r<z; r++)
                {
                    rv = fscanf(fq, "\t%hu", &x);
                    mu->regex = x;
                    mu->next = (mu + 1);
                    mu = mu->next;
                }
                mu--;
                mu->next = NULL;
            }
            rv = fscanf(fq, "\n");
        }
        fclose(fp);
        fclose(fq);
        t++;
    }
    du->nt = t;
    fp = fopen(MRMT_TABLE, "r");
    if (fp == NULL) {
        rv = -18;
        goto err;
    }
    rv = fscanf(fp, "%hu\t%hhu\n", &du->np, &w);
    MRT = (regex_t **)malloc((du->np+1)*sizeof(regex_t *));
    if (MRT == NULL) {
        rv = -19;
        goto err;
    }
    memset(MRT, 0, (du->np+1)*sizeof(regex_t *));
    for (r=1; r<=du->np; r++) {
        MRT[r] = (regex_t *)malloc(w*sizeof(regex_t));
        if (MRT[r] == NULL) {
            rv = -20;
            goto err;
        }
        memset(MRT[r], 0, w*sizeof(regex_t));
        for (l=0; l<w; l++) {
            rv = fscanf(fp, "%hu", &MRT[r][l]);
        }
        if (MRT[r][1] > du->nt) {
            MRT[r][1]--;
        }
        if (MRT[r][2] > du->nr) {
            du->nr = MRT[r][2];
        }
    }
    fclose(fp);
    printf("DEBUG: %s() end\n", __FUNCTION__);
    return 0;

err:
    printf("ERROR: %s() return %d\n", __FUNCTION__, rv);
    return rv;
}

static int load_input_data(datum_unit_t *du, char *data_from)
{
    FILE *fp = NULL;
    count_t n;
    int rv;
    printf("DEBUG: %s() start\n", __FUNCTION__);
    if (data_from == NULL || strlen(data_from) == 0) {
        rv = -21;
        goto err;
    }
    fp = fopen(data_from, "r");
    if (fp == NULL) {
        rv = -22;
        goto err;
    }
    rewind(fp);
    fseek(fp, 0, SEEK_END);
    du->ni = ftell(fp);
    if (du->ni == 0) {
        rv = -23;
        goto err;
    }
    rewind(fp);
    POOL = (input_t *)malloc(du->ni*sizeof(input_t));
    if (POOL == NULL) {
        rv = -24;
        goto err;
    }
    memset(POOL, 0, du->ni*sizeof(input_t));
    n = fread(POOL, 1, du->ni, fp);
    if (n != du->ni) {
        rv = -25;
        goto err;
    }
    fclose(fp);
    printf("DEBUG: %s() end\n", __FUNCTION__);
    return 0;

err:
    printf("ERROR: %s() return %d\n", __FUNCTION__, rv);
    return rv;
}

static int process(datum_unit_t *du)
{
    struct  timeval start, end;
    state_unit_t *sf = NULL, *work_queue = NULL, *rest_queue = NULL,
                 *sw = NULL, *sr = NULL, *su = NULL, *sv = NULL;
    match_unit_t *mu = NULL;
    index_unit_t *is = NULL, *iu = NULL, *idx_to_exec[du->nt+1];
    double  hz, us = 0, pe;
    count_t n = 0, rn = 0, zn = 0, ni = du->ni, stat[du->nr+1];
    state_t s = du->nn;
    regex_t r = 0, id = 0;
    table_t t = 0, m = du->nt;
    input_t c = 0;
    label_t x = 0, bug = 0, exec[du->nt+1];
    size_t  q = 0, q_size = 0, max_q_size = 0;
    size_t  b = 0, b_size = 0, max_b_size = 0;
    size_t  zero_count = 0, dead_count = 0, mem_times = 0, llc_times = 0;
    double  avg_q_size = 0, avg_b_size = 0;
    int i, rv;

    printf("DEBUG: %s() init\n", __FUNCTION__);
    //is = (index_unit_t *)malloc((du->nt+1)*sizeof(index_unit_t));
    //if (is == NULL) {
    //    rv = -31;
    //    goto err;
    //}
    //memset(is, 0, (du->nt+1)*sizeof(index_unit_t));
    //for (t=0; t<du->nt+1; t++) {
    //    idx_to_exec[t] = is;
    //    is++;
    //}
    //is = idx_to_exec[0];
    //for (t=0; t<du->nt+1; t++) {
    //    exec[t] = 0;
    //}
    for (r=0; r<du->nr+1; r++) {
        stat[r] = 0;
    }
    sf = (state_unit_t *)malloc(QUEUE_SIZE*sizeof(state_unit_t));
    if (sf == NULL) {
        rv = -32;
        goto err;
    }
    memset(sf, 0, QUEUE_SIZE*sizeof(state_unit_t));
    //iu = (index_unit_t *)malloc(QUEUE_SIZE*sizeof(index_unit_t));
    //if (iu == NULL) {
    //    rv = -33;
    //    goto err;
    //}
    //memset(iu, 0, QUEUE_SIZE*sizeof(index_unit_t));
    work_queue = sf;
    for (q=1, sw=work_queue; q<QUEUE_SIZE; q++, sw++) {
        sw->next = (sw + 1);
    }
    sw->next = NULL;
    sw = work_queue;
    for (t=0; t<1; t++) {
        sw->state = 0;
        sw->index = 0;
        sw->table = t;
        sw = sw->next;
        q_size++;
    }
    rest_queue = sw;
    sw--;
    sw->next = NULL;
    sf = work_queue;
    sw = work_queue;
    sr = rest_queue;
    su = sw;
    sv = sr;

    printf("DEBUG: %s() start\n", __FUNCTION__);
    gettimeofday(&start, NULL);
    while (n < ni) {
        c = POOL[n++];
        do {
            sw->state = DFA[sw->table][sw->state][c];
            //if (sw->table == 0 && sw->state == 0) {
            //    zero_count++;
            //}
            //if (sw->table == 1 && sw->state == 1) {
            //    dead_count++;
            //}
            //mem_times++;
            if (TIE[sw->table][sw->state] == NULL) {
                if (sw->state != END[sw->table]) {
                    su = sw;
                    sw = sw->next;
                } else {
                    if (sw == su->next) {
                        su->next = sw->next;
                    } else {
                        work_queue = sw->next;
                        su = sw->next;
                    }
                    sr = sw;
                    sw = sw->next;
                    sr->next = rest_queue;
                    rest_queue = sr;
                    q_size--;
                }
            } else {
                mu = TIE[sw->table][sw->state];
                do {
                    //llc_times++;
                    id = mu->regex;
                    if (MRT[id][0] == sw->index) {
                        if (MRT[id][1] != 0) {
                            bug = 1;
                            sr->table = MRT[id][1] - 1;
                            sr->state = 0;
                            sv = sr;
                            sr = sr->next;
                            if (++q_size >= QUEUE_SIZE) {
                                rv = -34;
                                goto err;
                            }
                        } else {
                            stat[MRT[id][2]]++;
                        }
                    }
                    mu = mu->next;
                } while (mu != NULL);
                if (bug) {
                    bug = 0;
                    if (sr != rest_queue) {
                        sv->next = work_queue;
                        work_queue = rest_queue;
                        rest_queue = sr;
                        if (q_size > QUEUE_WARN) {
                            x = 1;
                        }
                    }
                }
                su = sw;
                sw = sw->next;
            }
        } while (sw != NULL);
        if (!x) {
            sw = work_queue;
        } else {
            x = 0;
            for (sw=work_queue; sw!=NULL;) {
                for (su=sw->next, sv=sw; su!=NULL;) {
                    if (su->state == sw->state && su->index == sw->index && su->table == sw->table) {
                        sv->next = su->next;
                        su->next = rest_queue;
                        rest_queue = su;
                        q_size--;
                    } else {
                        sv = su;
                    }
                    su = sv->next;
                }
                sw=sw->next;
            }
            sw = work_queue;
            sr = rest_queue;
            su = sw;
            sv = sr;
            if (q_size > QUEUE_WARN) {
                QUEUE_WARN = q_size + 4;
            }
        }/*
        max_q_size = (max_q_size < q_size) ? q_size : max_q_size;
        avg_q_size = (avg_q_size * (n-1) + q_size) / n;
        do {
            for (i=0; i<__bit_num; i++) {
                if (idxcmp(i, sw->index)) {
                    b_size++;
                }
            }
            b++;
            max_b_size = (max_b_size < b_size) ? b_size : max_b_size;
            avg_b_size = (avg_b_size * (b-1) + b_size) / b;
            sw = sw->next;
        } while (sw != NULL);
        sw = work_queue;*/
    }
    gettimeofday(&end, NULL);
    printf("DEBUG: %s() end\n", __FUNCTION__);

    for (r=0; r<=du->nr; r++) {
        if (stat[r] > 0) {
            printf("rule %-5hu matched %8u times\n", r, stat[r]);
            rn += 1;
            zn += stat[r];
        }
    }
    for (t=0; t<du->nt; t++) {
        s += du->ns[t];
    }
    printf(">> storage efficiency: %u states\n", s);
    hz = F;
    us = (end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)/1000000.0;
    pe = (hz*1000000000*us)/(ni*du->nn);
    printf(">> running efficiency: %f cycle/byte (%fs/%ubytes)\n", pe, us, ni*du->nn);
    printf(">> # of matched rules: %u\n", rn);
    printf(">> # of matched times: %u\n", zn);
    printf(">> stats: zero_count = %u, dead_count = %u, mem_times = %u, llc_times = %u\n"
            ">> stats: max_q_size = %u, avg_q_size = %f\n>> stats: max_b_size = %u, avg_b_size = %f\n",
            zero_count, dead_count, mem_times, llc_times, max_q_size, avg_q_size, max_b_size, avg_b_size);

    if (is != NULL) {
        free(is);
    }
    if (iu != NULL) {
        free(iu);
    }
    if (sf != NULL) {
        free(sf);
    }
    if (POOL != NULL) {
        free(POOL);
    }
    for (r=0; r<du->nr; r++) {
        if (MRT[r] != NULL) {
            free(MRT[r]);
        }
    }
    for (t=0; t<du->nt; t++) {
        if (TIE[t] != NULL) {
            for (s=0; s<du->ns[t]; s++) {
                if (TIE[t][s] != NULL) {
                    free(TIE[t][s]);
                }
                if (DFA[t][s] != NULL) {
                    free(DFA[t][s]);
                }
            }
            if (TIE[t] != NULL) {
                free(TIE[t]);
            }
            if (DFA[t] != NULL) {
                free(DFA[t]);
            }
        }
    }
    return 0;

err:
    printf("ERROR: %s() return %d\n", __FUNCTION__, rv);
    return rv;
}

void test_throughput(char *argv, int nn)
{
    datum_unit_t datum;
    memset(&datum, 0, sizeof(datum_unit_t));
    int rv;
    rv = load_regex_data(&datum);
    if (rv < 0) {
        goto err;
    }
    rv = load_input_data(&datum, argv);
    if (rv < 0) {
        goto err;
    }
    if (DFA == NULL || END == NULL || TIE == NULL || MRT == NULL
            || POOL == NULL || datum.nt == 0 || datum.ns[0] == 0
            || datum.nr == 0 || datum.np == 0 || datum.ni == 0) {
        printf("ERROR: load data failed\n");
        goto err;
    }
    datum.nn = (count_t)nn;
    rv = process(&datum);
    if (rv < 0) {
        goto err;
    }
    return;

err:
    exit(1);
}
