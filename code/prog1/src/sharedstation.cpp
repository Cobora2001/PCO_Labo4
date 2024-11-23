//    ___  _________    ___  ___  ___ ____ //
//   / _ \/ ___/ __ \  |_  |/ _ \|_  / / / //
//  / ___/ /__/ /_/ / / __// // / __/_  _/ //
// /_/   \___/\____/ /____/\___/____//_/   //
//

#include <chrono>
#include <thread>

#include "sharedstation.h"

SharedStation::SharedStation(int nbTrains) : nbTrains(nbTrains), trainsAtStation(0) {

}

void SharedStation::trainArrived() {
    std::lock_guard<std::mutex> lock(stationMutex);
    ++trainsAtStation;
    if(trainsAtStation == nbTrains) {
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Pause pour montée/descente
        // Les deux trains sont à la gare, débloquer l'attente
        for (int i = 1; i < nbTrains; i++) {
            stationSemaphore.release();
        }

        trainsAtStation = 0; // Réinitialiser pour la prochaine attente
        stationMutex.unlock();
    } else {
        stationMutex.unlock();
        stationSemaphore.acquire(); // Attendre que l'autre train soit arrivé
    }
}
