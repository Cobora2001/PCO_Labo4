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
    LocomotiveBehavior(Locomotive& loco, std::shared_ptr<SharedSectionInterface> sharedSection, std::vector<Pair<int, int>> sharedSectionDirections), std::vector<int> contacts, int stationContact
        : loco(loco), sharedSection(sharedSection), sharedSectionDirections(sharedSectionDirections), contacts(contacts), stationContact(stationContact) {
        int entrance = sharedSection.getEntrance();
        int exit = sharedSection.getExit();

        // Find the indeex of the shared section in the contacts
        auto it = std::find(contacts.begin(), contacts.end(), stationContact);
        int stationIndex = -1;
        if(it != contacts.end()) {
            stationIndex = std::distance(contacts.begin(), it);
        }

        auto it2 = std::find(contacts.begin(), contacts.end(), entrance);
        int entranceIndex = -1;
        if(it2 != contacts.end()) {
            entranceIndex = std::distance(contacts.begin(), it2);
        }

        auto it3 = std::find(contacts.begin(), contacts.end(), exit);
        int exitIndex = -1;
        if(it3 != contacts.end()) {
            exitIndex = std::distance(contacts.begin(), it3);
        }

        if(stationIndex == -1 || entranceIndex == -1 || exitIndex == -1 || entranceIndex == exitIndex) {
            throw std::runtime_error("Invalid contacts");
        }

        directionIsForward = entranceIndex < exitIndex;


    }

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
    std::vector<Pair<int, int>> sharedSectionDirections;
    bool directionIsForward = true;

    int entranceIndex;
    int exitIndex;
};

#endif // LOCOMOTIVEBEHAVIOR_H
