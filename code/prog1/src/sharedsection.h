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
     */
    void access(Locomotive &loco) override {
        bool canGo = obtainRightToGoWithoutStop();

        int vitesse = loco.vitesse();

        if (!canGo) {
            loco.fixerVitesse(0);
        }

        semaphore.acquire();
        afficher_message(qPrintable(QString("The engine no. %1 accesses the shared section.").arg(loco.numero())));

        if(!canGo) {
            loco.fixerVitesse(vitesse);
        }
    }

    /**
     * @brief leave Méthode à appeler pour indiquer que la locomotive est sortie de la section
     * partagée. (Réveille un thread d'une locomotive potentiellement en attente).
     * @param loco La locomotive qui quitte la section partagée
     */
    void leave(Locomotive& loco) override {
        setCanGoWithoutStop();

        semaphore.release();
        afficher_message(qPrintable(QString("The engine no. %1 leaves the shared section.").arg(loco.numero())));
    }

private:

    /**
     * @brief obtainRightToGoWithoutStop Méthode à appeler pour obtenir le droit de passer sans s'arrêter
     * @return true si la locomotive peut passer sans s'arrêter, false sinon
     */
    bool obtainRightToGoWithoutStop() {
        mutex.lock();
        bool canGo = canGoWithoutStop;
        canGoWithoutStop = false;
        mutex.unlock();
        return canGo;
    }

    /**
     * @brief setCanGoWithoutStop Méthode à appeler pour donner le droit de passer sans s'arrêter après être passé
     */
    void setCanGoWithoutStop() {
        mutex.lock();
        canGoWithoutStop = true;
        mutex.unlock();
    }

    /**
     * @brief semaphore Sémaphore pour gérer l'accès à la section partagée
     */
    PcoSemaphore semaphore{1};

    /**
     * @brief mutex Mutex pour protéger l'accès à canGoWithoutStop
     */
    PcoMutex mutex;

    /**
     * @brief canGoWithoutStop Indique si la locomotive peut passer sans s'arrêter
     */
    bool canGoWithoutStop;
};


#endif // SHAREDSECTION_H