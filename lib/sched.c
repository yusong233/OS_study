#include <env.h>
#include <pmap.h>
#include <printf.h>

/* Overview:
 *  Implement simple round-robin scheduling.
 *  Search through 'envs' for a runnable environment ,
 *  in circular fashion statrting after the previously running env,
 *  and switch to the first such environment found.
 *
 * Hints:
 *  The variable which is for counting should be defined as 'static'.
 */


void sched_yield(void)
{
        static int cur_time = 0;
        static int x = 0;
        static struct Env *cur;
        while (--cur_time <= 0 || cur == NULL || cur->env_status != ENV_RUNNABLE) {
                //printf("in the while of sched\n");
                //printf("time : %d, cur : %d, cur->env_status : %d\n", cur_time, cur, cur->env_status);
                if (LIST_EMPTY(&env_sched_list[x])) {
                        x = 1 - x;
                }
                cur = LIST_FIRST(&env_sched_list[x]);
                if (cur == NULL) { 
                        continue;
                } else {
                        LIST_REMOVE(cur, env_sched_link);
                        LIST_INSERT_HEAD(&env_sched_list[1-x], cur, env_sched_link);
                        cur_time = cur->env_pri;
                        break;
                }
                //printf("begin to out in sched\n");
        }
        env_run(cur);
}

