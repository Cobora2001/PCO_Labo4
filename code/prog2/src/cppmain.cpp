//    ___  _________    ___  ___  ___ ____ //
//   / _ \/ ___/ __ \  |_  |/ _ \|_  / / / //
//  / ___/ /__/ /_/ / / __// // / __/_  _/ //
// /_/   \___/\____/ /____/\___/____//_/   //
//

#include "ctrain_handler.h"

#include "locomotive.h"
#include "locomotivebehavior.h"
#include "sharedsectioninterface.h"
#include "sharedsection.h"

// Locomotives :
// Vous pouvez changer les vitesses initiales, ou utiliser la fonction loco.fixerVitesse(vitesse);
// Laissez les numéros des locos à 0 et 1 pour ce laboratoire

// Locomotive A
static Locomotive locoA(0 /* Numéro (pour commande trains sur maquette réelle) */, 20 /* Vitesse */);
// Locomotive B
static Locomotive locoB(1 /* Numéro (pour commande trains sur maquette réelle) */, 24 /* Vitesse */);

std::vector<Locomotive> trainsExisting = {locoA, locoB};

//Arret d'urgence
void emergency_stop()
{
    for(auto& loco : trainsExisting) {
        loco.arreter();
        loco.fixerVitesse(0);
    }

    afficher_message("\nSTOP!");
}

//Fonction principale
int cmain()
{
    /************
     * Maquette *
     ************/

    //Choix de la maquette (A ou B)
    selection_maquette(MAQUETTE_A /*MAQUETTE_B*/);

    /**********************************
     * Initialisation des aiguillages *
     **********************************/

    // Initialisation des aiguillages
    // Positiion de base donnée comme exemple, vous pouvez la changer comme bon vous semble
    // Vous devrez utiliser cette fonction pour la section partagée pour aiguiller les locos
    // sur le bon parcours (par exemple à la sortie de la section partagée) vous pouvez l'
    // appeler depuis vos thread des locos par ex.
    diriger_aiguillage(1,  TOUT_DROIT, 0); // Train 1
    diriger_aiguillage(2,  DEVIE     , 0); // Train 0
    diriger_aiguillage(3,  DEVIE     , 0); // Train 0
    diriger_aiguillage(4,  TOUT_DROIT, 0); // Train 1
    diriger_aiguillage(5,  TOUT_DROIT, 0); // Train 0
    diriger_aiguillage(6,  TOUT_DROIT, 0);
    diriger_aiguillage(7,  TOUT_DROIT, 0); // Train 1
    diriger_aiguillage(8,  DEVIE     , 0); // Train 0
    diriger_aiguillage(9,  DEVIE     , 0); // Train 0
    diriger_aiguillage(10, TOUT_DROIT, 0); // Train 1
    diriger_aiguillage(11, TOUT_DROIT, 0); // Train 0
    diriger_aiguillage(12, TOUT_DROIT, 0);
    diriger_aiguillage(13, DEVIE     , 0); // Train 1
    diriger_aiguillage(14, DEVIE     , 0); // Partagé
    diriger_aiguillage(15, TOUT_DROIT, 0); // Train 1
    diriger_aiguillage(16, DEVIE     , 0); // Train 0
    diriger_aiguillage(17, TOUT_DROIT, 0);
    diriger_aiguillage(18, TOUT_DROIT, 0);
    diriger_aiguillage(19, DEVIE     , 0); // Train 0
    diriger_aiguillage(20, TOUT_DROIT, 0); // Train 1
    diriger_aiguillage(21, DEVIE     , 0); // Partagé
    diriger_aiguillage(22, DEVIE     , 0); // Train 1
    diriger_aiguillage(23, TOUT_DROIT, 0);
    diriger_aiguillage(24, TOUT_DROIT, 0);
    // diriger_aiguillage(/*NUMERO*/, /*TOUT_DROIT | DEVIE*/, /*0*/);

    /********************************
     * Position de départ des locos *
     ********************************/

    int beginStartTrain0 = 14;
    int endStartTrain0 = 7;
    int beginStartTrain1 = 10;
    int endStartTrain1 = 4;

    // Loco 0
    // Exemple de position de départ
    locoA.fixerPosition(endStartTrain0, beginStartTrain0);

    // Loco 1
    // Exemple de position de départ
    locoB.fixerPosition(endStartTrain1, beginStartTrain1);

    /***********
     * Message *
     **********/

    // Affiche un message dans la console de l'application graphique
    afficher_message("Hit play to start the simulation...");

    /*********************
     * Threads des locos *
     ********************/

    // Création de la section partagée
    std::shared_ptr<SharedSectionInterface> sharedSection = std::make_shared<SharedSection>();
    std::shared_ptr<SharedStation> sharedStation = std::make_shared<SharedStation>(trainsExisting.size());

    std::vector<std::pair<int, int>> directionsTrain0 = {{14, DEVIE}, {21, DEVIE}};
    std::vector<std::pair<int, int>> directionsTrain1 = {{14, TOUT_DROIT}, {21, TOUT_DROIT}};

    std::vector<int> contactsTrain0 = {14, 7, 6, 5, 34, 33, 28, 22, 24, 23, 16, 15};
    std::vector<int> contactsTrain1 = {10, 4, 3, 2, 1, 31, 33, 28, 22, 24, 19, 13, 12, 11};

    int entrance = 33;
    int exit = 24;

    int stationTrain0 = 6;
    int stationTrain1 = 12;

    bool isWrittenForwardTrain0 = true;
    bool isWrittenForwardTrain1 = true;

    /*Locomotive& loco, std::shared_ptr<SharedSectionInterface> sharedSection, 
                        std::vector<std::pair<int, int>> sharedSectionDirections, 
                        bool isWrittenForward, 
                        std::vector<int> contacts,
                        int entrance, int exit,
                        int trainFirstStart, int trainSecondStart*/

    // Création du thread pour la loco 0
    std::unique_ptr<Launchable> locoBehaveA = std::make_unique<LocomotiveBehavior>(locoA, sharedSection, directionsTrain0, isWrittenForwardTrain0, contactsTrain0, entrance, exit, beginStartTrain0, endStartTrain0, stationTrain0, sharedStation);
    // Création du thread pour la loco 1
    std::unique_ptr<Launchable> locoBehaveB = std::make_unique<LocomotiveBehavior>(locoB, sharedSection, directionsTrain1, isWrittenForwardTrain1, contactsTrain1, entrance, exit, beginStartTrain1, endStartTrain1, stationTrain1, sharedStation);

    // Lanchement des threads
    afficher_message(qPrintable(QString("Lancement thread loco A (numéro %1)").arg(locoA.numero())));
    locoBehaveA->startThread();
    afficher_message(qPrintable(QString("Lancement thread loco B (numéro %1)").arg(locoB.numero())));
    locoBehaveB->startThread();

    // Attente sur la fin des threads
    locoBehaveA->join();
    locoBehaveB->join();

    //Fin de la simulation
    mettre_maquette_hors_service();

    return EXIT_SUCCESS;
}
