//    ___  _________    ___  ___  ___ ____ //
//   / _ \/ ___/ __ \  |_  |/ _ \|_  / / / //
//  / ___/ /__/ /_/ / / __// // / __/_  _/ //
// /_/   \___/\____/ /____/\___/____//_/   //
//                                         //


#ifndef SHAREDSECTION_H
#define SHAREDSECTION_H

#include <QDebug>

#include <pcosynchro/pcosemaphore.h>
#include <pcosynchro/pcomutex.h>

#include "locomotive.h"
#include "ctrain_handler.h"
#include "sharedsectioninterface.h"

/**
 * @brief La classe SharedSection implémente l'interface SharedSectionInterface qui
 * propose les méthodes liées à la section partagée.
 */
class SharedSection final : public SharedSectionInterface {
public:

    /**
     * @brief SharedSection Constructeur de la classe qui représente la section partagée.
     * Initialisez vos éventuels attributs ici, sémaphores etc.
     */
    SharedSection() : mode(PriorityMode::HIGH_PRIORITY),
                    occupied(false), semaphore(1), mutex(), 
                    nbRequests(0), waitingSemaphore(0) {}

    /**
     * @brief request Méthode a appeler pour indiquer que la locomotive désire accéder à la
     * section partagée (deux contacts avant la section partagée).
     * @param loco
     * @param locoId id de la locomotive qui demande l'accès
     * @param priority priorité de la locomotive qui demande l'accès
     * @param loco La locomotive qui demande l'accès
     */
   void request(Locomotive& loco, int locoId, int priority) override {
        // On va modifier la file d'attente, on doit donc verrouiller le mutex
        mutex.lock();

        // Vérifie si la locomotive a déjà fait une demande (elle ne devrait pas, mais on ne sait jamais)
        auto it = std::find_if(requestQueue.begin(), requestQueue.end(),
                           [locoId](const std::pair<int, int>& req) { return req.second == locoId; });

        if (it == requestQueue.end()) { // Si la locomotive n'a pas encore fait de demande, c'est donc le cas normal
            // Ajoute la demande dans la file
            requestQueue.push_back({priority, locoId});
            ++nbRequests;
            loco.afficherMessage(QString("Locomotive %1 a demandé la section avec priorité %2.")
                                     .arg(locoId)
                                     .arg(priority));
        } else { // On ne devrait pas arriver ici
            loco.afficherMessage(QString("Locomotive %1 a déjà une demande active.").arg(locoId));
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
     */
    void access(Locomotive &loco) override {
        
        // Détermine si la locomotive peut accéder à la section partagée (elle va tenter tant qu'elle ne peut pas)
        bool canContinue = false;

        // On mémorise la vitesse actuelle de la locomotive au cas où elle devrait s'arrêter
        int vitesse = loco.vitesse();

        // On mémorise si la locomotive a dû s'arrêter
        bool hadToStop = false;

        // Tant que la locomotive ne peut pas accéder à la section partagée
        while (!canContinue) {
            // Vu qu'on va modifier la file d'attente, on doit verrouiller le mutex
            mutex.lock();

            // Vérifie si la file d'attente est vide. Elle ne devrait pas l'être, 
            // vu qu'au moins la locomotive actuelle devrait être dedans
            if(requestQueue.empty()) {
                mutex.unlock();
                throw std::runtime_error("No request in the queue");
            }

            // Vérifie si la section partagée est occupée
            if(occupied) {
                // On ne gère plus que des variables locales, on peut donc déverrouiller le mutex
                mutex.unlock();
                // Si on n'a pas déjà arrêté la locomotive, on le fait
                if(!hadToStop) {
                    loco.fixerVitesse(0);
                }
                // On mémorise qu'on a dû arrêter la locomotive
                hadToStop = true;
                // On attend que la section partagée soit libérée et qu'on nous réveille
                waitingSemaphore.acquire();
            } else {
                // Vérifie si la locomotive actuelle est la prochaine à accéder à la section partagée
                if (requestQueue.front().second == loco.numero()) {
                    // On mémorise qu'on va pouvoir accéder à la section partagée
                    occupied = true;
                    // On retire notre demande de la file
                    requestQueue.erase(requestQueue.begin());
                    // On décrémente le nombre de demandes, vu qu'on a retiré la nôtre
                    // (On n'a pas besoin de cette variable, on peut gérer ça avec la taille de la file...)
                    --nbRequests;
                    // On ne gère plus que des variables locales, on peut donc déverrouiller le mutex
                    mutex.unlock();
                    // On mémorise qu'on peut sortir de la boucle
                    canContinue = true;
                    // Si on a dû arrêter la locomotive, on la redémarre
                    if(hadToStop) {
                        loco.fixerVitesse(vitesse);
                    }
                    // On accède à la section partagée
                    semaphore.acquire();
                } else {
                    // On ne gère plus que des variables locales, on peut donc déverrouiller le mutex
                    mutex.unlock();
                    // Si on n'a pas déjà arrêté la locomotive, on le fait
                    if(!hadToStop) {
                        loco.fixerVitesse(0);
                    }
                    // On mémorise qu'on a dû arrêter la locomotive
                    hadToStop = true;
                    // On attend que la section partagée soit libérée et qu'on nous réveille
                    waitingSemaphore.acquire();
                }
            }
        }
        loco.afficherMessage(QString("Locomotive %1 accède à la section partagée.").arg(loco.numero()));
    }

    /**
     * @brief leave Méthode à appeler pour indiquer que la locomotive est sortie de la section
     * partagée. (reveille les threads des locomotives potentiellement en attente).
     * @param loco La locomotive qui quitte la section partagée
     */
    void leave(Locomotive& loco) override {
        // On se réserve le droit de modifier les variables partagées
        mutex.lock();
        // On a quitte la section partagée
        occupied = false;

        // On réveille les locomotives en attente
        for(int i = 0; i < nbRequests; ++i) {
            waitingSemaphore.release();
        }

        // On libère le mutex
        mutex.unlock();

        // On libère la section partagée
        semaphore.release();
        afficher_message(qPrintable(QString("The engine no. %1 leaves the shared section.").arg(loco.numero())));
    }

    void togglePriorityMode() {
        mutex.lock();
        // Change le mode de priorité
        mode = (mode == PriorityMode::HIGH_PRIORITY) ? PriorityMode::LOW_PRIORITY : PriorityMode::HIGH_PRIORITY;
        // Trie la file d'attente (normalement, on ne devrait pas avoir de locomotives en attente)
        sortRequestQueue();
        afficher_message(qPrintable(QString("Priority mode changed to %1")
                        .arg(mode == PriorityMode::HIGH_PRIORITY ? "HIGH" : "LOW")));
        mutex.unlock();
    }

private:

    void sortRequestQueue() {
        // Trie la file par priorité décroissante si le mode est HIGH_PRIORITY, sinon par priorité croissante
        // De la sorte, on pourra retirer l'élément le plus prioritaire de la file en retirant l'élément au début
        if (mode == PriorityMode::HIGH_PRIORITY) {
            std::stable_sort(requestQueue.begin(), requestQueue.end(),
                             [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                                 return a.first > b.first; // Tri par priorité décroissante
                             });
        } else {
            // Même chose que ci-dessus, mais en ordre décroissant, 
            //donc on pourra retirer l'élément le moins prioritaire de la file en retirant l'élément à la fin
            std::stable_sort(requestQueue.begin(), requestQueue.end(),
                             [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                                 return a.first < b.first; // Tri par priorité croissante
                             });
        }
    }

    /**
     * @brief semaphore Sémaphore pour gérer l'accès à la section partagée
     */
    PcoSemaphore semaphore;

    /**
     * @brief waitingSemaphore Sémaphore pour gérer l'attente des locomotives
     */
    PcoSemaphore waitingSemaphore;

    /**
     * @brief mutex Mutex pour protéger l'accès à canGoWithoutStop
     */
    PcoMutex mutex;

    /**
     * @brief occupied Indique si la section partagée est occupée (dont si l'accès a déjà été donné à une locomotive)
     */
    bool occupied;

    /**
     * @brief priorityMode Mode de priorité de la section partagée
     */
    PriorityMode mode;

    /**
     * @brief requestQueue File d'attente des requêtes pour la section partagée
     */
    std::deque<std::pair<int, int>> requestQueue;

    int nbRequests;
};


#endif // SHAREDSECTION_H
