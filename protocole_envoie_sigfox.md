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
|      |      |      |      |      |  D   |  D   |  D   |   1   |
|      |      |      |      |      |      |      |      |       |
| Pow  |  S1  |  S2  |  S3  |  S4  |      |      |      |   0   |
|      |      |      |      |      |      |      |      |       |

Légende : 

T = temperature

H = humidité

V = vitesse du vent

D = direction du vent

Po = poids

dM = delta Masse

Message envoyé en big endian:

- octet 0 est l'octet MSB (le plus à gauche).
- octet 11 est l'octet LSB (le plus à droite)
