// ==========================================================
// Fichier : sharedstation.h
// Auteur  : Thomas Vuillemier et Sebastian Diaz
// Date    : 25/11/2024
// Description : Définition de la classe SharedStation pour gérer
//               l'arrivée de plusieurs trains à leur station respective
// ==========================================================

#ifndef SHARED_STATION_H
#define SHARED_STATION_H

#include <pcosynchro/pcosemaphore.h>
#include <pcosynchro/pcomutex.h>

/**
 * @brief La classe SharedStation représente un moyen de coordiner 
 * l'arrivée de plusieurs trains à leur station respective
 */
class SharedStation
{
public:
    /**
     * @brief SharedStation Constructeur de la classe SharedStation
     * @param nbTrains Le nombre de trains qui doivent arriver à la station
     */
    SharedStation(int nbTrains);

    /**
     * @brief trainArrived Méthode à appeler lorsqu'un train arrive à la station
     */
    void trainArrived();

private:
    /**
     * @brief nbTrains Le nombre de trains qui doivent arriver à la station
     */
    int nbTrains;

    /**
     * @brief trainsAtStation Le nombre de trains actuellement à la station
     */
    int trainsAtStation;

    /**
     * @brief stationMutex Mutex pour protéger la variable trainsAtStation
     */
    PcoMutex stationMutex;

    /**
     * @brief stationSemaphore Sémaphore pour attendre que tous les trains soient arrivés à la station
     */
    PcoSemaphore stationSemaphore;
};

#endif // SHARED_STATION_H