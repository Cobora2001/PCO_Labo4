//    ___  _________    ___  ___  ___ ____ //
//   / _ \/ ___/ __ \  |_  |/ _ \|_  / / / //
//  / ___/ /__/ /_/ / / __// // / __/_  _/ //
// /_/   \___/\____/ /____/\___/____//_/   //
//

#ifndef LOCOMOTIVEBEHAVIOR_H
#define LOCOMOTIVEBEHAVIOR_H

#include "locomotive.h"
#include "launchable.h"
#include "sharedsectioninterface.h"

#include <vector>
#include <utility>

#define INCOMING_BUFFER 2
#define OUTGOING_BUFFER 1

/**
 * @brief La classe LocomotiveBehavior représente le comportement d'une locomotive
 */
class LocomotiveBehavior : public Launchable
{
public:
    /*!
     * \brief locomotiveBehavior Constructeur de la classe
     * \param loco la locomotive dont on représente le comportement
     */
    LocomotiveBehavior(Locomotive& loco, std::shared_ptr<SharedSectionInterface> sharedSection, 
                        std::vector<std::pair<int, int>> sharedSectionDirections, 
                        bool isWrittenForward, 
                        std::vector<int> contacts,
                        int entrance, int exit,
                        int trainFirstStart, int trainSecondStart,
                        int stationContact) : 
        loco(loco), 
        sharedSection(sharedSection), 
        sharedSectionDirections(sharedSectionDirections), 
        contacts(contacts), isWritenForward(isWrittenForward),  
        entrance(entrance), exit(exit) {

        // Find the indeex of the shared section in the contacts
        auto it = std::find(contacts.begin(), contacts.end(), stationContact);
        int stationIndex = -1;
        if(it != contacts.end()) {
            stationIndex = std::distance(contacts.begin(), it);
        }

        calculateEntranceAndExitIndexes();

        int trainFirstIndex = getIndexOfContact(trainFirstStart);
        int trainSecondIndex = getIndexOfContact(trainSecondStart);

        directionIsForward = isGoingForward(trainFirstIndex, trainSecondIndex);

        isStartingPositionValid(trainFirstIndex, trainSecondIndex);

        bool sharedSectionIsCut = isSharedSectionCut();

        int sizeOfSharedSection = sharedSectionIsCut ? contacts.size() - entranceIndex + exitIndex + 1 : exitIndex - entranceIndex + 1;

        // The shared section must be at least 2 * max(INCOMING_BUFFER, OUTGOING_BUFFER) + 1, because the station mustn't be in the shared section
        // or the buffer zone of the shared section either on the way forward or backward
        if (contacts.size() < sizeOfSharedSection + 2 * max(INCOMING_BUFFER, OUTGOING_BUFFER) + 1) {
            throw std::runtime_error("Invalid shared section");
        }

        determineContactPoints();

        setStationContact(stationContact);

        setNextDestination(trainSecondIndex);

        // Select a random number of turns between min and max
        nbOfTurns = rand() % (maxNbOfTurns - minNbOfTurns + 1) + minNbOfTurns;
        
    }

    bool isSharedSectionCut();

protected:
    /*!
     * \brief run Fonction lancée par le thread, représente le comportement de la locomotive
     */
    void run() override;

    /*!
     * \brief printStartMessage Message affiché lors du démarrage du thread
     */
    void printStartMessage() override;

    /*!
     * \brief printCompletionMessage Message affiché lorsque le thread a terminé
     */
    void printCompletionMessage() override;

    /*!
     * \brief determineContactPoints Détermine les points de contact de la locomotive avec la shared section selon la direction de la locomotive
    */
    void determineContactPoints();

    void calculateEntranceAndExitIndexes();

    void isStartingPositionValid(int firstIndex, int secondIndex);

    bool isGoingForward(int firstIndex, int secondIndex);

    int getIndexOfContact(int contact);

    void setNextDestination(int secondStartIndex);

    void setStationContact(int contact);

    /**
     * @brief loco La locomotive dont on représente le comportement
     */
    Locomotive& loco;

    /**
     * @brief sharedSection Pointeur sur la section partagée
     */
    std::shared_ptr<SharedSectionInterface> sharedSection;

    std::vector<int> contacts;
    int stationContact;
    int sharedSectionReserveContact;
    int sharedSectionReleaseContact;
    std::vector<std::pair<int, int>> sharedSectionDirections;

    bool directionIsForward;
    bool isWritenForward;
    bool goingTowardsSharedSection; // If false, it means the next destination is the station

    int entranceIndex;
    int exitIndex;

    int entrance;
    int exit;

    int nbOfTurns;

    static const int maxNbOfTurns = 10;
    static const int minNbOfTurns = 1;
};

#endif // LOCOMOTIVEBEHAVIOR_H
