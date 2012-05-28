#ifndef _CONNCTRL_H_
#define _CONNCTRL_H_

#ifdef __cplusplus
extern "C" {
#endif

void conn_counter_init(int _max_cnt);
void conn_counter_fini(void);
void conn_counter_inc(void);
void conn_counter_dec(void);
int  conn_counter_limit(void);

#ifdef __cplusplus
}
#endif

#endif
