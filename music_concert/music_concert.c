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
#define MSIZE 1000

int performers, acoustic, electric, coordinators, t1, t2, waittime;
sem_t astage, estage, tshirt, forsinger, formusician;
pthread_mutex_t lock;
struct performer
{
    int id, arrivaltime, performtime, stage, singer;
    char name[50];
    char instrument;
    pthread_t ptid;
};
struct performer *pa[MSIZE];

void *taketsirt(void *inp) // take tshirt
{
    struct performer *p = (struct performer *)inp;
    sem_wait(&tshirt);
    printf(GREEN "%s receiving tshirt\n", p->name);
    sleep(2);
    sem_post(&tshirt);
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
    a = sem_timedwait(&forsinger, &ts); // set timer
    if (a == -1)
    {
        if (errno != ETIMEDOUT)
            perror("sem_timed_wait");
    }
    if (a == -1) // singer not found
    {
        sem_wait(&formusician); // claim back signal sent to singers
    }
    else
    {
        pthread_mutex_lock(&lock);
        for (int i = 0; i < performers; i++) // search for singer who sent singal
        {
            if (pa[i]->stage == 2 && pa[i]->instrument == 's')
            {
                pa[i]->stage = 3; // collaborating
                printf(MAGENTA "%s joined %s's performance,performance extended by 2 secs\n" RESET, pa[i]->name, p->name);
                p->singer = i; // collaborating
                break;
            }
        }
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void perform(struct performer *p, int type) // start performing on stage
{
    int time = rand() % (t2 - t1 + 1);
    time += t1;
    p->performtime = time;
    if (p->instrument != 's')
    {
        pthread_t singer;
        pthread_create(&singer, NULL, checksinger, (void *)p);
    }
    if (type == 0)
        printf(YELLOW "%s performing %c at acoustic stage for time %d\n" RESET, p->name, p->instrument, time);
    else if (type == 1)
        printf(BLUE "%s performing %c at electric stage for time %d\n" RESET, p->name, p->instrument, time);
    sleep(time);
    if (p->singer >= 0) // if found a singer extend performance by 2 seconds
        sleep(2);
    if (type == 0)
        printf(YELLOW "%s performance at acoustic stage finished\n" RESET, p->name);
    else if (type == 1)
        printf(BLUE "%s performance at electric stage finished\n" RESET, p->name);
    if (type == 0)
        sem_post(&astage);
    else if (type == 1)
        sem_post(&estage);
    pthread_t tshirt;
    pthread_create(&tshirt, NULL, taketsirt, (void *)p);
    if (p->singer >= 0)
    {
        pthread_t anothertshirt;
        pthread_create(&anothertshirt, NULL, taketsirt, (void *)pa[p->singer]); // tshirt for singer
        pthread_join(tshirt, NULL);                                             // tsirt for musician/singer
        pthread_join(anothertshirt, NULL);
    }
    else
        pthread_join(tshirt, NULL); // tsirt for musician/singer
}

void *timerfor_astage(void *inp) // timer to search for acoustic stage
{
    struct performer *p = (struct performer *)inp;
    struct timespec ts;
    long long l;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return NULL;
    ts.tv_sec += waittime;
    l = sem_timedwait(&astage, &ts);
    if (l == -1) // not found
    {
        if (errno != ETIMEDOUT)
            perror("sem_timed_wait");
        return NULL;
    }
    if (p->stage == -1)
    {
        p->stage = 0;
        perform(p, 0); // perform at acoustic stage
    }
    else
    {
        sem_post(&astage); // Already performing so leave the received stage
    }
}

void *timerfor_estage(void *inp) // timer to search for electic stage
{
    struct performer *p = (struct performer *)inp;
    struct timespec ts;
    long long l;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return NULL;
    ts.tv_sec += waittime;
    l = sem_timedwait(&estage, &ts);
    if (l == -1) // not found
    {
        if (errno != ETIMEDOUT)
            perror("sem_timed_wait");
        return NULL;
    }
    // printf("in electric timer\n");
    if (p->stage == -1)
    {
        // printf("set electric\n");
        p->stage = 1;
        perform(p, 1);
    }
    else
    {
        sem_post(&estage);
    }
}

void *timerfor_musician(void *inp) // timer to search for  musician performing solo
{
    struct performer *p = (struct performer *)inp;
    struct timespec ts;
    long long l;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return NULL;
    ts.tv_sec += waittime;
    l = sem_timedwait(&formusician, &ts);
    if (l == -1) // musician not found
    {
        if (errno != ETIMEDOUT)
            perror("sem_timed_wait");
        return NULL;
    }
    // printf("in singer\n");
    if (p->stage == -1)
    {
        // printf("set singer\n");
        p->stage = 2;
        sem_post(&forsinger);
    }
    else
    {
        sem_post(&formusician);
    }
}

void *getastage(void *inp)
{
    int type;
    struct performer *p = (struct performer *)inp;
    char ins = p->instrument;
    if (ins == 'v')
        type = 0; // acoustic
    else if (ins == 'b')
        type = 1; // electric
    else
        type = 2; // both
    sleep(p->arrivaltime);
    printf(GREEN "%s %c arrived\n" RESET, p->name, p->instrument);
    if (type == 0) // musician looking for acoustic stage
    {
        pthread_t atimer;
        pthread_create(&atimer, NULL, timerfor_astage, (void *)p); // look for acoustic stage
        pthread_join(atimer, NULL);
        if (p->stage == -1)
        {
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument); // acostic stage not available
        }
    }
    else if (type == 1) // musician looking for electric stage
    {
        pthread_t etimer;
        pthread_create(&etimer, NULL, timerfor_estage, (void *)p); // look for electric stage
        pthread_join(etimer, NULL);
        if (p->stage == -1)
        {
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument); // electric stage not available
        }
    }
    else if (type == 2 && ins != 's') // musician looking for acoustic/electrical stage
    {
        pthread_t atimer, etimer;
        pthread_create(&atimer, NULL, timerfor_astage, (void *)p); // look for acoustic stage
        pthread_create(&etimer, NULL, timerfor_estage, (void *)p); // lok for electric stage
        pthread_join(atimer, NULL);
        pthread_join(etimer, NULL);
        if (p->stage == -1)
        {
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument); // no stage is available
        }
    }
    else if (ins == 's' && type == 2) // singer looking for a stage
    {
        pthread_t jtimer, atimer, etimer;
        pthread_create(&jtimer, NULL, timerfor_musician, (void *)p); //  looking to join a musician
        pthread_create(&atimer, NULL, timerfor_astage, (void *)p);   //  looking for acoustic stage
        pthread_create(&etimer, NULL, timerfor_estage, (void *)p);   //  looking for electric stage
        pthread_join(jtimer, NULL);
        pthread_join(atimer, NULL);
        pthread_join(etimer, NULL);
        if (p->stage == -1)
        {
            printf(RED "%s %c left because of impatience\n" RESET, p->name, p->instrument); // no stage available to perform solo or to join
            return NULL;
        }
    }
}

int main()
{
    srand(time(0));
    pthread_mutex_init(&lock, NULL);
    scanf("%d%d%d%d%d%d%d", &performers, &acoustic, &electric, &coordinators, &t1, &t2, &waittime);
    sem_init(&astage, 0, acoustic);     // number of acoustic stages
    sem_init(&estage, 0, electric);     // number of electric stages
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
        pa[i]->singer = -1;
        pa[i]->stage = -1;
        pthread_create(&pa[i]->ptid, NULL, getastage, (void *)pa[i]);
    }
    for (int i = 0; i < performers; i++)
    {
        pthread_join(pa[i]->ptid, NULL);
    }
    printf(YELLOW "Finished\n" RESET);
    sem_destroy(&astage);
    sem_destroy(&estage);
    sem_destroy(&tshirt);
    sem_destroy(&forsinger);
    sem_destroy(&formusician);
}