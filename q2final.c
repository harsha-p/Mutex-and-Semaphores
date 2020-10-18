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

int sno, cno, zno, wstudents = 0, totstudents = 0;
pthread_mutex_t wslock, tslock;
struct student
{
    int id;
    int turn;
    int vac;
    int zone;
    double prob;
    pthread_t stid;
    pthread_mutex_t slock;
};

struct company
{
    int id;
    int batchrem;
    int count;
    double prob;
    int completed;
    pthread_t ctid;
    pthread_mutex_t clock;
};

struct zone
{
    int id;
    int comp;
    int vrem;
    int srem;
    double prob;
    pthread_t ztid;
    pthread_mutex_t zlock;
};

struct student *sa[400];
struct company *ca[400];
struct zone *za[400];

int min(int a, int b)
{
    if (a <= b)
        return a;
    return b;
}

void antibody_checkup(struct student *s);

void delivery(struct company *c)
{
    while (1)
    {
        if (totstudents == 0)
        {
            return;
        }
        if (c->batchrem > 0)
            ;
        else
        {
            c->completed = 1;
            for (int i = 0; i < zno; i++)
            {
                if (za[i]->comp == c->id)
                {
                    c->completed = 0;
                    break;
                }
            }
            if (c->completed == 1)
            {
                break;
            }
        }
    }
}

void wait_for_slot(struct student *s) // students get a slot , waits till vaccination and  goes for antibody checkup
{
    pthread_mutex_lock(&s->slock);
    pthread_mutex_lock(&wslock);
    wstudents++;
    pthread_mutex_unlock(&wslock);
    printf(ANSI_COLOR_CYAN "Student %d is waiting to be allocated a slot on a Vaccination Zone\n" ANSI_COLOR_RESET, s->id);
    int cnt = 0;
    while (1)
    {
        cnt += 1;
        cnt %= zno;
        struct zone *z = za[cnt];
        int V = pthread_mutex_trylock(&z->zlock);
        if (V == 0)
        {
            if (z->srem > 0)
            {
                printf(ANSI_COLOR_MAGENTA "Student %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated\n" ANSI_COLOR_RESET, s->id, z->id);
                pthread_mutex_lock(&wslock);
                wstudents--;
                pthread_mutex_unlock(&wslock);
                s->zone = z->id;
                z->srem--;
                s->vac = 0;
                s->turn += 1;
                s->prob = z->prob;
                pthread_mutex_unlock(&s->slock);
                pthread_mutex_unlock(&z->zlock);
                break;
            }
            else
            {
                pthread_mutex_unlock(&s->slock);
                pthread_mutex_unlock(&z->zlock);
            }
        }
    }
    while (s->vac != 1)
        ;
    antibody_checkup(s);
}

void antibody_checkup(struct student *s)
{
    double r = (double)rand() / (double)RAND_MAX;
    pthread_mutex_lock(&tslock);
    if (r < s->prob)
    {
        printf(ANSI_COLOR_GREEN "Student %d has tested positive for antibodies\n" ANSI_COLOR_RESET, s->id);
        printf(ANSI_COLOR_RED "Student %d left \n" ANSI_COLOR_RESET, s->id);
        totstudents--;
        pthread_mutex_unlock(&tslock);
    }
    else
    {
        printf(ANSI_COLOR_RED "Student %d has tested negative for antibodies\n" ANSI_COLOR_RESET, s->id);
        if (s->turn <= 3)
        {
            s->vac = -1;
            printf(ANSI_COLOR_RED "Student %d has arrived for his %d round of Vaccination\n" ANSI_COLOR_RESET, s->id, s->turn);
            pthread_mutex_unlock(&tslock);
            wait_for_slot(s);
        }
        else
        {
            totstudents--;
            printf(ANSI_COLOR_RED "Student %d left \n" ANSI_COLOR_RESET, s->id);
            pthread_mutex_unlock(&tslock);
            return;
        }
    }
}

void vaccinate_students(struct zone *z) // vaccinate students who got a slot
{
    int id = z->id;
    while (z->srem && wstudents)
        ;
    if (totstudents == 0)
        return;
    pthread_mutex_lock(&z->zlock);
    printf(ANSI_COLOR_YELLOW "zone %d entering vaccination phase\n" ANSI_COLOR_RESET, z->id);
    for (int i = 0; i < sno; i++)
    {
        struct student *s = sa[i];
        int V = pthread_mutex_trylock(&s->slock);
        if (V == 0)
        {
            if (s->vac == 0 && s->zone == z->id)
            {
                printf(ANSI_COLOR_GREEN "Student %d on Vaccination Zone %d has been vaccinated which has success probability %lf\n" ANSI_COLOR_RESET, s->id, z->id, z->prob);
                s->vac = 1;
                s->zone = -1;
            }
            pthread_mutex_unlock(&s->slock);
        }
    }
    z->vrem += z->srem;
    z->srem = 0;
    pthread_mutex_unlock(&z->zlock);
    sleep(2);
}

void *company(void *inp) // produce vaccines
{
    struct company *c = (struct company *)inp;
    while (1)
    {
        int w = 2 + rand() % 4;
        int r = 1 + rand() % 5;
        int p = 10 + rand() % 11;
        if (totstudents == 0)
        {
            return NULL;
        }
        printf(ANSI_COLOR_RED "Pharmaceutical Company %d is preparing %d batches of vaccines which have success probability %lf\n" ANSI_COLOR_RESET, c->id, r, c->prob);
        sleep(w);
        pthread_mutex_lock(&c->clock);
        c->batchrem = r;
        c->count = p;
        printf(ANSI_COLOR_YELLOW "Pharmaceutical Company %d has prepared %d batches of vaccines which have success probability %lf\n" ANSI_COLOR_RESET, c->id, c->batchrem, c->prob);
        pthread_mutex_unlock(&c->clock);
        delivery(c);
        if (c->completed && totstudents)
        {
            printf(ANSI_COLOR_RED "All the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now\n" ANSI_COLOR_RESET, c->id);
        }
    }
    return NULL;
}

void create_slots(struct zone *z) // create slots  for students and take back unused slots
{
    printf(ANSI_COLOR_CYAN "Pharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now\n" ANSI_COLOR_RESET, z->comp, z->id);
    while (z->vrem)
    {
        pthread_mutex_lock(&z->zlock);
        int r = z->vrem;
        r = min(r, min(8, wstudents));
        if (totstudents == 0)
        {
            pthread_mutex_unlock(&z->zlock);
            return;
        }
        else if (r == 0)
        {
            pthread_mutex_unlock(&z->zlock);
            continue;
        }
        int slots = 1 + rand() % r;
        z->srem = slots;
        z->vrem -= slots;
        sleep(1);
        printf(ANSI_COLOR_BLUE "Vaccination Zone %d is ready to vaccinate with %d slots\n" ANSI_COLOR_RESET, z->id, z->srem);
        pthread_mutex_unlock(&z->zlock);
        vaccinate_students(z);
    }
    printf(ANSI_COLOR_RED "Vaccination Zone %d has run out of vaccines\n" ANSI_COLOR_RESET, z->id);
}

void acquire_vaccines(struct zone *z) // acquire vaccines from a company and create slots
{
    int cnt = 0;
    pthread_mutex_lock(&z->zlock);
    while (1)
    {
        if (totstudents == 0)
            break;
        cnt++;
        cnt %= cno;
        struct company *c = ca[cnt];
        int Z = pthread_mutex_trylock(&c->clock);
        if (Z == 0)
        {
            if (c->batchrem > 0)
            {
                c->batchrem--;
                z->comp = c->id;
                z->vrem = c->count;
                z->prob = c->prob;
                printf(ANSI_COLOR_BLUE "Pharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %lf\n" ANSI_COLOR_RESET, c->id, z->id, c->prob);
                sleep(1);
                pthread_mutex_unlock(&c->clock);
                break;
            }
            else
                pthread_mutex_unlock(&c->clock);
        }
    }
    pthread_mutex_unlock(&z->zlock);
    return create_slots(z);
}

void *zone(void *inp)
{
    struct zone *z = (struct zone *)inp;
    while (1)
    {
        printf(ANSI_COLOR_RED "Zone %d waiting for vaccine\n" ANSI_COLOR_RESET, z->id);
        acquire_vaccines(z);
        z->comp = -1;
        if (totstudents == 0)
        {
            break;
        }
    }
    return NULL;
}

void *student(void *inp)
{
    struct student *s = (struct student *)inp;
    int w = rand() % 10;
    sleep(w);
    s->turn += 1;
    printf(ANSI_COLOR_RED "Student %d has arrived for his %d round of Vaccination\n" ANSI_COLOR_RESET, s->id, s->turn);
    wait_for_slot(s);
    return NULL;
}

int main()
{
    printf("Enter number of companies:");
    scanf("%d", &cno);
    printf("Enter number of vaccine zones:");
    scanf("%d", &zno);
    printf("Enter number of students:");
    scanf("%d", &sno);
    pthread_mutex_lock(&tslock);
    totstudents = sno;
    pthread_mutex_unlock(&tslock);
    for (int i = 0; i < cno; i++)
    {
        ca[i] = (struct company *)malloc(sizeof(struct company));
        ca[i]->id = i;
        ca[i]->batchrem = 0;
        ca[i]->completed = -1;
        scanf("%lf", &(ca[i]->prob));
        pthread_create(&ca[i]->ctid, NULL, company, (void *)ca[i]);
    }
    for (int i = 0; i < zno; i++)
    {
        za[i] = (struct zone *)malloc(sizeof(struct zone));
        za[i]->id = i;
        za[i]->comp = -1;
        pthread_create(&za[i]->ztid, NULL, zone, (void *)za[i]);
    }
    for (int i = 0; i < sno; i++)
    {
        sa[i] = (struct student *)malloc(sizeof(struct student));
        sa[i]->id = i;
        sa[i]->turn = 0;
        sa[i]->vac = 0;
        sa[i]->zone = -1;
        pthread_create(&sa[i]->stid, NULL, student, (void *)sa[i]);
    }
    for (int i = 0; i < sno; i++)
    {
        pthread_join(sa[i]->stid, NULL);
    }
    for (int i = 0; i < zno; i++)
    {
        pthread_join(za[i]->ztid, NULL);
    }
    for (int i = 0; i < cno; i++)
    {
        pthread_join(ca[i]->ctid, NULL);
    }
    printf(ANSI_COLOR_YELLOW "\nSIMULATION COMPLETED :)\n\n" ANSI_COLOR_RESET);
}