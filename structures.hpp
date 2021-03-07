// Solutionnaire du TD2 INF1015 hiver 2021
// Par Francois-R.Boyer@PolyMtl.ca
#pragma once
// Structures mémoires pour une collection de films.

#include <string>
#include <memory>
#include <functional>
#include "gsl/span"

using namespace std;
using gsl::span;


struct Film; struct Acteur; // Permet d'utiliser les types alors qu'ils seront défini après.
#define ListeActeur Liste<Acteur>

class ListeFilms {
public:
	ListeFilms() = default;
	void ajouterFilm(Film* film);
	void enleverFilm(const Film* film);
	shared_ptr<Acteur> trouverActeur(const std::string& nomActeur) const;
	span<Film*> enSpan() const;
	int size() const { return nElements; }
	void detruire(bool possedeLesFilms = false);
	

	Film* operator[](int i) { return enSpan()[i]; }

	string trouverFilmQui(const function<bool(Film film)>& critere);
private:
	void changeDimension(int nouvelleCapacite);

	int capacite = 0, nElements = 0;
	Film** elements = nullptr; // Pointeur vers un tableau de Film*, chaque Film* pointant vers un Film.
};

template <typename T>
class Liste {
public:
	Liste() = default;

	int getNElements() const { return nElements_; }
	int getCapacite() const { return capacite_; }

	void setNElements(unsigned int nElements) { nElements_ = nElements; };

	void createEmptyList(unsigned nElements) {
		elements = make_unique<shared_ptr<T>[]>(nElements);
		capacite_ = nElements;
	};

	Liste<T>& operator=(const Liste<T>& autre){
		nElements_ = autre.getNElements();
		capacite_ = autre.getCapacite();
		elements = make_unique<shared_ptr<T>[]>(nElements_);
		for (int i = 0; i < nElements_; ++i) {	
			elements[i] = autre.elements[i];
		}
		return *this;
	}

	Liste<T> (Liste<T>& autre) {
		*this = move(autre);
	}

	shared_ptr<T>& operator[](int i) { return spanListe()[i]; }
	span<shared_ptr<T>> spanListe() const { return span(elements.get(), nElements_); }
private:
	unsigned int capacite_ = 0, nElements_ = 0;
	unique_ptr<shared_ptr<T>[]> elements;
};




struct Film
{
	Film() = default;
	Film(const Film& autre) {
		titre = autre.titre;
		realisateur = autre.realisateur;
		anneeSortie = autre.anneeSortie;
		recette = autre.recette;
		acteurs = autre.acteurs;
	}

	std::string titre, realisateur; // Titre et nom du réalisateur (on suppose qu'il n'y a qu'un réalisateur).
	int anneeSortie = 0, recette = 0; // Année de sortie et recette globale du film en millions de dollars
	ListeActeur acteurs;

	friend ostream& operator<< (ostream& o, const Film& film);
};

struct Acteur
{
	std::string nom; int anneeNaissance; char sexe;
	//ListeFilms joueDans;
};


