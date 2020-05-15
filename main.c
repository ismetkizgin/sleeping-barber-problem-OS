#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define MUSTERI_SINIRI 10       /* Müşteriler için iş parçası sınırı */
#define KESME_SURESI 1          /* Saç kesme için kullanılacak süre */

/* semaforlar */
sem_t berberler;                /* berberler için semafor */
sem_t musteriler;               /* müşteriler için semafor */
sem_t mutex;                    /* berber koltuğuna karşılıklı münhasır erişim sağlar */

/* fonksiyonlar */
void ip_berber(void* sayi);
void ip_musteri(void* sayi);
void bekle();

/* değişkenler */
int koltukSayisi = 0;           /* berber koltuğu sayısı */
int musteriSayisi = 0;          /* müşteri sayısı */
int sandalyeSayisi = 0;         /* bekleme odasındaki sandalye sayısı */
int bosSandalyeSayisi = 0;      /* bekleme odasındaki boş sandalye sayısı */
int hizmetEdilecekMusteri = 0;  /* hizmet edilecek müşteri kimliği */
int oturulacakSandalye = 0;     /* müşterinin oturacağı sandalye kimliği */
int* koltuk;                    /* berber - müşteri arasında kimlik takası için */

int main(int argc, char** args)
{
    if (argc != 4)
    {
        printf("\nKullanım Hatası!\nKullanım Şekli:\t uyuyan-berber <Müşteri Sayısı> <Sandalye Sayısı> <Koltuk Sayısı>\n\n");
        return EXIT_FAILURE;
    }

    musteriSayisi = atoi(args[1]);
    sandalyeSayisi = atoi(args[2]);
    koltukSayisi = atoi(args[3]);
    bosSandalyeSayisi = sandalyeSayisi;
    koltuk = (int*) malloc(sizeof(int) * sandalyeSayisi);

    if (musteriSayisi > MUSTERI_SINIRI)
    {
        printf("\nMüşteri sınırı: %d\n\n", MUSTERI_SINIRI);
        return EXIT_FAILURE;
    }

    printf("\n\nGirilen Müşteri Sayısı:\t\t%d", musteriSayisi);
    printf("\nGirilen Sandalye Sayısı:\t%d", sandalyeSayisi);
    printf("\nGirilen Berber Koltuğu Sayısı:\t%d\n\n", koltukSayisi);

    pthread_t berber[koltukSayisi], musteri[musteriSayisi]; /* iş parçaları */

    /* semaforların oluşturulması */
    sem_init(&berberler, 0, 0);
    sem_init(&musteriler, 0, 0);
    sem_init(&mutex, 0, 1);

    printf("\nBerber dükkanı açıldı.\n\n");

    /* berber iş parçalarının oluşturulması */
    for (int i = 0; i < koltukSayisi; i++)
    {
        pthread_create(&berber[i], NULL, (void*)ip_berber, (void*)&i);
        sleep(1);
    }

    /* müşteri iş parçalarının oluşturulması */
    for (int i = 0; i < musteriSayisi; i++)
    {
        pthread_create(&musteri[i], NULL, (void*)ip_musteri, (void*)&i);
        bekle();    /* müşterileri rastgele aralıklarda oluşturmak için */
    }

    /* dükkanı kapatmadan önce tüm müşteriler ile ilgilen */
    for (int i = 0; i < musteriSayisi; i++)
    {
        pthread_join(musteri[i], NULL);
    }


    printf("\nTüm müşterilere hizmet verildi. Berber dükkanı kapandı. Berberler dükkandan ayrıldı.\n\n");

    return EXIT_SUCCESS;
}

void ip_berber(void* sayi)
{
    int s = *(int*)sayi + 1;
    int sonrakiMusteri, musteri_kimligi;

    printf("[Berber: %d]\tdükkana geldi.\n", s);

    while (1)
    {
        if (!musteri_kimligi)
        {
            printf("[Berber: %d]\tuyumaya gitti.\n\n", s);
        }

        sem_wait(&berberler);   /* uyuyan berberlerin kuyruğuna katıl */
        sem_wait(&mutex);       /* koltuğa erişimi kilitle */

        /* hizmet edilecek müşterinin bekleyenlerin arasından seçilmesi */
        hizmetEdilecekMusteri = (++hizmetEdilecekMusteri) % sandalyeSayisi;
        sonrakiMusteri = hizmetEdilecekMusteri;
        musteri_kimligi = koltuk[sonrakiMusteri];
        koltuk[sonrakiMusteri] = pthread_self();

        sem_post(&mutex);       /* koltuğa erişim kilidini kaldır */
        sem_post(&musteriler);  /* seçilen müşteriyle ilgilen */

        printf("[Berber: %d]\t%d. müşterinin saçını kesmeye başladı.\n\n", s, musteri_kimligi);
        sleep(KESME_SURESI);
        printf("[Berber: %d]\t%d. müşterinin saçını kesmeyi bitirdi.\n\n", s, musteri_kimligi);
    }
}

void ip_musteri(void* sayi)
{
    int s = *(int*)sayi + 1;
    int oturulanSandalye, berber_kimligi;

    sem_wait(&mutex);   /* koltuğu korumak için erişimi kilitle */

    printf("[Müşteri: %d]\tdükkana geldi.\n", s);

    /* bekleme odasında boş sandalye varsa */
    if (bosSandalyeSayisi > 0)
    {
        bosSandalyeSayisi--;

        printf("[Müşteri: %d]\tbekleme salonunda bekliyor.\n\n", s);

        /* bekleme salonundan bir sandalye seçip otur */
        oturulacakSandalye = (++oturulacakSandalye) % sandalyeSayisi;
        oturulanSandalye = oturulacakSandalye;
        koltuk[oturulanSandalye] = s;

        sem_post(&mutex);           /* koltuğa erişim kilidini kaldır */
        sem_post(&berberler);       /* uygun berberi uyandır */

        sem_wait(&musteriler);      /* bekleyen müşteriler kuyruğuna katıl */
        sem_wait(&mutex);           /* koltuğu korumak için erişimi kilitle */

        /* berber koltuğuna geç */
        berber_kimligi = koltuk[oturulanSandalye];
        bosSandalyeSayisi++;

        sem_post(&mutex);           /* koltuğa erişim kilidini kaldır */
    }
    else
    {
        sem_post(&mutex);           /* koltuğa erişim kilidini kaldır */
        printf("[Müşteri: %d]\tbekleme salonunda yer bulamadı. Dükkandan ayrılıyor.\n\n", s);
    }
    pthread_exit(0);
}

void bekle()
{
    srand((unsigned int)time(NULL));
    usleep(rand() % (250000 - 50000 + 1) + 50000); /* 50000 - 250000 ms */
}