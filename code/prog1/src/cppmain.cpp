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

static Locomotive locoA(0 /* Numéro (pour commande trains sur maquette réelle) */, 15 /* Vitesse */);
// Versions alternatives pour différents tests
// static Locomotive locoA(0 /* Numéro (pour commande trains sur maquette réelle) */, 4 /* Vitesse */);
// static Locomotive locoA(0 /* Numéro (pour commande trains sur maquette réelle) */, 20 /* Vitesse */);

// Locomotive B

static Locomotive locoB(1 /* Numéro (pour commande trains sur maquette réelle) */, 18 /* Vitesse */);
// Versions alternatives pour différents tests
// static Locomotive locoB(1 /* Numéro (pour commande trains sur maquette réelle) */, 3 /* Vitesse */);
// static Locomotive locoB(1 /* Numéro (pour commande trains sur maquette réelle) */, 10 /* Vitesse */);

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

    // Les aiguillages ont été défini de tel sorte à ce qu'il marche avec notre test
    // Si vous voulez changer les trajectoires des trains, vous devez changer les directions des aiguillages
    // par la même occasion
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

    /**********************************
     * Initialisation des trajets     *
     **********************************/

    // Section critique: 33, 28, 22, 24
    // On définit que 33 est l'entrée de la section critique, et 24 la sortie. 
    // Cela ne va jamais changer, peu importe le trajet ou la direction
    // C'est juste que des fois la locomotive rentrera par 33 et sortira par 24, et des fois l'inverse, 
    // mais cela ne change pas entrée et sortie

    // On définit 4 tests différents pour les trajets, et on les commente/décommente selon le test qu'on veut faire
    // On fait ça afin de pouvoir vérifier les 4 scénarios possibles pour la section partagée dans les contacts
    // Normalement, on devrait avoir exactement les mêmes résultats pour les 4 tests
    // On vérifie qu'on peut s'adapter à toutes les rédactions possibles de la section critique et du trajet

    // Test 1 : la section critique est écrite en allant de gauche à droite dans la liste des contacts, 
    // et elle est en un seul morceau

    // std::vector<int> contactsTrain0 = {14, 7, 6, 5, 34, 33, 28, 22, 24, 23, 16, 15};
    // std::vector<int> contactsTrain1 = {10, 4, 3, 2, 1, 31, 33, 28, 22, 24, 19, 13, 12, 11};

    // Test 2 : la section critique est écrite en allant de droite à gauche dans la liste des contacts, 
    // et elle est en un seul morceau

    std::vector<int> contactsTrain0 = {15, 16, 23, 24, 22, 28, 33, 34, 5, 6, 7, 14};
    std::vector<int> contactsTrain1 = {11, 12, 13, 19, 24, 22, 28, 33, 31, 1, 2, 3, 4, 10};

    // Test 3 : la section critique est écrite en allant de gauche à droite dans la liste des contacts, 
    // et elle est en deux morceaux

    // std::vector<int> contactsTrain0 = {22, 24, 23, 16, 15, 14, 7, 6, 5, 34, 33, 28};
    // std::vector<int> contactsTrain1 = {22, 24, 19, 13, 12, 11, 10, 4, 3, 2, 1, 31, 33, 28};

    // Test 4 : la section critique est écrite en allant de droite à gauche dans la liste des contacts, 
    // et elle est en deux morceaux

    // std::vector<int> contactsTrain0 = {28, 33, 34, 5, 6, 7, 14, 15, 16, 23, 24, 22};
    // std::vector<int> contactsTrain1 = {28, 33, 31, 1, 2, 3, 4, 10, 11, 12, 13, 19, 24, 22};

    /***************************************************************************************************
     * Directions des aiguillages pour la section partagée (à changer selon votre maquette et trajets) *
     **************************************************************************************************/

    // Ceci ne change pas entre nos tests ici

    std::vector<std::pair<int, int>> directionsTrain0 = {{14, DEVIE}, {21, DEVIE}};
    std::vector<std::pair<int, int>> directionsTrain1 = {{14, TOUT_DROIT}, {21, TOUT_DROIT}};

    /********************************
     * Position de départ des locos *
     ********************************/

    // Les positions doivent faire partie des contacts du trajet de la locomotive, 
    // et être à une distance de 1 entre elles
    // Elles ne doivent pas non plus être l'entrée ou la sortie de la section partagée, 
    // ni dans le buffer d'entrée ou de sortie
    // Ne vous en faites pas, en cas d'erreur ici, 
    // la simulation s'arrêtera et vous affichera un message d'erreur via un throw

    // On ne change pas ces valeurs entre nos tests ici, 
    // mais on peut faire des tests avec des valeurs différentes

    //int contactBehindTrain0Start  = 14; //merder ici
    //int contactInFrontTrain0Start = 7;
    //int contactBehindTrain1Start  = 10;
    //int contactInFrontTrain1Start = 4;

    // Si on veut aller dans l'autre sens, on change ici:

    //int contactBehindTrain0Start  = 7;
    //int contactInFrontTrain0Start = 14;
    //int contactBehindTrain1Start  = 4;
    //int contactInFrontTrain1Start = 10;

    // Si on veut que les trains n'aillent pas dans le même sens, on change ici:

    int contactBehindTrain0Start  = 14;
    int contactInFrontTrain0Start = 7;
    int contactBehindTrain1Start  = 4;
    int contactInFrontTrain1Start = 10;

    // On remarque qu'on doit mettre celui qui est devant en premier dans les paramètres de la fonction fixerPosition,
    // et celui qui est derrière en deuxième
    // À noter que nous avons choisi l'autre sens pour les paramètres dans le constructeur de LocomotiveBehavior, 
    // donc à faire attention

    // Loco 0
    // Exemple de position de départ
    locoA.fixerPosition(contactInFrontTrain0Start, contactBehindTrain0Start);

    // Loco 1
    // Exemple de position de départ
    locoB.fixerPosition(contactInFrontTrain1Start, contactBehindTrain1Start);

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

    // Création de la station partagée
    std::shared_ptr<SharedStation> sharedStation = std::make_shared<SharedStation>(trainsExisting.size());

    // On définit les contacts d'entrée et de sortie de la section partagée. Cela ne change pas entre nos tests ici
    int entrance = 33;
    int exit = 24;

    // On définit le contact de la station. Cela ne change pas entre nos tests ici
    int stationTrain0 = 6;
    int stationTrain1 = 12; //ici faut casser

    // Test 1 et Test 3 : la section critique est écrite en allant de gauche à droite dans la liste des contacts

    // bool isWrittenForwardTrain0 = true;
    // bool isWrittenForwardTrain1 = true;

    // Test 2 et Test 4 : la section critique est écrite en allant de droite à gauche dans la liste des contacts

    bool isWrittenForwardTrain0 = false;
    bool isWrittenForwardTrain1 = false;

    // Création des threads pour les locos
    // Cela ne change pas entre nos tests ici

    // Création du thread pour la loco 0
    std::unique_ptr<Launchable> locoBehaveA = std::make_unique<LocomotiveBehavior>(locoA, sharedSection, directionsTrain0, 
    isWrittenForwardTrain0, contactsTrain0, entrance, exit, 
    contactBehindTrain0Start, contactInFrontTrain0Start, stationTrain0, sharedStation);
    // Création du thread pour la loco 1
    std::unique_ptr<Launchable> locoBehaveB = std::make_unique<LocomotiveBehavior>(locoB, sharedSection, directionsTrain1, 
    isWrittenForwardTrain1, contactsTrain1, entrance, exit, 
    contactBehindTrain1Start, contactInFrontTrain1Start, stationTrain1, sharedStation);

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
