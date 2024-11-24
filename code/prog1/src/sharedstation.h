#ifndef SHARED_STATION_H
#define SHARED_STATION_H

#include <pcosynchro/pcosemaphore.h>
#include <pcosynchro/pcomutex.h>

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
    int nbTrains;
    int trainsAtStation;
    PcoMutex stationMutex;
    PcoSemaphore stationSemaphore;
};

#endif // SHARED_STATION_H