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

    // Si jamais on a besoin d'avoir l'état initial de la locomotive pour du troubleshooting
    // loco.afficherMessage(this->toString());

    while(true) {
        // On mémorise la vitesse actuelle de la locomotive
        int vitesse = loco.vitesse();

        // On attend le contact suivant: soit avec la shared section, soit avec la station
        if(goingTowardsSharedSection){ // Gestion de la shared section

            // On attend le contact de la shared section (plus exactement, le point de réservation de la seciton partagée calculée via la définition du incoming buffer)
            attendre_contact(sharedSectionReserveContact);

            // À améliorer pour ne pas arrêter la locomotive si elle acquiert le contact de la shared section
            // Edit: L'arrêt en question est imperceptible dans nos tests, donc on le laisse tel quel pour l'instant

            // On arrête la locomotive
            loco.fixerVitesse(0);

            // On réserve la section partagée
            sharedSection->access(loco);
            loco.afficherMessage("Réservation de la section partagée.");

            // On dirige les aiguillages pour que la locomotive puisse entrer dans la section partagée, et en sortir
            for(auto& direction : sharedSectionDirections) {
                diriger_aiguillage(direction.first, direction.second, 0);
            }

            // On redémarre la locomotive
            loco.fixerVitesse(vitesse);

            // On affiche un message pour indiquer que la locomotive est entrée dans la section partagée (donc qu'elle est sortie du buffer)
            if(directionIsForward && isWritenForward || !directionIsForward && !isWritenForward) {
                attendre_contact(entrance);
            } else {
                attendre_contact(exit);
            }
            loco.afficherMessage("Entrée dans la section partagée.");

            // On attend le contact de sortie de la section partagée
            if(directionIsForward && isWritenForward || !directionIsForward && !isWritenForward) {
                attendre_contact(exit);
            } else {
                attendre_contact(entrance);
            }
            loco.afficherMessage("Sortie de la section partagée.");

            // On libère la section partagée une fois qu'on est assez loin, au point de release de la section partagée, calculé via la définition de l'outgoing buffer
            attendre_contact(sharedSectionReleaseContact);
            loco.afficherMessage("Libération de la section partagée.");
            sharedSection->leave(loco);

            // On définit qu'on se dirige vers la station
            goingTowardsSharedSection = false;
        }  else { // Gestion de la station

            // Attendre le contact de la station
            attendre_contact(stationContact);
            loco.afficherMessage("Arrivée à la gare.");

            // Réduire le nombre de tours restants
            --nbOfTurns;

            // Si on a fait le nombre de tours demandé (donc s'il ne reste aucun tour à faire)
            if (nbOfTurns == 0) {
                // Arrêter la locomotive
                loco.fixerVitesse(0);

                // Synchroniser avec l'autre locomotive à la gare.
                // On attend que l'autre locomotive soit aussi à la gare, puis on attend deux secondes, puis on démarre les deux locomotives dans le sens opposé
                loco.afficherMessage("Arrêt en gare. Synchronisation...");
                sharedStation->trainArrived();

                // Inverser le sens
                loco.inverserSens();
                directionIsForward = !directionIsForward;
                loco.afficherMessage("Inversion du sens.");

                // Déterminer les nouveaux points de contact (point de réservation et de libération de la section partagée)
                determineContactPoints();

                // Réinitialiser le nombre de tours restants avec un nombre aléatoire entre minNbOfTurns et maxNbOfTurns
                nbOfTurns = getRandomTurnNumber();

                loco.fixerVitesse(vitesse);
            }

            goingTowardsSharedSection = true;
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

    // La station ne doit pas être dans la zone tampon de la section partagée, dans le sens aller ou retour
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

QString LocomotiveBehavior::toString() {
    QString str = "LocomotiveBehavior : \n" +
            QString("Locomotive : %1\n").arg(loco.numero()) +
            QString("Shared section reserve contact : %1\n").arg(sharedSectionReserveContact) +
            QString("Shared section release contact : %1\n").arg(sharedSectionReleaseContact) +
            QString("Shared section directions : \n") +
            QString("Entrance index : %1\n").arg(entranceIndex) +
            QString("Exit index : %1\n").arg(exitIndex) +
            QString("Entrance : %1\n").arg(entrance) +
            QString("Exit : %1\n").arg(exit) +
            QString("Station contact : %1\n").arg(stationContact) +
            QString("Direction is forward : %1\n").arg(directionIsForward) +
            QString("Is written forward : %1\n").arg(isWritenForward) +
            QString("Going towards shared section : %1\n").arg(goingTowardsSharedSection) +
            QString("Number of turns : %1\n").arg(nbOfTurns) +
            QString("Max number of turns : %1\n").arg(maxNbOfTurns) +
            QString("Min number of turns : %1\n").arg(minNbOfTurns);
    return str;
}

int LocomotiveBehavior::getRandomTurnNumber() {
    return rand() % (maxNbOfTurns - minNbOfTurns + 1) + minNbOfTurns;
}

void LocomotiveBehavior::checkMinimalSizeOfContacts(int sizeOfSharedSection) {
    // La section partagée doit être d'au moins 2 * max(INCOMING_BUFFER, OUTGOING_BUFFER) + 1, car la station ne doit pas être dans la section partagée
    // ou la zone tampon de la section partagée non plus sur le chemin aller ou retour
    if (contacts.size() < sizeOfSharedSection + 2 * std::max(INCOMING_BUFFER, OUTGOING_BUFFER) + 1) {
        throw std::runtime_error("Invalid shared section");
    }
}

int LocomotiveBehavior::sizeSharedSection(bool sharedSectionIsCut) {

    if(sharedSectionIsCut) {
        if(directionIsForward) {
            return contacts.size() - entranceIndex + exitIndex + 1;
        } else {
            return contacts.size() - exitIndex + entranceIndex + 1;
        }
    } else {
        if(directionIsForward) {
            return exitIndex - entranceIndex + 1;
        } else {
            return entranceIndex - exitIndex + 1;
        }
    }
}