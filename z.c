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

int companyno, zoneno, studentno;

struct company
{
    int id;
    int batchesrem;
    int waittime;
    int vaccinecount;
    float proability;
    pthread_t companytid;
    pthread_cond_t companycond;
    pthread_mutex_t companylock;
};

struct zone
{
    int vaccinerem;
    int id;
    int slots;
    float proability;
    pthread_t zonetid;
    pthread_cond_t zonecond;
    pthread_mutex_t zonelock;
};

struct student
{
    int id;
    int turn;
    int zoneno;
    pthread_t studenttid;
};

struct company *companyarr[400];
struct zone *zonearr[400];
struct student *studentarr[400];

int min(int a, int b)
{
    if (a <= b)
        return a;
    return b;
}

void send_vaccines(struct company *inp)
{
    while (1)
    {
        if (inp->batchesrem <= 0)
        {
            printf(ANSI_COLOR_RED "All batches prepared by Company %d were used\n" ANSI_COLOR_RESET, inp->id);
            break;
        }
        else
        {
            pthread_cond_wait(&inp->companycond, &inp->companylock);
        }
    }
    pthread_mutex_unlock(&inp->companylock);
}

void wait_for_slot(struct student *inp)
{
    int cnt = 0;
    int found = 0;
    while (!found)
    {
        cnt += 1;
        cnt %= zoneno;
        printf(ANSI_COLOR_BLUE "Student %d is waiting to be allocated a slot on a Vaccination Zone\n" ANSI_COLOR_RESET, inp->id);
        struct zone *z = zonearr[cnt];
        pthread_mutex_lock(&z->zonelock);
        if (z->slots > 0)
        {
            z->slots--;
            found = 1;
            pthread_cond_signal(&z->zonecond);
            printf(ANSI_COLOR_GREEN "Student %d assigned a slot on the Vaccination Zone %d and waiting to be Vaccinated\n" ANSI_COLOR_RESET, inp->id, z->id);
            printf(ANSI_COLOR_BLUE "Student %d on Vaccination Zone %d has been vaccinated which has success probability %f\n" ANSI_COLOR_RESET, inp->id, z->id, z->proability);
        }
        pthread_mutex_unlock(&z->zonelock);
    }
}

void zone_ready(struct zone *inp)
{
    while (1)
    {
        printf("Vaccination Zone %d is ready to vaccinate with %d slots\n", inp->id, inp->slots);
        if (inp->slots < 0)
        {
            printf(ANSI_COLOR_RED "Vaccination Zone %d has run out of vaccines\n" ANSI_COLOR_RESET, inp->id);
            break;
        }
        else
        {
            printf(ANSI_COLOR_BLUE "Vaccination Zone %d entering Vaccination Phase\n" ANSI_COLOR_RESET, inp->id);
            pthread_cond_wait(&inp->zonecond, &inp->zonelock);
        }
    }
    pthread_mutex_unlock(&inp->zonelock);
}

void *company(void *inp)
{
    struct company *input = (struct company *)inp;
    printf("Company %d started production\n", input->id);
    while (1)
    {
        int w = 2 + rand() % 4;
        int r = 1 + rand() % 5;
        int p = 10 + rand() % 11;
        input->waittime = w;
        sleep(w);
        pthread_mutex_lock(&input->companylock);
        input->batchesrem = r;
        input->vaccinecount = p;
        printf(ANSI_COLOR_BLUE "Company %d prepared %d batches of %d vaccines each \n" ANSI_COLOR_RESET, input->id, input->batchesrem, input->vaccinecount);
        send_vaccines(input);
    }
    return NULL;
}

void *zone(void *inp)
{
    struct zone *input = (struct zone *)inp;
    int cnt = 0;
    while (1)
    {
        printf(ANSI_COLOR_RED "Waiting for vaccines zone %d\n" ANSI_COLOR_RED, input->id);
        while (1)
        {
            cnt++;
            cnt %= companyno;
            struct company *z = companyarr[cnt];
            pthread_mutex_lock(&z->companylock);
            if (z->batchesrem > 0)
            {
                input->vaccinerem = z->vaccinecount;
                z->batchesrem--;
                pthread_cond_signal(&z->companycond);
                pthread_mutex_unlock(&z->companylock);
                break;
            }
            else
            {
                pthread_mutex_unlock(&z->companylock);
            }
        }
        printf(ANSI_COLOR_RED "Zone %d acquired vaccines from Company %d of count %d\n" ANSI_COLOR_RESET, input->id, companyarr[cnt]->id, companyarr[cnt]->vaccinecount);
        while (input->vaccinerem > 0)
        {
            int slots = 1 + rand() % (min(8, input->vaccinerem));
            pthread_mutex_lock(&input->zonelock);
            input->slots = slots;
            input->vaccinerem -= slots;
            printf(ANSI_COLOR_GREEN "%d slots are prepared in zone %d\n" ANSI_COLOR_RESET, input->slots, input->id);
            zone_ready(input);
        }
        printf(ANSI_COLOR_RED "Vaccines at zone %d are used\n" ANSI_COLOR_RESET, input->id);
    }
    return NULL;
}

void *student(void *inp)
{
    struct student *input = (struct student *)inp;
    int wtime = rand() % 10;
    sleep(wtime);
    input->turn++;
    printf(ANSI_COLOR_CYAN "\nStudent %d has arrived for his/her %d round of Vaccination\n" ANSI_COLOR_RESET, input->id, input->turn);
    wait_for_slot(input);
    return NULL;
}

int main()
{
    printf("Enter number of companies:");
    scanf("%d", &companyno);
    printf("Enter number of vaccine zones:");
    scanf("%d", &zoneno);
    printf("Enter number of students:");
    scanf("%d", &studentno);
    for (int i = 0; i < companyno; i++)
    {
        companyarr[i] = (struct company *)malloc(sizeof(struct company));
        companyarr[i]->id = i;
        companyarr[i]->batchesrem = 0;
        // scanf("%f", &companyarr[i]->proability);
        pthread_cond_init(&companyarr[i]->companycond, NULL);
        pthread_create(&companyarr[i]->companytid, NULL, company, (void *)companyarr[i]);
    }
    for (int i = 0; i < zoneno; i++)
    {
        zonearr[i] = (struct zone *)malloc(sizeof(struct zone));
        zonearr[i]->id = i;
        pthread_cond_init(&zonearr[i]->zonecond, NULL);
        pthread_create(&zonearr[i]->zonetid, NULL, zone, (void *)zonearr[i]);
    }
    for (int i = 0; i < studentno; i++)
    {
        studentarr[i] = (struct student *)malloc(sizeof(struct student));
        studentarr[i]->id = i;
        studentarr[i]->turn = 0;
        pthread_create(&studentarr[i]->studenttid, NULL, student, (void *)studentarr[i]);
    }
    for (int i = 0; i < studentno; i++)
    {
        pthread_join(studentarr[i]->studenttid, NULL);
        printf("Student %d left \n", studentarr[i]->id);
    }
    printf(ANSI_COLOR_YELLOW "\nSIMULATION COMPLETED\n" ANSI_COLOR_RESET);
}