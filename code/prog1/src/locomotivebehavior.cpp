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
        int vitesse = loco.vitesse();

        if(goingTowardsSharedSection){
            attendre_contact(sharedSectionReserveContact);

            // À améliorer pour ne pas arrêter la locomotive si elle acquiert le contact de la shared section

            loco.fixerVitesse(0);

            sharedSection->access(loco);
            loco.afficherMessage("Entrée dans la section partagée.");

            for(auto& direction : sharedSectionDirections) {
                diriger_aiguillage(direction.first, direction.second, 0);
            }

            loco.fixerVitesse(vitesse);

            attendre_contact(sharedSectionReleaseContact);
            loco.afficherMessage("Sortie de la section partagée.");
        
            sharedSection->leave(loco);

            goingTowardsSharedSection = false;
        }  else {
            // Gestion de la station

            // Attendre le contact de la station
            attendre_contact(stationContact);
            loco.afficherMessage("Arrivée à la gare.");

            // Réduire le nombre de tours restants
            --nbOfTurns;

            if (nbOfTurns == 0) {
                // Arrêter la locomotive
                loco.fixerVitesse(0);

                // Synchroniser avec l'autre locomotive à la gare
                sharedStation->trainArrived();
                loco.afficherMessage("Arrêt en gare. Synchronisation...");

                // Inverser le sens
                loco.inverserSens();
                directionIsForward = !directionIsForward;
                loco.afficherMessage("Inversion du sens.");

                // Réinitialiser le nombre de tours
                nbOfTurns = rand() % (maxNbOfTurns - minNbOfTurns + 1) + minNbOfTurns;

                // Reconfigurer les contacts
                setNextDestination(getIndexOfContact(stationContact));

                loco.fixerVitesse(vitesse);
            }
        }
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

    // FIXME : La station peut actuellement être dans la zone où on aura déjà réservé la zone partagée, pouvant donc mener à un interblocage

    int stationIndex = getIndexOfContact(contact);

    if(stationIndex == -1) {
        throw std::runtime_error("Invalid station contact 1");
    }

    bool sharedSectionIsCut = isSharedSectionCut();

    bool stationError;

    if (isWritenForward) {
        stationError = sharedSectionIsCut 
                ? (stationIndex > entranceIndex || stationIndex < exitIndex)
                : (stationIndex > entranceIndex && stationIndex < exitIndex);
    } else {
        stationError = sharedSectionIsCut 
                ? (stationIndex < entranceIndex || stationIndex > exitIndex)
                : (stationIndex < entranceIndex && stationIndex > exitIndex);
    }

    if (stationError) {
        throw std::runtime_error("Invalid station contact 2");
    }

    // Station can't be in the buffer zone of the shared section (this time it needs to be in either direction, meaning we must take the max of the incoming and outgoing buffer)
    if(isWritenForward) {
        for(int i = 1; i <= std::max(INCOMING_BUFFER, OUTGOING_BUFFER); ++i) {
            if(contacts[(stationIndex - i + contacts.size()) % contacts.size()] == exit) {
                throw std::runtime_error("Invalid station contact 3");
            }
            if(contacts[(stationIndex + i) % contacts.size()] == entrance) {
                throw std::runtime_error("Invalid station contact 4");
            }
        }
    } else {
        for(int i = 1; i <= std::max(INCOMING_BUFFER, OUTGOING_BUFFER); ++i) {
            if(contacts[(stationIndex + i) % contacts.size()] == exit) {
                throw std::runtime_error("Invalid station contact 5");
            }
            if(contacts[(stationIndex - i + contacts.size()) % contacts.size()] == entrance) {
                throw std::runtime_error("Invalid station contact 6");
            }
        }
    }

    stationContact = contact;
}

bool LocomotiveBehavior::isSharedSectionCut() {
    return (entranceIndex > exitIndex) && isWritenForward || (entranceIndex < exitIndex) && !isWritenForward;
}

bool LocomotiveBehavior::isGoingForward(int firstIndex, int secondIndex) {
    return firstIndex < secondIndex;
}

void LocomotiveBehavior::isStartingPositionValid(int firstIndex, int secondIndex) {
    
    if(firstIndex == -1 || secondIndex == -1 || firstIndex == secondIndex || firstIndex == entranceIndex || firstIndex == exitIndex || secondIndex == entranceIndex || secondIndex == exitIndex) {
        throw std::runtime_error("Invalid starting position 1");
    }

    if(abs(firstIndex - secondIndex) != 1) {
        throw std::runtime_error("Invalid starting position 2");
    }

    bool sharedSectionIsCut = isSharedSectionCut();

    if(isWritenForward) {
        if(sharedSectionIsCut) {
            if(firstIndex > entranceIndex || firstIndex < exitIndex || secondIndex > entranceIndex || secondIndex < exitIndex) {
                throw std::runtime_error("Invalid starting position 3");
            }
        } else {
            if(firstIndex > entranceIndex && firstIndex < exitIndex || secondIndex > entranceIndex && secondIndex < exitIndex) {
                throw std::runtime_error("Invalid starting position 4");
            }
        }
    } else {
        if(sharedSectionIsCut) {
            if(firstIndex < entranceIndex || firstIndex > exitIndex || secondIndex < entranceIndex || secondIndex > exitIndex) {
                throw std::runtime_error("Invalid starting position 5");
            }
        } else {
            if(firstIndex < entranceIndex && firstIndex > exitIndex || secondIndex < entranceIndex && secondIndex > exitIndex) {
                throw std::runtime_error("Invalid starting position 6");
            }
        }
    }

    // We also can't be in the sector of the buffer zone
    // Wel'll iterate through the contacts for the distance of the buffer from the starting point of the train to check that the train is not in the buffer zone
    if(directionIsForward) {
        if(isWritenForward) {
            for(int i = 1; i < INCOMING_BUFFER; ++i) {
                if(contacts[(secondIndex + i) % contacts.size()] == entrance) {
                    throw std::runtime_error("Invalid starting position 7");
                }
            }
            for(int i = 1; i < OUTGOING_BUFFER; ++i) {
                if(contacts[(firstIndex - i + contacts.size()) % contacts.size()] == exit) {
                    throw std::runtime_error("Invalid starting position 8");
                }
            }
        } else {
            for(int i = 1; i < INCOMING_BUFFER; ++i) {
                if(contacts[(secondIndex + i) % contacts.size()] == exit) {
                    throw std::runtime_error("Invalid starting position 9");
                }
            }
            for(int i = 1; i < OUTGOING_BUFFER; ++i) {
                if(contacts[(firstIndex - i + contacts.size()) % contacts.size()] == entrance) {
                    throw std::runtime_error("Invalid starting position 10");
                }
            }
        }
    } else {
        if(isWritenForward) {
            for(int i = 1; i < INCOMING_BUFFER; ++i) {
                if(contacts[(secondIndex - i + contacts.size()) % contacts.size()] == exit) {
                    throw std::runtime_error("Invalid starting position 11");
                }
            }
            for(int i = 1; i < OUTGOING_BUFFER; ++i) {
                if(contacts[(firstIndex + i) % contacts.size()] == entrance) {
                    throw std::runtime_error("Invalid starting position 12");
                }
            }
        } else {
            for(int i = 1; i < INCOMING_BUFFER; ++i) {
                if(contacts[(secondIndex - i + contacts.size()) % contacts.size()] == entrance) {
                    throw std::runtime_error("Invalid starting position 13");
                }
            }
            for(int i = 1; i < OUTGOING_BUFFER; ++i) {
                if(contacts[(firstIndex + i) % contacts.size()] == exit) {
                    throw std::runtime_error("Invalid starting position 14");
                }
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

void LocomotiveBehavior::setNextDestination(int secondStartIndex) {
    goingTowardsSharedSection = true;

    int i = secondStartIndex;

    int targetEntrance;

    if(isWritenForward) {
        if(directionIsForward) {
            targetEntrance = entranceIndex;
        } else {
            targetEntrance = exitIndex;
        }
    } else {
        if(directionIsForward) {
            targetEntrance = exitIndex;
        } else {
            targetEntrance = entranceIndex;
        }
    }

    while(true) {
        if(i == targetEntrance) {
            goingTowardsSharedSection = true;
            break;
        }

        if(i == stationContact) {
            goingTowardsSharedSection = false;
            break;
        }

        i = (i + 1) % contacts.size();
    }

}
