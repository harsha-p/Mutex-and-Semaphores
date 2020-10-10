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

int sno, cno, zno;
struct student
{
    int id;
    int turn;
    int zone;
    pthread_t stid;
    pthread_mutex_t slock;
};

struct company
{
    int id;
    int batchrem;
    int count;
    float prob;
    int completed;
    pthread_t ctid;
    pthread_mutex_t clock;
    pthread_cond_t ccond;
};

struct zone
{
    int id;
    int comp;
    int vrem;
    int srem;
    float prob;
    pthread_t ztid;
    pthread_mutex_t zlock;
    pthread_cond_t zcond;
};

struct student *sa[400];
struct company *ca[400];
struct zone *za[400];

void delivery(struct company *c)
{
    while (1)
    {
        if (c->batchrem > 0)
            pthread_cond_wait(&c->ccond, &c->clock);
        c->completed = 1;
        for (int i = 0; i < zno; i++)
        {
            if (za[i]->comp == c->id && (za[i]->vrem > 0 || za[i]->srem > 0))
                c->completed = 0;
        }
        if (c->completed == 1)
        {
            break;
        }
    }
    pthread_mutex_unlock(&c->clock);
}

void wait_for_slot(struct student *s)
{
    printf(ANSI_COLOR_BLUE "Student %d is waiting to be allocated a slot on a Vaccination Zone\n" ANSI_COLOR_RESET, s->id);
    int cnt = 0;
    while (1)
    {
        cnt += 1;
        cnt %= zno;
        struct zone *z = za[cnt];
        pthread_mutex_lock(&z->zlock);
        if (z->srem > 0)
        {
            pthread_cond_signal(&z->zcond);
            printf(ANSI_COLOR_MAGENTA "Student %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated\n" ANSI_COLOR_RESET, s->id, z->id);
            printf(ANSI_COLOR_GREEN "Student %d on Vaccination Zone %d has been vaccinated which has success probability %f\n" ANSI_COLOR_RESET, s->id, z->id, z->prob);
            z->srem--;
            pthread_mutex_unlock(&z->zlock);
            break;
        }
        else
            pthread_mutex_unlock(&z->zlock);
    }
}

void vaccinate_students(struct zone *z)
{
    while (1)
    {
        if (z->srem <= 0)
            break;
        else
        {
            pthread_cond_wait(&z->zcond, &z->zlock);
        }
    }
    pthread_mutex_unlock(&z->zlock);
}

void *company(void *inp)
{
    struct company *c = (struct company *)inp;
    while (1)
    {
        int w = 2 + rand() % 4;
        int r = 1 + rand() % 5;
        int p = 10 + rand() % 11;
        if (c->completed == 1)
        {
            sleep(0.1);
            printf(ANSI_COLOR_RED "All the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now\n" ANSI_COLOR_RESET, c->id);
        }
        printf(ANSI_COLOR_RED "Pharmaceutical Company %d is preparing %d batches of vaccines which have success probability %f\n" ANSI_COLOR_RESET, c->id, r, c->prob);
        sleep(w);
        pthread_mutex_lock(&c->clock);
        c->batchrem = r;
        c->count = p;
        printf("%d %d batch count\n", r, p);
        printf(ANSI_COLOR_BLUE "Pharmaceutical Company %d has prepared %d batches of vaccines which have success probability %f\n" ANSI_COLOR_RESET, c->id, c->batchrem, c->prob);
        delivery(c);
    }
    return NULL;
}

void create_slots(struct zone *z)
{
    printf(ANSI_COLOR_GREEN "Pharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now\n" ANSI_COLOR_RESET, z->comp, z->id);
    printf(ANSI_COLOR_YELLOW "Vaccination Zone %d entering Vaccination Phase\n" ANSI_COLOR_RESET, z->id);
    while (z->vrem > 0)
    {
        int r = z->vrem;
        if (r > 8)
            r = 8;
        int slots = 1 + rand() % r;
        pthread_mutex_lock(&z->zlock);
        z->srem = slots;
        z->vrem -= slots;
        printf(ANSI_COLOR_GREEN "Vaccination Zone %d is ready to vaccinate with %d slots\n" ANSI_COLOR_RESET, z->id, z->srem);
        vaccinate_students(z);
    }
    printf(ANSI_COLOR_RED "Vaccination Zone %d has run out of vaccines\n" ANSI_COLOR_RESET, z->id);
}

void acquire_vaccines(struct zone *z)
{
    int cnt = 0;
    while (1)
    {
        cnt++;
        cnt %= cno;
        struct company *c = ca[cnt];
        pthread_mutex_lock(&c->clock);
        if (c->batchrem > 0)
        {
            c->batchrem--;
            z->comp = c->id;
            z->vrem = c->count;
            z->prob = c->prob;
            printf(ANSI_COLOR_BLUE "Pharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %f\n" ANSI_COLOR_RESET, c->id, z->id, c->prob);
            pthread_cond_signal(&c->ccond);
            pthread_mutex_unlock(&c->clock);
            break;
        }
        else
            pthread_mutex_unlock(&c->clock);
    }
    create_slots(z);
}

void *zone(void *inp)
{
    struct zone *z = (struct zone *)inp;
    while (1)
    {
        printf(ANSI_COLOR_RED "Zone %d waiting for vaccine\n" ANSI_COLOR_RESET, z->id);
        acquire_vaccines(z);
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
    // anti body checkup;
}

int main()
{
    printf("Enter number of companies:");
    scanf("%d", &cno);
    printf("Enter number of vaccine zones:");
    scanf("%d", &zno);
    printf("Enter number of students:");
    scanf("%d", &sno);
    for (int i = 0; i < cno; i++)
    {
        ca[i] = (struct company *)malloc(sizeof(struct company));
        ca[i]->id = i;
        ca[i]->batchrem = 0;
        ca[i]->completed = -1;
        scanf("%f", &ca[i]->prob);
        pthread_cond_init(&ca[i]->ccond, NULL);
        pthread_create(&ca[i]->ctid, NULL, company, (void *)ca[i]);
    }
    for (int i = 0; i < zno; i++)
    {
        za[i] = (struct zone *)malloc(sizeof(struct zone));
        za[i]->id = i;
        pthread_cond_init(&za[i]->zcond, NULL);
        pthread_create(&za[i]->ztid, NULL, zone, (void *)za[i]);
    }
    for (int i = 0; i < sno; i++)
    {
        sa[i] = (struct student *)malloc(sizeof(struct student));
        sa[i]->id = i;
        sa[i]->turn = 0;
        pthread_create(&sa[i]->stid, NULL, student, (void *)sa[i]);
    }
    for (int i = 0; i < sno; i++)
    {
        pthread_join(sa[i]->stid, NULL);
        printf("Student %d left \n", sa[i]->id);
    }
    printf(ANSI_COLOR_YELLOW "\nSIMULATION COMPLETED\n" ANSI_COLOR_RESET);
}