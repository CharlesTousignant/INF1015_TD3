// TP3 INF1015 Hiver 2021
// Par: Charles Tousignant		2087807
//		Ahmed Sidi Ould Ahmedou 2001565


#pragma region "Includes"//{
#define _CRT_SECURE_NO_WARNINGS // On permet d'utiliser les fonctions de copies de chaînes qui sont considérées non sécuritaires.

#include "structures.hpp"      // Structures de données pour la collection de films en mémoire.

#include "bibliotheque_cours.hpp"
#include "verification_allocation.hpp" // Nos fonctions pour le rapport de fuites de mémoire.

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <limits>
#include <algorithm>
#include <functional>
#include "cppitertools/range.hpp"
#include "gsl/span"
#include "debogage_memoire.hpp"        // Ajout des numéros de ligne des "new" dans le rapport de fuites.  Doit être après les include du système, qui peuvent utiliser des "placement new" (non supporté par notre ajout de numéros de lignes).
using namespace std;
using namespace iter;
using namespace gsl;
#pragma endregion//}

typedef uint8_t UInt8;
typedef uint16_t UInt16;

#pragma region "Fonctions de base pour lire le fichier binaire"//{

UInt8 lireUint8(istream& fichier)
{
	UInt8 valeur = 0;
	fichier.read((char*)&valeur, sizeof(valeur));
	return valeur;
}
UInt16 lireUint16(istream& fichier)
{
	UInt16 valeur = 0;
	fichier.read((char*)&valeur, sizeof(valeur));
	return valeur;
}
string lireString(istream& fichier)
{
	string texte;
	texte.resize(lireUint16(fichier));
	fichier.read((char*)&texte[0], streamsize(sizeof(texte[0])) * texte.length());
	return texte;
}

#pragma endregion//}

// Fonctions pour ajouter un Film à une ListeFilms.
//[
void ListeFilms::changeDimension(int nouvelleCapacite)
{
	Film** nouvelleListe = new Film*[nouvelleCapacite];
	
	if (elements != nullptr) {  // Noter que ce test n'est pas nécessaire puique nElements sera zéro si elements est nul, donc la boucle ne tentera pas de faire de copie, et on a le droit de faire delete sur un pointeur nul (ça ne fait rien).
		nElements = min(nouvelleCapacite, nElements);
		for (int i : range(nElements))
			nouvelleListe[i] = elements[i];
		delete[] elements;
	}
	
	elements = nouvelleListe;
	capacite = nouvelleCapacite;
}

void ListeFilms::ajouterFilm(Film* film)
{
	if (nElements == capacite)
		changeDimension(max(1, capacite * 2));
	elements[nElements++] = film;
}
//]

// Fonction pour enlever un Film d'une ListeFilms (enlever le pointeur) sans effacer le film; la fonction prenant en paramètre un pointeur vers le film à enlever.  L'ordre des films dans la liste n'a pas à être conservé.
//[
// On a juste fait une version const qui retourne un span non const.  C'est valide puisque c'est la struct qui est const et non ce qu'elle pointe.  Ça ne va peut-être pas bien dans l'idée qu'on ne devrait pas pouvoir modifier une liste const, mais il y aurais alors plusieurs fonctions à écrire en version const et non-const pour que ça fonctionne bien, et ce n'est pas le but du TD (il n'a pas encore vraiment de manière propre en C++ de définir les deux d'un coup).
span<Film*> ListeFilms::enSpan() const { return span(elements, nElements); }

void ListeFilms::enleverFilm(const Film* film)
{
	for (Film*& element : enSpan()) {  // Doit être une référence au pointeur pour pouvoir le modifier.
		if (element == film) {
			if (nElements > 1)
				element = elements[nElements - 1];
			nElements--;
			return;
		}
	}
}

string ListeFilms::trouverFilmQui(const function<bool(Film film)>& critere) {
	for (int i = 0; i < nElements; ++i) {
		if (critere(*elements[i]))
			return elements[i]->titre;
	}

	return "Aucun film trouve";
}
//]

// Fonction pour trouver un Acteur par son nom dans une ListeFilms, qui retourne un pointeur vers l'acteur, ou nullptr si l'acteur n'est pas trouvé.  Devrait utiliser span.
//[
// Voir la NOTE ci-dessous pourquoi Acteur* n'est pas const.  Noter que c'est valide puisque c'est la struct uniquement qui est const dans le paramètre, et non ce qui est pointé par la struct.

//NOTE: Doit retourner un Acteur modifiable, sinon on ne peut pas l'utiliser pour modifier l'acteur tel que demandé dans le main, et on ne veut pas faire écrire deux versions aux étudiants dans le TD2.
shared_ptr<Acteur> ListeFilms::trouverActeur(const string& nomActeur) const
{
	for (const Film* film : enSpan()) {
		for (shared_ptr<Acteur> acteur : film->acteurs.enSpan()) {
			if (acteur->nom == nomActeur)
				return acteur;
		}
	}
	return nullptr;
}
//]

// Les fonctions pour lire le fichier et créer/allouer une ListeFilms.

shared_ptr<Acteur> lireActeur(istream& fichier, ListeFilms& listeFilms)
{
	Acteur acteur = {};
	acteur.nom            = lireString(fichier);
	acteur.anneeNaissance = lireUint16 (fichier);
	acteur.sexe           = lireUint8  (fichier);

	auto acteurExistant = listeFilms.trouverActeur(acteur.nom);
	if (acteurExistant != nullptr)
		return acteurExistant;
	else {
		cout << "Création Acteur " << acteur.nom << endl;
		return make_shared<Acteur>(acteur);
	}
}

Film* lireFilm(istream& fichier, ListeFilms& listeFilms)
{
	Film* film = new Film();
	film->titre       = lireString(fichier);
	film->realisateur = lireString(fichier);
	film->anneeSortie = lireUint16 (fichier);
	film->recette     = lireUint16 (fichier);
	film->acteurs.setNElements(lireUint8(fichier));

	cout << "Création Film " << film->titre << endl;
	film->acteurs.allocateElements(film->acteurs.getNElements());
	for (shared_ptr<Acteur>& acteur : film->acteurs.enSpan()) {
		acteur = lireActeur(fichier, listeFilms);
		//acteur->joueDans.ajouterFilm(film);
	}
	return film;
}

ListeFilms creerListe(string nomFichier)
{
	ifstream fichier(nomFichier, ios::binary);
	fichier.exceptions(ios::failbit);
	
	int nElements = lireUint16(fichier);

	ListeFilms listeFilms;
	for ([[maybe_unused]] int i : range(nElements)) { //NOTE: On ne peut pas faire un span simple avec ListeFilms::enSpan car la liste est vide et on ajoute des éléments à mesure.
		listeFilms.ajouterFilm(lireFilm(fichier, listeFilms));
	}
	
	return listeFilms;
}

// Fonctions pour détruire un film (relâcher toute la mémoire associée à ce film, et les acteurs qui ne jouent plus dans aucun films de la collection).  Enlève aussi le film détruit des films dans lesquels jouent les acteurs.  Pour fins de débogage, les noms des acteurs sont affichés lors de leur destruction.
//[
void detruireActeur(Acteur* acteur)
{
	cout << "Destruction Acteur " << acteur->nom << endl;
	//acteur->joueDans.detruire();
	delete acteur;
}

//bool joueEncore(const Acteur* acteur)
//{
//	return acteur->joueDans.size() != 0;
//}

void detruireFilm(Film* film)
{
	/*for (shared_ptr<Acteur> acteur : (film->acteurs.spanListeActeurs())) {
		acteur->joueDans.enleverFilm(film);
		if (!joueEncore(acteur.get()))
			detruireActeur(acteur.get());
	}*/
	cout << "Destruction Film " << film->titre << endl;
	//delete[] film->acteurs.elements;
	delete film;
}
//]

// Fonction pour détruire une ListeFilms et tous les films qu'elle contient.
//[
//NOTE: La bonne manière serait que la liste sache si elle possède, plutôt qu'on le dise au moment de la destruction, et que ceci soit le destructeur.  Mais ça aurait complexifié le TD2 de demander une solution de ce genre, d'où le fait qu'on a dit de le mettre dans une méthode.
void ListeFilms::detruire(bool possedeLesFilms)
{
	if (possedeLesFilms)
		for (Film* film : enSpan())
			detruireFilm(film);
	delete[] elements;
}
//]

void afficherActeur(const Acteur& acteur)
{
	cout << "  " << acteur.nom << ", " << acteur.anneeNaissance << " " << acteur.sexe << endl;
}

// Fonction pour afficher un film avec tous ces acteurs (en utilisant la fonction afficherActeur ci-dessus).
//[
void afficherFilm(const Film& film)
{
	cout << "Titre: " << film.titre << endl;
	cout << "  Réalisateur: " << film.realisateur << "  Année :" << film.anneeSortie << endl;
	cout << "  Recette: " << film.recette << "M$" << endl;

	cout << "Acteurs:" << endl;
	for (auto const &acteur : (film.acteurs.enSpan()))
		afficherActeur(*acteur);
}
//]

void afficherListeFilms(const ListeFilms& listeFilms)
{
	static const string ligneDeSeparation = //[
		"\033[32m────────────────────────────────────────\033[0m\n";
	cout << ligneDeSeparation;
	for (const Film* film : listeFilms.enSpan()) {
		afficherFilm(*film);
		cout << ligneDeSeparation;
	}
}

//void afficherFilmographieActeur(const ListeFilms& listeFilms, const string& nomActeur)
//{
//	const Acteur* acteur = listeFilms.trouverActeur(nomActeur);
//	if (acteur == nullptr)
//		cout << "Aucun acteur de ce nom" << endl;
//	else
//		afficherListeFilms(acteur->joueDans);
//}


ostream& operator<< (ostream& o, const Film& film) {
	static const string ligneDeSeparation = //[
		"\033[32m────────────────────────────────────────\033[0m\n";
	string texteActeurs = "";
	for (auto const& acteur : (film.acteurs.enSpan())) {
		texteActeurs +=  "  " + acteur->nom + ", " + to_string(acteur->anneeNaissance) + " " + acteur->sexe + "\n";
	}
	return o << ligneDeSeparation <<
				"Titre: " << film.titre << endl <<
				"  Réalisateur: " << film.realisateur <<
				"  Année :" << film.anneeSortie << endl <<
				"  Recette: " << film.recette << "M$" << endl <<
				"Acteurs: \n" << texteActeurs << endl;
}

int main()
{
	#ifdef VERIFICATION_ALLOCATION_INCLUS
	bibliotheque_cours::VerifierFuitesAllocations verifierFuitesAllocations;
	#endif
	bibliotheque_cours::activerCouleursAnsi();  // Permet sous Windows les "ANSI escape code" pour changer de couleurs https://en.wikipedia.org/wiki/ANSI_escape_code ; les consoles Linux/Mac les supportent normalement par défaut.

	static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";

	ListeFilms listeFilms = creerListe("films.bin");
	
	cout << ligneDeSeparation << "Le premier film de la liste est:" << endl;
	// Le premier film de la liste.  Devrait être Alien.
	afficherFilm(*listeFilms.enSpan()[0]);


	cout << ligneDeSeparation << "Les films sont:" << endl;
	// Affiche la liste des films.  Il devrait y en avoir 7.
	afficherListeFilms(listeFilms);

	listeFilms.trouverActeur("Benedict Cumberbatch")->anneeNaissance = 1976;

	//cout << ligneDeSeparation << "Liste des films où Benedict Cumberbatch joue sont:" << endl;
	// Affiche la liste des films où Benedict Cumberbatch joue.  Il devrait y avoir Le Hobbit et Le jeu de l'imitation.
	//afficherFilmographieActeur(listeFilms, "Benedict Cumberbatch");
	

	cout << ligneDeSeparation << "Chapitre 7" << endl;
	cout << "affichage de deux films: " << endl << *listeFilms[0] << *listeFilms[1];
	ostringstream tamponStringStream;
	tamponStringStream << *listeFilms[0] << *listeFilms[1];
	string filmEnString = tamponStringStream.str();

	cout << "Test avec ostringstream, devrait encore afficher les deux premiers films: \n" << filmEnString;


	cout << ligneDeSeparation << "Chapitre 7-8 " << endl;
	Film skylien = *listeFilms[0];
	skylien.titre = "Skylien";
	skylien.acteurs[0] = listeFilms[1]->acteurs[0];
	skylien.acteurs[0].get()->nom = "Daniel Wroughton Craig";
	cout << "Skylien devrait avoir les memes informations que Alien a part le titre et le premier acteur: " << endl;
	cout << skylien;

	cout << "listeFilm[0] devrait etre le meme film qu'avant (Alien): " << endl;
	cout << *listeFilms[0];

	cout << "listeFilm[1] devrait etre le meme film qu'avant (Skyfall) avec le nouvel acteur " << endl;
	cout << *listeFilms[1];


	cout << ligneDeSeparation << "Chapitre 10 " << endl;
	string filmCritere = listeFilms.trouverFilmQui([](Film film) {return film.recette == 955; });
	cout << "Le film avec une recette de 955 est: " << filmCritere << endl;

	
	cout << ligneDeSeparation << "Chapitre 9 " << endl;
	//On cree une Liste<string> vide avec deux elements
	Liste<string> listeTextes;
	listeTextes.allocateElements(2);
	listeTextes.setNElements(2);

	//On 
	listeTextes[0] = make_shared<string>("string 0");
	listeTextes[1] = make_shared<string>("string 1");
	Liste<string> listeTextes2 = listeTextes;
	listeTextes[0] = make_shared<string>("nouvelle string 0");
	*listeTextes[1] = "string 1 modifie";

	cout << "ListeTextes[0]: " << *listeTextes[0] << endl;
	cout << "ListeTextes[1]: " << *listeTextes[1] << endl;
	cout << "ListeTextes2[0]: " << *listeTextes2[0] << endl;
	cout << "ListeTextes2[1]: " << *listeTextes2[1] << endl;

	cout << ligneDeSeparation;
	// Détruit et enlève le premier film de la liste (Alien).
	detruireFilm(listeFilms.enSpan()[0]);
	listeFilms.enleverFilm(listeFilms.enSpan()[0]);

	cout << ligneDeSeparation << "Les films sont maintenant:" << endl;
	afficherListeFilms(listeFilms);

	// Pour une couverture avec 0% de lignes non exécutées:
	listeFilms.enleverFilm(nullptr); // Enlever un film qui n'est pas dans la liste (clairement que nullptr n'y est pas).
	//afficherFilmographieActeur(listeFilms, "N'existe pas"); // Afficher les films d'un acteur qui n'existe pas.

	// Détruire tout avant de terminer le programme.
	listeFilms.detruire(true);
}
