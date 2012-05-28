#ifndef _CONNCTRL_H_
#define _CONNCTRL_H_

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CONN_EXT = 0,
    CONN_INT,
    CONN_TYPE_MAX
};

void conn_counter_init(int type, int _max_cnt);
void conn_counter_fini(int type);
void conn_counter_inc(int type);
void conn_counter_dec(int type);
int  conn_counter_limit(int type);

#ifdef __cplusplus
}
#endif

#endif
