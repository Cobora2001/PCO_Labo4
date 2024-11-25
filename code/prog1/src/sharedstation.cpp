//    ___  _________    ___  ___  ___ ____ //
//   / _ \/ ___/ __ \  |_  |/ _ \|_  / / / //
//  / ___/ /__/ /_/ / / __// // / __/_  _/ //
// /_/   \___/\____/ /____/\___/____//_/   //
//

// ==========================================================
// Fichier : sharedstation.cpp
// Auteur  : Thomas Vuillemier et Sebastian Diaz
// Date    : 25/11/2024
// Description : Implémentation de la classe SharedStation pour gérer
//               l'arrivée de plusieurs trains à leur station respective
// ==========================================================

#include <chrono>
#include <thread>

#include "sharedstation.h"

SharedStation::SharedStation(int nbTrains) : nbTrains(nbTrains), trainsAtStation(0), 
stationSemaphore(0), stationMutex() {
}

void SharedStation::trainArrived() {
    // Quand un train arrive, on réserve le droit de modification de la variable trainsAtStation
    stationMutex.lock();
    ++trainsAtStation;
    if(trainsAtStation == nbTrains) { // Si tous les trains sont arrivés
        // Attendre que les passagers montent/descendent
        std::this_thread::sleep_for(std::chrono::seconds(2));
        // Les trains sont à la gare, débloquer l'attente
        for (int i = 1; i < nbTrains; ++i) { // i = 1 car le train actuel n'a pas acquit le sémaphore
            stationSemaphore.release();
        }

        // Réinitialiser le nombre de trains à la gare
        trainsAtStation = 0;

        // Libérer le mutex
        stationMutex.unlock();
    } else { // Sinon, attendre que le reste des trains arrive
        // On libère le mutex pour que les autres trains puissent incrémenter la variable
        stationMutex.unlock();
        // On attend que le reste des trains arrive
        stationSemaphore.acquire();
    }
}
