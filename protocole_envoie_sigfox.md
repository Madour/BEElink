Colonne: bit
Ligne  : octet 

|  7   |  6   |  5   |  4   |  3   |  2   |  1   |  0   | Octet |
| :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :---: |
|      |      |      |      |      |      |      |      |       |
|  T2  |  T2  |  T2  |  T1  |  T1  |  T1  |  T1  |  T1  |  11   |
|      |      |      |      |      |      |      |      |       |
|  T4  |  T3  |  T3  |  T3  |  T3  |  T3  |  T2  |  T2  |  10   |
|      |      |      |      |      |      |      |      |       |
|  T5  |  T5  |  T5  |  T5  |  T4  |  T4  |  T4  |  T4  |   9   |
|      |      |      |      |      |      |      |      |       |
|  T7  |  T7  |  T6  |  T6  |  T6  |  T6  |  T6  |  T5  |   8   |
|      |      |      |      |      |      |      |      |       |
|  T8  |  T8  |  T8  |  T8  |  T8  |  T7  |  T7  |  T7  |   7   |
|      |      |      |      |      |      |      |      |       |
|  H1  |  H1  |  H1  |  H1  |  H1  |  H1  |  H1  |  H1  |   6   |
|      |      |      |      |      |      |      |      |       |
|  Po  |  Po  |  Po  |  Po  |  Po  |  Po  |  Po  |  Po  |   5   |
|      |      |      |      |      |      |      |      |       |
|  Po  |  Po  |  Po  |  Po  |  Po  |  Po  |  Po  |  Po  |   4   |
|      |      |      |      |      |      |      |      |       |
| Freq | Freq | Freq | Freq | Freq | Freq | Freq | Freq |   3   |
|      |      |      |      |      |      |      |      |       |
|  V   |  V   |  V   |  V   |  V   |  V   |  V   |  V   |   2   |
|      |      |      |      |      |      |      |      |       |
|      |      |      |      |      |  D   |  D   |  D   |   1   |
|      |      |      |      |      |      |      |      |       |
|      |      |      |      |      |      |      |      |   0   |
|      |      |      |      |      |      |      |      |       |

Légende : 

T = temperature

H = humidité

Freq = freq de bourdonnement

V = vitesse du vent

D = direction du vent

Pr = pression

Po = poids

Message envoyé en big endian:

- octet 0 est l'octet MSB (le plus à gauche).
- octet 11 est l'octet LSB (le plus à droite)
