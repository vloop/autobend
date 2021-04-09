## Résumé

1) autobend
Applique un pitch bend selon la note active.

Ce programme fournit une des nombreus fonctionnalités offertes par le programme microtonal Scala.
Il vise avant tout la simplicité d'utilisation et la légèreté de l'outil.
Il permet un microtuning sur les synthés mono qui ne l'ont pas d'origine.
Ceci ne peut marcher qu'en mode mono: le pitch bend est un message de canal, si on joue un accord le même pitch bend s'applique à toutes les notes.
Scala attempts to work around this by rotating note channels, depending on your polysynth this may work or not.
voir aussi Scala sur http://www.huygens-fokker.org/scala/

2) autohold
Implémente les pédales pour les synthés polyphoniques qui ne les gèrent pas d'origine.

Convient pour les pédales triples (pédale forte, sostenuto, una corda)
La pédale forte est traitée en tout ou rien.

## Installation

L'installation est manuelle.

Les seuls prérequis sont alsa et fltk

Les commandes suivantes ont été testées sous Linux Mint 19.1

- Installation des fichiers de développement ALSA:
```
sudo apt install libasound2-dev
```
- Installation de FLTK et de ses fichiers de développement:
```
sudo apt install libfltk1.3-dev
```
- Compilation:
```
gcc autobend.cxx -o autobend -lfltk -lasound -lpthread -lstdc++
gcc autohold.cxx -o autohold -lasound -lpthread -lstdc++
```
- Installation:
```
sudo cp autobend /usr/local/bin/
sudo cp po/fr/autobend.mo /usr/share/locale/fr/LC_MESSAGES/
sudo cp autohold /usr/local/bin/
```
## Utilisation
```
autobend [fichier]
```
L'utilisation des curseurs avec la souris permet seulement un ajustement sommaire,
pour ajuster finement il faut utiliser les raccourcis clavier flèche haut et bas, + et - sur le pavé numérique,
ou la molette de défilement de la souris.
Les touches Suppr, Début et Fin fixent la valeur à 0, -8192 et +8191 respectivement.

Autobend ne peut pas connaître la plage du pitch bend sur votre synthétiseur.
La plage des messages pitch bend midi est fixe de -8192 à 8191
mais l'intervalle musical correspondant dépend de votre synthé, l'accord doit être fait à l'oreille.

L'accordage peut être enregistré dans un fichier de type .conf.
Il s'agit d'un simple fichier texte, avec une syntaxe élémentaire:
chaque ligne est de la forme "note espace décalage", par exemple E -2048.

```
autohold [option]...
```
Il s'agit d'un outil en ligne de commande.
Utilisez l'option -h pour plus d'information.

```
Connexions
```
Dans la plupart des cas, il vous faudra connecter la sortie de votre clavier maître à l'entrée de l'outil (autobend ou autohold)
et la sortie de l'outil à l'entrée du synthétiseur.
Un outil pratique pour gérer les connexions est qjackctl. aconnect fait aussi l'affaire.

## Remerciements
Merci à jmechmech pour l'idée de départ et les tests.
