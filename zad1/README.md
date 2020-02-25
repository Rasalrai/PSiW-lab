# Kompilacja
Uruchom w katalogu `zad1`:
```
gcc barbers.c -Wall -o barbers.out
```
lub
```
./compile.sh
```

# Uruchamianie
```
./barbers.out F K N P
```

F: liczba fryzjerów,

K: liczba klientów,

N: liczba foteli fryzjerskich,

P: liczba miejsc w poczekalni.

Jeśli niektóre (lub żadne) argumenty nie zostaną podane, użyte będą wartości domyślne:

`F = 7, K = 12, N = 4, P = 5`

Poprawność argumentów (`K, N, P > 0`, `F > 1`, `N < F`) nie jest sprawdzana.

**Uwaga** Program wypisuje dużo na standardowe wyjście (loguje operacje wszystkich procesów).
