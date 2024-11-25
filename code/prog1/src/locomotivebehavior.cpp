//    ___  _________    ___  ___  ___ ____ //
//   / _ \/ ___/ __ \  |_  |/ _ \|_  / / / //
//  / ___/ /__/ /_/ / / __// // / __/_  _/ //
// /_/   \___/\____/ /____/\___/____//_/   //
//

#include "locomotivebehavior.h"
#include "ctrain_handler.h"

LocomotiveBehavior::LocomotiveBehavior(Locomotive& loco, std::shared_ptr<SharedSectionInterface> sharedSection, 
                        std::vector<std::pair<int, int>> sharedSectionDirections, 
                        bool isWrittenForward, 
                        std::vector<int> contacts,
                        int entrance, int exit,
                        int trainFirstStart, int trainSecondStart,
                        int stationContact,
                        std::shared_ptr<SharedStation> sharedStation) : 
        loco(loco), 
        sharedSection(sharedSection), 
        sharedSectionDirections(sharedSectionDirections), 
        contacts(contacts), isWrittenForward(isWrittenForward),  
        entrance(entrance), exit(exit), sharedStation(sharedStation) {

    // Initialisation des indices d'entrée et de sortie de la section partagée
    calculateEntranceAndExitIndexes();

    // Index des contacts de démarrage
    int trainFirstIndex = getIndexOfContact(trainFirstStart);
    int trainSecondIndex = getIndexOfContact(trainSecondStart);

    // Détermine si la locomotive va en avant ou en arrière, 
    // en assumant que les positions de départ sont valides 
    // (on vérifie ça plus tard, et on a besoin de cette information pour la suite)
    directionIsForward = isGoingForward(trainFirstIndex, trainSecondIndex);

    // Vérifie si la position de départ est valide
    isStartingPositionValid(trainFirstIndex, trainSecondIndex);

    // Vérifie si la section partagée est coupée (dans la liste des contacts)
    bool sharedSectionIsCut = isSharedSectionCut();

    // Détermine si la locomotive va vers la section partagée ou vers la station
    int sizeOfSharedSection = sizeSharedSection(sharedSectionIsCut);

    // Vérifie que la trajectoire de la locomotive est assez grande pour avoir de la place pour
    // la section partagée, son buffer, la position initial de la locomotive et la station
    checkMinimalSizeOfContacts(sizeOfSharedSection);

    // Détermine les points de réserve et de libération de la section partagée
    determineContactPoints();

    // Détermine le contact de la station
    setStationContact(stationContact);

    // Détermine si la locomotive va vers la section partagée ou vers la station
    setNextDestination(trainSecondIndex);

    // Sélectionne un nombre aléatoire de tours à effectuer
    nbOfTurns = getRandomTurnNumber();

    // Affiche l'état initial de la locomotive pour les tests
    // loco.afficherMessage(toString());
}


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

            // On attend le contact de la shared section (plus exactement, 
            // le point de réservation de la seciton partagée calculée via la définition du incoming buffer)
            attendre_contact(sharedSectionReserveContact);

            // On réserve la section partagée
            sharedSection->access(loco);
            loco.afficherMessage("Shared section requested.");

            // On dirige les aiguillages pour que la locomotive puisse entrer dans la section partagée, et en sortir
            for(auto& direction : sharedSectionDirections) {
                diriger_aiguillage(direction.first, direction.second, 0);
            }

            // On affiche un message pour indiquer que la locomotive est entrée dans la section partagée 
            // (donc qu'elle est sortie du buffer)
            if(directionIsForward && isWrittenForward || !directionIsForward && !isWrittenForward) {
                attendre_contact(entrance);
            } else {
                attendre_contact(exit);
            }
            loco.afficherMessage("Shared section entered.");

            // On attend le contact de sortie de la section partagée
            if(directionIsForward && isWrittenForward || !directionIsForward && !isWrittenForward) {
                attendre_contact(exit);
            } else {
                attendre_contact(entrance);
            }
            loco.afficherMessage("Exit from shared section.");

            // On attend le contact de libération de la section partagée
            attendre_contact(sharedSectionReleaseContact);
            loco.afficherMessage("Shared section liberated.");

            // On libère la section partagée
            sharedSection->leave(loco);

            // On définit qu'on se dirige vers la station
            goingTowardsSharedSection = false;
        }  else { // Gestion de la station

            // Attendre le contact de la station
            attendre_contact(stationContact);
            loco.afficherMessage("Arrived at the station.");

            // Réduire le nombre de tours restants
            --nbOfTurns;

            // Si on a fait le nombre de tours demandé (donc s'il ne reste aucun tour à faire)
            if (nbOfTurns == 0) {
                // On mémorise la vitesse actuelle de la locomotive
                int vitesse = loco.vitesse();

                // Arrêter la locomotive
                loco.fixerVitesse(0);

                // Synchroniser avec l'autre locomotive à la gare.
                // On attend que l'autre locomotive soit aussi à la gare, puis on attend deux secondes, 
                // puis on démarre les deux locomotives dans le sens opposé
                loco.afficherMessage("Stopped at station. Synchronizing...");
                sharedStation->trainArrived();

                // Inverser le sens
                loco.inverserSens();
                directionIsForward = !directionIsForward;
                loco.afficherMessage("Reverse course.");

                // Déterminer les nouveaux points de contact 
                // (point de réservation et de libération de la section partagée)
                determineContactPoints();

                // Réinitialiser le nombre de tours restants avec un nombre aléatoire 
                // entre minNbOfTurns et maxNbOfTurns
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
    qDebug() << "[START] Thread of loco number " << loco.numero() << " launched";
    loco.afficherMessage("I am launched !");
}

void LocomotiveBehavior::printCompletionMessage() {
    qDebug() << "[STOP] Thread of loco number " << loco.numero() << "correctly stopped";
    loco.afficherMessage("I've finished");
}

void LocomotiveBehavior::determineContactPoints() {
    // Variables temporaires pour les index d'entrée et de sortie
    int targetIndexEntry;
    int targetIndexExit;

    // On détermine les points de réservation et de libération de la section partagée
    if(isWrittenForward) {
        // Si la locomotive va en avant et que la section partagée est écrite de gauche à droite
        if(directionIsForward) { 
            
            // On recule l'indexe depuis notre point d'entrée, qui sert bien d'entrée dans cette configuration
            targetIndexEntry = entranceIndex - INCOMING_BUFFER; 

            // On avance l'indexe depuis notre point de sortie, qui sert bien de sortie dans cette configuration
            targetIndexExit  = exitIndex     + OUTGOING_BUFFER; 
        } else { // Si la locomotive va en arrière et que la section partagée est écrite de gauche à droite
            
            // On avance l'indexe depuis notre point de sortie, qui sert d'enrée si on va en arrière
            targetIndexEntry = exitIndex     + INCOMING_BUFFER; 

            // On recule l'indexe depuis notre point d'entrée, qui sert de sortie si on va en arrière
            targetIndexExit  = entranceIndex - OUTGOING_BUFFER; 
        }
    } else {
        // Si la locomotive va en avant et que la section partagée est écrite de droite à gauche
        if(directionIsForward) { 
            
            // On recule l'indexe depuis notre point de sortie, 
            // qui sert d'entrée si on va en avant alors que la section est écrite de droite à gauche
            targetIndexEntry = exitIndex     - INCOMING_BUFFER; 

            // On avance l'indexe depuis notre point d'entrée, 
            // qui sert de sortie si on va en avant alors que la section est écrite de droite à gauche
            targetIndexExit  = entranceIndex + OUTGOING_BUFFER; 
        } else { // Si la locomotive va en arrière et que la section partagée est écrite de droite à gauche

            // On avance l'indexe depuis notre point d'entrée, qui sert bien d'entrée dans cette configuration, 
            // même si on va en arrière
            targetIndexEntry = entranceIndex + INCOMING_BUFFER; 

            // On recule l'indexe depuis notre point de sortie, qui sert bien de sortie dans cette configuration, 
            // même si on va en arrière
            targetIndexExit  = exitIndex     - OUTGOING_BUFFER; 

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
    if(entranceIndex == -1) {
        throw std::runtime_error("Invalid entrance contacts -- outside trajectory");
    }

    if(exitIndex == -1) {
        throw std::runtime_error("Invalid exit contacts -- outside trajectory");
    }

    if(entranceIndex == exitIndex) {
        throw std::runtime_error("Invalid entrance and exit contacts -- same contact");
    }
}

void LocomotiveBehavior::setStationContact(int contact) {
    // On obtient l'index du contact de la station
    int stationIndex = getIndexOfContact(contact);

    // On vérifie que le contact est dans la liste des contacts
    if(stationIndex == -1) {
        throw std::runtime_error("Invalid station contact -- outside trajectory");
    }

    // La station ne doit pas être dans la section partagée ou son buffer (dans notre solution), 
    // et pour cela on doit savoir si la section partagée est coupée
    bool sharedSectionIsCut = isSharedSectionCut();

    // On prépare un flag
    bool stationError;

    // On vérifie si la station est dans la section partagée
    if (isWrittenForward) {
        stationError = sharedSectionIsCut 
                // Si la section partagée est coupée et qu'on a écrit de gauche à droite la section partagée, 
                //alors on vérifie les bornes des contacts
                ? (stationIndex > entranceIndex || stationIndex < exitIndex) 

                // Sinon, on vérifie que la station est bien entre les bornes de la section partagée, 
                //écrite de gauche à droite
                : (stationIndex > entranceIndex && stationIndex < exitIndex); 
    } else {
        stationError = sharedSectionIsCut 
                // Si la section partagée est coupée et qu'on a écrit de droite à gauche la section partagée, 
                //alors on vérifie les bornes des contacts, mais dans l'autre sens
                ? (stationIndex < entranceIndex || stationIndex > exitIndex)  

                // Sinon, on vérifie que la station est bien entre les bornes de la section partagée,
                // écrite de droite à gauche
                : (stationIndex < entranceIndex && stationIndex > exitIndex); 
    }

    // On lance une exception si la station est dans la section partagée
    if (stationError) {
        throw std::runtime_error("Invalid station contact -- in shared section");
    }

    // La station ne doit pas être dans la zone tampon de la section partagée, dans le sens aller ou retour
    // On vérifie que la station n'est pas dans la zone tampon de la section partagée 
    // en avançant ou reculant dans la liste des contacts, selon le sens de la section partagée
    if(isWrittenForward) {
        for(int i = 1; i <= std::max(INCOMING_BUFFER, OUTGOING_BUFFER) && !stationError; ++i) { 
            // Vu qu'on veut l'aller-retour, on prend le max des deux buffers
            if(contacts[(stationIndex - i + contacts.size()) % contacts.size()] == exit) {
                stationError = true;
            }
            if(!stationError && contacts[(stationIndex + i) % contacts.size()] == entrance) {
                stationError = true;
            }
        }
    } else {
        for(int i = 1; i <= std::max(INCOMING_BUFFER, OUTGOING_BUFFER) && !stationError; ++i) {
            if(contacts[(stationIndex + i) % contacts.size()] == exit) {
                stationError = true;
            }
            if(!stationError && contacts[(stationIndex - i + contacts.size()) % contacts.size()] == entrance) {
                stationError = true;
            }
        }
    }

    // On lance une exception si la station est dans la zone tampon de la section partagée
    if (stationError) {
        throw std::runtime_error("Invalid station contact -- in shared section buffer");
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
    if(firstIndex == -1) {
        throw std::runtime_error("Invalid starting position -- first contact not in trajectory");
    }

    if(secondIndex == -1) {
        throw std::runtime_error("Invalid starting position -- second contact not in trajectory");
    }

    if(firstIndex == secondIndex) {
        throw std::runtime_error("Invalid starting position -- same contact");
    }

    if(firstIndex == entranceIndex) {
        throw std::runtime_error("Invalid starting position -- first contact is entrance");
    }

    if(firstIndex == exitIndex) {
        throw std::runtime_error("Invalid starting position -- first contact is exit");
    }

    if(secondIndex == entranceIndex) {
        throw std::runtime_error("Invalid starting position -- second contact is entrance");
    }

    if(secondIndex == exitIndex) {
        throw std::runtime_error("Invalid starting position -- second contact is exit");
    }

    // On vérifie que les index sont consécutifs
    if(abs(firstIndex - secondIndex)  != 1 && abs(firstIndex - secondIndex) != contacts.size() - 1) {
        throw std::runtime_error("Invalid starting position -- contacts are not consecutive");
    }

    // On obtient l'informationd de si la section partagée est coupée
    bool sharedSectionIsCut = isSharedSectionCut();

    bool error = false;

    // On vérifie qu'on ne commence pas dans la section partagée 
    //(ni le contact juste avant la position de départ de la locomotive, ni le contact juste après)
    if(isWrittenForward) {
        if(sharedSectionIsCut) { 
            // Si la section partagée est coupée dans la liste des contacts 
            //et que la section partagée est écrite de gauche à droite
            if(firstIndex > entranceIndex || firstIndex < exitIndex 
            || secondIndex > entranceIndex || secondIndex < exitIndex) {
                error = true;
            }
        } else { 
            // Si la section partagée n'est pas coupée dans la liste des contacts 
            //et que la section partagée est écrite de gauche à droite
            if(firstIndex > entranceIndex && firstIndex < exitIndex 
            || secondIndex > entranceIndex && secondIndex < exitIndex) {
                error = true;
            }
        }
    } else {
        if(sharedSectionIsCut) { 
            // Si la section partagée est coupée dans la liste des contacts 
            // et que la section partagée est écrite de droite à gauche
            if(firstIndex < entranceIndex || firstIndex > exitIndex 
            || secondIndex < entranceIndex || secondIndex > exitIndex) {
                error = true;
            }
        } else { 
            // Si la section partagée n'est pas coupée dans la liste des contacts 
            // et que la section partagée est écrite de droite à gauche
            if(firstIndex < entranceIndex && firstIndex > exitIndex 
            || secondIndex < entranceIndex && secondIndex > exitIndex) {
                error = true;
            }
        }
    }

    // On lance une exception si on commence dans la section partagée
    if(error) {
        throw std::runtime_error("Invalid starting position -- in shared section");
    }

    // On ne peut aussi pas être dans la zone tampon de la section partagée
    // On va itérer à travers les contacts pour la distance du buffer 
    // depuis le point de départ du train pour vérifier que le train n'est pas dans la zone tampon
    if(directionIsForward) {
        if(isWrittenForward) { 
            // Si la section partagée est écrite de gauche à droite dans la liste des contacts 
            // et que la locomotive va en avant (de gauche à droite dans sa liste de contacts)


            // On vérifie que la locomotive n'est pas dans la zone tampon de la section partagée
            // en avançant dans la liste des contacts depuis le contact juste devant la locomotive
            // et en vérifiant qu'on n'entre pas dans la section partagée (selon la taille du buffer)
            for(int i = 1; i < INCOMING_BUFFER && !error; ++i) {
                if(contacts[(secondIndex + i) % contacts.size()] == entrance) {
                    error = true;
                }
            }
            // Même chose, mais en reculant dans la liste des contacts depuis le contact juste derrière la locomotive
            for(int i = 1; i < OUTGOING_BUFFER && !error; ++i) {
                if(contacts[(firstIndex - i + contacts.size()) % contacts.size()] == exit) {
                    error = true;
                }
            }
        } else { 
            // On effectue le même type de vérification, 
            // mais pour le cas où la section partagée est écrite de droite à gauche dans la liste des contacts,
            // mais que la locomotive va en avant
            for(int i = 1; i < INCOMING_BUFFER && !error; ++i) {
                if(contacts[(secondIndex + i) % contacts.size()] == exit) {
                    error = true;
                }
            }
            for(int i = 1; i < OUTGOING_BUFFER && !error; ++i) {
                if(contacts[(firstIndex - i + contacts.size()) % contacts.size()] == entrance) {
                    error = true;
                }
            }
        }
    } else { 
        // On effectue le même type de vérification, mais pour le cas où la locomotive va en arrière, 
        //donc en sens inverse de la liste des contacts, mais que la section partagée est écrite de gauche à droite
        if(isWrittenForward) {
            for(int i = 1; i < INCOMING_BUFFER && !error; ++i) {
                if(contacts[(secondIndex - i + contacts.size()) % contacts.size()] == exit) {
                    error = true;
                }
            }
            for(int i = 1; i < OUTGOING_BUFFER && !error; ++i) {
                if(contacts[(firstIndex + i) % contacts.size()] == entrance) {
                    error = true;
                }
            }
        } else { 
            // On effectue le même type de vérification, 
            // mais pour le cas où la section partagée est écrite de droite à gauche dans la liste des contacts, 
            // et que la locomotive va en arrière
            for(int i = 1; i < INCOMING_BUFFER && !error; ++i) {
                if(contacts[(secondIndex - i + contacts.size()) % contacts.size()] == entrance) {
                    error = true;
                }
            }
            for(int i = 1; i < OUTGOING_BUFFER && !error; ++i) {
                if(contacts[(firstIndex + i) % contacts.size()] == exit) {
                    error = true;
                }
            }
        }
    }

    // On lance une exception si on commence dans la zone tampon de la section partagée
    if(error) {
        throw std::runtime_error("Invalid starting position -- in shared section buffer");
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
        if(directionIsForward) { 
            // Si la section partagée est écrite de gauche à droite dans la liste des contacts 
            // et que la locomotive va en avant
            targetEntrance = entrance;
        } else { 
            // Si la section partagée est écrite de gauche à droite dans la liste des contacts 
            // et que la locomotive va en arrière
            targetEntrance = exit;
        }
    } else { 
        // Si la section partagée est écrite de droite à gauche dans la liste des contacts 
        //et que la locomotive va en avant
        if(directionIsForward) {
            targetEntrance = exit;
        } else { 
            // Si la section partagée est écrite de droite à gauche dans la liste des contacts 
            // et que la locomotive va en arrière
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
        i = (directionIsForward ? i + 1 : i - 1 + contacts.size()) % contacts.size();
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
    // La section partagée doit être d'au moins 2 * max(INCOMING_BUFFER, OUTGOING_BUFFER) + 1, 
    // car la station ne doit pas être dans la section partagée
    // ou la zone tampon de la section partagée non plus sur le chemin aller ou retour
    if (contacts.size() < sizeOfSharedSection + 2 * std::max(INCOMING_BUFFER, OUTGOING_BUFFER) + 1) {
        throw std::runtime_error("Invalid contacts size -- not enough contacts given the shared section size");
    }
}

int LocomotiveBehavior::sizeSharedSection(bool sharedSectionIsCut) {

    // On calcule la taille de la section partagée
    if(sharedSectionIsCut) {
        if(isWrittenForward) { 
            // Si la section partagée est coupée et que la section partagée est écrite de gauche à droite
            return contacts.size() - entranceIndex + exitIndex + 1;
        } else { 
            // Si la section partagée est coupée et que la section partagée est écrite de droite à gauche
            return contacts.size() - exitIndex + entranceIndex + 1;
        }
    } else {
        if(isWrittenForward) { 
            // Si la section partagée n'est pas coupée et que la section partagée est écrite de gauche à droite
            return exitIndex - entranceIndex + 1;
        } else { 
            // Si la section partagée n'est pas coupée et que la section partagée est écrite de droite à gauche
            return entranceIndex - exitIndex + 1;
        }
    }
}