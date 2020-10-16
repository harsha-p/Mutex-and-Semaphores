#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define RED "\x1b[1;31m"
#define GREEN "\x1b[1;32m"
#define YELLOW "\x1b[1;33m"
#define BLUE "\x1b[1;34m"
#define MAGENTA "\x1b[1;35m"
#define CYAN "\x1b[1;36m"
#define RESET "\x1b[0m"

int performers, acoustic, electrical, coordinators, t1, t2, waittime, at, et;
sem_t astage, estage, stage;
sem_t taketshirt, forsinger, joinstage;
pthread_mutex_t tshirtlock, stagelock, singerlock;
pthread_t getasinger;
pthread_t atimer, etimer, jtimer;

long long s;
struct performer
{
    int id;
    int solo;
    char name[50];
    char instrument;
    int arrivaltime;
    int performtime;
    pthread_t ptid;
    long long a[3];
};
struct performer *pa[400];

long long min(long long a, long long b)
{
    if (a != -1 && b != -1)
    {
        if (a < b)
            return a;
        else
            return b;
    }
    else if (a != -1)
        return a;
    else if (b != -1)
        return b;
    else
        return INT_MAX;
}

void *timerfor_joinstage(void *inp)
{
    struct performer *p = (struct performer *)inp;
    struct timespec ts;
    long long l;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return NULL;
    ts.tv_sec += waittime;
    while ((l = sem_timedwait(&joinstage, &ts)) == -1 && errno == EINTR)
        continue;
    if (l == -1)
    {
        if (errno == ETIMEDOUT)
        {
            // printf("timed out\n" RESET);
        }
        else
            perror("sem_timed_wait");
        pthread_mutex_unlock(&stagelock);
        // p->a[0] = -1;
        // printf("lol0\n");
        return NULL;
    }
    clock_gettime(CLOCK_REALTIME, &ts);
    p->a[0] = ts.tv_nsec;
}

void *timerfor_astage(void *inp)
{
    struct performer *p = (struct performer *)inp;
    struct timespec ts;
    long long l;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return NULL;
    ts.tv_sec += waittime;
    while ((l = sem_timedwait(&astage, &ts)) == -1 && errno == EINTR)
        continue;
    if (l == -1)
    {
        if (errno == ETIMEDOUT)
        {
            // printf("timed out\n" RESET);
        }
        else
            perror("sem_timed_wait");
        pthread_mutex_unlock(&stagelock);
        // p->a[1] = -1;
        // printf("lol1\n");
        return NULL;
    }
    clock_gettime(CLOCK_REALTIME, &ts);
    p->a[1] = ts.tv_nsec;
}

void *timerfor_estage(void *inp)
{
    struct performer *p = (struct performer *)inp;
    struct timespec ts;
    long long l;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return NULL;
    ts.tv_sec += waittime;

    while ((l = sem_timedwait(&estage, &ts)) == -1 && errno == EINTR)
        continue;
    if (l == -1)
    {
        if (errno == ETIMEDOUT)
        {
            // printf("timed out\n" RESET);
        }
        else
            perror("sem_timed_wait");
        pthread_mutex_unlock(&stagelock);
        // p->a[2] = -1;
        // printf("lol2\n");
        return NULL;
    }
    clock_gettime(CLOCK_REALTIME, &ts);
    p->a[2] = ts.tv_nsec;
}

long long timed(sem_t s, int wt)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return -1;
    long long a;
    ts.tv_sec += wt;
    while ((a = sem_timedwait(&s, &ts)) == -1 && errno == EINTR)
        continue;
    // printf("%d a\n", a);
    if (a == -1)
    {
        if (errno == ETIMEDOUT)
        {
            printf("timed out\n");
        }
        else
            perror("sem_timed_wait");
        return -1;
    }
    // printf("%d a\n", a);

    clock_gettime(CLOCK_REALTIME, &ts);
    // printf("%d time\n", ts.tv_nsec);
    return ts.tv_nsec;
}

void *checksinger(void *inp)
{
    sem_post(&joinstage);
    struct performer *p = (struct performer *)inp;
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return NULL;
    long long a;
    ts.tv_sec += p->performtime;
    while ((a = sem_timedwait(&forsinger, &ts)) == -1 && errno == EINTR)
        continue;
    if (a == -1)
    {
        if (errno == ETIMEDOUT)
        {
            // printf("timed out\n" RESET);
        }
        else
            perror("sem_timed_wait");
        pthread_mutex_unlock(&stagelock);
    }
    if (a == -1)
    {
        sem_wait(&joinstage);
        // printf("nope:(");
        return 0;
    }
    else
    {
        // printf("ig:'(");
        for (int i = 0; i < performers; i++)
        {
            if (pa[i]->solo == 2 && pa[i]->instrument == 's')
            {
                printf("%d id in selection\n", pa[i]->id);
                pa[i]->solo = 1;
                printf(MAGENTA "%s joined %s's performance,performance extended by 2 secs\n" RESET, pa[i]->name, p->name);
            }
        }
        return NULL;
    }
}

void perform(struct performer *p, int type)
{
    int time = rand() % (t2 - t1 + 1);
    time += t1;
    int a = 0;
    p->performtime = time;
    // printf("%d time\n", time);
    if (p->instrument != 's')
    {
        // pthread_mutex_lock(&singerlock);
        pthread_create(&getasinger, NULL, checksinger, (void *)p);
        // pthread_mutex_unlock(&singerlock);
    }
    if (type == 0)
        printf(BLUE "%s performing %c at acoustic stage for time %d\n" RESET, p->name, p->instrument, time);
    if (type == 1)
        printf(BLUE "%s performing %c at electric stage for time %d\n" RESET, p->name, p->instrument, time);
    sleep(time);
    // printf("break\n"RESET);
    if (a)
        sleep(2);
    if (type == 0)
        printf(BLUE "%s performance at acoustic stage finished\n" RESET, p->name);
    if (type == 1)
        printf(BLUE "%s performance at electric stage finished\n" RESET, p->name);
    if (type == 0)
        sem_post(&astage);
    else if (type == 1)
        sem_post(&estage);
    if (p->instrument != 's')
    {
        sem_wait(&taketshirt);
        printf(GREEN "%s collecting tshirt\n" RESET, p->name);
        sleep(2);
        sem_post(&taketshirt);
    }
}

void *getastage(void *inp)
{
    int type;
    struct performer *p = (struct performer *)inp;
    char ins = p->instrument;
    if (ins == 'v')
        type = 0;
    else if (ins == 'b')
        type = 1;
    else
        type = 2;
    sleep(p->arrivaltime);
    printf(GREEN "%s %c arrived\n" RESET, p->name, p->instrument);
    // pthread_mutex_lock(&stagelock);
    // printf("%d %c type ins\n", type, ins);
    if (type == 2 && ins != 's')
    {
        long long a, b;
        a = timed(astage, waittime);
        b = timed(estage, waittime);
        if (a != -1 && b != -1)
        {
            if (a <= b)
            {
                sem_post(&estage);
                // pthread_mutex_unlock(&stagelock);

                printf(GREEN "%s got acoustic stage\n" RESET, p->name);
                perform(p, 0);
                // pthread_mutex_unlock(&stagelock);
                sem_post(&astage);
            }
            else
            {
                sem_post(&astage);
                printf(GREEN "%s got electric stage\n" RESET, p->name);
                perform(p, 1);
                // pthread_mutex_unlock(&stagelock);
                sem_post(&estage);
            }
        }
        else if (a != -1)
        {
            printf(GREEN "%s got acoustic stage\n" RESET, p->name);
            perform(p, 0);
            // pthread_mutex_unlock(&stagelock);
            sem_post(&astage);
        }
        else if (b != -1)
        {
            printf(GREEN "%s got electric stage\n" RESET, p->name);
            perform(p, 1);
            // pthread_mutex_unlock(&stagelock);
            sem_post(&estage);
        }
        else
        {
            // pthread_mutex_unlock(&stagelock);
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument);
        }
    }
    else if (type == 0)
    {
        long long a = timed(astage, waittime);
        if (a != -1)
        {
            printf(GREEN "%s got acoustic stage\n" RESET, p->name);
            perform(p, 0);
            // pthread_mutex_unlock(&stagelock);
            sem_post(&astage);
        }
        else
        {
            // pthread_mutex_unlock(&stagelock);
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument);
        }
    }
    else if (type == 1)
    {
        long long a = timed(astage, waittime);
        if (a != -1)
        {
            printf(GREEN "%s got electric stage\n" RESET, p->name);
            perform(p, 1);
            // pthread_mutex_unlock(&stagelock);
            sem_post(&astage);
        }
        else
        {
            // pthread_mutex_unlock(&stagelock);
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument);
        }
    }
    else if (ins == 's' && type == 2)
    {
        // long long a = timed(joinstage, waittime);
        // long long b = timed(astage, waittime);
        // long long c = timed(estage, waittime);
        pthread_create(&jtimer, NULL, timerfor_joinstage, (void *)p);
        pthread_create(&atimer, NULL, timerfor_astage, (void *)p);
        pthread_create(&etimer, NULL, timerfor_estage, (void *)p);
        long long a = p->a[0];
        long long b = p->a[1];
        long long c = p->a[2];
        long long mn = -1;
        mn = min(a, min(b, c));
        // printf("\n\n");
        // printf("%d %c id\n", p->id, p->instrument);
        // printf("%d %d %d %d %d a b c mn intmax\n", a, b, c, mn, INT_MAX);
        // printf("%d %d %d eq\n", a == mn, b == mn, c == mn);
        // printf("\n\n");
        if (a == mn)
        {
            // printf("lol1\n");
            p->solo = 2;
            sem_post(&forsinger);
            if (b != -1)
                sem_post(&astage);
            if (c != -1)
                sem_post(&estage);
            // pthread_mutex_unlock(&stagelock);
        }
        else if (b == mn)
        {
            // printf("lol2\n");
            p->solo = 1;
            // if (c != -1)
            //     sem_post(&estage);
            perform(p, 0);
            // pthread_mutex_unlock(&stagelock);
        }
        else if (c == mn)
        {
            // printf("lol3\n");
            p->solo = 1;
            if (b != -1)
                sem_post(&astage);
            perform(p, 1);
            // pthread_mutex_unlock(&stagelock);
        }
        else
        {
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument);
            // pthread_mutex_unlock(&stagelock);
            return NULL;
        }
    }
}

int main()
{
    scanf("%d", &performers);
    scanf("%d", &acoustic);
    scanf("%d", &electrical);
    scanf("%d", &coordinators);
    scanf("%d", &t1);
    scanf("%d", &t2);
    scanf("%d", &waittime);
    // semaphore initialization
    sem_init(&astage, 0, acoustic);
    sem_init(&estage, 0, electrical);
    sem_init(&taketshirt, 0, coordinators);
    sem_init(&forsinger, 0, 0);
    sem_init(&joinstage, 0, 0);
    // lock
    pthread_mutex_init(&singerlock, NULL);
    // performers
    for (int i = 0; i < performers; i++)
    {
        pa[i] = (struct performer *)malloc(sizeof(struct performer));
        scanf("%s", pa[i]->name);
        scanf(" %c", &pa[i]->instrument);
        scanf("%d", &pa[i]->arrivaltime);
        pa[i]->id = i;
        pa[i]->solo = 1;
        pa[i]->a[0] = pa[i]->a[1] = pa[i]->a[2] = -1;
        pthread_create(&pa[i]->ptid, NULL, getastage, (void *)pa[i]);
    }
    for (int i = 0; i < performers; i++)
    {
        pthread_join(pa[i]->ptid, NULL);
    }
    printf(YELLOW "Finished\n" RESET);
}