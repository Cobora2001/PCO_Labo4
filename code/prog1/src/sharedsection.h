//    ___  _________    ___  ___  ___ ____ //
//   / _ \/ ___/ __ \  |_  |/ _ \|_  / / / //
//  / ___/ /__/ /_/ / / __// // / __/_  _/ //
// /_/   \___/\____/ /____/\___/____//_/   //
//

#ifndef SHAREDSECTION_H
#define SHAREDSECTION_H

#include <QDebug>

#include <pcosynchro/pcosemaphore.h>

#include "locomotive.h"
#include "ctrain_handler.h"
#include "sharedsectioninterface.h"

#include <vector>
#include <utility>

/**
 * @brief La classe SharedSection implémente l'interface SharedSectionInterface qui
 * propose les méthodes liées à la section partagée.
 */
class SharedSection final : public SharedSectionInterface
{
public:

    /**
     * @brief SharedSection Constructeur de la classe qui représente la section partagée.
     */
    SharedSection() {}

    /**
     * @brief access Méthode à appeler pour accéder à la section partagée, doit arrêter le thread
     * de la locomotive si la section est occupée par une autre locomotive
     * @param loco La locomotive qui essaie accéder à la section partagée
     */
    void access(Locomotive &loco) override {

        // On mémorise la vitesse actuelle de la locomotive au cas où elle devrait s'arrêter
        int vitesse = loco.vitesse();

        // On mémorise si la locomotive a dû s'arrêter ou non
        bool couldGoWithoutStop = true;

        // Flag pour savoir si la locomotive peut accéder à la section partagée et sortir de la boucle
        bool canGo = false;

        while(!canGo) {
            mutex.lock();
            // Si la section partagée est occupée, on met la locomotive en attente
            if(occupied) {
                ++nbWaiting;
                mutex.unlock();
                // Si la locomotive n'a pas déjà été arrêtée, on l'arrête
                if(couldGoWithoutStop) {
                    loco.fixerVitesse(0);
                    couldGoWithoutStop = false;
                }
                // On attend que qu'on réveille la locomotive
                waitingSemaphore.acquire();
            } else {
                // On occupe la section partagée
                occupied = true;
                mutex.unlock();
                // On mémorise qu'on peut sortir de la boucle
                canGo = true;
                // On accède à la section partagée
                semaphore.acquire();
            }

        }
        afficher_message(qPrintable(QString("The engine no. %1 accesses the shared section.").arg(loco.numero())));

        // On redémarre la locomotive si elle a été arrêtée
        if(!couldGoWithoutStop) {
            loco.fixerVitesse(vitesse);
        }
    }

    /**
     * @brief leave Méthode à appeler pour indiquer que la locomotive est sortie de la section
     * partagée. (Réveille un thread d'une locomotive potentiellement en attente).
     * @param loco La locomotive qui quitte la section partagée
     */
    void leave(Locomotive& loco) override {
        mutex.lock();
        occupied = false;
        for(int i = 0; i < nbWaiting; ++i) {
            waitingSemaphore.release();
        }

        nbWaiting = 0;
        mutex.unlock();

        semaphore.release();
        afficher_message(qPrintable(QString("The engine no. %1 leaves the shared section.").arg(loco.numero())));
    }

private:

    /**
     * @brief semaphore Sémaphore pour gérer l'accès à la section partagée
     */
    PcoSemaphore semaphore{1};

    /**
     * @brief mutex Mutex pour protéger l'accès à occupied
     */
    PcoMutex mutex;

    /**
     * @brief occupied Indique si la section partagée est occupée
     */
    bool occupied = false;

    /**
     * @brief nbWaiting Nombre de locomotives en attente
     */
    int nbWaiting = 0;

    /**
     * @brief waitingSemaphore Sémaphore pour gérer l'attente des locomotives
     */
    PcoSemaphore waitingSemaphore{0};
};


#endif // SHAREDSECTION_H