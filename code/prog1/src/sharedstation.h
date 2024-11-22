#ifndef SHARED_STATION_H
#define SHARED_STATION_H

#include <pcosynchro/pcosemaphore.h>

class SharedStation
{
public:
    SharedStation(int nbTrains, int nbTours);

    /* Implémentez toute la logique que vous avez besoin pour que les locomotives
     * s'attendent correctement à la station */
    void trainArrived();

private:
    /* TODO */
    int nbTrains;
    int trainsAtStation;
    std::mutex stationMutex;
    PcoSemaphore stationSemaphore{0};
};

#endif // SHARED_STATION_H