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
#include "sharedstation.h"

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
     * \param sharedSection la section partagée
     * \param sharedSectionDirections les directions des aiguillages pour la section partagée
     * \param isWrittenForward si la section partagée est rédigée de gauche à droite dans la liste des contacts (relativement à ce qu'on a défini comme étant l'entrée et la sortie de la section partagée)
     * \param contacts les contacts de la locomotive
     * \param entrance le contact d'entrée de la section partagée
     * \param exit le contact de sortie de la section partagée
     * \param trainFirstStart le contact à l'arrière de la locomotive au démarrage
     * \param trainSecondStart le contact à l'avant de la locomotive au démarrage
     * \param stationContact le contact de la station
     * \param sharedStation la station partagée
     */
    LocomotiveBehavior(Locomotive& loco, std::shared_ptr<SharedSectionInterface> sharedSection, 
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

        // Détermine si la locomotive va en avant ou en arrière, en assumant que les positions de départ sont valides (on vérifie ça plus tard, et on a besoin de cette information pour la suite)
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

    /*!
     * \brief calculateEntranceAndExitIndexes Calcule les indices d'entrée et de sortie de la section partagée
     */
    void calculateEntranceAndExitIndexes();

    /*!
     * \brief isStartingPositionValid Vérifie si la position de départ de la locomotive est valide
     * \param firstIndex l'index juste derrière la locomotive à son démarrage
     * \param secondIndex l'index juste devant la locomotive à son démarrage
     */
    void isStartingPositionValid(int firstIndex, int secondIndex);

    /*!
     * \brief isGoingForward Détermine si la locomotive va en avant ou en arrière
     * \param firstIndex l'index juste derrière la locomotive
     * \param secondIndex l'index juste devant la locomotive
     * \return true si la locomotive va en avant, false sinon
     */
    bool isGoingForward(int firstIndex, int secondIndex);

    /*!
     * \brief getIndexOfContact Retourne l'index d'un contact dans la liste des contacts de la locomotive
     * \param contact le contact dont on veut l'index
     * \return l'index du contact, -1 si le contact n'est pas dans la liste
     */
    int getIndexOfContact(int contact);

    /*!
     * \brief setNextDestination Détermine le prochain contact de la locomotive (section partagée ou station)
     * \param secondStartIndex l'index du contact juste devant la locomotive
     */
    void setNextDestination(int secondStartIndex);

    /*!
     * \brief setStationContact Vériifie si le contact de la station est valide et le fixe
     * \param contact le contact de la station
     */
    void setStationContact(int contact);

    /*!
     * \brief getRandomTurnNumber Retourne un nombre de tours aléatoire entre minNbOfTurns et maxNbOfTurns
     * \return le nombre de tours
     */
    int getRandomTurnNumber();

    /*!
     * \brief checkMinimalSizeOfContacts Vérifie si la liste des contacts est assez grande pour contenir la section partagée, les buffers et la station
     * \param sizeOfSharedSection la taille de la section partagée
     */
    void checkMinimalSizeOfContacts(int sizeOfSharedSection);

    /*!
     * \brief isSharedSectionCut Vérifie si la section partagée est coupée dans la liste des contacts
     * \return true si la section est coupée, false sinon
     */
    bool isSharedSectionCut();

    /*!
     * \brief sizeSharedSection Calcule la taille de la section partagée
     * \param sharedSectionIsCut true si la section est coupée, false sinon
     * \return la taille de la section partagée
     */
    int sizeSharedSection(bool sharedSectionIsCut);

    /*!
     * \brief locoToString Retourne une représentation de la locomotive
     * \return la représentation de la locomotive
     */
    QString toString();

    /**
     * @brief loco La locomotive dont on représente le comportement
     */
    Locomotive& loco;

    /**
     * @brief sharedSection Pointeur sur la section partagée
     */
    std::shared_ptr<SharedSectionInterface> sharedSection;

    /**
     * @brief sharedStation Pointeur sur la station partagée
     */
    std::shared_ptr<SharedStation> sharedStation;

    /**
     * @brief contacts Les contacts de la locomotive
     */
    std::vector<int> contacts;

    /**
     * @brief stationContact Le contact de la station
     */
    int stationContact;

    /**
     * @brief sharedSectionReserveContact Le contact de réservation de la section partagée
     */
    int sharedSectionReserveContact;

    /**
     * @brief sharedSectionReleaseContact Le contact de libération de la section partagée
     */
    int sharedSectionReleaseContact;

    /**
     * @brief sharedSectionDirections Les directions des aiguillages pour la section partagée
     */
    std::vector<std::pair<int, int>> sharedSectionDirections;

    /**
     * @brief directionIsForward true si la locomotive va en avant (de gauche à droite dans sa liste de contacts), false sinon
     */
    bool directionIsForward;

    /**
     * @brief isWrittenForward true si la section partagée est rédigée de gauche à droite dans la liste des contacts, false sinon
     */
    bool isWrittenForward;

    /**
     * @brief goingTowardsSharedSection true si la locomotive va vers la section partagée, false si elle va vers la station
     */
    bool goingTowardsSharedSection;

    /**
     * @brief entranceIndex Index du contact d'entrée de la section partagée
     */
    int entranceIndex;

    /**
     * @brief exitIndex Index du contact de sortie de la section partagée
     */
    int exitIndex;

    /**
     * @brief entrance Contact d'entrée de la section partagée
     */
    int entrance;

    /**
     * @brief exit Contact de sortie de la section partagée
     */
    int exit;

    /**
     * @brief nbOfTurns Nombre de tours (restants) à effectuer
     */
    int nbOfTurns;

    /**
     * @brief maxNbOfTurns Nombre maximal de tours à effectuer
     */
    static const int maxNbOfTurns = 10;

    /**
     * @brief minNbOfTurns Nombre minimal de tours à effectuer
     */
    static const int minNbOfTurns = 1;
};

#endif // LOCOMOTIVEBEHAVIOR_H
