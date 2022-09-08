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

#define raw_table "./MD2FA"
#define new_table "./MD2FA.tmp"

#define __bit_num 512

#define __word_t uint32_t
#define BL ( sizeof(__word_t) * 8 - 1 )
#define BH ({ __word_t x = 0; __word_t y = BL; while (y) { y = y >> 1; x++; } x; })
#define WQ ( ( (__bit_num) - 1 ) / ( sizeof(__word_t) * 8 ) + 1 )
#define QUEUE_SIZE 64
#define QUEUE_FAST 8

#define F 2.4
#define N 3
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
    struct index_unit *index;
    state_t state;
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
static mstate_t *MFA[N] = {NULL};
static match_unit_t **TIE[N] = {NULL};
static regex_t **MRT = NULL;
static input_t *POOL = NULL;

static size_t QUEUE_WARN = QUEUE_FAST;

void MD2FA(size_t _size, state_t **state_table)
{
    char        cmd[C];
    FILE        *fp;

    int         c, i, j;
    state_t     s, t;
    label_t     c2cid[C], cmask[C];

    state_t     default_trans[_size];        // default transition in default state
    label_t     default_stats[_size];        // number of default transitions in default state
    state_t     default_state[_size];        // default state for labeled states
    label_t     default_masks[_size];

    unsigned long    ddt_trans = 0, ldt_trans = 0, llt_trans = 0;

    // Alphabet Encoding
    fp = fopen(raw_table, "w");
    for (c=0; c<C; c++)
    {
        fprintf(fp, "@");
        for (s=0; s<_size; s++)
        {
            fprintf(fp, "%8u", state_table[s][c]);
        }
        fprintf(fp, ">%8u\n", c);
    }
    fclose(fp);

    sprintf(cmd, "sort %s > %s", raw_table, new_table);
    if (system(cmd) != 0)
    {
        printf("WARNING: system command executes not successfully!\n");
    }
    c = 0;

    fp = fopen(new_table, "r");
    while (1)
    {
        char        valid_mark;
        if (EOF == fscanf(fp, "%c", &valid_mark))
        {
            break;
        }
        if (valid_mark != '@')
        {
            continue;
        }
        for (s=0; s<_size; s++)
        {
            i = fscanf(fp, "%u", &t);
        }
        i = fscanf(fp, ">%hhu", &(c2cid[c++]));
    }
    fclose(fp);

    sprintf(cmd, "rm %s %s -rf", raw_table, new_table);
    if (system(cmd) != 0)
    {
        printf("WARNING: system command executes not successfully!\n");
    }
    for (s=0; s<_size; s++)
    {
        default_state[s] = 0;
    }

    for (c=0; c<C; c++)
    {
        label_t        cid = c2cid[c];
        cmask[cid] = 0;
        for (s=0; s<_size; s++)
        {
            if (state_table[s][cid] != default_state[s])
            {
                default_state[s] = state_table[s][cid];
                cmask[cid] = 1;
            }
        }
    }
    // Alphabet Encoding

    struct timeval start, end;
    gettimeofday(&start, NULL);

    // Default Transition
    for (s=0; s<_size; s++)
    {
        label_t        labeled_index[C];        // index for labeled transitions
        state_t        labeled_trans[C];        // labeled transitions in default state and labeled states
        label_t        overall_stats[C];
        i = 0;
        overall_stats[i] = 1;
        labeled_index[0] = i;
        labeled_trans[i] = state_table[s][0];
        for (c=1; c<C; c++)
        {
            for (j=0; j<=i; j++)
            {
                if (state_table[s][c] == labeled_trans[j])
                {
                    overall_stats[j]++;
                    break;
                }
            }
            if (j > i)
            {
                i++;
                overall_stats[i] = 1;
                labeled_index[c] = i;
                labeled_trans[i] = state_table[s][c];
            }
        }
        default_stats[s] = 0;
        for (j=0; j<=i; j++)
        {
            if (overall_stats[j] > default_stats[s])
            {
                default_stats[s] = overall_stats[j];
                default_trans[s] = labeled_trans[j];
            }
        }
        default_state[s] = G;
    }
    // Default Transition

    // Default State *
    for (s=1; s<_size; s++)
    {
        state_t dt = default_trans[s];
        state_t ds = default_stats[s];
        label_t commax = 0;
        label_t dftmax = 0;
        for (t=0; t<s; t++)
        {
            if ((default_state[t] == G) && (default_trans[t] == dt))
            {
                label_t dfault = default_stats[t];
                label_t common = 0;
                for (c=0; c<C; c++)
                {
                    if (state_table[s][c] == state_table[t][c])
                    {
                        if (state_table[t][c] != dt)
                            common++;
                    }
                }
                if (common > commax)
                {
                    commax = common;
                    dftmax = dfault;
                    default_state[s] = t;
                }
                else if (common == commax)
                {
                    if (dfault > dftmax)
                    {
                        dftmax = dfault;
                        default_state[s] = t;
                    }
                }
            }
        }
        if (commax < (C-ds)*4/5)
        {
            default_state[s] = G;
        }
    }
    // Default State *

    gettimeofday(&end, NULL);
    printf("\n>> MD2FA PREPROCESS\n#time: %ldms\n", (end.tv_sec-start.tv_sec)*1000+(end.tv_usec-start.tv_usec)/1000);

    state_t        g = 0;
    // Output MD2FA
    fp = fopen(raw_table, "w");
    for (s=0; s<_size; s++)
    {
        ddt_trans++;
        if (default_state[s] == G)
        {
            fprintf(fp, "# group %4u=========== %3u (@NO,%5u) ===========\n", ++g, default_stats[s], default_trans[s]);
            for (c=0; c<C; c++)
            {
                if (cmask[c] && (state_table[s][c] != default_trans[s]))
                {
                    ldt_trans++;
                    fprintf(fp, "(%2x,%5u)\t", c, state_table[s][c]);
                }
            }
            fprintf(fp, "\n");

            for (t=0; t<_size; t++)
            {
                if (default_state[t] == s)
                {
                    for (c=0; c<C; c++)
                    {
                        if (cmask[c] && (state_table[t][c] != state_table[s][c]))
                        {
                            llt_trans++;
                            fprintf(fp, "(%2x,%5u)\t", c, state_table[t][c]);
                        }
                    }
                    fprintf(fp, "\n");
                }
            }

        }
    }
    fclose(fp);
    // Output MD2FA

    printf(">> MD2FA  PERFORMANCE\n#trans: %lu   #dtds: %lu   #ltds: %lu   #ltls: %lu   ratio: %f\n", ddt_trans+ldt_trans+llt_trans, ddt_trans, ldt_trans, llt_trans, (256*_size - ddt_trans - ldt_trans - llt_trans)*1.0/(256*_size));
}

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
    // MD2FA
    for (t=0; t<du->nt; t++) {
        MFA[t] = (mstate_t *)malloc(du->ns[t] * sizeof(mstate_t));
        if (!MFA[t]) {
            rv = -16;
            goto err;
        }
        memset(MFA[t], 0, du->ns[t] * sizeof(mstate_t));
        for (s=0; s<du->ns[t]; s++) {
            for (i=0; i<C; i++) {
                magic[i] = 0;
                count[i] = 0;
            }
            k = 0;
            for (i=0; i<C; i++) {
                for (j=0; j<k; j++) {
                    if (DFA[t][s][i] == magic[j]) {
                        break;
                    }
                }
                count[j]++;
                if (j == k) {
                    magic[j] = DFA[t][s][i];
                    k++;
                }
            }
            i = 0;
            for (j=1; j<k; j++) {
                if (count[j] > count[i]) {
                    i = j;
                }
            }
            MFA[t][s].magic = magic[i];
            printf("DFA[%hhu], state %u, magic: %u", t, s, magic[i]);
            MFA[t][s].num = C - count[i];
            num++;
            max_t_num = (max_t_num < MFA[t][s].num) ? MFA[t][s].num : max_t_num;
            avg_t_num = (avg_t_num * (num-1) + MFA[t][s].num) / num;
            mtrans_num += MFA[t][s].num;
            trans_num += 256;
            MFA[t][s].mtrans = (mtrans_t *)malloc(MFA[t][s].num * sizeof(mtrans_t));
            if (!MFA[t][s].mtrans) {
                rv = -17;
                goto err;
            }
            memset(MFA[t][s].mtrans, 0, MFA[t][s].num * sizeof(mtrans_t));
            j = 0;
            for (i=0; i<C; i++) {
                if (DFA[t][s][i] != MFA[t][s].magic) {
                    printf(" %hhu: %u", i, DFA[t][s][i]);
                    MFA[t][s].mtrans[j].state = DFA[t][s][i];
                    MFA[t][s].mtrans[j].input = i;
                    j++;
                }
            }
            printf("\n");
        }
    }
    printf(">> stats: max_t_num = %u, avg_t_num = %f, comp_ratio: %u / %u = %f\n",
            max_t_num, avg_t_num, mtrans_num, trans_num, mtrans_num*1.0/trans_num);
    for (t=0; t<du->nt; t++) {
        MD2FA(du->ns[t], DFA[t]);
    }
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
    mstate_t *tmp;
    double  hz, us = 0, pe;
    count_t n = 0, rn = 0, zn = 0, ni = du->ni, stat[du->nr+1];
    state_t s = du->nn;
    regex_t r = 0, id = 0;
    table_t t = 0, m = du->nt;
    input_t c = 0;
    label_t x = 0, bug = 0, exec[du->nt+1];
    size_t  q = 0, q_size = 0, max_q_size = 0;
    size_t  b = 0, b_size = 0, max_b_size = 0;
    size_t  zero_count = 0, dead_count = 0, mem_times = 0, llc_times = 0, adjust_times;
    double  avg_q_size = 0, avg_b_size = 0;
    int i, rv;

    printf("DEBUG: %s() init\n", __FUNCTION__);
    is = (index_unit_t *)malloc((du->nt+1)*sizeof(index_unit_t));
    if (is == NULL) {
        rv = -31;
        goto err;
    }
    memset(is, 0, (du->nt+1)*sizeof(index_unit_t));
    for (t=0; t<du->nt+1; t++) {
        idx_to_exec[t] = is;
        is++;
    }
    is = idx_to_exec[0];
    for (t=0; t<du->nt+1; t++) {
        exec[t] = 0;
    }
    for (r=0; r<du->nr+1; r++) {
        stat[r] = 0;
    }
    sf = (state_unit_t *)malloc(QUEUE_SIZE*sizeof(state_unit_t));
    if (sf == NULL) {
        rv = -32;
        goto err;
    }
    memset(sf, 0, QUEUE_SIZE*sizeof(state_unit_t));
    iu = (index_unit_t *)malloc(QUEUE_SIZE*sizeof(index_unit_t));
    if (iu == NULL) {
        rv = -33;
        goto err;
    }
    memset(iu, 0, QUEUE_SIZE*sizeof(index_unit_t));
    work_queue = sf;
    for (q=1, sw=work_queue; q<QUEUE_SIZE; q++, sw++, iu++) {
        sw->next = (sw + 1);
        sw->index = iu;
    }
    sw->next = NULL;
    sw->index = iu;
    iu = sf->index;
    sw = work_queue;
    for (t=0; t<1; t++) {
        sw->table = t;
        sw->state = 0;
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
            if (sw->table == 0 && sw->state == 0) {
                zero_count++;
            }
            if (sw->table == 1 && sw->state == 1) {
                dead_count++;
            }
            mem_times++;
            //tmp = &MFA[sw->table][sw->state];
            //if (c < tmp->mtrans[0].input || c > tmp->mtrans[tmp->num-1].input) {
            //    sw->state = tmp->magic;
            //} else {
            //    for (i=0; i<tmp->num; i++) {
            //        if (c == tmp->mtrans[i].input) {
            //            sw->state = tmp->mtrans[i].state;
            //            break;
            //        }
            //    }
            //}
            //if (i == tmp->num) {
            //    sw->state = tmp->magic;
            //}
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
                    llc_times++;
                    id = mu->regex;
                    if (MRT[id][0] == 0 || idxcmp(MRT[id][0], sw->index)) {
                        if (MRT[id][1] == 0) {
                            stat[MRT[id][2]]++;
                        } else {
                            bug = 1;
                            t = MRT[id][1] - 1;
                            if (t != sw->table) {
                                exec[t] = 1;
                                idxadd(id, idx_to_exec[t]);
                            } else {
                                exec[m] = 1;
                                idxadd(id, idx_to_exec[m]);
                            }
                        }
                    }
                    mu = mu->next;
                } while (mu != NULL);
                if (bug) {
                    bug = 0;
                    for (t=0; t<m; t++) {
                        if (exec[t]) {
                            exec[t] = 0;
                            idxcpy(idx_to_exec[t], sr->index);
                            sr->table = t;
                            sr->state = 0;
                            sv = sr;
                            sr = sr->next;
                            if (++q_size >= QUEUE_SIZE) {
                                rv = -34;
                                goto err;
                            }
                        }
                    }
                    if (sr != rest_queue) {
                        sv->next = work_queue;
                        work_queue = rest_queue;
                        rest_queue = sr;
                        if (q_size > QUEUE_WARN) {
                            x = 1;
                        }
                    }
                    if (exec[m]) {
                        exec[m] = 0;
                        idxcat(idx_to_exec[m], sw->index);
                    }
                }
                su = sw;
                sw = sw->next;
            }
        } while (sw != NULL);
        if (!x) {
            sw = work_queue;
        } else {
            adjust_times++;
            x = 0;
            for (sw=work_queue; sw!=NULL;) {
                for (su=sw->next, sv=sw; su!=NULL;) {
                    if (su->state == sw->state && su->table == sw->table) {
                        idxcat(su->index, sw->index);
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
            if (q_size > QUEUE_WARN) {
                QUEUE_WARN = q_size + 4;
            }
        }
        max_q_size = (max_q_size < q_size) ? q_size : max_q_size;
        avg_q_size = (avg_q_size * (n-1) + q_size) / n;
        do {
            b_size = 0;
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
        sw = work_queue;
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
    printf(">> stats: zero_count = %u, dead_count = %u, mem_times = %u, llc_times = %u, adjust_times = %u\n"
            ">> stats: max_q_size = %u, avg_q_size = %f\n>> stats: max_b_size = %u, avg_b_size = %f\n",
            zero_count, dead_count, mem_times, llc_times, adjust_times, max_q_size, avg_q_size, max_b_size, avg_b_size);

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
