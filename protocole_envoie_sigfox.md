Colonne: bit
Ligne  : octet 

|  7   |  6   |  5   |  4   |  3   |  2   |  1   |  0   | Octet |
| :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :---: |
|      |      |      |      |      |      |      |      |       |
|  T1  |  T1  |  T1  |  T1  |  T1  |  T1  |  T1  |  T1  |  11   |
|      |      |      |      |      |      |      |      |       |
|  T2  |  T2  |  T2  |  T2  |  T2  |  T2  |  T2  |  T2  |  10   |
|      |      |      |      |      |      |      |      |       |
|  T3  |  T3  |  T3  |  T3  |  T3  |  T3  |  T3  |  T3  |   9   |
|      |      |      |      |      |      |      |      |       |
|  T4  |  T4  |  T4  |  T4  |  T4  |  T4  |  T4  |  T4  |   8   |
|      |      |      |      |      |      |      |      |       |
|  T5  |  T5  |  T5  |  T5  |  T5  |  T5  |  T5  |  T5  |   7   |
|      |      |      |      |      |      |      |      |       |
|  H1  |  H1  |  H1  |  H1  |  H1  |  H1  |  H1  |  H1  |   6   |
|      |      |      |      |      |      |      |      |       |
|  Po  |  Po  |  Po  |  Po  |  Po  |  Po  |  Po  |  Po  |   5   |
|      |      |      |      |      |      |      |      |       |
|  Po  |  Po  |  Po  |  Po  |  Po  |  Po  |  Po  |  Po  |   4   |
|      |      |      |      |      |      |      |      |       |
|  dM  |  dM  |  dM  |  dM  |  dM  |  dM  |  dM  |  dM  |   3   |
|      |      |      |      |      |      |      |      |       |
|  V   |  V   |  V   |  V   |  V   |  V   |  V   |  V   |   2   |
|      |      |      |      |      |      |      |      |       |
|  B   |  B   |  B   |  B   |  B   |  B   |  B   |  B   |   1   |
|      |      |      |      |      |      |      |      |       |
| Flag | Flag | Flag | Flag | Flag |  D   |  D   |  D   |   0   |
|      |      |      |      |      |      |      |      |       |

Légende :

T = temperature

H = humidité

V = vitesse du vent

D = direction du vent

Po = poids

dM = delta Masse

Flags :

- 1 = probleme de masse
- 2 = probleme de temperature interne
- 3 = probleme de variation de masse
Message envoyé en big endian:

- octet 0 est l'octet MSB (le plus à gauche).
- octet 11 est l'octet LSB (le plus à droite)
