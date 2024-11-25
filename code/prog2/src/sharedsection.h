//    ___  _________    ___  ___  ___ ____ //
//   / _ \/ ___/ __ \  |_  |/ _ \|_  / / / //
//  / ___/ /__/ /_/ / / __// // / __/_  _/ //
// /_/   \___/\____/ /____/\___/____//_/   //
//                                         //


#ifndef SHAREDSECTION_H
#define SHAREDSECTION_H

#include <QDebug>

#include <pcosynchro/pcosemaphore.h>

#include "locomotive.h"
#include "ctrain_handler.h"
#include "sharedsectioninterface.h"

/**
 * @brief La classe SharedSection implémente l'interface SharedSectionInterface qui
 * propose les méthodes liées à la section partagée.
 */
class SharedSection final : public SharedSectionInterface
{
public:

    /**
     * @brief SharedSection Constructeur de la classe qui représente la section partagée.
     * Initialisez vos éventuels attributs ici, sémaphores etc.
     */
    SharedSection() :  mode(PriorityMode::HIGH_PRIORITY), occupied(false) {}

    /**
     * @brief request Méthode a appeler pour indiquer que la locomotive désire accéder à la
     * section partagée (deux contacts avant la section partagée).
     * @param loco La locomotive qui désire accéder
     * @param locoId L'identidiant de la locomotive qui fait l'appel
     * @param entryPoint Le point d'entree de la locomotive qui fait l'appel
     */
   void request(Locomotive& loco) override {
        mutex.lock();

        // Vérifie si la locomotive a déjà fait une demande
        auto it = std::find_if(requestQueue.begin(), requestQueue.end(),
                               [&loco](const std::pair<int, int>& req) { return req.second == loco.numero(); });

        if (it == requestQueue.end()) {
            // Ajoute la demande dans la file
            requestQueue.push_back({priority, loco.numero()});
            loco.afficherMessage(QString("Locomotive %1 a demandé la section avec priorité %2.")
                                     .arg(loco.numero())
                                     .arg(priority));
        } else {
            loco.afficherMessage(QString("Locomotive %1 a déjà une demande active.").arg(loco.numero()));
        }

        // Trie la file en fonction du mode de priorité
        sortRequestQueue();

        mutex.unlock();
    }

    /**
     * @brief getAccess Méthode à appeler pour accéder à la section partagée, doit arrêter la
     * locomotive et mettre son thread en attente si la section est occupée ou va être occupée
     * par une locomotive de plus haute priorité. Si la locomotive et son thread ont été mis en
     * attente, le thread doit être reveillé lorsque la section partagée est à nouveau libre et
     * la locomotive redémarée. (méthode à appeler un contact avant la section partagée).
     * @param loco La locomotive qui essaie accéder à la section partagée
     * @param locoId L'identidiant de la locomotive qui fait l'appel
     */
    void access(Locomotive &loco, int priority) override {
        // TODO
          while (true) {
            mutex.lock();

            // Vérifie si la locomotive est en tête de file
            if (!requestQueue.empty() && requestQueue.front().second == loco.numero() && !occupied) {
                occupied = true;
                requestQueue.erase(requestQueue.begin());
                mutex.unlock();
                break; // La locomotive peut entrer
            }

            mutex.unlock();

            // Si la locomotive ne peut pas entrer, elle attend
            loco.arreter();
            semaphore.acquire(); // Attend que la section soit libre
            loco.demarrer();
        }

        loco.afficherMessage(QString("Locomotive %1 accède à la section partagée.").arg(loco.numero()));
    }

    /**
     * @brief leave Méthode à appeler pour indiquer que la locomotive est sortie de la section
     * partagée. (reveille les threads des locomotives potentiellement en attente).
     * @param loco La locomotive qui quitte la section partagée
     * @param locoId L'identidiant de la locomotive qui fait l'appel
     */
    void leave(Locomotive& loco) override {
        // TODO
        mutex.lock();

        occupied = false;

        if (!requestQueue.empty()) {
            semaphore.release(); // Réveille la prochaine locomotive
        }

        loco.afficherMessage(QString("Locomotive %1 quitte la section partagée.").arg(loco.numero()));

        mutex.unlock();
    }

    void togglePriorityMode() {
        mutex.lock();
        mode (mode == PriorityMode::HIGH_PRIORITY) ? PriorityMode::LOW_PRIORITY : PriorityMode::HIGH_PRIORITY;
        sortRequestQueue();
        afficher_message(qPrintable(QString("Priority mode changed to %1").arg(mode == PriorityMode::HIGH_PRIORITY ? "HIGH" : "LOW")));
        mutex.unlock();
    }

private:

    void sortRequestQueue() {
        if (priorityMode == PriorityMode::HIGH_PRIORITY) {
            std::sort(requestQueue.begin(), requestQueue.end(),
                      [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                          return a.first > b.first; // Tri par priorité décroissante
                      });
        } else {
            std::sort(requestQueue.begin(), requestQueue.end(),
                      [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                          return a.first < b.first; // Tri par priorité croissante
                      });
        }
    }

    /* A vous d'ajouter ce qu'il vous faut */

    // Méthodes privées ...
    // Attributes privés ...

    PcoSemaphore semaphore{0};
    PcoMutex mutex;
    PriorityMode mode;
    std::vector<std::pair<int, int>> requestQueue;
    bool occupied; 
};


#endif // SHAREDSECTION_H
