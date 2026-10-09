#include "algo.h"

typedef enum {
    ST_SETTLING,        /* wait in the current cell until stress has converged */
    ST_PROBING,         /* try a neighbouring cell and watch the response */
    ST_PANIC_RECOVERY,  /* stress shot up: get the baby out of panic */
    ST_DONE             /* at (F1, A1) and calm */
} AlgoState;

#define CRY_FLOOR 5.0f   /* ASSUMED (%): crying at 10 % stress, measure on twin */
#define CRY_SAT 98.0f    /*ASSUMED*/

static AlgoState state;
static Command   cmd;
static float     t_entered;   /* time the current state/cell was entered */

void algo_init(void)
{
    state     = ST_SETTLING;
    cmd.f     = 5;            /* the baby always starts at K9 = (F5, A5) */
    cmd.a     = 5;
    t_entered = 0.0f;
}

float algo_estimate_stress(Measurement m, int *src)
{
    /* Crying responds instantly below 50% stress (Figure 2), so it is used until it
       saturates at CRY_SAT; above that, heart rate is the only usable estimate.
       Justified in Section 3.3. */
    float s;
    float s_hr = 10.0f + (m.bpm - 60.0f)/2.0f;
    float s_cry = 10.0f + 40.0f*(m.cry_pct-CRY_FLOOR)/(100.0f-CRY_FLOOR);

    if (s_hr < 10.0f) s_hr = 10.0f;
    if (s_hr > 100.0f) s_hr = 100.0f;

    if (s_cry < 10.0f) s_cry = 10.0f;
    if (s_cry > 50.0f) s_cry = 50.0f;

    if (m.cry_pct < CRY_SAT) {
        s = s_cry;
        *src = 1;
    } else {
        s = s_hr;
        *src = 0;
    }

    return s;
}

Command algo_step(Measurement m, float t)
{
    int src;
    float s = algo_estimate_stress(m, &src);   /* src: 0 = heart rate, 1 = crying */
    (void)s; (void)src;   /* remove once the state machine uses them */

    switch (state) {
    case ST_SETTLING:
        /* TODO: how do you decide stress has stopped changing?
           Think: slope over a time window, minimum wait time, noise. */
        break;

    case ST_PROBING:
        /* TODO: which neighbour do you try first, and how long before you judge it?
           Remember the cell you came from, and which neighbours you already tried. */
        break;

    case ST_PANIC_RECOVERY:
        /* TODO: detect panic (stress rising fast or near 100%) and decide how to recover. */
        break;

    case ST_DONE:
        break;
    }

    (void)t_entered; (void)t;
    return cmd;
}
