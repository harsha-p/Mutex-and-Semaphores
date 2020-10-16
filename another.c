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
sem_t taketshirt, forsinger, forpool;
pthread_mutex_t tshirtlock, stagelock;
pthread_t checksinger, timeout;

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

long long timed(sem_t s, int wt)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return -1;
    long long a;
    ts.tv_sec += wt;
    while ((a = sem_timedwait(&s, &ts)) == -1 && errno == EINTR)
        continue;
    // printf("%d wt\n", wt);
    if (a == -1)
    {
        if (errno == ETIMEDOUT)
        {
            printf("timed out\n" RESET);
        }
        else
            perror("sem_timed_wait");
        pthread_mutex_unlock(&stagelock);
        return -1;
    }
    // printf("%d a\n", a);

    clock_gettime(CLOCK_REALTIME, &ts);
    // printf("%d time\n", ts.tv_nsec);
    return ts.tv_nsec;
}

void *checkfortime(void *inp)
{
    sem_post(&forpool);
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
        sem_wait(&forpool);
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
        pthread_create(&timeout, NULL, checkfortime, (void *)p);
    }
    if (type == 0)
        printf(BLUE "%s performing %c at acoustic stage for time %d\n" RESET, p->name, p->instrument, time);
    if (type == 1)
        printf(BLUE "%s performing %c at electric stage for time %d\n" RESET, p->name, p->instrument, time);
    sleep(time);
    // printf("break\n"RESET);
    if (a)
        sleep(2);
    if (type = 0)
        printf(BLUE "%s performance at acoustic stage finished\n" RESET, p->name);
    if (type = 1)
        printf(BLUE "%s performance at electric stage finished\n" RESET, p->name);
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
    pthread_mutex_lock(&stagelock);
    pthread_mutex_unlock(&stagelock);
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
                printf(GREEN "%s got acoustic stage\n" RESET, p->name);
                pthread_mutex_unlock(&stagelock);
                perform(p, 0);
                sem_post(&astage);
            }
            else
            {
                sem_post(&astage);
                printf(GREEN "%s got electric stage\n" RESET, p->name);
                pthread_mutex_unlock(&stagelock);
                perform(p, 1);
                sem_post(&estage);
            }
        }
        else if (a != -1)
        {
            printf(GREEN "%s got acoustic stage\n" RESET, p->name);
            pthread_mutex_unlock(&stagelock);
            perform(p, 0);
            sem_post(&astage);
        }
        else if (b != -1)
        {
            printf(GREEN "%s got electric stage\n" RESET, p->name);
            pthread_mutex_unlock(&stagelock);
            perform(p, 1);
            sem_post(&estage);
        }
        else
        {
            pthread_mutex_unlock(&stagelock);
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument);
        }
    }
    else if (type == 0)
    {
        long long a = timed(astage, waittime);
        if (a != -1)
        {
            printf(GREEN "%s got acoustic stage\n" RESET, p->name);
            pthread_mutex_unlock(&stagelock);
            perform(p, 0);
            sem_post(&astage);
        }
        else
        {
            pthread_mutex_unlock(&stagelock);
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument);
        }
    }
    else if (type == 1)
    {
        long long a = timed(astage, waittime);
        if (a != -1)
        {
            printf(GREEN "%s got electric stage\n" RESET, p->name);
            pthread_mutex_unlock(&stagelock);
            perform(p, 1);
            sem_post(&astage);
        }
        else
        {
            pthread_mutex_unlock(&stagelock);
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument);
        }
    }
    else if (ins == 's')
    {
        long long a = timed(forpool, waittime);
        long long b = timed(astage, waittime);
        long long c = timed(estage, waittime);
        long long mn = -1;
        mn = min(a, min(b, c));
        // printf("%d %d %d %d %d a b c mn intmax\n", a, b, c, mn, INT_MAX);
        // printf("%d %d %d eq\n", a == mn, b == mn, c == mn);
        if (a == mn)
        {
            p->solo = 2;
            sem_post(&forsinger);
            if (b != -1)
                sem_post(&astage);
            if (c != -1)
                sem_post(&estage);
            pthread_mutex_unlock(&stagelock);
        }
        else if (b == mn)
        {
            p->solo = 1;
            if (c != -1)
                sem_post(&estage);
            pthread_mutex_unlock(&stagelock);
            perform(p, 0);
        }
        else if (c == mn)
        {
            p->solo = 1;
            if (b != -1)
                sem_post(&astage);
            pthread_mutex_unlock(&stagelock);
            perform(p, 1);
        }
        else
        {
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument);
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
    sem_init(&astage, 0, acoustic);
    sem_init(&estage, 0, electrical);
    sem_init(&taketshirt, 0, coordinators);
    sem_init(&forsinger, 0, 0);
    sem_init(&forpool, 0, 0);
    // performers
    for (int i = 0; i < performers; i++)
    {
        pa[i] = (struct performer *)malloc(sizeof(struct performer));
        scanf("%s", pa[i]->name);
        scanf(" %c", &pa[i]->instrument);
        scanf("%d", &pa[i]->arrivaltime);
        pa[i]->id = i;
        pa[i]->solo = 1;
        pthread_create(&pa[i]->ptid, NULL, getastage, (void *)pa[i]);
    }
    for (int i = 0; i < performers; i++)
    {
        pthread_join(pa[i]->ptid, NULL);
    }
    printf(YELLOW "Finished\n" RESET);
}