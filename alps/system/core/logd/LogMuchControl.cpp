#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/capability.h>
#include <sys/klog.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include <cstdbool>
#include <memory>

#include <android-base/macros.h>
#include <cutils/android_get_control_file.h>
#include <cutils/properties.h>
#include <cutils/sched_policy.h>
#include <cutils/sockets.h>
#include <log/event_tag_map.h>
#include <packagelistparser/packagelistparser.h>
#include <private/android_filesystem_config.h>
#include <private/android_logger.h>
#include <utils/threads.h>

#include "CommandListener.h"
#include "LogAudit.h"
#include "LogBuffer.h"
#include "LogKlog.h"
#include "LogListener.h"
#include "LogUtils.h"

#ifdef MTK_LOGD_ENHANCE
#define INTERVAL      5LL
#define MAX_COUNT     25
void prdebug_ratelimit(const char* fmt, ...) {
    static int cnt = 0;
    static int miss_cnt = 0;
    static struct timespec ts_0 = {0, 0};
    struct timespec ts_1;
    int64_t differ = 0;

    if (cnt == 0) {
        clock_gettime(CLOCK_MONOTONIC, &ts_0);
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_1);

    differ = ts_1.tv_sec - ts_0.tv_sec;

    // if more than MAX_COUNT entry msg during  (INTERVAL - 1, INTERVAL + 1), then drop
    if (cnt >= MAX_COUNT && differ < INTERVAL) {
        miss_cnt++;
        return;
    }

    char buffer[192];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    if (n > 0) {
        buffer[sizeof(buffer) - 1] = '\0';

        if (cnt < MAX_COUNT) {
            android::prdebug("%s", buffer);
            cnt++;
        } else if(differ >= INTERVAL) {
            // write dropped statistic info
            if (miss_cnt != 0)
                android::prdebug("dropped %d its own logs by limitation.\n", miss_cnt);

            // write normal log
            android::prdebug("%s", buffer);
            clock_gettime(CLOCK_MONOTONIC, &ts_0); // update ts_0
            cnt  = 1;     // cnt = 1
            miss_cnt = 0; // reset to Zero
        }
    }
}

#if defined(MTK_LOGD_FILTER)
int log_reader_count = 0;
void logd_reader_del(void) {
    char property[PROPERTY_VALUE_MAX];

    if (log_reader_count == 1) {
        property_get("persist.log.tag", property, "I");
        property_set("log.tag", property);
        prdebug_ratelimit("logd no log reader, set log level to %s!\n", property);
    }
    log_reader_count--;
}

void logd_reader_add(void) {
    char property[PROPERTY_VALUE_MAX];

    if (log_reader_count == 0) {
        property_get("persist.log.tag", property, "M");
        property_set("log.tag", property);
        prdebug_ratelimit("logd first log reader, set log level to %s!\n", property);
    }
    log_reader_count++;
}

#endif

#if defined(HAVE_AEE_FEATURE) && defined(ANDROID_LOG_MUCH_COUNT)

sem_t logmuch_sem;

int log_detect_value;
int log_much_delay_detect = 0;      // log much detect pause, may use double detect value
int build_type;     // eng:0, userdebug:1 user:2
int detect_time = 1;

// adjust logmuch feature
void* logmuch_adjust_thread_start(void* /*obj*/) {
    prctl(PR_SET_NAME, "logd.logmuch.D");

    set_sched_policy(0, SP_FOREGROUND);
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_FOREGROUND);

    cap_t caps = cap_init();
    (void)cap_clear(caps);
    (void)cap_set_proc(caps);
    (void)cap_free(caps);

    // If we are AID_ROOT, we should drop to AID_LOGD+AID_SYSTEM, if we are
    // anything else, we have even lesser privileges and accept our fate. Not
    // worth checking for error returns setting this thread's privileges.
    (void)setgid(AID_SYSTEM); // readonly access to /data/system/packages.list
    (void)setuid(AID_LOGD);   // access to everything logd, eg /data/misc/logd


    char property[PROPERTY_VALUE_MAX];
    bool value;
    int count;
    int delay;

    while (!sem_wait(&logmuch_sem)) {
        android::prdebug("logmuch adjust thread wakeup");

        property_get("ro.aee.build.info", property, "");
        value = !strcmp(property, "mtk");
        if (value != true) {
            log_detect_value = 0;
            continue;
        }

        value = property_get_bool("persist.logmuch.detect", true);
        if (value == true) {
            property_get("ro.build.type", property, "");
            if (!strcmp(property, "eng")) {
                build_type = 0;
            } else if (!strcmp(property, "userdebug")) {
                build_type = 1;
            } else {
                build_type = 2;
            }

            if (log_detect_value == 0) {
                log_detect_value = ANDROID_LOG_MUCH_COUNT;
            }

            property_get("logmuch.detect.value", property, "-1");
            count = atoi(property);
            if (count == 0) {
                count = ANDROID_LOG_MUCH_COUNT;
            }
            android::prdebug("logmuch detect, build type %d, detect value %d:%d.\n",
                build_type, count, log_detect_value);

            if (count > 0 && count != log_detect_value) {  // set new log level
                log_detect_value = count;
                log_much_delay_detect = 1;
            }
            detect_time = (log_detect_value > 1000) ? 1 : 6;

            property_get("logmuch.detect.delay", property, "");
            delay = atoi(property);

            if (delay > 0) {
                log_much_delay_detect = 3*60;
                property_set("logmuch.detect.delay", "0");
            }
        } else {
            log_detect_value = 0;
            android::prdebug("logmuch detect disable.");
        }

    }

    return nullptr;
}

void trigger_logmuch_adjust() {
    sem_post(&logmuch_sem);
}

// logmuch need to be adjusted.
int logmuch_adjust() {
    cap_t caps = cap_init();
    (void)cap_clear(caps);
    (void)cap_set_proc(caps);
    (void)cap_free(caps);

    int sock = TEMP_FAILURE_RETRY(socket_local_client(
        "logd", ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM));
    if (sock < 0) return -errno;

    static const char logmuchStr[] = "logmuch";
    ssize_t ret = TEMP_FAILURE_RETRY(write(sock, logmuchStr, sizeof(logmuchStr)));
    if (ret < 0) return -errno;

    struct pollfd p;
    memset(&p, 0, sizeof(p));
    p.fd = sock;
    p.events = POLLIN;
    ret = TEMP_FAILURE_RETRY(poll(&p, 1, 1000));
    if (ret < 0) return -errno;
    if ((ret == 0) || !(p.revents & POLLIN)) return -ETIME;

    static const char success[] = "success";
    char buffer[sizeof(success) - 1];
    memset(buffer, 0, sizeof(buffer));
    ret = TEMP_FAILURE_RETRY(read(sock, buffer, sizeof(buffer)));
    if (ret < 0) return -errno;

    return strncmp(buffer, success, sizeof(success) - 1) != 0;
}
#endif

#if defined(LOGD_FORCE_DIRECTCOREDUMP)
#define SIGNUM 7

// need sync with vendor\mediatek\proprietary\external\aee\direct-coredump\direct-coredump.c
void directcoredump_init() {
    int sigtype[SIGNUM] = {SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGSEGV, SIGTRAP, SIGSYS};
    char value[PROPERTY_VALUE_MAX] = {'\0'};

    // eng&userdebug load direct-coredump default enable
    // user load direct-coredump default disable due to libdirect-coredump.so will not be preloaded
    property_get("persist.aee.core.direct", value, "default");
    if (strncmp(value, "disable", sizeof("disable"))) {
        int loop;
        for (loop = 0; loop < SIGNUM; loop++) {
            signal(sigtype[loop], SIG_DFL);
        }
    }
    android::prdebug("init directcoredump done!\n");
}
#endif
#endif

