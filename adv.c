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
sem_t astage, estage, tshirt, forsinger, formusician;
pthread_mutex_t tshirtlock;
pthread_t getasinger, atimer, etimer, jtimer;
struct performer
{
    int id;
    int solo;
    char name[50];
    char instrument;
    int arrivaltime;
    int performtime;
    pthread_t ptid;
    pthread_t performance;
    pthread_t lol;
    long long a[3];
    pthread_t atimer, jtimer, etimer;
};
struct performer *pa[400];

long long min(long long a, long long b)
{
    if (a > 0 && b > 0)
    {
        if (a < b)
            return a;
        else
            return b;
    }
    else if (a > 0)
        return a;
    else if (b > 0)
        return b;
    else
        return INT_MAX;
}

void *timerfor_astage(void *inp) // timer to search for acoustic stage
{
    struct performer *p = (struct performer *)inp;
    struct timespec ts;
    long long l;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return NULL;
    ts.tv_sec += waittime;
    while ((l = sem_timedwait(&astage, &ts)) == -1 && errno == EINTR) // search for acoustic stage
        continue;
    // printf("%d l1\n", l);
    if (l == -1) // not found
    {
        if (errno == ETIMEDOUT)
            ;
        else
            perror("sem_timed_wait");
        p->a[1] = -1;
        return NULL;
    }
    clock_gettime(CLOCK_REALTIME, &ts);
    p->a[1] = ts.tv_nsec;
    // printf("%lld a[0]\n", p->a[0]);
}

void *timerfor_estage(void *inp) // timer to search for electic stage
{
    struct performer *p = (struct performer *)inp;
    struct timespec ts;
    long long l;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return NULL;
    ts.tv_sec += waittime;

    while ((l = sem_timedwait(&estage, &ts)) == -1 && errno == EINTR) // search for an electric stage
        continue;
    // printf("%d l2\n", l);
    if (l == -1) // not found
    {
        if (errno == ETIMEDOUT)
            ;
        else
            perror("sem_timed_wait");
        p->a[2] = -1;
        return NULL;
    }
    clock_gettime(CLOCK_REALTIME, &ts);
    p->a[2] = ts.tv_nsec;
    // printf("%lld a[1]\n", p->a[1]);
}

void *timerfor_musician(void *inp) // timer to search for solo musician
{
    struct performer *p = (struct performer *)inp;
    struct timespec ts;
    long long l;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return NULL;
    ts.tv_sec += waittime;
    while ((l = sem_timedwait(&formusician, &ts)) == -1 && errno == EINTR) // wait for signal from musician
        continue;
    if (l == -1) // musician not found
    {
        if (errno == ETIMEDOUT)
            ;
        else
            perror("sem_timed_wait");
        p->a[0] = -1;
        return NULL;
    }
    clock_gettime(CLOCK_REALTIME, &ts);
    p->a[0] = ts.tv_nsec;
}

void *checksinger(void *inp) // timer to search for a singer
{
    sem_post(&formusician); // send signal to singers
    struct performer *p = (struct performer *)inp;
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return NULL;
    long long a;
    ts.tv_sec += p->performtime;
    while ((a = sem_timedwait(&forsinger, &ts)) == -1 && errno == EINTR) // wait for signal from singer
        continue;
    if (a == -1)
    {
        if (errno == ETIMEDOUT)
            ;
        else
            perror("sem_timed_wait");
    }
    if (a == -1) // singer not found
    {
        sem_wait(&formusician); // claim back signal sent to singers
        return 0;
    }
    else
    {
        for (int i = 0; i < performers; i++) // search for singer who sent singal
        {
            if (pa[i]->solo == 2 && pa[i]->instrument == 's')
            {
                pa[i]->solo = 1;
                printf(MAGENTA "%s joined %s's performance,performance extended by 2 secs\n" RESET, pa[i]->name, p->name);
                // break;
            }
        }
        p->solo = 3;
        return NULL;
    }
}

void perform(struct performer *p, int type) // start performing on stage
{
    int time = rand() % (t2 - t1 + 1);
    time += t1;
    p->performtime = time;
    if (p->instrument != 's')
    {
        pthread_create(&getasinger, NULL, checksinger, (void *)p);
    }
    if (type == 0)
        printf(BLUE "%s performing %c at acoustic stage for time %d\n" RESET, p->name, p->instrument, time);
    if (type == 1)
        printf(BLUE "%s performing %c at electric stage for time %d\n" RESET, p->name, p->instrument, time);
    sleep(time);
    if (p->solo) // if found a singer extend performance by 2 seconds
        sleep(2);
    if (type == 0)
        printf(BLUE "%s performance at acoustic stage finished\n" RESET, p->name);
    if (type == 1)
        printf(BLUE "%s performance at electric stage finished\n" RESET, p->name);
    if (type == 0)
        sem_post(&astage);
    else if (type == 1)
        sem_post(&estage);
    if (p->instrument != 's') // get a tshirt after performance
    {
        sem_wait(&tshirt);
        printf(GREEN "%s collecting tshirt\n" RESET, p->name);
        sleep(2);
        sem_post(&tshirt);
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
    if (type == 2 && ins != 's') // musician looking for acoustic/electrical stage
    {
        pthread_create(&p->atimer, NULL, timerfor_astage, (void *)p); // look for acoustic stage
        pthread_create(&p->etimer, NULL, timerfor_estage, (void *)p); // lok for electric stage
        while (!((p->a[1] > 0 || p->a[2] > 0) || (p->a[1] == -1 && p->a[2] == -1)))
            ;
        sleep(1);
        long long *a = &p->a[1];
        long long *b = &p->a[2];
        printf("%d %d id type\n", p->id, type);
        printf("%lld %lld a b\n", *a, *b);
        if (*a > 0 && *b > 0) // both types of stages are available
        {
            if (*a <= *b)
            {
                sem_post(&estage); // release electric stage
                printf(GREEN "%s got acoustic stage\n" RESET, p->name);
                perform(p, 0);
            }
            else
            {
                sem_post(&astage); //release acoustic stage
                printf(GREEN "%s got electric stage\n" RESET, p->name);
                perform(p, 1);
            }
        }
        else if (*a > 0)
        {
            printf(GREEN "%s got acoustic stage\n" RESET, p->name); // acoustic stage is available
            perform(p, 0);
        }
        else if (*b > 0)
        {
            printf(GREEN "%s got electric stage\n" RESET, p->name); // electric stage is available
            perform(p, 1);
        }
        else
        {
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument); // no stage is available
        }
    }
    else if (type == 0) // musician looking for acoustic stage
    {
        pthread_create(&atimer, NULL, timerfor_astage, (void *)p); // look for acoustic stage
        while (p->a[1] == 0)
            ;
        long long a = p->a[1];
        if (a != -1)
        {
            printf(GREEN "%s got acoustic stage\n" RESET, p->name);
            perform(p, 0);
        }
        else
        {
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument); // acostic stage not available
        }
    }
    else if (type == 1) // musician looking for electric stage
    {
        pthread_create(&p->atimer, NULL, timerfor_estage, (void *)p); // look for electric stage
        while (p->a[2] == 0)
            ;
        long long b = p->a[2];
        if (b > 0)
        {
            printf(GREEN "%s got electric stage\n" RESET, p->name);
            perform(p, 1);
        }
        else
        {
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument); // electric stage not available
        }
    }
    else if (ins == 's' && type == 2) // singer looking for a stage
    {
        pthread_create(&p->jtimer, NULL, timerfor_musician, (void *)p); //  looking to join a musician
        pthread_create(&p->atimer, NULL, timerfor_astage, (void *)p);   //  looking for acoustic stage
        pthread_create(&p->etimer, NULL, timerfor_estage, (void *)p);   //  looking for electric stage
        while (!(p->a[0] > 0 || p->a[1] > 0 || p->a[2] > 0 || ((p->a[0] == -1) && (p->a[1] == -1) && (p->a[2] == -1))))
            ;
        long long *a = &p->a[0];
        long long *b = &p->a[1];
        long long *c = &p->a[2];
        long long mn = -1;
        mn = min(*a, min(*b, *c));
        printf("%d %d id type\n", p->id, type);
        printf("%d %d %d %d a b c mn\n", *a, *b, *c, mn);
        // printf("%d %d %d %d %d a b c mn \n", a, b, c, mn);
        if (*a == mn)
        {
            p->solo = 2;
            sem_post(&forsinger); // signal to musician that singer is available
            if (*b > 0)
                sem_post(&astage); // release acoustic stage
            if (*c > 0)
                sem_post(&estage); // release electic stage
        }
        else if (*b == mn)
        {
            p->solo = 1;
            if (*c > 0)
                sem_post(&estage); // release electric stage
            perform(p, 0);
        }
        else if (*c == mn)
        {
            p->solo = 1;
            if (*b > 0)
                sem_post(&astage); // release acoustic stage
            perform(p, 1);
        }
        else
        {
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument); // no stage available to perform solo or to join
            return NULL;
        }
    }
}

void *fresh(void *inp)
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
    if (type == 2 && ins != 's') // musician looking for acoustic/electrical stage
    {
        int a = sem_trywait(&astage);
        if (a != -1)
            perform(p, 0);
        else if (a == -1)
        {
            a = sem_trywait(&estage);
            if (a != -1)
                perform(p, 1);
            else
            {
                pthread_create(&p->atimer, NULL, timerfor_astage, (void *)p); // look for acoustic stage
                pthread_create(&p->etimer, NULL, timerfor_estage, (void *)p); // lok for electric stage
                while (!((p->a[1] > 0 || p->a[2] > 0) || (p->a[1] == -1 && p->a[2] == -1)))
                    ;
                sleep(1);
                long long *a = &p->a[1];
                long long *b = &p->a[2];
                printf("%d %d id type\n", p->id, type);
                printf("%lld %lld a b\n", *a, *b);
                if (*a > 0 && *b > 0) // both types of stages are available
                {
                    if (*a <= *b)
                    {
                        sem_post(&estage); // release electric stage
                        printf(GREEN "%s got acoustic stage\n" RESET, p->name);
                        perform(p, 0);
                    }
                    else
                    {
                        sem_post(&astage); //release acoustic stage
                        printf(GREEN "%s got electric stage\n" RESET, p->name);
                        perform(p, 1);
                    }
                }
                else if (*a > 0)
                {
                    printf(GREEN "%s got acoustic stage\n" RESET, p->name); // acoustic stage is available
                    perform(p, 0);
                }
                else if (*b > 0)
                {
                    printf(GREEN "%s got electric stage\n" RESET, p->name); // electric stage is available
                    perform(p, 1);
                }
                else
                {
                    printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument); // no stage is available
                }
            }
        }
    }
    else if (type == 0) // musician looking for acoustic stage
    {
        int a = sem_trywait(&astage);
        if (a != -1)
            perform(p, 0);
        else
        {
            pthread_create(&atimer, NULL, timerfor_astage, (void *)p); // look for acoustic stage
            while (p->a[1] == 0)
                ;
            long long a = p->a[1];
            if (a != -1)
            {
                printf(GREEN "%s got acoustic stage\n" RESET, p->name);
                perform(p, 0);
            }
            else
            {
                printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument); // acostic stage not available
            }
        }
    }
    else if (type == 1) // musician looking for electric stage
    {
        int a = sem_trywait(&estage);
        if (a != -1)
            perform(p, 1);
        else
        {
            pthread_create(&p->atimer, NULL, timerfor_estage, (void *)p); // look for electric stage
            while (p->a[2] == 0)
                ;
            long long b = p->a[2];
            if (b > 0)
            {
                printf(GREEN "%s got electric stage\n" RESET, p->name);
                perform(p, 1);
            }
            else
            {
                printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument); // electric stage not available
            }
        }
    }
    else if (ins == 's' && type == 2) // singer looking for a stage
    {
        int a = sem_trywait(&astage);
        if (a != -1)
            perform(p, 0);
        else if (a == -1)
        {
            a = sem_trywait(&estage);
            if (a != -1)
                perform(p, 1);
            else
            {
                pthread_create(&p->jtimer, NULL, timerfor_musician, (void *)p); //  looking to join a musician
                pthread_create(&p->atimer, NULL, timerfor_astage, (void *)p);   //  looking for acoustic stage
                pthread_create(&p->etimer, NULL, timerfor_estage, (void *)p);   //  looking for electric stage
                while (!(p->a[0] > 0 || p->a[1] > 0 || p->a[2] > 0 || ((p->a[0] == -1) && (p->a[1] == -1) && (p->a[2] == -1))))
                    ;
                long long *a = &p->a[0];
                long long *b = &p->a[1];
                long long *c = &p->a[2];
                long long mn = -1;
                mn = min(*a, min(*b, *c));
                printf("%d %d id type\n", p->id, type);
                printf("%d %d %d %d a b c mn\n", *a, *b, *c, mn);
                // printf("%d %d %d %d %d a b c mn \n", a, b, c, mn);
                if (*a == mn)
                {
                    p->solo = 2;
                    sem_post(&forsinger); // signal to musician that singer is available
                    if (*b > 0)
                        sem_post(&astage); // release acoustic stage
                    if (*c > 0)
                        sem_post(&estage); // release electic stage
                }
                else if (*b == mn)
                {
                    p->solo = 1;
                    if (*c > 0)
                        sem_post(&estage); // release electric stage
                    perform(p, 0);
                }
                else if (*c == mn)
                {
                    p->solo = 1;
                    if (*b > 0)
                        sem_post(&astage); // release acoustic stage
                    perform(p, 1);
                }
                else
                {
                    printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument); // no stage available to perform solo or to join
                    return NULL;
                }
            }
        }
    }
    else
    {
        printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument); // no stage available to perform solo or to join
        return NULL;
    }
}

int main()
{
    srand(time(0));
    scanf("%d", &performers);
    scanf("%d", &acoustic);
    scanf("%d", &electrical);
    scanf("%d", &coordinators);
    scanf("%d", &t1);
    scanf("%d", &t2);
    scanf("%d", &waittime);
    // semaphore initialization
    sem_init(&astage, 0, acoustic);     // number of acoustic stages
    sem_init(&estage, 0, electrical);   // number of electric stages
    sem_init(&tshirt, 0, coordinators); // number of coordinators
    sem_init(&forsinger, 0, 0);         // musician searching for a singer to join
    sem_init(&formusician, 0, 0);       // singer searching for a musician to join
    // performers
    for (int i = 0; i < performers; i++)
    {
        pa[i] = (struct performer *)malloc(sizeof(struct performer));
        scanf("%s", pa[i]->name);
        scanf(" %c", &pa[i]->instrument);
        scanf("%d", &pa[i]->arrivaltime);
        pa[i]->id = i;
        pa[i]->solo = 1;
        pa[i]->a[0] = pa[i]->a[1] = pa[i]->a[2] = 0;
        // pthread_create(&pa[i]->ptid, NULL, getastage, (void *)pa[i]);
        pthread_create(&pa[i]->ptid, NULL, fresh, (void *)pa[i]);
    }
    for (int i = 0; i < performers; i++)
    {
        // printf(GREEN "%s %c left\n" RESET, pa[i]->name, pa[i]->instrument);
        pthread_join(pa[i]->ptid, NULL);
    }
    printf(YELLOW "Finished\n" RESET);
}