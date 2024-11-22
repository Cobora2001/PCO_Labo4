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
                        int trainFirstStart, int trainSecondStart) : 
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

        int trainFirstIndex = -1;
        int trainSecondIndex = -1;

        isStartingPositionValid(trainFirstStart, trainSecondStart);

        directionIsForward = isGoingForward(trainFirstStart, trainSecondStart);

        bool sharedSectionIsCut = isSharedSectionCut();

        int sizeOfSharedSection = sharedSectionIsCut ? contacts.size() - entranceIndex + exitIndex + 1 : exitIndex - entranceIndex + 1;

        if (contacts.size() < sizeOfSharedSection + INCOMING_BUFFER + OUTGOING_BUFFER) {
            throw std::runtime_error("Invalid shared section");
        }

        determineContactPoints();

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

    void setStationContact(int contact);

    int firstIndex = getIndexOfContact(trainFirstStart);
    int secondIndex = getIndexOfContact(trainSecondStart);

    void isStartingPositionValid(int firstIndex, int secondIndex);

    bool isGoingForward(int firstIndex, int secondIndex);

    int getIndexOfContact(int contact);

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

    int entranceIndex;
    int exitIndex;

    int entrance;
    int exit;

    int nbOfTurns;

    static const int maxNbOfTurns = 10;
    static const int minNbOfTurns = 1;
};

#endif // LOCOMOTIVEBEHAVIOR_H
