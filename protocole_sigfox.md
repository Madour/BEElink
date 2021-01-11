Byte 0 is MSB
Byte 11 is LSB


|  7   |  6   |  5   |  4   |  3   |  2   |  1   |  0   | Bits / Byte |
| :--: | :--: | :--: | :--: | :--: | :--: | :--: | :--: | :---: |
|  T1  |  T1  |  T1  |  T1  |  T1  |  T1  |  T1  |  T1  |  11   |
|  T2  |  T2  |  T2  |  T2  |  T2  |  T2  |  T2  |  T2  |  10   |
|  T3  |  T3  |  T3  |  T3  |  T3  |  T3  |  T3  |  T3  |   9   |
|  T4  |  T4  |  T4  |  T4  |  T4  |  T4  |  T4  |  T4  |   8   |
|  T5  |  T5  |  T5  |  T5  |  T5  |  T5  |  T5  |  T5  |   7   |
|  H1  |  H1  |  H1  |  H1  |  H1  |  H1  |  H1  |  H1  |   6   |
|  M   |  M   |  M   |  M   |  M   |  M   |  M   |  M   |   5   |
|  M   |  M   |  M   |  M   |  M   |  M   |  M   |  M   |   4   |
|  dM  |  dM  |  dM  |  dM  |  dM  |  dM  |  dM  |  dM  |   3   |
|  V   |  V   |  V   |  V   |  V   |  V   |  V   |  V   |   2   |
|  B   |  B   |  B   |  B   |  B   |  B   |  B   |  B   |   1   |
| Flag | Flag | Flag | Flag | Flag |  D   |  D   |  D   |   0   |

T = temperature

H = humidity

V = wind speed

D = wind direction

M = mass

dM = delta masse

Flags :

- 1 = mass alert
- 2 = interior temperature alert
- 3 = mass variation alert
- 4 : unused
- 5 : unused
