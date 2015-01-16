/*
 * drivers/cpufreq/cpufreq_fantasys.c
 *
 * copyright (c) 2012-2013 allwinnertech
 *
 * based on ondemand policy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/threads.h>
#include <linux/suspend.h>
#include <linux/delay.h>
#include <linux/delay.h>
#include <mach/sys_config.h>
#include <mach/powernow.h>
#include <linux/input.h>


#define FANTASY_DEBUG_HOTPLUG               (1)
#define FANTASY_DEBUG_CPUFREQ               (2)

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */
#define DEF_FREQUENCY_DOWN_DIFFERENTIAL     (10)
#define DEF_FREQUENCY_UP_THRESHOLD          (80)
#define DEF_SAMPLING_DOWN_FACTOR            (2)
#define MAX_SAMPLING_DOWN_FACTOR            (100000)
#define MICRO_FREQUENCY_DOWN_DIFFERENTIAL   (3)
#define MICRO_FREQUENCY_UP_THRESHOLD        (90)
#define MICRO_FREQUENCY_MIN_SAMPLE_RATE     (10000)
#define MIN_FREQUENCY_UP_THRESHOLD          (11)
#define MAX_FREQUENCY_UP_THRESHOLD          (100)

/*
 * The polling frequency of this governor depends on the capability of
 * the processor. Default polling frequency is 1000 times the transition
 * latency of the processor. The governor will work on any processor with
 * transition latency <= 10mS, using appropriate sampling
 * rate.
 * For CPUs with transition latency > 10mS (mostly drivers with CPUFREQ_ETERNAL)
 * this governor will not work.
 * All times here are in uS.
 */
#define MIN_SAMPLING_RATE_RATIO             (2)

static unsigned int min_sampling_rate;

#define LATENCY_MULTIPLIER                  (50)
#define MIN_LATENCY_MULTIPLIER              (10)
#define TRANSITION_LATENCY_LIMIT            (10 * 1000 * 1000)

#define POWERNOW_PERFORM_MAX       (1008000)    /* config the maximum frequency of performance mode */
#define POWERNOW_NORMAL_MAX        (1008000)    /* config the maximum frequency of normal mode */
#define POWERNOW_USER_EVENT_FREQ   (720000)    /* config the userevent frequency of normal mode */

#define LOAD_INT(x) ((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1-1)) * 100)

/* check cpu needs plugin only (cur freq > DEF_CPU_UP_FREQ) */
#define DEF_CPU_UP_LOADING        ((1<<FSHIFT) + (60)*(FIXED_1-1)/100)
#define DEF_CPU_DOWN_LOADING      ((1<<FSHIFT) + (80)*(FIXED_1-1)/100) 
#define DEF_CPU_UP_ACTIVE_TASK    4
#define DEF_CPU_DOWN_ACTIVE_TASK  1 

/* Sampling types */
enum {DBS_NORMAL_SAMPLE, DBS_SUB_SAMPLE};

static int start_powernow(unsigned long mode);
static int powernow_notifier_call(struct notifier_block *nfb,
                    unsigned long mode, void *cmd);

static DEFINE_PER_CPU(struct work_struct, dbs_refresh_work);
static struct workqueue_struct *input_wq;

/*
 ----------------------------------------------------------
 define policy table for cpu-hotplug
 ----------------------------------------------------------
 */

/*
 * define data structure for dbs
 */
struct cpu_dbs_info_s {
    u64 prev_cpu_idle;          /* previous idle time statistic */
    u64 prev_cpu_iowait;        /* previous iowait time stat    */
    u64 prev_cpu_wall;          /* previous total time stat     */
    u64 prev_cpu_nice;          /* previous nice time           */
    struct cpufreq_policy *cur_policy;
    struct delayed_work work;
    struct cpufreq_frequency_table *freq_table;
    unsigned int freq_lo;
    unsigned int freq_lo_jiffies;
    unsigned int freq_hi_jiffies;
    unsigned int rate_mult;
    int cpu;                    /* current cpu number           */
    unsigned int sample_type:1;
    struct work_struct up_work;     /* cpu plug-in processor    */
    struct work_struct down_work;   /* cpu plug-out processer   */
    /*
     * percpu mutex that serializes governor limit change with
     * do_dbs_timer invocation. We do not want do_dbs_timer to run
     * when user is changing the governor or limits.
     */
    struct mutex timer_mutex;   /* semaphore for protection     */
};

/*
 * define percpu variable for dbs information
 */
static DEFINE_PER_CPU(struct cpu_dbs_info_s, od_cpu_dbs_info);
static struct workqueue_struct *dvfs_workqueue;
static int cpufreq_governor_dbs(struct cpufreq_policy *policy, unsigned int event);

/* number of CPUs using this policy */
static int dbs_enable;

/*
 * dbs_mutex protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);


struct dbs_tuners {
    unsigned int sampling_rate;     /* dvfs sample rate                             */
    unsigned int up_threshold;
    unsigned int down_differential;
    unsigned int ignore_nice;
    unsigned int sampling_down_factor;
    unsigned int powersave_bias;
    unsigned int io_is_busy;        /* flag to mark if iowait calculate as cpu busy */

    /* pegasusq tuners */
    atomic_t hotplug_lock;          /* lock cpu online number, disable plug-in/out  */
    unsigned int dvfs_debug;        /* dvfs debug flag, print dbs information       */
    unsigned int powernow;          /* power now mode                               */
    unsigned int pn_perform_freq;   /* performance mode max freq                    */
    unsigned int pn_normal_freq;    /* normal mode max freq                         */
    unsigned int user_event_freq;    /* normal mode max freq                         */
    unsigned int freq_lock;         /* lock cpu freq                                */
} ;

static struct dbs_tuners dbs_tuners_ins = {
    .up_threshold = DEF_FREQUENCY_UP_THRESHOLD,
    .sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR,
    .down_differential = DEF_FREQUENCY_DOWN_DIFFERENTIAL,
    .ignore_nice = 0,
    .powersave_bias = 0,
    .hotplug_lock = ATOMIC_INIT(0),
    .dvfs_debug = 0,
    .powernow = SW_POWERNOW_NORMAL,
    .pn_perform_freq = POWERNOW_PERFORM_MAX,
    .pn_normal_freq = POWERNOW_NORMAL_MAX,
    .user_event_freq = POWERNOW_USER_EVENT_FREQ,
    .freq_lock = 0,
};


struct cpufreq_governor cpufreq_gov_fantasys = {
    .name                   = "fantasys",
    .governor               = cpufreq_governor_dbs,
    .owner                  = THIS_MODULE,
};

struct cpufreq_frequency_table fantasys_freq_tbl[] = {
    { .frequency = 60000,  .index = 0, },
    { .frequency = 132000, .index = 0, },
    { .frequency = 288000, .index = 0, },
    { .frequency = 408000, .index = 0, },
    { .frequency = 528000, .index = 0, },
    { .frequency = 600000, .index = 0, },
    { .frequency = 720000, .index = 0, },
    { .frequency = 768000, .index = 0, },
    { .frequency = 864000, .index = 0, },
    { .frequency = 912000, .index = 0, },
    { .frequency = 1008000, .index = 0,},
    { .frequency = 1056000, .index = 0,},
    /* table end */
    { .frequency = CPUFREQ_TABLE_END,  .index = 0,              },
};


/*
 * CPU hotplug lock interface
 */
static atomic_t g_hotplug_lock = ATOMIC_INIT(0);

static inline u64 get_cpu_idle_time_jiffy(unsigned int cpu, u64 *wall)
{
    u64 idle_time;
    u64 cur_wall_time;
    u64 busy_time;

    cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

    busy_time  = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];
    idle_time = cur_wall_time - busy_time;
    if (wall)
        *wall = jiffies_to_usecs(cur_wall_time);

    return jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
    u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

    if (idle_time == -1ULL)
        return get_cpu_idle_time_jiffy(cpu, wall);
    else
        idle_time += get_cpu_iowait_time_us(cpu, wall);

    return idle_time;
}

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu, cputime64_t *wall)
{
    u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

    if (iowait_time == -1ULL)
        return 0;

    return iowait_time;
}

/*
 * Find right freq to be set now with powersave_bias on.
 * Returns the freq_hi to be used right now and will set freq_hi_jiffies,
 * freq_lo, and freq_lo_jiffies in percpu area for averaging freqs.
 */
static unsigned int powersave_bias_target(struct cpufreq_policy *policy,
                      unsigned int freq_next,
                      unsigned int relation)
{
    unsigned int freq_req, freq_reduc, freq_avg;
    unsigned int freq_hi, freq_lo;
    unsigned int index = 0;
    unsigned int jiffies_total, jiffies_hi, jiffies_lo;
    struct cpu_dbs_info_s *dbs_info = &per_cpu(od_cpu_dbs_info,
                           policy->cpu);

    if (!dbs_info->freq_table) {
        dbs_info->freq_lo = 0;
        dbs_info->freq_lo_jiffies = 0;
        return freq_next;
    }

    cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_next,
            relation, &index);
    freq_req = dbs_info->freq_table[index].frequency;
    freq_reduc = freq_req * dbs_tuners_ins.powersave_bias / 1000;
    freq_avg = freq_req - freq_reduc;

    /* Find freq bounds for freq_avg in freq_table */
    index = 0;
    cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_avg,
            CPUFREQ_RELATION_H, &index);
    freq_lo = dbs_info->freq_table[index].frequency;
    index = 0;
    cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_avg,
            CPUFREQ_RELATION_L, &index);
    freq_hi = dbs_info->freq_table[index].frequency;

    /* Find out how long we have to be in hi and lo freqs */
    if (freq_hi == freq_lo) {
        dbs_info->freq_lo = 0;
        dbs_info->freq_lo_jiffies = 0;
        return freq_lo;
    }
    jiffies_total = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);
    jiffies_hi = (freq_avg - freq_lo) * jiffies_total;
    jiffies_hi += ((freq_hi - freq_lo) / 2);
    jiffies_hi /= (freq_hi - freq_lo);
    jiffies_lo = jiffies_total - jiffies_hi;
    dbs_info->freq_lo = freq_lo;
    dbs_info->freq_lo_jiffies = jiffies_lo;
    dbs_info->freq_hi_jiffies = jiffies_hi;
    return freq_hi;
}

static void ondemand_powersave_bias_init_cpu(int cpu)
{
    struct cpu_dbs_info_s *dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
    dbs_info->freq_table = fantasys_freq_tbl;
    dbs_info->freq_lo = 0;
}

static void ondemand_powersave_bias_init(void)
{
    int i;
    for_each_online_cpu(i) {
        ondemand_powersave_bias_init_cpu(i);
    }
}

/*
 * apply cpu hotplug lock, up or down cpu
 */
static void apply_hotplug_lock(void)
{
    int online, possible, lock, flag;
    struct work_struct *work;
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    online = num_online_cpus();
    possible = num_possible_cpus();
    lock = atomic_read(&g_hotplug_lock);
    flag = lock - online;

    if (flag == 0)
        return;

    work = flag > 0 ? &dbs_info->up_work : &dbs_info->down_work;

    printk(KERN_DEBUG "%s online:%d possible:%d lock:%d flag:%d %d\n",
         __func__, online, possible, lock, flag, (int)abs(flag));

    queue_work_on(dbs_info->cpu, dvfs_workqueue, work);
}

/*
 * lock cpu number, the number of onlie cpu should less then num_core
 */
int cpufreq_fantasys_cpu_lock(int num_core)
{
    int prev_lock;
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);

    if (num_core < 1 || num_core > num_possible_cpus()) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    prev_lock = atomic_read(&g_hotplug_lock);
    if (prev_lock != 0 && prev_lock < num_core) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    atomic_set(&g_hotplug_lock, num_core);
    apply_hotplug_lock();
    mutex_unlock(&dbs_info->timer_mutex);

    return 0;
}

/*
 * unlock cpu hotplug number
 */
int cpufreq_fantasys_cpu_unlock(int num_core)
{
    int prev_lock = atomic_read(&g_hotplug_lock);
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);

    if (prev_lock != num_core) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    atomic_set(&g_hotplug_lock, 0);
    mutex_unlock(&dbs_info->timer_mutex);

    return 0;
}
/************************** sysfs interface ************************/

static ssize_t show_sampling_rate_min(struct kobject *kobj,
                      struct attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", min_sampling_rate);
}

define_one_global_ro(sampling_rate_min);


/* cpufreq_pegasusq Governor Tunables */
#define show_one(file_name, object)                    \
static ssize_t show_##file_name                        \
(struct kobject *kobj, struct attribute *attr, char *buf)        \
{                                    \
    return sprintf(buf, "%u\n", dbs_tuners_ins.object);        \
}
show_one(sampling_rate, sampling_rate);
show_one(io_is_busy, io_is_busy);
show_one(up_threshold, up_threshold);
show_one(sampling_down_factor, sampling_down_factor);
show_one(ignore_nice_load, ignore_nice);
show_one(powersave_bias, powersave_bias);
show_one(dvfs_debug, dvfs_debug);
show_one(powernow, powernow);
show_one(pn_perform_freq, pn_perform_freq);
show_one(pn_normal_freq, pn_normal_freq);
/**
 * update_sampling_rate - update sampling rate effective immediately if needed.
 * @new_rate: new sampling rate
 *
 * If new rate is smaller than the old, simply updaing
 * dbs_tuners_int.sampling_rate might not be appropriate. For example,
 * if the original sampling_rate was 1 second and the requested new sampling
 * rate is 10 ms because the user needs immediate reaction from ondemand
 * governor, but not sure if higher frequency will be required or not,
 * then, the governor may change the sampling rate too late; up to 1 second
 * later. Thus, if we are reducing the sampling rate, we need to make the
 * new value effective immediately.
 */
static void update_sampling_rate(unsigned int new_rate)
{
    int cpu;

    dbs_tuners_ins.sampling_rate = new_rate
                     = max(new_rate, min_sampling_rate);

    for_each_online_cpu(cpu) {
        struct cpufreq_policy *policy;
        struct cpu_dbs_info_s *dbs_info;
        unsigned long next_sampling, appointed_at;

        policy = cpufreq_cpu_get(cpu);
        if (!policy)
            continue;
        dbs_info = &per_cpu(od_cpu_dbs_info, policy->cpu);
        cpufreq_cpu_put(policy);

        mutex_lock(&dbs_info->timer_mutex);

        if (!delayed_work_pending(&dbs_info->work)) {
            mutex_unlock(&dbs_info->timer_mutex);
            continue;
        }

        next_sampling  = jiffies + usecs_to_jiffies(new_rate);
        appointed_at = dbs_info->work.timer.expires;


        if (time_before(next_sampling, appointed_at)) {

            mutex_unlock(&dbs_info->timer_mutex);
            cancel_delayed_work_sync(&dbs_info->work);
            mutex_lock(&dbs_info->timer_mutex);

            queue_delayed_work_on(dbs_info->cpu, dvfs_workqueue, &dbs_info->work,
                         usecs_to_jiffies(new_rate));

        }
        mutex_unlock(&dbs_info->timer_mutex);
    }
}

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b,
                   const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    update_sampling_rate(input);
    return count;
}

static ssize_t store_io_is_busy(struct kobject *a, struct attribute *b,
                   const char *buf, size_t count)
{
    unsigned int input;
    int ret;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dbs_tuners_ins.io_is_busy = !!input;
    return count;
}

static ssize_t store_up_threshold(struct kobject *a, struct attribute *b,
                  const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);

    if (ret != 1 || input > MAX_FREQUENCY_UP_THRESHOLD ||
            input < MIN_FREQUENCY_UP_THRESHOLD) {
        return -EINVAL;
    }
    dbs_tuners_ins.up_threshold = input;
    return count;
}

static ssize_t store_sampling_down_factor(struct kobject *a,
            struct attribute *b, const char *buf, size_t count)
{
    unsigned int input, j;
    int ret;
    ret = sscanf(buf, "%u", &input);

    if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR || input < 1)
        return -EINVAL;
    dbs_tuners_ins.sampling_down_factor = input;

    /* Reset down sampling multiplier in case it was active */
    for_each_online_cpu(j) {
        struct cpu_dbs_info_s *dbs_info;
        dbs_info = &per_cpu(od_cpu_dbs_info, j);
        dbs_info->rate_mult = 1;
    }
    return count;
}

static ssize_t store_ignore_nice_load(struct kobject *a, struct attribute *b,
                      const char *buf, size_t count)
{
    unsigned int input;
    int ret;

    unsigned int j;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    if (input > 1)
        input = 1;

    if (input == dbs_tuners_ins.ignore_nice) { /* nothing to do */
        return count;
    }
    dbs_tuners_ins.ignore_nice = input;

    /* we need to re-evaluate prev_cpu_idle */
    for_each_online_cpu(j) {
        struct cpu_dbs_info_s *dbs_info;
        dbs_info = &per_cpu(od_cpu_dbs_info, j);
        dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
                        &dbs_info->prev_cpu_wall);
        if (dbs_tuners_ins.ignore_nice)
            dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];

    }
    return count;
}

static ssize_t store_powersave_bias(struct kobject *a, struct attribute *b,
                    const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);

    if (ret != 1)
        return -EINVAL;

    if (input > 1000)
        input = 1000;

    dbs_tuners_ins.powersave_bias = input;
    ondemand_powersave_bias_init();
    return count;
}
static ssize_t show_hotplug_lock(struct kobject *kobj, struct attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", atomic_read(&g_hotplug_lock));
}

static ssize_t store_hotplug_lock(struct kobject *a, struct attribute *b,
                  const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    int prev_lock;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    input = min(input, num_possible_cpus());
    prev_lock = atomic_read(&dbs_tuners_ins.hotplug_lock);

    if (prev_lock)
        cpufreq_fantasys_cpu_unlock(prev_lock);

    if (input == 0) {
        atomic_set(&dbs_tuners_ins.hotplug_lock, 0);
        return count;
    }

    ret = cpufreq_fantasys_cpu_lock(input);
    if (ret) {
        printk(KERN_ERR "[HOTPLUG] already locked with smaller value %d < %d\n",
            atomic_read(&g_hotplug_lock), input);
        return ret;
    }

    atomic_set(&dbs_tuners_ins.hotplug_lock, input);

    return count;
}

static ssize_t store_dvfs_debug(struct kobject *a, struct attribute *b,
                const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dbs_tuners_ins.dvfs_debug = input;
    return count;
}

static ssize_t store_powernow(struct kobject *a, struct attribute *b,
                const char *buf, size_t count)
{
    unsigned int input = 0;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (dbs_tuners_ins.powernow != input)
    {
        dbs_tuners_ins.powernow = input;
        powernow_notifier_call(NULL, dbs_tuners_ins.powernow, NULL);
    }
    return count;
}
static ssize_t store_pn_normal_freq(struct kobject *a, struct attribute *b,
                const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    if (input < POWERNOW_NORMAL_MAX && input != dbs_tuners_ins.pn_normal_freq)
    {
        dbs_tuners_ins.pn_normal_freq = input ;
        if (dbs_tuners_ins.powernow == SW_POWERNOW_NORMAL)
        {
            powernow_notifier_call(NULL, SW_POWERNOW_NORMAL, NULL);
        }
    }
    return count;
}
static ssize_t store_pn_perform_freq(struct kobject *a, struct attribute *b,
                const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    if (input < POWERNOW_PERFORM_MAX && input != dbs_tuners_ins.pn_perform_freq)
    {
        dbs_tuners_ins.pn_perform_freq = input ;
        if (dbs_tuners_ins.powernow == SW_POWERNOW_PERFORMANCE)
        {
            powernow_notifier_call(NULL, SW_POWERNOW_PERFORMANCE, NULL);
        }
    }
    return count;
}

define_one_global_rw(sampling_rate);
define_one_global_rw(io_is_busy);
define_one_global_rw(up_threshold);
define_one_global_rw(sampling_down_factor);
define_one_global_rw(ignore_nice_load);
define_one_global_rw(powersave_bias);
define_one_global_rw(hotplug_lock);
define_one_global_rw(dvfs_debug);
define_one_global_rw(powernow);
define_one_global_rw(pn_normal_freq);
define_one_global_rw(pn_perform_freq);

static struct attribute *dbs_attributes[] = {
    &sampling_rate_min.attr,
    &sampling_rate.attr,
    &up_threshold.attr,
    &sampling_down_factor.attr,
    &ignore_nice_load.attr,
    &powersave_bias.attr,
    &io_is_busy.attr,
    &hotplug_lock.attr,
    &dvfs_debug.attr,
    &powernow.attr,
    &pn_normal_freq.attr,
    &pn_perform_freq.attr,
    NULL
};

static struct attribute_group dbs_attr_group = {
    .attrs = dbs_attributes,
    .name = "fantasys",
};


/*
 * cpu hotplug, just plug in one cpu
 */
static void cpu_up_work(struct work_struct *work)
{
    int cpu, nr_up, online, hotplug_lock;
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
//    mutex_lock(&dbs_info->timer_mutex);
    online = num_online_cpus();
    hotplug_lock = atomic_read(&g_hotplug_lock);

    if (hotplug_lock) {
        nr_up = (hotplug_lock - online) > 0? (hotplug_lock-online) : 0;
    } else {
        nr_up = 1;
    }

    for_each_cpu_not(cpu, cpu_online_mask) {
        if (cpu == 0)
            continue;

        if (nr_up-- == 0)
            break;

        printk(KERN_DEBUG "cpu up:%d\n", cpu);
        cpu_up(cpu);
    }
//    mutex_unlock(&dbs_info->timer_mutex);
}

/*
 * cpu hotplug, cpu plugout
 */
static void cpu_down_work(struct work_struct *work)
{
    int cpu, nr_down, online, hotplug_lock;
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
//    mutex_lock(&dbs_info->timer_mutex);
    online = num_online_cpus();
    hotplug_lock = atomic_read(&g_hotplug_lock);

    if (hotplug_lock) {
        nr_down = (online - hotplug_lock) > 0? (online-hotplug_lock) : 0;
    } else {
        nr_down = 1;
    }

    for_each_online_cpu(cpu) {
        if (cpu == 0)
            continue;

        if (nr_down-- == 0)
            break;

        cpu_down(cpu);
    }
//    mutex_unlock(&dbs_info->timer_mutex);
}


/*
 * check if need plug in one cpu core
 */
static int check_hotplug(struct cpu_dbs_info_s *this_dbs_info)
{
    unsigned long load[3];
    int online;
    int ncpus;
    int hotplug_lock = atomic_read(&g_hotplug_lock);

    online = num_online_cpus();
    ncpus = online;

    /* hotplug has been locked, do nothing */
    if (hotplug_lock > 0){
        ncpus = hotplug_lock;
        goto __do_hotplug;
    }
    
    get_avenrun_sec(load, FIXED_1/200, 0);
    if (unlikely(dbs_tuners_ins.dvfs_debug & FANTASY_DEBUG_HOTPLUG)) {
        printk("load0:%lx,(%lu.%02lu); load1:%lx,(%lu.%02lu), active task:%lu\n", 
                load[0], LOAD_INT(load[0]), LOAD_FRAC(load[0]),
                load[1], LOAD_INT(load[1]), LOAD_FRAC(load[1]),
                load[2]);
    }

    if (load[2] >= DEF_CPU_UP_ACTIVE_TASK && online < num_possible_cpus())
    {
        ncpus = online + 1;
    }
    else if (load[2] <= DEF_CPU_DOWN_ACTIVE_TASK 
        && load[0] < DEF_CPU_DOWN_LOADING && online > 1){
        ncpus = online - 1;
    }
    
__do_hotplug:
    if (ncpus > online){
        queue_work_on(this_dbs_info->cpu, dvfs_workqueue, &this_dbs_info->up_work);
    }else if (ncpus < online) {
        queue_work_on(this_dbs_info->cpu, dvfs_workqueue, &this_dbs_info->down_work);
    }
    
    return 0;
}

static void dbs_freq_increase(struct cpufreq_policy *p, unsigned int freq)
{
    if (dbs_tuners_ins.powersave_bias)
        freq = powersave_bias_target(p, freq, CPUFREQ_RELATION_H);
    else if (p->cur == p->max)
        return;

    if (unlikely(dbs_tuners_ins.dvfs_debug & FANTASY_DEBUG_CPUFREQ)) {
        printk("%s, %d : try to switch cpu freq to %d \n", __func__, __LINE__, freq);
    }
    __cpufreq_driver_target(p, freq, dbs_tuners_ins.powersave_bias ?
            CPUFREQ_RELATION_L : CPUFREQ_RELATION_H);
}

static void check_freq(struct cpu_dbs_info_s *this_dbs_info)
{
    unsigned int max_load_freq;

    struct cpufreq_policy *policy;
    unsigned int j;

    this_dbs_info->freq_lo = 0;
    policy = this_dbs_info->cur_policy;

    /*
     * Every sampling_rate, we check, if current idle time is less
     * than 20% (default), then we try to increase frequency
     * Every sampling_rate, we look for a the lowest
     * frequency which can sustain the load while keeping idle time over
     * 30%. If such a frequency exist, we try to decrease to this frequency.
     *
     * Any frequency increase takes it to the maximum frequency.
     * Frequency reduction happens at minimum steps of
     * 5% (default) of current frequency
     */

    /* Get Absolute Load - in terms of freq */
    max_load_freq = 0;

    for_each_cpu(j, policy->cpus) {
        struct cpu_dbs_info_s *j_dbs_info;
        cputime64_t cur_wall_time, cur_idle_time, cur_iowait_time;
        unsigned int idle_time, wall_time, iowait_time;
        unsigned int load, load_freq;
        int freq_avg;

        j_dbs_info = &per_cpu(od_cpu_dbs_info, j);

        cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);
        cur_iowait_time = get_cpu_iowait_time(j, &cur_wall_time);

        wall_time = (unsigned int)
            (cur_wall_time - j_dbs_info->prev_cpu_wall);
        j_dbs_info->prev_cpu_wall = cur_wall_time;

        idle_time = (unsigned int)
            (cur_idle_time - j_dbs_info->prev_cpu_idle);
        j_dbs_info->prev_cpu_idle = cur_idle_time;

        iowait_time = (unsigned int)
            (cur_iowait_time - j_dbs_info->prev_cpu_iowait);
        j_dbs_info->prev_cpu_iowait = cur_iowait_time;

        if (dbs_tuners_ins.ignore_nice) {
            u64 cur_nice;
            unsigned long cur_nice_jiffies;

            cur_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE] -
                     j_dbs_info->prev_cpu_nice;
            /*
             * Assumption: nice time between sampling periods will
             * be less than 2^32 jiffies for 32 bit sys
             */
            cur_nice_jiffies = (unsigned long)
                    cputime64_to_jiffies64(cur_nice);

            j_dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];
            idle_time += jiffies_to_usecs(cur_nice_jiffies);
        }

        /*
         * For the purpose of ondemand, waiting for disk IO is an
         * indication that you're performance critical, and not that
         * the system is actually idle. So subtract the iowait time
         * from the cpu idle time.
         */

        if (dbs_tuners_ins.io_is_busy && idle_time >= iowait_time)
            idle_time -= iowait_time;

        if (unlikely(!wall_time || wall_time < idle_time))
            continue;

        load = 100 * (wall_time - idle_time) / wall_time;

        freq_avg = __cpufreq_driver_getavg(policy, j);
        if (freq_avg <= 0)
            freq_avg = policy->cur;

        load_freq = load * freq_avg;
        if (load_freq > max_load_freq)
            max_load_freq = load_freq;
    }

    /* Check for frequency increase */
    if (max_load_freq > dbs_tuners_ins.up_threshold * policy->cur) {
        /* If switching to max speed, apply sampling_down_factor */
        if (policy->cur < policy->max)
            this_dbs_info->rate_mult =
                dbs_tuners_ins.sampling_down_factor;
        dbs_freq_increase(policy, policy->max);
        return;
    }

    /* Check for frequency decrease */
    /* if we cannot reduce the frequency anymore, break out early */
    if (policy->cur == policy->min)
        return;

    /*
     * The optimal frequency is the frequency that is the lowest that
     * can support the current CPU usage without triggering the up
     * policy. To be safe, we focus 10 points under the threshold.
     */
    if (max_load_freq <
        (dbs_tuners_ins.up_threshold - dbs_tuners_ins.down_differential) *
         policy->cur) {
        unsigned int freq_next;
        freq_next = max_load_freq /
                (dbs_tuners_ins.up_threshold -
                 dbs_tuners_ins.down_differential);

        /* No longer fully busy, reset rate_mult */
        this_dbs_info->rate_mult = 1;

        if (freq_next < policy->min)
            freq_next = policy->min;

        if (!dbs_tuners_ins.powersave_bias) {
            unsigned int index = 0;
            cpufreq_frequency_table_target(policy, this_dbs_info->freq_table, freq_next,
                    CPUFREQ_RELATION_L, &index);
            freq_next = this_dbs_info->freq_table[index].frequency;
            if (freq_next != policy->cur){
                if (unlikely(dbs_tuners_ins.dvfs_debug & FANTASY_DEBUG_CPUFREQ)) {
                    printk("%s, %d : try to switch cpu freq to %d \n", __func__, __LINE__, freq_next);
                }
                __cpufreq_driver_target(policy, freq_next, CPUFREQ_RELATION_L);
            }
        } else {
            int freq = powersave_bias_target(policy, freq_next,
                    CPUFREQ_RELATION_L);
            if (unlikely(dbs_tuners_ins.dvfs_debug & FANTASY_DEBUG_CPUFREQ)) {
                printk("%s, %d : try to switch cpu freq to %d \n", __func__, __LINE__, freq);
            }
            __cpufreq_driver_target(policy, freq,
                CPUFREQ_RELATION_L);
        }
    }
}

static void do_dbs_timer(struct work_struct *work)
{
    struct cpu_dbs_info_s *dbs_info =
        container_of(work, struct cpu_dbs_info_s, work.work);
    unsigned int cpu = dbs_info->cpu;
    int sample_type = dbs_info->sample_type;

    int delay;

    mutex_lock(&dbs_info->timer_mutex);

    /* Common NORMAL_SAMPLE setup */
    dbs_info->sample_type = DBS_NORMAL_SAMPLE;
    if (!dbs_tuners_ins.powersave_bias ||
        sample_type == DBS_NORMAL_SAMPLE) {
        
        if (!dbs_tuners_ins.freq_lock){
            check_freq(dbs_info);
        }
        /* Check for CPU hotplug */
        check_hotplug(dbs_info);

        if (dbs_info->freq_lo) {
            /* Setup timer for SUB_SAMPLE */
            dbs_info->sample_type = DBS_SUB_SAMPLE;
            delay = dbs_info->freq_hi_jiffies;
        } else {
            /* We want all CPUs to do sampling nearly on
             * same jiffy
             */
            delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate
                * dbs_info->rate_mult);

            if (num_online_cpus() > 1)
                delay -= jiffies % delay;
        }
    } else {
        if (unlikely(dbs_tuners_ins.dvfs_debug & FANTASY_DEBUG_CPUFREQ)) {
            printk("%s, %d : try to switch cpu freq to %d \n", __func__, __LINE__, dbs_info->freq_lo);
        }
        __cpufreq_driver_target(dbs_info->cur_policy,
            dbs_info->freq_lo, CPUFREQ_RELATION_H);
        delay = dbs_info->freq_lo_jiffies;
    }
    if (dbs_enable > 0){
        queue_delayed_work_on(cpu, dvfs_workqueue, &dbs_info->work, delay);
    }
    mutex_unlock(&dbs_info->timer_mutex);
}

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
    /* We want all CPUs to do sampling nearly on same jiffy */
    int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate * dbs_info->rate_mult);

    if (num_online_cpus() > 1)
        delay -= jiffies % delay;

    dbs_info->sample_type = DBS_NORMAL_SAMPLE;
    INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
    queue_delayed_work_on(dbs_info->cpu, dvfs_workqueue, &dbs_info->work, delay);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
    cancel_delayed_work_sync(&dbs_info->work);
    //cancel_work_sync(&dbs_info->up_work);
    //cancel_work_sync(&dbs_info->down_work);
}


/*
 * cpufreq dbs governor
 */
static struct input_handler dbs_input_handler;
static int cpufreq_governor_dbs(struct cpufreq_policy *policy, unsigned int event)
{
    unsigned int cpu = policy->cpu;
    struct cpu_dbs_info_s *this_dbs_info;
    unsigned int j;

    this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

    switch (event) {
        case CPUFREQ_GOV_START:
        {
            printk(KERN_DEBUG "%s %d\n",__FILE__,__LINE__);
            if ((!cpu_online(cpu)) || (!policy->cur))
                return -EINVAL;


            mutex_lock(&dbs_mutex);

            dbs_enable++;
            for_each_possible_cpu(j) {
                struct cpu_dbs_info_s *j_dbs_info;
                j_dbs_info = &per_cpu(od_cpu_dbs_info, j);
                j_dbs_info->cur_policy = policy;

                j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j, &j_dbs_info->prev_cpu_wall);
            if (dbs_tuners_ins.ignore_nice)
                j_dbs_info->prev_cpu_nice =
                        kcpustat_cpu(j).cpustat[CPUTIME_NICE];
            }
            this_dbs_info->cpu = cpu;
            this_dbs_info->rate_mult = 1;
            ondemand_powersave_bias_init_cpu(cpu);
            this_dbs_info->freq_table = fantasys_freq_tbl;

            /*
             * Start the timerschedule work, when this governor
             * is used for first time
             */
            if (dbs_enable == 1) {
                unsigned int latency;
                /* policy latency is in nS. Convert it to uS first */
                latency = policy->cpuinfo.transition_latency / 1000;
                if (latency == 0)
                    latency = 1;
                /* Bring kernel and HW constraints together */
                min_sampling_rate = max(min_sampling_rate,
                        MIN_LATENCY_MULTIPLIER * latency);
                dbs_tuners_ins.sampling_rate =
                    max(min_sampling_rate,
                        latency * LATENCY_MULTIPLIER);
                dbs_tuners_ins.io_is_busy = 1;
            }
            start_powernow(dbs_tuners_ins.powernow);
            if (!cpu)
                if (input_register_handler(&dbs_input_handler)){
                    printk(KERN_DEBUG "input_register_handler dbs_input_handler failed!\n");
                }
            mutex_unlock(&dbs_mutex);
            printk(KERN_DEBUG "%s %d\n",__FILE__,__LINE__);
            break;
        }

        case CPUFREQ_GOV_STOP:
        {

            printk(KERN_DEBUG "%s %d\n",__FILE__,__LINE__);
            mutex_lock(&dbs_mutex);
            if (!cpu)
                input_unregister_handler(&dbs_input_handler);
            dbs_enable--;
            dbs_timer_exit(this_dbs_info);
            mutex_unlock(&dbs_mutex);
            printk(KERN_DEBUG "%s %d\n",__FILE__,__LINE__);

            break;
        }

        case CPUFREQ_GOV_LIMITS:
        {
            mutex_lock(&this_dbs_info->timer_mutex);

            if (policy->max < this_dbs_info->cur_policy->cur)
                __cpufreq_driver_target(this_dbs_info->cur_policy, policy->max, CPUFREQ_RELATION_H);
            else if (policy->min > this_dbs_info->cur_policy->cur)
                __cpufreq_driver_target(this_dbs_info->cur_policy, policy->min, CPUFREQ_RELATION_L);
            else if (dbs_tuners_ins.freq_lock)
                __cpufreq_driver_target(this_dbs_info->cur_policy, dbs_tuners_ins.freq_lock, CPUFREQ_RELATION_H);

            mutex_unlock(&this_dbs_info->timer_mutex);
            break;
        }

        default:
            break;
    }
    return 0;
}

static int start_powernow(unsigned long mode)
{
    int retval = 0;
    struct cpu_dbs_info_s *this_dbs_info;
    struct cpufreq_policy *policy;

    this_dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    policy = this_dbs_info->cur_policy;

    if (policy == NULL){
        if (mode != SW_POWERNOW_MAXPOWER && mode != SW_POWERNOW_USEREVENT){
            dbs_tuners_ins.powernow = mode;
        }
        return -EINVAL;
    }
    if (policy->governor != &cpufreq_gov_fantasys){
        printk(KERN_DEBUG "scaling cur governor is not fantasys!\n");
        if (mode != SW_POWERNOW_MAXPOWER && mode != SW_POWERNOW_USEREVENT){
            dbs_tuners_ins.powernow = mode;
        }
        return -EINVAL;
    }
    
    /* do not wait in userevent mode*/
    if (mode == SW_POWERNOW_USEREVENT){
        if (dbs_tuners_ins.user_event_freq > policy->cur){
            if (!mutex_trylock(&this_dbs_info->timer_mutex)) {
                printk(KERN_DEBUG "SW_POWERNOW_USEREVENT try to lock mutex failed!\n");
                return -EINVAL;
            }
            if (unlikely(dbs_tuners_ins.dvfs_debug & FANTASY_DEBUG_CPUFREQ)) {
                printk("%s, %d : try to switch cpu freq to %d \n", __func__, __LINE__, dbs_tuners_ins.user_event_freq);
            }
            __cpufreq_driver_target(policy, dbs_tuners_ins.user_event_freq, CPUFREQ_RELATION_H);
            mutex_unlock(&this_dbs_info->timer_mutex);
        }
        this_dbs_info->rate_mult = dbs_tuners_ins.sampling_down_factor;
        return 0;
    }
    /* do not wait in userevent mode*/
    if (mode == SW_POWERNOW_MAXPOWER){
        if (policy->max > policy->cur){
            if (!mutex_trylock(&this_dbs_info->timer_mutex)) {
                printk(KERN_DEBUG "SW_POWERNOW_USEREVENT try to lock mutex failed!\n");
                return -EINVAL;
            }
            if (unlikely(dbs_tuners_ins.dvfs_debug & FANTASY_DEBUG_CPUFREQ)) {
                printk("%s, %d : try to switch cpu freq to %d \n", __func__, __LINE__, policy->max);
            }
            __cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
            mutex_unlock(&this_dbs_info->timer_mutex);
        }
        if (num_online_cpus() == 1){
            queue_work_on(this_dbs_info->cpu, dvfs_workqueue, &this_dbs_info->up_work);
        }
        this_dbs_info->rate_mult = dbs_tuners_ins.sampling_down_factor;
        return 0;
    }
    printk(KERN_DEBUG "start_powernow :%d!\n", (int)mode);
    
    cancel_delayed_work_sync(&this_dbs_info->work);
    
    mutex_lock(&this_dbs_info->timer_mutex);
    switch (mode) {
    case SW_POWERNOW_EXTREMITY:
#if 0
        // plugin offline cpu
        if (num_online_cpus() == 1){
            queue_work_on(this_dbs_info->cpu, dvfs_workqueue, &this_dbs_info->up_work);
        }

        //set max freq
        dbs_tuners_ins.freq_lock = 0;
        policy->max = policy->cpuinfo.max_freq;
        __cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
        dbs_tuners_ins.powernow = mode;
#else
        atomic_set(&g_hotplug_lock, num_possible_cpus());
        apply_hotplug_lock();

        policy->max = policy->cpuinfo.max_freq;
        dbs_tuners_ins.freq_lock = policy->max;
        if (unlikely(dbs_tuners_ins.dvfs_debug & FANTASY_DEBUG_CPUFREQ)) {
            printk("%s, %d : try to switch cpu freq to %d \n", __func__, __LINE__, policy->max);
        }
        __cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
#endif
        break;
                
    case SW_POWERNOW_PERFORMANCE:
        atomic_set(&g_hotplug_lock, 0);
        dbs_tuners_ins.freq_lock = 0;
        policy->max = dbs_tuners_ins.pn_perform_freq;
        break;
        
    case SW_POWERNOW_NORMAL:
        atomic_set(&g_hotplug_lock, 0);
        dbs_tuners_ins.freq_lock = 0;
        policy->max = dbs_tuners_ins.pn_normal_freq;
        break;
        
    case SW_POWERNOW_USB:
        atomic_set(&g_hotplug_lock, 0);
        policy->max = dbs_tuners_ins.pn_perform_freq;
        dbs_tuners_ins.freq_lock = policy->max;
        if (unlikely(dbs_tuners_ins.dvfs_debug & FANTASY_DEBUG_CPUFREQ)) {
            printk("%s, %d : try to switch cpu freq to %d \n", __func__, __LINE__, policy->max);
        }
        __cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
        break;
        
    default:
        printk(KERN_ERR "start_powernow uncare mode:%d!\n", (int)mode);
        atomic_set(&g_hotplug_lock, 0);
        dbs_tuners_ins.freq_lock = 0;
        mode = dbs_tuners_ins.powernow;
        retval = -EINVAL;
        break;
    }
    
#if 0
    if (mode != SW_POWERNOW_EXTREMITY && dbs_enable > 0){
#else
    if (dbs_enable > 0){
#endif
        dbs_timer_init(this_dbs_info);
    }
    dbs_tuners_ins.powernow = mode;
    mutex_unlock(&this_dbs_info->timer_mutex);
    return retval;
}

static int powernow_notifier_call(struct notifier_block *nfb,
                    unsigned long mode, void *cmd)
{
    int retval = NOTIFY_DONE;
    mutex_lock(&dbs_mutex);
    retval = start_powernow(mode);
    mutex_unlock(&dbs_mutex);
    retval = retval ? NOTIFY_DONE:NOTIFY_OK;
    return retval;
}
static struct notifier_block fantasys_powernow_notifier = {
    .notifier_call = powernow_notifier_call,
};

static int fantasys_pm_notify(struct notifier_block *nb, unsigned long event, void *dummy)
{
    struct cpu_dbs_info_s *dbs_info;
    struct cpufreq_policy *policy;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    policy = dbs_info->cur_policy;
    
    if (policy == NULL){
        return -EINVAL;
    }
    if (policy->governor != &cpufreq_gov_fantasys){
        return -EINVAL;
    }

    if (event == PM_SUSPEND_PREPARE) {
        cancel_delayed_work_sync(&dbs_info->work);
        /*disable freq lock to avoid adjust cpufreq before PM_POST_SUSPEND in CPUFREQ_GOV_LIMITS cmd*/
        dbs_tuners_ins.freq_lock = 0;
        cancel_work_sync(&dbs_info->up_work);
        cancel_work_sync(&dbs_info->down_work);
        cpu_down_work(&dbs_info->down_work);
        if (policy->cur != 720000){
            __cpufreq_driver_target(dbs_info->cur_policy, 720000, CPUFREQ_RELATION_H);
        }
    } else if (event == PM_POST_SUSPEND) {
        start_powernow(dbs_tuners_ins.powernow);
    }

    return NOTIFY_OK;
}

static struct notifier_block fantasys_pm_notifier = {
    .notifier_call = fantasys_pm_notify,
    .priority = 0xFF,
};


/*
 * init cpu max frequency from sysconfig;
 * return: 0 - init cpu max/min successed, !0 - init cpu max/min failed;
 */
static int __init_syscfg(void)
{
    script_item_u item;
    script_item_value_type_e type;

    type = script_get_item("dvfs_table", "max_freq", &item);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type || item.val/1000 > POWERNOW_PERFORM_MAX) {
        printk(KERN_ERR "get cpu max frequency from sysconfig failed\n");
        dbs_tuners_ins.pn_perform_freq = POWERNOW_PERFORM_MAX;
    } else {
        dbs_tuners_ins.pn_perform_freq = item.val/1000;
    }
    
    #if 0
    type = script_get_item("dvfs_table", "normal_freq", &item);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type || item.val/1000 > POWERNOW_NORMAL_MAX) {
        printk(KERN_ERR "get cpu normal frequency from sysconfig failed\n");
        dbs_tuners_ins.pn_normal_freq = POWERNOW_NORMAL_MAX;
    } else {
        dbs_tuners_ins.pn_normal_freq = item.val/1000;
    }
    
    if(dbs_tuners_ins.pn_perform_freq < dbs_tuners_ins.pn_normal_freq){
        dbs_tuners_ins.pn_normal_freq = dbs_tuners_ins.pn_perform_freq;
    }
    
    if (dbs_tuners_ins.user_event_freq > dbs_tuners_ins.pn_normal_freq){
        dbs_tuners_ins.user_event_freq = dbs_tuners_ins.pn_normal_freq;
    }
    #endif

    return 0;
}

static void dbs_refresh_callback(struct work_struct *unused)
{
	struct cpufreq_policy *policy;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int cpu = smp_processor_id();

	get_online_cpus();

	if (lock_policy_rwsem_write(cpu) < 0)
		goto bail_acq_sema_failed;

	this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
	policy = this_dbs_info->cur_policy;
	if (!policy) {
		/* CPU not using ondemand governor */
		goto bail_incorrect_governor;
	}

	if (policy->cur < dbs_tuners_ins.user_event_freq) {
		policy->cur = dbs_tuners_ins.user_event_freq;

		__cpufreq_driver_target(policy, dbs_tuners_ins.user_event_freq,
					CPUFREQ_RELATION_L);
		this_dbs_info->prev_cpu_idle = get_cpu_idle_time(cpu,
				&this_dbs_info->prev_cpu_wall);
	}

bail_incorrect_governor:
	unlock_policy_rwsem_write(cpu);

bail_acq_sema_failed:
	put_online_cpus();
	return;
}

static void dbs_input_event(struct input_handle *handle, unsigned int type,
		unsigned int code, int value)
{
	int i;

	for_each_online_cpu(i) {
		queue_work_on(i, input_wq, &per_cpu(dbs_refresh_work, i));
	}
}

static int dbs_input_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

    pr_info("%s: connect to %s\n", __func__, dev->name);
	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "cpufreq";

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void dbs_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id dbs_ids[] = {
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT |
             INPUT_DEVICE_ID_MATCH_ABSBIT,
        .evbit = { BIT_MASK(EV_ABS) },
        .absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
                BIT_MASK(ABS_MT_POSITION_X) |
                BIT_MASK(ABS_MT_POSITION_Y) },
    }, /* multi-touch touchscreen */
    {
        .flags = INPUT_DEVICE_ID_MATCH_KEYBIT |
             INPUT_DEVICE_ID_MATCH_ABSBIT,
        .keybit = { [BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH) },
        .absbit = { [BIT_WORD(ABS_X)] =
                BIT_MASK(ABS_X) | BIT_MASK(ABS_Y) },
    }, /* touchpad */
    { },
};

static struct input_handler dbs_input_handler = {
	.event		= dbs_input_event,
	.connect	= dbs_input_connect,
	.disconnect	= dbs_input_disconnect,
	.name		= "cpufreq_fan",
	.id_table	= dbs_ids,
};


/*
 * cpufreq governor dbs initiate
 */
static int __init cpufreq_gov_dbs_init(void)
{
    int i, ret = -1;
    u64 idle_time;
    int cpu = get_cpu();

    idle_time = get_cpu_idle_time_us(cpu, NULL);
    put_cpu();
    if (idle_time != -1ULL) {
        /* Idle micro accounting is supported. Use finer thresholds */
        dbs_tuners_ins.up_threshold = MICRO_FREQUENCY_UP_THRESHOLD;
        dbs_tuners_ins.down_differential =
                    MICRO_FREQUENCY_DOWN_DIFFERENTIAL;
        /*
         * In nohz/micro accounting case we set the minimum frequency
         * not depending on HZ, but fixed (very low). The deferred
         * timer might skip some samples if idle/sleeping as needed.
        */
        min_sampling_rate = MICRO_FREQUENCY_MIN_SAMPLE_RATE;
    } else {
        /* For correct statistics, we need 10 ticks for each measure */
        min_sampling_rate =
            MIN_SAMPLING_RATE_RATIO * jiffies_to_usecs(10);
    }

    /* create dvfs daemon */
    dvfs_workqueue = create_singlethread_workqueue("fantasys");
    if (!dvfs_workqueue) {
        pr_err("%s cannot create workqueue\n", __func__);
        ret = -ENOMEM;
        goto err_queue;
    }

	input_wq = create_workqueue("iewq");
    if (!input_wq) {
        printk(KERN_ERR "Failed to create iewq workqueue\n");
        goto err_queue1;
    }

    for_each_possible_cpu(i) {
        struct cpu_dbs_info_s *j_dbs_info;
        j_dbs_info = &per_cpu(od_cpu_dbs_info, i);
        INIT_WORK(&j_dbs_info->up_work, cpu_up_work);
        INIT_WORK(&j_dbs_info->down_work, cpu_down_work);
        mutex_init(&j_dbs_info->timer_mutex);
        INIT_WORK(&per_cpu(dbs_refresh_work, i), dbs_refresh_callback);
    }
    
    /* register cpu freq governor */
    ret = cpufreq_register_governor(&cpufreq_gov_fantasys);
    if (ret){
        goto err_reg;
    }

    ret = sysfs_create_group(cpufreq_global_kobject, &dbs_attr_group);
    if (ret) {
        goto err_governor;
    }
    __init_syscfg();
    ret = register_sw_powernow_notifier(&fantasys_powernow_notifier);
    register_pm_notifier(&fantasys_pm_notifier);
    return ret;

err_governor:
    printk("cpufreq_gov_fantasys err\n");
    cpufreq_unregister_governor(&cpufreq_gov_fantasys);
err_reg:
    destroy_workqueue(dvfs_workqueue);
err_queue1:
    destroy_workqueue(input_wq);
err_queue:
    return ret;
}

/*
 * cpufreq governor dbs exit
 */
static void __exit cpufreq_gov_dbs_exit(void)
{
    int i;
    for_each_possible_cpu(i) {
        struct cpu_dbs_info_s *j_dbs_info;
        j_dbs_info = &per_cpu(od_cpu_dbs_info, i);
        cancel_work_sync(&j_dbs_info->up_work);
        cancel_work_sync(&j_dbs_info->down_work);
        cancel_work_sync(&per_cpu(dbs_refresh_work, i));
        cancel_delayed_work_sync(&j_dbs_info->work);
        mutex_destroy(&j_dbs_info->timer_mutex);
    }
    cpufreq_unregister_governor(&cpufreq_gov_fantasys);
    destroy_workqueue(dvfs_workqueue);
	destroy_workqueue(input_wq);
}

MODULE_AUTHOR("xiechr <xiechr@allwinnertech.com>");
MODULE_DESCRIPTION("'cpufreq_fantasys' - A dynamic cpufreq/cpuhotplug governor");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_FANTASY
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
