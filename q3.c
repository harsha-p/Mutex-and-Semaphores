#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <time.h>
#include <errno.h>

int performers, acoustic, electrical, coordinators, t1, t2, waittime, at, et;
sem_t astage, estage, stage;
sem_t givetshirt;
pthread_mutex_t tshirtlock, stagelock;
struct timespec ts;
long long s;
struct performer
{
    int id;
    char name[50];
    char instrument;
    int arrivaltime;
    pthread_t ptid;
    int tshirt;
};

struct stage
{
    int id;
    int type; // 0 for acoustic and 1 for electrical
    pthread_t ptid;
    int status; // 0 uncooupied , 1 musician,2 singer, 3 dual
    int performer;
};

struct coordinator
{
    int id;
    pthread_t ctid;
};

struct performer *pa[400];
struct coordinator *ca[400];
struct stage *st[400];

void timedwait(int type, int ins, struct performer *p)
{
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return;
    ;
    ts.tv_sec += waittime;
    if (type == 0)
        while ((s = sem_timedwait(&astage, &ts)) == -1 && errno == EINTR)
            continue;
    else if (type == 1)
        while ((s = sem_timedwait(&estage, &ts)) == -1 && errno == EINTR)
            continue;
    else if (type == 2)
        while ((s = sem_timedwait(&stage, &ts)) == -1 && errno == EINTR)
            continue;
    if (s == -1)
    {
        if (errno == ETIMEDOUT)
        {
            printf("timed out\n");
        }
        else
            perror("sem_timed_wait");
        pthread_mutex_unlock(&stagelock);
        return;
    }
    else if (s == 0)
    {
        printf("waited\n");
        if (type == 0)
        {
            for (int i = 0; i < acoustic; i++)
            {
                if (st[i]->status == 0)
                {
                    st[i]->performer = p->id;
                    st[i]->status = 1;
                    at -= 1;
                    printf("%s got acoustic\n", p->name);
                    break;
                }
            }
            pthread_mutex_unlock(&stagelock);
        }
        else if (type == 1)
        {
            for (int i = acoustic; i < electrical; i++)
            {
                if (st[i]->status == 0)
                {
                    st[i]->performer = p->id;
                    st[i]->status = 1;
                    et -= 1;
                    printf("%s got electrical\n", p->name);
                    break;
                }
            }
            pthread_mutex_unlock(&stagelock);
        }
        else if (type == 2 && ins != 's')
        {
            for (int i = 0; i < electrical + acoustic; i++)
            {
                if (st[i]->status == 0)
                {
                    st[i]->performer = p->id;
                    st[i]->status = 1;
                    if (i < acoustic)
                    {
                        at -= 1;
                        printf("%s got acoustic\n", p->name);
                    }
                    else
                    {
                        et -= 1;
                        printf("%s got electrical\n", p->name);
                    }
                    break;
                }
            }
            pthread_mutex_unlock(&stagelock);
        }
        else if (type == 2 && ins == 's')
        {
            for (int i = 0; i < electrical + acoustic; i++)
            {
                if (st[i]->status == 0)
                {
                    st[i]->performer = p->id;
                    st[i]->status = 1;
                    if (i < acoustic)
                    {
                        at -= 1;
                        printf("%s got acoustic\n", p->name);
                    }
                    else
                    {
                        et -= 1;
                        printf("%s got electrical\n", p->name);
                    }
                    break;
                }
                if (st[i]->status == 1)
                {
                    st[i]->status = 3;
                    printf("joined\n");
                    break;
                }
            }
            pthread_mutex_unlock(&stagelock);
        }
        //goto ride ----------------------------------------------------
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
    printf("%s %c arrived\n", p->name, p->instrument);
    pthread_mutex_lock(&stagelock);
    if (type == 0) //acoustic only no pool
    {
        if (at)
        {
            for (int i = 0; i < acoustic; i++)
            {
                if (st[i]->status == 0)
                {
                    st[i]->performer = p->id;
                    printf("performer %s %c got acoustic stage %d\n", p->name, p->instrument, st[i]->id);
                    st[i]->status = 1;
                    at -= 1;
                    break;
                }
            }
            pthread_mutex_unlock(&stagelock);
            //goto ride ----------------------------------------------------
        }
    }
    if (type == 1) // electrical only no pool
    {
        if (et)
        {
            for (int i = acoustic; i < electrical + acoustic; i++)
            {
                if (st[i]->status == 0)
                {
                    st[i]->performer = p->id;
                    printf("performer %s %c got electrical stage %d\n", p->name, p->instrument, st[i]->id);
                    st[i]->status = 1;
                    et -= 1;
                    break;
                }
            }
            pthread_mutex_unlock(&stagelock);
            //goto ride ----------------------------------------------------
        }
    }
    if (ins != 's' && type == 2) // acoustic or electrical no pool
    {
        if (et + at)
        {
            for (int i = 0; i < electrical + acoustic; i++)
            {
                if (st[i]->status == 0)
                {
                    st[i]->performer = p->id;
                    st[i]->status = 1;
                    if (i < acoustic)
                    {
                        at -= 1;
                        printf("performer %s %c got acoustic stage %d\n", p->name, p->instrument, st[i]->id);
                    }
                    else
                    {
                        et -= 1;
                        printf("performer %s %c got electrical stage %d\n", p->name, p->instrument, st[i]->id);
                    }
                    break;
                }
            }
            pthread_mutex_unlock(&stagelock);
            //goto ride ----------------------------------------------------
        }
    }
    if (ins == 's') // singer any stage, pool or no pool
    {
        if (et + at)
        {
            for (int i = 0; i < electrical + acoustic; i++)
            {
                if (st[i]->status == 0)
                {
                    st[i]->performer = p->id;
                    if (i < acoustic)
                    {
                        printf("performer %s %c got acoustic stage %d\n", p->name, p->instrument, st[i]->id);
                        at -= 1;
                    }
                    else
                    {
                        printf("performer %s %c got electrical stage %d\n", p->name, p->instrument, st[i]->id);
                        et -= 1;
                    }
                    st[i]->status = 3;
                    break;
                }
                if (st[i]->status == 1)
                {
                    if (i < acoustic)
                    {
                        printf("performer %s %c joined acoustic stage %d\n", p->name, p->instrument, st[i]->id);
                    }
                    else
                    {
                        printf("performer %s %c joined electrical stage %d\n", p->name, p->instrument, st[i]->id);
                    }
                    st[i]->status = 3;
                    break;
                }
            }
            pthread_mutex_unlock(&stagelock);
            //goto ride ----------------------------------------------------
        }
    }
    pthread_mutex_unlock(&stagelock);
    // wait for stage

    timedwait(type, ins, p);
}

void *tshirt(void *inp)
{
    struct coordinator *co = (struct coordinator *)inp;
    while (1)
    {
        sem_wait(&givetshirt);
        pthread_mutex_lock(&tshirtlock);
        int a = -1;
        for (int i = 0; i < performers; i++)
        {
            if (pa[i]->tshirt == 1)
                a = i;
            break;
        }
        pthread_mutex_unlock(&tshirtlock);
        if (a != -1)
        {
            printf("%s receiving tshirt\n", pa[a]->name);
            sleep(2);
        }
        else
            break;
    }
}

int main()
{
    // printf("\nEnter number of performers: ");
    // printf("\nEnter number of singers: ");
    // printf("\nEnter number of musicians: ");
    // printf("\nEnter number of co-ordinators: ");
    // printf("\nEnter number of t1: ");
    // printf("\nEnter number of t2: ");
    // printf("\nEnter number of waittime: ");
    scanf("%d", &performers);
    scanf("%d", &acoustic);
    scanf("%d", &electrical);
    scanf("%d", &coordinators);
    scanf("%d", &t1);
    scanf("%d", &t2);
    scanf("%d", &waittime);
    at = acoustic;
    et = electrical;
    //stage
    for (int i = 0; i < acoustic + electrical; i++)
    {
        st[i] = (struct stage *)malloc(sizeof(struct stage));
        st[i]->id = i;
        st[i]->performer = -1;
        if (i < acoustic)
            st[i]->type = 0;
        else
            st[i]->type = 1;
        st[i]->status = 0;
    }

    // performers
    for (int i = 0; i < performers; i++)
    {
        pa[i] = (struct performer *)malloc(sizeof(struct performer));
        scanf("%s", pa[i]->name);
        scanf(" %c", &pa[i]->instrument);
        scanf("%d", &pa[i]->arrivaltime);
        pa[i]->id = i;
        pa[i]->tshirt = 0;
        pthread_create(&pa[i]->ptid, NULL, getastage, (void *)pa[i]);
    }
    // coordinator give tshirt
    sem_init(&givetshirt, 0, 0);
    for (int i = 0; i < coordinators; i++)
    {
        ca[i] = (struct coordinator *)malloc(sizeof(struct coordinator));
        ca[i]->id = i;
        pthread_create(&(ca[i]->ctid), NULL, tshirt, (void *)ca[i]);
    }

    for (int i = 0; i < performers; i++)
    {
        pthread_join(pa[i]->ptid, NULL);
    }
}