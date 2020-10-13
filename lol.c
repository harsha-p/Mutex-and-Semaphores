#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define ANSI_COLOR_RED "\x1b[1;31m"
#define ANSI_COLOR_GREEN "\x1b[1;32m"
#define ANSI_COLOR_YELLOW "\x1b[1;33m"
#define ANSI_COLOR_BLUE "\x1b[1;34m"
#define ANSI_COLOR_MAGENTA "\x1b[1;35m"
#define ANSI_COLOR_CYAN "\x1b[1;36m"
#define ANSI_COLOR_RESET "\x1b[0m"

struct chef
{
    int vesselrem;
    int chefid;
    pthread_t thid;
    pthread_mutex_t cheflock;
    pthread_cond_t chefcond;
    int waittime;
    int vesselcap;
};

int rcheffno;
int tableno;
int stdno;

struct table
{
    int containers;
    int slots;
    pthread_t tabthid;
    int slotsrem;
    pthread_mutex_t tablelock;
    int tableid;
    pthread_cond_t tablecond;
};

struct chef *cheffarr[400];

void biryani_ready(struct chef *currchef)
{
    while (1)
    {
        if (currchef->vesselrem <= 0)
        {
            printf(ANSI_COLOR_RED "All the vessels prepared by Robot Chef %d are emptied. Resuming cooking now. \n" ANSI_COLOR_RESET, currchef->chefid);
            break;
        }
        else
        {
            pthread_cond_wait(&currchef->chefcond, &currchef->cheflock);
        }
    }
    pthread_mutex_unlock(&currchef->cheflock);
}

void *chef(void *inp)
{
    struct chef *me = (struct chef *)inp;
    printf("chef %d arrived \n", me->chefid);
    while (1)
    {
        int w = 2 + rand() % 4;
        int r = 1 + rand() % 5;
        int p = 25 + rand() % 25;
        me->waittime = w;
        sleep(w);

        pthread_mutex_lock(&me->cheflock);
        me->vesselrem = r;
        me->vesselcap = p;

        printf(ANSI_COLOR_RED "chef %d prepared %d vessels of %d capacity each  \n" ANSI_COLOR_RESET, me->chefid, me->vesselrem, me->vesselcap);
        biryani_ready(me);
    }
    return NULL;
}

void ready_to_serve(struct table *tabol)
{
    while (1)
    {
        printf("slots remaining on %d are %d\n", tabol->tableid, tabol->slotsrem);
        if (tabol->slotsrem <= 0)
            break;
        else
        {
            pthread_cond_wait(&tabol->tablecond, &tabol->tablelock);
        }
    }
    pthread_mutex_unlock(&tabol->tablelock);
}

void *table(void *inp)
{
    //put in while 1
    struct table *self = (struct table *)inp;
    int itr = 0;

    while (1)
    {

        printf(ANSI_COLOR_GREEN " Table %d Waiting for Biryani \n" ANSI_COLOR_RESET, self->tableid);
        while (1)
        {
            itr++;
            itr = itr % rcheffno;
            struct chef *ichef = cheffarr[itr];
            pthread_mutex_lock(&ichef->cheflock);
            if (ichef->vesselrem > 0)
            {
                self->containers = ichef->vesselcap;
                ichef->vesselrem--;
                pthread_cond_signal(&ichef->chefcond);
                pthread_mutex_unlock(&ichef->cheflock);
                break;
            }
            else
                pthread_mutex_unlock(&ichef->cheflock);
        }

        printf(ANSI_COLOR_GREEN " Table %d got Biryani from chef %d \n" ANSI_COLOR_RESET, self->tableid, itr);

        while (self->containers > 0)
        {
            int number_of_slots = 1 + rand() % 10;
            pthread_mutex_lock(&self->tablelock);
            self->slots = number_of_slots;

            if (self->slots > self->containers)
            {
                self->slots = self->containers;
            }
            self->slotsrem = self->slots;
            self->containers = self->containers - self->slots;

            printf(ANSI_COLOR_GREEN "NOW TABLE %d READY TO SERVE TO %d SLOTS\n" ANSI_COLOR_RESET, self->tableid, self->slots);

            ready_to_serve(self);
        }
        printf(ANSI_COLOR_GREEN "CONTAINER OF SERVING TABLE %d EMPTY\n" ANSI_COLOR_RESET, self->tableid);
    }
    return NULL;
}

struct student
{
    int stuid;
    pthread_t stuthid;
    int status;
    int tableno;
};

struct student *stuarr[400];

struct table *tablearr[400];

void wait_for_slot(struct student *thisisme)
{
    int i = 0;
    int found = 0;
    while (found == 0)
    {
        i++;
        i %= tableno;
        struct table *itrtable = tablearr[i];
        pthread_mutex_lock(&itrtable->tablelock);

        if (itrtable->slotsrem > 0)
        {
            itrtable->slotsrem--;
            found++;
            pthread_cond_signal(&itrtable->tablecond);
            printf(ANSI_COLOR_YELLOW "student %d assigned a slot on table %d\n" ANSI_COLOR_RESET
                       ANSI_COLOR_BLUE "student %d will be served \n" ANSI_COLOR_RESET,
                   thisisme->stuid, i, thisisme->stuid);
        }

        pthread_mutex_unlock(&itrtable->tablelock);
    }
}

void *students(void *inp)
{
    struct student *thisisme = (struct student *)inp;
    int waittimeba = rand() % 10;

    sleep(waittimeba);
    printf("STUDENT %d ARRIVED IN THE MESS \n", thisisme->stuid);
    wait_for_slot(thisisme);

    return NULL;
}

int studentno;

int main()
{
    printf("enter number of robots :");
    scanf("%d", &rcheffno);
    printf("enter number of tables :");
    scanf("%d", &tableno);
    printf("enter number of students  :");
    scanf("%d", &studentno);

    for (int i = 0; i < rcheffno; i++)
    {
        cheffarr[i] = (struct chef *)malloc(sizeof(struct chef));
        cheffarr[i]->chefid = i;
        pthread_cond_init(&cheffarr[i]->chefcond, NULL);
        pthread_create(&cheffarr[i]->thid, NULL, chef, (void *)cheffarr[i]);
        //        printf("created cheff thread %d\n",i);
    }

    for (int i = 0; i < tableno; i++)
    {
        tablearr[i] = (struct table *)malloc(sizeof(struct table));
        tablearr[i]->tableid = i;
        pthread_cond_init(&tablearr[i]->tablecond, NULL);
        pthread_create(&tablearr[i]->tabthid, NULL, table, (void *)tablearr[i]);
        //       printf("created table thread %d\n",i);
    }

    for (int i = 0; i < studentno; i++)
    {
        stuarr[i] = (struct student *)malloc(sizeof(struct student));
        stuarr[i]->stuid = i;
        pthread_create(&stuarr[i]->stuthid, NULL, students, (void *)stuarr[i]);
        //     printf("created student thread %d\n",i);
    }

    for (int i = 0; i < studentno; i++)
    {
        pthread_join(stuarr[i]->stuthid, NULL);
        printf("Student %d left the mess \n", i);
    }
    printf(ANSI_COLOR_MAGENTA "\nSIMULATION OVER\n" ANSI_COLOR_RESET);

    return 0;
}
