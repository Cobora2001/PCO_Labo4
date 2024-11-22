//    ___  _________    ___  ___  ___ ____ //
//   / _ \/ ___/ __ \  |_  |/ _ \|_  / / / //
//  / ___/ /__/ /_/ / / __// // / __/_  _/ //
// /_/   \___/\____/ /____/\___/____//_/   //
//

#include "locomotivebehavior.h"
#include "ctrain_handler.h"

void LocomotiveBehavior::run()
{
    //Initialisation de la locomotive
    loco.allumerPhares();
    loco.demarrer();
    loco.afficherMessage("Ready!");

    /* A vous de jouer ! */

    // Vous pouvez appeler les méthodes de la section partagée comme ceci :
    //sharedSection->access(loco);
    //sharedSection->leave(loco);

    while(true) {
        // On attend qu'une locomotive arrive sur le contact 1.
        // Pertinent de faire ça dans les deux threads? Pas sûr...
        attendre_contact(1);
        loco.afficherMessage("J'ai atteint le contact 1");
    }
}

void LocomotiveBehavior::printStartMessage()
{
    qDebug() << "[START] Thread de la loco" << loco.numero() << "lancé";
    loco.afficherMessage("Je suis lancée !");
}

void LocomotiveBehavior::printCompletionMessage()
{
    qDebug() << "[STOP] Thread de la loco" << loco.numero() << "a terminé correctement";
    loco.afficherMessage("J'ai terminé");
}

void LocomotiveBehavior::determineContactPoints() {
    int targetIndexEntry;
    int targetIndexExit;


    if(isWritenForward) {
        if(directionIsForward) {
            targetIndexEntry = entranceIndex - INCOMING_BUFFER;
            targetIndexExit  = exitIndex     + OUTGOING_BUFFER;
        } else {
            targetIndexEntry = exitIndex     + INCOMING_BUFFER;
            targetIndexExit  = entranceIndex - OUTGOING_BUFFER;
        }
    } else {
        if(directionIsForward) {
            targetIndexEntry = exitIndex     - INCOMING_BUFFER;
            targetIndexExit  = entranceIndex + OUTGOING_BUFFER;
        } else {
            targetIndexEntry = entranceIndex + INCOMING_BUFFER;
            targetIndexExit  = exitIndex     - OUTGOING_BUFFER;

        }
    }


    targetIndexEntry = (targetIndexEntry + contacts.size()) % contacts.size();
    targetIndexExit  = (targetIndexExit  + contacts.size()) % contacts.size();


    sharedSectionReserveContact = contacts[targetIndexEntry];
    sharedSectionReleaseContact = contacts[targetIndexExit];

}

void LocomotiveBehavior::calculateEntranceAndExitIndexes() {
    entranceIndex = getIndexOfContact(entrance);
    exitIndex = getIndexOfContact(exit);

    if(entranceIndex == -1 || exitIndex == -1 || entranceIndex == exitIndex) {
        throw std::runtime_error("Invalid contacts");
    }
}

void LocomotiveBehavior::setStationContact(int contact) {
    int stationIndex = getIndexOfContact(contact);

    if(stationIndex == -1) {
        throw std::runtime_error("Invalid station contact");
    }

    bool sharedSectionIsCut = isSharedSectionCut();

    bool stationError;

    if (isWritenForward) {
        stationError = sharedSectionIsCut 
                ? (stationIndex > entranceIndex || stationIndex < exitIndex)
                : (stationIndex < entranceIndex || stationIndex > exitIndex);
    } else {
        stationError = sharedSectionIsCut 
                ? (stationIndex < entranceIndex || stationIndex > exitIndex)
                : (stationIndex > entranceIndex || stationIndex < exitIndex);
    }

    if (stationError) {
        throw std::runtime_error("Invalid station contact");
    }

    stationContact = contact;
}

bool LocomotiveBehavior::isSharedSectionCut() {
    return (entranceIndex > exitIndex) && !isWritenForward || (entranceIndex < exitIndex) && isWritenForward;
}

bool LocomotiveBehavior::isGoingForward(int firstIndex, int secondIndex) {
    return firstIndex < secondIndex;
}

void LocomotiveBehavior::isStartingPositionValid(int firstIndex, int secondIndex) {

    if(firstIndex == -1 || secondIndex == -1 || firstIndex == secondIndex || firstIndex == entranceIndex || firstIndex == exitIndex || secondIndex == entranceIndex || secondIndex == exitIndex) {
        throw std::runtime_error("Invalid starting position");
    }

    bool sharedSectionIsCut = isSharedSectionCut();

    if(isWritenForward) {
        if(sharedSectionIsCut) {
            if(firstIndex > entranceIndex || firstIndex < exitIndex || secondIndex > entranceIndex || secondIndex < exitIndex) {
                throw std::runtime_error("Invalid starting position");
            }
        } else {
            if(firstIndex > entranceIndex && firstIndex < exitIndex || secondIndex > entranceIndex && secondIndex < exitIndex) {
                throw std::runtime_error("Invalid starting position");
            }
        }
    } else {
        if(sharedSectionIsCut) {
            if(firstIndex < entranceIndex || firstIndex > exitIndex || secondIndex < entranceIndex || secondIndex > exitIndex) {
                throw std::runtime_error("Invalid starting position");
            }
        } else {
            if(firstIndex < entranceIndex && firstIndex > exitIndex || secondIndex < entranceIndex && secondIndex > exitIndex) {
                throw std::runtime_error("Invalid starting position");
            }
        }
    }

}

int LocomotiveBehavior::getIndexOfContact(int contact) {
    auto it = std::find(contacts.begin(), contacts.end(), contact);
    int index = -1;
    if(it != contacts.end()) {
        index = std::distance(contacts.begin(), it);
    }

    return index;
}
