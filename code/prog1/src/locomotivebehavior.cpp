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
        // On attend le contact suivant: soit avec la shared section, soit avec la station
        if(goingTowardsSharedSection){ // Gestion de la shared section

            // On attend le contact de la shared section (plus exactement, le point de réservation de la seciton partagée calculée via la définition du incoming buffer)
            attendre_contact(sharedSectionReserveContact);

            // On réserve la section partagée
            sharedSection->access(loco);
            loco.afficherMessage("Réservation de la section partagée.");

            // On dirige les aiguillages pour que la locomotive puisse entrer dans la section partagée, et en sortir
            for(auto& direction : sharedSectionDirections) {
                diriger_aiguillage(direction.first, direction.second, 0);
            }

            // On affiche un message pour indiquer que la locomotive est entrée dans la section partagée (donc qu'elle est sortie du buffer)
            if(directionIsForward && isWrittenForward || !directionIsForward && !isWrittenForward) {
                attendre_contact(entrance);
            } else {
                attendre_contact(exit);
            }
            loco.afficherMessage("Entrée dans la section partagée.");

            // On attend le contact de sortie de la section partagée
            if(directionIsForward && isWrittenForward || !directionIsForward && !isWrittenForward) {
                attendre_contact(exit);
            } else {
                attendre_contact(entrance);
            }
            loco.afficherMessage("Sortie de la section partagée.");

            // On attend le contact de libération de la section partagée
            attendre_contact(sharedSectionReleaseContact);
            loco.afficherMessage("Libération de la section partagée.");

            // On libère la section partagée
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
                // On mémorise la vitesse actuelle de la locomotive
                int vitesse = loco.vitesse();

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

                // On redémarre la locomotive
                loco.fixerVitesse(vitesse);
            }

            // On définit qu'on se dirige vers la section partagée
            goingTowardsSharedSection = true;
        }
    }
}

void LocomotiveBehavior::printStartMessage() {
    qDebug() << "[START] Thread de la loco" << loco.numero() << "lancé";
    loco.afficherMessage("Je suis lancée !");
}

void LocomotiveBehavior::printCompletionMessage() {
    qDebug() << "[STOP] Thread de la loco" << loco.numero() << "a terminé correctement";
    loco.afficherMessage("J'ai terminé");
}

void LocomotiveBehavior::determineContactPoints() {
    // Variables temporaires pour les index d'entrée et de sortie
    int targetIndexEntry;
    int targetIndexExit;

    // On détermine les points de réservation et de libération de la section partagée
    if(isWrittenForward) {
        if(directionIsForward) { // Si la locomotive va en avant et que la section partagée est écrite de gauche à droite
            targetIndexEntry = entranceIndex - INCOMING_BUFFER; // On recule l'indexe depuis notre point d'entrée, qui sert bien d'entrée dans cette configuration
            targetIndexExit  = exitIndex     + OUTGOING_BUFFER; // On avance l'indexe depuis notre point de sortie, qui sert bien de sortie dans cette configuration
        } else { // Si la locomotive va en arrière et que la section partagée est écrite de gauche à droite
            targetIndexEntry = exitIndex     + INCOMING_BUFFER; // On avance l'indexe depuis notre point de sortie, qui sert d'enrée si on va en arrière
            targetIndexExit  = entranceIndex - OUTGOING_BUFFER; // On recule l'indexe depuis notre point d'entrée, qui sert de sortie si on va en arrière
        }
    } else {
        if(directionIsForward) { // Si la locomotive va en avant et que la section partagée est écrite de droite à gauche
            targetIndexEntry = exitIndex     - INCOMING_BUFFER; // On recule l'indexe depuis notre point de sortie, qui sert d'entrée si on va en avant alors que la section est écrite de droite à gauche
            targetIndexExit  = entranceIndex + OUTGOING_BUFFER; // On avance l'indexe depuis notre point d'entrée, qui sert de sortie si on va en avant alors que la section est écrite de droite à gauche
        } else { // Si la locomotive va en arrière et que la section partagée est écrite de droite à gauche
            targetIndexEntry = entranceIndex + INCOMING_BUFFER; // On avance l'indexe depuis notre point d'entrée, qui sert bien d'entrée dans cette configuration, même si on va en arrière
            targetIndexExit  = exitIndex     - OUTGOING_BUFFER; // On recule l'indexe depuis notre point de sortie, qui sert bien de sortie dans cette configuration, même si on va en arrière

        }
    }

    // On s'assure que les index sont dans les limites du vecteur
    targetIndexEntry = (targetIndexEntry + contacts.size()) % contacts.size();
    targetIndexExit  = (targetIndexExit  + contacts.size()) % contacts.size();

    // On assigne les contacts de réservation et de libération de la section partagée
    sharedSectionReserveContact = contacts[targetIndexEntry];
    sharedSectionReleaseContact = contacts[targetIndexExit];
}

void LocomotiveBehavior::calculateEntranceAndExitIndexes() {
    // On calcule les index d'entrée et de sortie de la section partagée
    entranceIndex = getIndexOfContact(entrance);
    exitIndex = getIndexOfContact(exit);

    // On vérifie que les contacts sont valides
    // Ils ne peuvent pas être égaux l'un à l'autre
    if(entranceIndex == -1 || exitIndex == -1 || entranceIndex == exitIndex) {
        throw std::runtime_error("Invalid contacts");
    }
}

void LocomotiveBehavior::setStationContact(int contact) {
    // On obtient l'index du contact de la station
    int stationIndex = getIndexOfContact(contact);

    // On vérifie que le contact est dans la liste des contacts
    if(stationIndex == -1) {
        throw std::runtime_error("Invalid station contact");
    }

    // La station ne doit pas être dans la section partagée ou son buffer (dans notre solution), et pour cela on doit savoir si la section partagée est coupée
    bool sharedSectionIsCut = isSharedSectionCut();

    // On prépare un flag
    bool stationError;

    // On vérifie si la station est dans la section partagée
    if (isWrittenForward) {
        stationError = sharedSectionIsCut 
                ? (stationIndex > entranceIndex || stationIndex < exitIndex)  // Si la section partagée est coupée et qu'on a écrit de gauche à droite la section partagée, alors on vérifie les bornes des contacts
                : (stationIndex > entranceIndex && stationIndex < exitIndex); // Sinon, on vérifie que la station est bien entre les bornes de la section partagée, écrite de gauche à droite
    } else {
        stationError = sharedSectionIsCut 
                ? (stationIndex < entranceIndex || stationIndex > exitIndex)  // Si la section partagée est coupée et qu'on a écrit de droite à gauche la section partagée, alors on vérifie les bornes des contacts, mais dans l'autre sens
                : (stationIndex < entranceIndex && stationIndex > exitIndex); // Sinon, on vérifie que la station est bien entre les bornes de la section partagée, écrite de droite à gauche
    }

    // On lance une exception si la station est dans la section partagée
    if (stationError) {
        throw std::runtime_error("Invalid station contact");
    }

    // La station ne doit pas être dans la zone tampon de la section partagée, dans le sens aller ou retour
    // On vérifie que la station n'est pas dans la zone tampon de la section partagée en avançant ou reculant dans la liste des contacts, selon le sens de la section partagée
    if(isWrittenForward) {
        for(int i = 1; i <= std::max(INCOMING_BUFFER, OUTGOING_BUFFER); ++i) { // Vu qu'on veut l'aller-retour, on prend le max des deux buffers
            if(contacts[(stationIndex - i + contacts.size()) % contacts.size()] == exit) {
                throw std::runtime_error("Invalid station contact");
            }
            if(contacts[(stationIndex + i) % contacts.size()] == entrance) {
                throw std::runtime_error("Invalid station contact");
            }
        }
    } else {
        for(int i = 1; i <= std::max(INCOMING_BUFFER, OUTGOING_BUFFER); ++i) {
            if(contacts[(stationIndex + i) % contacts.size()] == exit) {
                throw std::runtime_error("Invalid station contact");
            }
            if(contacts[(stationIndex - i + contacts.size()) % contacts.size()] == entrance) {
                throw std::runtime_error("Invalid station contact");
            }
        }
    }

    // On fixe le contact de la station si tout est bon
    stationContact = contact;
}

bool LocomotiveBehavior::isSharedSectionCut() {
    return (entranceIndex > exitIndex) && isWrittenForward || (entranceIndex < exitIndex) && !isWrittenForward;
}

bool LocomotiveBehavior::isGoingForward(int firstIndex, int secondIndex) {
    return firstIndex < secondIndex;
}

void LocomotiveBehavior::isStartingPositionValid(int firstIndex, int secondIndex) {
    // On vérifie que les index sont valides
    // Ils ne peuvent pas être égaux l'un à l'autre, ni égaux à l'entrée ou à la sortie de la section partagée
    if(firstIndex == -1 || secondIndex == -1 || firstIndex == secondIndex || firstIndex == entranceIndex || firstIndex == exitIndex || secondIndex == entranceIndex || secondIndex == exitIndex) {
        throw std::runtime_error("Invalid starting position");
    }

    // On vérifie que les index sont consécutifs
    if(abs(firstIndex - secondIndex) != 1) {
        throw std::runtime_error("Invalid starting position");
    }

    // On obtient l'informationd de si la section partagée est coupée
    bool sharedSectionIsCut = isSharedSectionCut();

    // On vérifie qu'on ne commence pas dans la section partagée (ni le contact juste avant la position de départ de la locomotive, ni le contact juste après)
    if(isWrittenForward) {
        if(sharedSectionIsCut) { // Si la section partagée est coupée dans la liste des contacts et que la section partagée est écrite de gauche à droite
            if(firstIndex > entranceIndex || firstIndex < exitIndex || secondIndex > entranceIndex || secondIndex < exitIndex) {
                throw std::runtime_error("Invalid starting position");
            }
        } else { // Si la section partagée n'est pas coupée dans la liste des contacts et que la section partagée est écrite de gauche à droite
            if(firstIndex > entranceIndex && firstIndex < exitIndex || secondIndex > entranceIndex && secondIndex < exitIndex) {
                throw std::runtime_error("Invalid starting position");
            }
        }
    } else {
        if(sharedSectionIsCut) { // Si la section partagée est coupée dans la liste des contacts et que la section partagée est écrite de droite à gauche
            if(firstIndex < entranceIndex || firstIndex > exitIndex || secondIndex < entranceIndex || secondIndex > exitIndex) {
                throw std::runtime_error("Invalid starting position");
            }
        } else { // Si la section partagée n'est pas coupée dans la liste des contacts et que la section partagée est écrite de droite à gauche
            if(firstIndex < entranceIndex && firstIndex > exitIndex || secondIndex < entranceIndex && secondIndex > exitIndex) {
                throw std::runtime_error("Invalid starting position");
            }
        }
    }

    // On ne peut aussi pas être dans la zone tampon de la section partagée
    // On va itérer à travers les contacts pour la distance du buffer depuis le point de départ du train pour vérifier que le train n'est pas dans la zone tampon
    if(directionIsForward) {
        if(isWrittenForward) { // Si la section partagée est écrite de gauche à droite dans la liste des contacts et que la locomotive va en avant (de gauche à droite dans sa liste de contacts)
            // On vérifie que la locomotive n'est pas dans la zone tampon de la section partagée
            // en avançant dans la liste des contacts depuis le contact juste devant la locomotive
            // et en vérifiant qu'on n'entre pas dans la section partagée (selon la taille du buffer)
            for(int i = 1; i < INCOMING_BUFFER; ++i) {
                if(contacts[(secondIndex + i) % contacts.size()] == entrance) {
                    throw std::runtime_error("Invalid starting position");
                }
            }
            // Même chose ici, mais en reculant dans la liste des contacts depuis le contact juste derrière la locomotive
            for(int i = 1; i < OUTGOING_BUFFER; ++i) {
                if(contacts[(firstIndex - i + contacts.size()) % contacts.size()] == exit) {
                    throw std::runtime_error("Invalid starting position");
                }
            }
        } else { // On effectue le même type de vérification, mais pour le cas où la section partagée est écrite de droite à gauche dans la liste des contacts, mais que la locomotive va en avant
            for(int i = 1; i < INCOMING_BUFFER; ++i) {
                if(contacts[(secondIndex + i) % contacts.size()] == exit) {
                    throw std::runtime_error("Invalid starting position");
                }
            }
            for(int i = 1; i < OUTGOING_BUFFER; ++i) {
                if(contacts[(firstIndex - i + contacts.size()) % contacts.size()] == entrance) {
                    throw std::runtime_error("Invalid starting position");
                }
            }
        }
    } else { // On effectue le même type de vérification, mais pour le cas où la locomotive va en arrière, donc en sens inverse de la liste des contacts, mais que la section partagée est écrite de gauche à droite
        if(isWrittenForward) {
            for(int i = 1; i < INCOMING_BUFFER; ++i) {
                if(contacts[(secondIndex - i + contacts.size()) % contacts.size()] == exit) {
                    throw std::runtime_error("Invalid starting position");
                }
            }
            for(int i = 1; i < OUTGOING_BUFFER; ++i) {
                if(contacts[(firstIndex + i) % contacts.size()] == entrance) {
                    throw std::runtime_error("Invalid starting position");
                }
            }
        } else { // On effectue le même type de vérification, mais pour le cas où la section partagée est écrite de droite à gauche dans la liste des contacts, et que la locomotive va en arrière
            for(int i = 1; i < INCOMING_BUFFER; ++i) {
                if(contacts[(secondIndex - i + contacts.size()) % contacts.size()] == entrance) {
                    throw std::runtime_error("Invalid starting position");
                }
            }
            for(int i = 1; i < OUTGOING_BUFFER; ++i) {
                if(contacts[(firstIndex + i) % contacts.size()] == exit) {
                    throw std::runtime_error("Invalid starting position");
                }
            }
        }
    }
}

int LocomotiveBehavior::getIndexOfContact(int contact) {
    // On obtient l'index du contact dans la liste des contacts
    auto it = std::find(contacts.begin(), contacts.end(), contact);

    // On retourne l'index du contact, ou -1 si le contact n'est pas dans la liste
    int index = -1;

    if(it != contacts.end()) {
        index = std::distance(contacts.begin(), it);
    }

    return index;
}

void LocomotiveBehavior::setNextDestination(int secondStartIndex) {
    // On détermine si la locomotive va vers la section partagée ou vers la station en premier

    // On donne une valeur par défaut à goingTowardsSharedSection
    goingTowardsSharedSection = true;

    // On part de l'index du contact juste devant la locomotive
    int i = secondStartIndex;

    // On cherche l'indexe du contact de l'endroit par lequel on va effectivement entre dans la section partagée
    int targetEntrance;

    // On détermine le contact de l'endroit par lequel on va effectivement entrer dans la section partagée
    if(isWrittenForward) {
        if(directionIsForward) { // Si la section partagée est écrite de gauche à droite dans la liste des contacts et que la locomotive va en avant
            targetEntrance = entrance;
        } else { // Si la section partagée est écrite de gauche à droite dans la liste des contacts et que la locomotive va en arrière
            targetEntrance = exit;
        }
    } else { // Si la section partagée est écrite de droite à gauche dans la liste des contacts et que la locomotive va en avant
        if(directionIsForward) {
            targetEntrance = exit;
        } else { // Si la section partagée est écrite de droite à gauche dans la liste des contacts et que la locomotive va en arrière
            targetEntrance = entrance;
        }
    }

    // On cherche si on va d'abord impacter la section partagée ou la station
    while(true) {
        int j = contacts[i];
        if(j == targetEntrance) { // Si on arrive à l'entrée de la section partagée
            goingTowardsSharedSection = true;
            break;
        }

        if(j == stationContact) { // Si on arrive à la station
            goingTowardsSharedSection = false;
            break;
        }

        // On avance dans la liste des contacts
        i = (directionIsForward ? i + 1 : i - 1 + contacts.size()s) % contacts.size();
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
            QString("Is written forward : %1\n").arg(isWrittenForward) +
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

    // On calcule la taille de la section partagée
    if(sharedSectionIsCut) {
        if(isWrittenForward) { // Si la section partagée est coupée et que la section partagée est écrite de gauche à droite
            return contacts.size() - entranceIndex + exitIndex + 1;
        } else { // Si la section partagée est coupée et que la section partagée est écrite de droite à gauche
            return contacts.size() - exitIndex + entranceIndex + 1;
        }
    } else {
        if(isWrittenForward) { // Si la section partagée n'est pas coupée et que la section partagée est écrite de gauche à droite
            return exitIndex - entranceIndex + 1;
        } else { // Si la section partagée n'est pas coupée et que la section partagée est écrite de droite à gauche
            return entranceIndex - exitIndex + 1;
        }
    }
}